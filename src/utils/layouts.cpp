#include "layouts.h"
#include "engine/scene.h"
#include "engine/drawSys.h"
#include "engine/world.h"

// LAYOUT

Layout::Layout(const Size& size, vector<Widget*>&& children, Direction dir, Select select, int space, bool pad) :
	Widget(size),
	spacing(space),
	margin(pad),
	selection(select),
	direction(dir)
{
	initWidgets(std::move(children));
}

Layout::~Layout() {
	clearWidgets();
}

void Layout::drawSelf(const Recti& view) {
	for (Widget* it : widgets)
		it->drawSelf(view);
}

void Layout::drawAddr(const Recti& view) {
	World::drawSys()->drawLayoutAddr(this, view);
}

void Layout::onResize() {
	calculateWidgetPositions();
	for (Widget* it : widgets)
		it->onResize();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::postInit() {
	calculateWidgetPositions();
	for (Widget* it : widgets)
		it->postInit();
}

void Layout::calculateWidgetPositions() {
	// get amount of space for widgets with percent size and get total sum
	int vi = direction.vertical();
	int pad = margin ? spacing : 0;
	ivec2 wsiz = size() - pad * 2;
	vector<int> pixSizes(widgets.size());
	int space = wsiz[vi] - (widgets.size() - 1) * spacing;
	float total = 0;
	for (size_t i = 0; i < widgets.size(); ++i)
		switch (const Size& siz = widgets[i]->getRelSize(); siz.mod) {
		using enum Size::Mode;
		case rela:
			total += siz.prc;
			break;
		case pixv:
			pixSizes[i] = siz.pix;
			space -= std::min(pixSizes[i], space);
			break;
		case calc:
			pixSizes[i] = siz(widgets[i]);
			space -= std::min(pixSizes[i], space);
		}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(pad);
	for (size_t i = 0; i < widgets.size(); ++i) {
		positions[i] = pos;
		if (const Size& siz = widgets[i]->getRelSize(); siz.mod != Size::rela)
			pos[vi] += pixSizes[i] + spacing;
		else if (float val = siz.prc * float(space); val != 0.f)
			pos[vi] += int(val / total) + spacing;
	}
	positions.back() = vswap(wsiz[!vi], pos[vi], !vi);
}

void Layout::onMouseMove(ivec2 mPos, ivec2 mMov) {
	for (Widget* it : widgets)
		it->onMouseMove(mPos, mMov);
}

void Layout::onDisplayChange() {
	for (Widget* it : widgets)
		it->onDisplayChange();
}

bool Layout::navSelectable() const {
	return !widgets.empty();
}

void Layout::navSelectNext(size_t id, int mid, Direction dir) {
	if (dir.vertical() == direction.vertical() && (dir.positive() ? id < widgets.size() - 1 : id))
		scanSequential(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::navSelectFrom(int mid, Direction dir) {
	if (dir.vertical() == direction.vertical())
		scanSequential(dir.positive() ? SIZE_MAX : widgets.size(), mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::scanSequential(size_t id, int mid, Direction dir) {
	for (size_t mov = btom<size_t>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->navSelectable(););
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	size_t id = 0;
	for (uint hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->navSelectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); ++id);
	if (id == widgets.size())
		while (--id < widgets.size() && !widgets[id]->navSelectable());

	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::navSelectWidget(size_t id, int mid, Direction dir) {
	if (Layout* lay = dynamic_cast<Layout*>(widgets[id]))
		lay->navSelectFrom(mid, dir);
	else if (widgets[id]->navSelectable())
		World::scene()->select = widgets[id];
}

void Layout::initWidgets(vector<Widget*>&& wgts) {
	clearWidgets();
	widgets = std::move(wgts);
	positions.resize(widgets.size() + 1);

	if (direction.negative())
		rng::reverse(widgets);
	for (size_t i = 0; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
}

void Layout::clearWidgets() {
	for (Widget* it : widgets)
		delete it;
	widgets.clear();
}

void Layout::setWidgets(vector<Widget*>&& wgts) {
	initWidgets(std::move(wgts));
	postInit();
	World::renderer()->synchTransfer();
	World::scene()->updateSelect();
}

void Layout::insertWidget(size_t id, Widget* wgt) {
	widgets.insert(widgets.begin() + id, wgt);
	positions.resize(positions.size() + 1);
	for (size_t i = id; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	widgets[id]->postInit();
	postWidgetsChange();
}

void Layout::replaceWidget(size_t id, Widget* widget) {
	delete widgets[id];
	widgets[id] = widget;
	widget->setParent(this, id);
	calculateWidgetPositions();
	widget->postInit();
	postWidgetsChange();
}

void Layout::deleteWidget(size_t id) {
	delete widgets[id];
	widgets.erase(widgets.begin() + ptrdiff_t(id));
	positions.pop_back();
	for (size_t i = id; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	postWidgetsChange();
}

void Layout::postWidgetsChange() {
	for (Widget* it : widgets)
		it->onResize();
	World::renderer()->synchTransfer();
	World::scene()->updateSelect();
}

ivec2 Layout::wgtPosition(size_t id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(size_t id) const {
	int di = direction.vertical();
	return vswap(positions.back()[!di], positions[id + 1][di] - positions[id][di] - spacing, !di);
}

void Layout::selectWidget(size_t id) {
	switch (selection) {
	using enum Select;
	case one:
		selected = { widgets[id] };
		break;
	case any:
		if (const uint8* keys = SDL_GetKeyboardState(nullptr); keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
			if (selected.empty())
				selected.insert(widgets[id]);
			else if (mvec2 lims = findMinMaxSelectedID(); outRange(id, lims.x, lims.y))
				selected.insert(widgets.begin() + ptrdiff_t(id < lims.x ? id : lims.y+1), widgets.begin() + ptrdiff_t((id < lims.x) ? lims.y : id+1));
			else
				selected.erase(widgets[id]);
		} else if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
			if (uset<Widget*>::iterator it = selected.find(widgets[id]); it != selected.end())
				selected.erase(it);
			else
				selected.insert(widgets[id]);
		} else
			selected = { widgets[id] };
	}
}

mvec2 Layout::findMinMaxSelectedID() const {
	mvec2 idm((*selected.begin())->getIndex());
	for (uset<Widget*>::const_iterator it = std::next(selected.begin()); it != selected.end(); ++it) {
		if ((*it)->getIndex() < idm.x)
			idm.x = (*it)->getIndex();
		else if ((*it)->getIndex() > idm.y)
			idm.y = (*it)->getIndex();
	}
	return idm;
}

// ROOT LAYOUT

ivec2 RootLayout::position() const {
	return ivec2(0);
}

ivec2 RootLayout::size() const {
	return World::drawSys()->getViewRes();
}

Recti RootLayout::frame() const {
	return Recti(ivec2(0), World::drawSys()->getViewRes());
}

void RootLayout::setSize(const Size& size) {
	relSize = size;
	onResize();
	World::renderer()->synchTransfer();
}

// POPUP

Popup::Popup(const svec2& size, vector<Widget*>&& children, PCall cancelCall, Widget* first, Color background, Direction dir, int space, bool pad) :
	RootLayout(size.x, std::move(children), dir, Select::none, space, pad),
	bgColor(background),
	firstNavSelect(first),
	ccall(cancelCall),
	sizeY(size.y)
{}

void Popup::drawSelf(const Recti& view) {
	World::drawSys()->drawPopup(this, view);
}

ivec2 Popup::position() const {
	return (World::drawSys()->getViewRes() - size()) / 2;
}

ivec2 Popup::size() const {
	ivec2 res = World::drawSys()->getViewRes();
	return ivec2(sizeToPixAbs(relSize, res.x), sizeToPixAbs(sizeY, res.y));
}

// OVERLAY

Overlay::Overlay(const svec2& position, const svec2& size, const svec2& activationPos, const svec2& activationSize, vector<Widget*>&& children, Color background, Direction dir, int space, bool pad) :
	Popup(size, std::move(children), nullptr, nullptr, background, dir, space, pad),
	pos(position),
	actPos(activationPos),
	actSize(activationSize)
{}

ivec2 Overlay::position() const {
	ivec2 res = World::drawSys()->getViewRes();
	return ivec2(sizeToPixAbs(pos.x, res.x), sizeToPixAbs(pos.y, res.y));
}

Recti Overlay::actRect() const {
	ivec2 res = World::drawSys()->getViewRes();
	return Recti(sizeToPixAbs(actPos.x, res.x), sizeToPixAbs(actPos.y, res.y), sizeToPixAbs(actSize.x, res.x), sizeToPixAbs(actSize.y, res.y));
}

// CONTEXT

Context::Context(const svec2& position, const svec2& size, vector<Widget*>&& children, Widget* first, Widget* owner, Color background, LCall resize, Direction dir, int space, bool pad) :
	Popup(size, std::move(children), nullptr, first, background, dir, space, pad),
	pos(position),
	resizeCall(resize)
{
	parent = reinterpret_cast<Layout*>(owner);	// parent shouldn't be in use anyway
}

void Context::onResize() {
	World::prun(resizeCall, this);
	Layout::onResize();
}

ivec2 Context::position() const {
	ivec2 res = World::drawSys()->getViewRes();
	return ivec2(sizeToPixAbs(pos.x, res.x), sizeToPixAbs(pos.y, res.y));
}

void Context::setRect(const Recti& rct) {
	pos = rct.pos();
	relSize = rct.w;
	sizeY = rct.h;
}

// SCROLL AREA

void ScrollArea::drawSelf(const Recti& view) {
	World::drawSys()->drawScrollArea(this, view);
}

void ScrollArea::onResize() {
	Layout::onResize();
	setListPos(listPos);
}

void ScrollArea::tick(float dSec) {
	Scrollable::tick(dSec);
	Layout::tick(dSec);
}

void ScrollArea::calculateWidgetPositions() {
	Layout::calculateWidgetPositions();

	int hi = direction.horizontal();
	setLimits(vswap(positions.back()[hi], positions.back()[!hi] - spacing, hi), size(), !hi);
}

void ScrollArea::postWidgetsChange() {
	for (Widget* it : widgets)
		it->onResize();
	setListPos(listPos);
	World::renderer()->synchTransfer();
	World::scene()->updateSelect();
}

void ScrollArea::onHold(ivec2 mPos, uint8 mBut) {
	hold(mPos, mBut, this, position(), size(), direction.vertical());
}

void ScrollArea::onDrag(ivec2 mPos, ivec2 mMov) {
	drag(mPos, mMov, position(), direction.vertical());
}

void ScrollArea::onUndrag(ivec2 mPos, uint8 mBut) {
	undrag(mPos, mBut, direction.vertical());
}

void ScrollArea::onScroll(ivec2 wMov) {
	scroll(wMov, direction.vertical());
	World::scene()->updateSelect();
}

void ScrollArea::navSelectNext(size_t id, int mid, Direction dir) {
	Layout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, Direction dir) {
	Layout::navSelectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::scrollToSelected() {
	size_t cid = World::scene()->findSelectedID(this);
	if (cid >= widgets.size())
		return;

	uint di = direction.vertical();
	if (int cpos = widgets[cid]->position()[di], fpos = position()[di]; cpos < fpos)
		scrollToWidgetPos(cid);
	else if (cpos + widgets[cid]->size()[di] > fpos + size()[di])
		scrollToWidgetEnd(cid);
}

void ScrollArea::scrollToWidgetPos(size_t id) {
	int di = direction.vertical();
	listPos[di] = std::min(wgtRPos(id), getListMax()[di]);
}

void ScrollArea::scrollToWidgetEnd(size_t id) {
	int di = direction.vertical();
	listPos[di] = std::max(wgtREnd(id) - size()[di], 0);
}

bool ScrollArea::scrollToNext() {
	bool dp = direction.positive();
	int dv = direction.vertical();
	if (dp ? listPos[dv] >= getListMax()[dv] : listPos[dv] <= 0)
		return false;

	scrollToFollowing(dp ? firstWidgetAt(listPos[dv]) + 1 : visibleWidgets().y - 2, false);
	return true;
}

bool ScrollArea::scrollToPrevious() {
	bool dp = direction.positive();
	int dv = direction.vertical();
	if (dp ? listPos[dv] <= 0 : listPos[dv] >= getListMax()[dv])
		return false;

	size_t id;
	if (dp) {
		id = firstWidgetAt(listPos[dv]);
		if (listPos[dv] <= wgtRPos(id))
			--id;
	} else {
		id = visibleWidgets().y - 1;
		if (listPos[dv] + size()[dv] >= wgtREnd(id))
			++id;
	}
	scrollToFollowing(id, true);
	return true;
}

void ScrollArea::scrollToFollowing(size_t id, bool prev) {
	if (id < widgets.size()) {
		if (direction.positive())
			scrollToWidgetPos(id);
		else
			scrollToWidgetEnd(id);
	} else
		scrollToLimit(prev);
	motion = vec2(0.f);
}

void ScrollArea::scrollToLimit(bool start) {
	int di = direction.vertical();
	listPos[di] = direction.positive() == start ? 0 : getListMax()[di];
	motion = vec2(0.f);
}

float ScrollArea::getScrollLocation() const {
	int di = direction.vertical();
	return float(listPos[di]) / float(size()[di]);
}

void ScrollArea::setScrollLocation(float loc) {
	int di = direction.vertical();
	ivec2 siz = size();
	setListPos(vswap(int(loc * float(siz[di])), listPos[!di], di));
}

void ScrollArea::setWidgets(vector<Widget*>&& wgts) {
	initWidgets(std::move(wgts));
	postInit();
	setListPos(listPos);
	World::renderer()->synchTransfer();
	World::scene()->updateSelect();
}

Recti ScrollArea::frame() const {
	return parent ? rect().intersect(parent->frame()) : rect();
}

ivec2 ScrollArea::wgtPosition(size_t id) const {
	return Layout::wgtPosition(id) - listPos;
}

ivec2 ScrollArea::wgtSize(size_t id) const {
	int di = direction.vertical();
	return Layout::wgtSize(id) - vswap(barSize(size(), di), 0, !di);
}

int ScrollArea::wgtRPos(size_t id) const {
	return positions[id][direction.vertical()];
}

int ScrollArea::wgtREnd(size_t id) const {
	return positions[id + 1][direction.vertical()] - spacing;
}

mvec2 ScrollArea::visibleWidgets() const {
	mvec2 ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	int di = direction.vertical();
	ival.x = firstWidgetAt(listPos[di]);
	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = listPos[di] + size()[di]; ival.y < widgets.size() && wgtRPos(ival.y) <= end; ++ival.y);
	return ival;
}

size_t ScrollArea::firstWidgetAt(int rpos) const {
	size_t i;
	for (i = 0; i < widgets.size() && wgtREnd(i) < rpos; ++i);
	return i;
}

// TILE BOX

TileBox::TileBox(const Size& size, vector<Widget*>&& children, int childHeight, Direction dir, Select select, int space, bool pad) :
	ScrollArea(size, std::move(children), dir, select, space, pad),
	wheight(childHeight)
{}

void TileBox::calculateWidgetPositions() {
	positions[0] = ivec2(0);
	int hi = direction.horizontal();
	int wsiz = size()[hi] - Scrollable::barSizeVal;
	ivec2 pos(!widgets.empty() ? widgets[0]->sizeToPixAbs(widgets[0]->getRelSize(), wsiz) + spacing : 0, 0);
	for (size_t i = 1; i < widgets.size(); ++i) {
		if (int end = pos.x + widgets[i]->sizeToPixAbs(widgets[i]->getRelSize(), wsiz); end > wsiz && positions[i - 1].y == pos.y) {
			pos = ivec2(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = ivec2(wsiz, pos.y + wheight);
	setLimits(positions.back(), size(), !hi);
	positions.back()[!hi] += spacing;
}

void TileBox::navSelectNext(size_t id, int mid, Direction dir) {
	if (dir.vertical())
		scanVertically(id, mid, dir);
	else
		scanHorizontally(id, mid, dir);
	scrollToSelected();
}

void TileBox::navSelectFrom(int mid, Direction dir) {
	if (dir.positive())
		scanFromStart(mid, dir);
	else
		scanFromEnd(mid, dir);
	scrollToSelected();
}

void TileBox::scanVertically(size_t id, int mid, Direction dir) {
	if (int ypos = widgets[id]->position().y; dir.positive())
		while (++id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x + widgets[id]->size().x < mid));
	else
		while (--id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x > mid));
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanHorizontally(size_t id, int mid, Direction dir) {
	for (size_t mov = btom<size_t>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->navSelectable(););
	if (id < widgets.size() && widgets[id]->center().y == mid)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void TileBox::scanFromStart(int mid, Direction dir) {
	size_t id = 0;
	for (uint di = dir != Direction::down; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] + widgets[id]->size()[di] < mid); ++id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, Direction dir) {
	size_t id = widgets.size() - 1;
	for (uint di = dir != Direction::up; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] > mid); --id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::navSelectIfInRange(size_t id, int mid, Direction dir) {
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

ivec2 TileBox::wgtSize(size_t id) const {
	return ivec2(widgets[id]->getRelSize().pix, wheight);
}

int TileBox::wgtREnd(size_t id) const {
	return positions[id].y + wheight;
}

// READER BOX

ReaderBox::ReaderBox(const Size& size, Direction dir, float fzoom, int space, bool pad) :
	ScrollArea(size, {}, dir, Select::none, space, pad),
	zoom(fzoom)
{}

void ReaderBox::drawSelf(const Recti& view) {
	World::drawSys()->drawReaderBox(this, view);
}

void ReaderBox::tick(float dSec) {
	ScrollArea::tick(dSec);

	if (countDown) {
		cursorTimer -= dSec;
		if (cursorTimer <= 0.f) {
			SDL_ShowCursor(SDL_DISABLE);
			countDown = false;
		}
	}
}

void ReaderBox::calculateWidgetPositions() {
	// figure out the width of the list
	int hi = direction.horizontal();
	ivec2 siz = size();
	int maxSecs = 0;
	for (const Widget* it : widgets)
		if (int rsiz = static_cast<const Picture*>(it)->getTex()->getRes()[hi]; rsiz > maxSecs)
			maxSecs = rsiz;
	maxSecs = std::max(int(float(maxSecs) * zoom), siz[hi]);

	// set position of each picture
	int rpos = 0;
	for (size_t i = 0; i < widgets.size(); ++i) {
		ivec2 psz = vec2(static_cast<Picture*>(widgets[i])->getTex()->getRes()) * zoom;
		positions[i] = vswap((maxSecs - psz[hi]) / 2, rpos, hi);
		rpos += psz[!hi] + spacing;
	}
	positions.back() = vswap(maxSecs, rpos, hi);
	setLimits(vswap(maxSecs, rpos ? rpos - spacing : 0, hi), siz, !hi);
}

void ReaderBox::onMouseMove(ivec2 mPos, ivec2 mMov) {
	Layout::onMouseMove(mPos, mMov);

	countDown = World::scene()->getSelectedScrollArea() == this && !showBar() && World::scene()->getCapture() != this && cursorTimer > 0.f;
	if (cursorTimer < menuHideTimeout) {
		cursorTimer = menuHideTimeout;
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void ReaderBox::setPictures(vector<pair<string, Texture*>>& imgs, string_view startPic) {
	clearWidgets();
	widgets.resize(imgs.size());
	positions.resize(imgs.size() + 1);
	picNames.resize(imgs.size());

	if (direction.negative())
		rng::reverse(imgs);
	for (size_t i = 0; i < imgs.size(); ++i) {
		widgets[i] = new Picture(0, false, pair(imgs[i].second, true), 0);
		widgets[i]->setParent(this, i);
		picNames[i] = std::move(imgs[i].first);
	}
	startPicId = rng::find(picNames, startPic) - picNames.begin();
	postInit();

	// scroll down to opened picture if it exists, otherwise start at beginning
	if (startPicId < picNames.size()) {
		if (direction.positive())
			scrollToWidgetPos(startPicId);
		else
			scrollToWidgetEnd(startPicId);
	} else
		listPos = ivec2(0);
	centerList();
}

bool ReaderBox::showBar() const {
	return barRect().contains(World::winSys()->mousePos()) || getDraggingSlider();
}

void ReaderBox::setZoom(float factor) {
	if (widgets.empty()) {
		zoom *= factor;
		return;
	}
	int vi = direction.vertical();
	ivec2 sh = size() / 2;
	ivec2 vpos = listPos + sh;
	size_t id = firstWidgetAt(vpos[vi]);
	int preSpace = id * spacing;
	if (int ipos = wgtRPos(id); ipos > vpos[vi])
		preSpace -= spacing - (ipos - vpos[vi]);
	int totSpace = (widgets.size() - 1) * spacing;
	vec2 loc = vec2(vswap(vpos[!vi], vpos[vi] - preSpace, !vi)) / vec2(vswap(getListSize()[!vi], getListSize()[vi] - totSpace, !vi));
	zoom *= factor;
	Layout::onResize();
	setListPos(ivec2(loc * vec2(vswap(getListSize()[!vi], getListSize()[vi] - totSpace, !vi))) + vswap(0, preSpace, !vi) - sh);
}

void ReaderBox::centerList() {
	int di = direction.horizontal();
	listPos[di] = getListMax()[di] / 2;
}

ivec2 ReaderBox::wgtSize(size_t id) const {
	return vec2(static_cast<Picture*>(widgets[id])->getTex()->getRes()) * zoom;
}
