#include "layouts.h"
#include "engine/drawSys.h"
#include "engine/scene.h"
#include "engine/world.h"

Children::Children(std::initializer_list<Widget*> ilist) :
	wgts(std::make_unique_for_overwrite<Widget*[]>(ilist.size())),
	num(ilist.size())
{
	rng::copy(ilist, wgts.get());
}

Children::Children(uint size) :
	wgts(std::make_unique_for_overwrite<Widget*[]>(size)),
	num(size)
{}

Children& Children::operator=(Children&& ch) {
	wgts = std::move(ch.wgts);
	num = ch.num;
	return *this;
}

// LAYOUT

Layout::Layout(const Size& size, Children&& children, Direction dir, ushort space, bool pad) :
	Widget(size),
	spacing(space),
	margin(pad),
	direction(dir)
{
	initWidgets(std::move(children));
}

Layout::~Layout() {
	clearWidgets();
}

void Layout::drawSelf(const Recti& view) {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->drawSelf(view);
}

void Layout::drawAddr(const Recti& view) {
	World::drawSys()->drawLayoutAddr(this, view);
}

void Layout::onResize() {
	calculateWidgetPositions();
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->onResize();
}

void Layout::tick(float dSec) {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->tick(dSec);
}

void Layout::postInit() {
	calculateWidgetPositions();
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->postInit();
}

void Layout::calculateWidgetPositions() {
	// get amount of space for widgets with percent size and get total sum
	int vi = direction.vertical();
	int pad = margin ? spacing : 0;
	ivec2 wsiz = size() - pad * 2;
	vector<int> pixSizes(numWgts);
	int space = wsiz[vi] - (numWgts - 1) * spacing;
	float total = 0;
	for (uint i = 0; i < numWgts; ++i)
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
	for (uint i = 0; i < numWgts; ++i) {
		positions[i] = pos;
		if (const Size& siz = widgets[i]->getRelSize(); siz.mod != Size::rela)
			pos[vi] += pixSizes[i] + spacing;
		else if (float val = siz.prc * float(space); val != 0.f)
			pos[vi] += int(val / total) + spacing;
	}
	positions[numWgts] = vswap(wsiz[!vi], pos[vi], !vi);
}

void Layout::onMouseMove(ivec2 mPos, ivec2 mMov) {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->onMouseMove(mPos, mMov);
}

void Layout::onDisplayChange() {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->onDisplayChange();
}

bool Layout::navSelectable() const {
	return numWgts;
}

void Layout::navSelectNext(uint id, int mid, Direction dir) {
	if (dir.vertical() == direction.vertical() && (dir.positive() ? id < numWgts - 1 : id))
		scanSequential(id, mid, dir);
	else if (parent)
		parent->navSelectNext(relSize.id, mid, dir);
}

void Layout::navSelectFrom(int mid, Direction dir) {
	if (dir.vertical() == direction.vertical())
		scanSequential(dir.positive() ? UINT_MAX : numWgts, mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::scanSequential(uint id, int mid, Direction dir) {
	for (uint mov = btom<uint>(dir.positive()); (id += mov) < numWgts && !widgets[id]->navSelectable(););
	if (id < numWgts)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(relSize.id, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	uint id = 0;
	for (uint hori = dir.horizontal(); id < numWgts && (!widgets[id]->navSelectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); ++id);
	if (id == numWgts)
		while (--id < numWgts && !widgets[id]->navSelectable());

	if (id < numWgts)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(relSize.id, mid, dir);
}

void Layout::navSelectWidget(uint id, int mid, Direction dir) {
	if (auto lay = dynamic_cast<Layout*>(widgets[id]))
		lay->navSelectFrom(mid, dir);
	else if (widgets[id]->navSelectable())
		World::scene()->updateSelect(widgets[id]);
}

void Layout::initWidgets(Children&& children) {
	clearWidgets();
	widgets = std::move(children.wgts);
	positions = std::make_unique_for_overwrite<ivec2[]>(children.num + 1);
	numWgts = children.num;

	if (direction.negative())
		std::reverse(widgets.get(), widgets.get() + numWgts);
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->setParent(this, i);
}

void Layout::clearWidgets() {
	for (uint i = 0; i < numWgts; ++i)
		delete widgets[i];
}

void Layout::setWidgets(Children&& children) {
	deselectWidgets();
	initWidgets(std::move(children));
	postInit();
	World::drawSys()->getRenderer()->synchTransfer();
	World::scene()->updateSelect();
}

void Layout::insertWidget(uint id, Widget* wgt) {
	uptr<Widget*[]> wgts = std::make_unique_for_overwrite<Widget*[]>(numWgts + 1);
	std::copy_n(widgets.get(), id, wgts.get());
	wgts[id] = wgt;
	std::copy_n(widgets.get() + id, numWgts - id, wgts.get() + id + 1);
	widgets = std::move(wgts);
	++numWgts;
	positions = std::make_unique_for_overwrite<ivec2[]>(numWgts + 1);
	for (uint i = id; i < numWgts; ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	widgets[id]->postInit();
	postWidgetsChange();
}

void Layout::replaceWidget(uint id, Widget* widget) {
	deselectWidget(widgets[id]);
	delete widgets[id];
	widgets[id] = widget;
	widget->setParent(this, id);
	calculateWidgetPositions();
	widget->postInit();
	postWidgetsChange();
}

void Layout::deleteWidget(uint id) {
	deselectWidget(widgets[id]);
	delete widgets[id];
	uptr<Widget*[]> wgts = std::make_unique_for_overwrite<Widget*[]>(--numWgts);
	std::copy_n(widgets.get(), id, wgts.get());
	std::copy_n(widgets.get() + id + 1, numWgts - id, wgts.get() + id);
	widgets = std::move(wgts);
	positions = std::make_unique_for_overwrite<ivec2[]>(numWgts + 1);
	for (uint i = id; i < numWgts; ++i)
		widgets[i]->setParent(this, i);
	calculateWidgetPositions();
	postWidgetsChange();
}

void Layout::postWidgetsChange() {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->onResize();
	World::drawSys()->getRenderer()->synchTransfer();
	World::scene()->updateSelect();
}

ivec2 Layout::wgtPosition(uint id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(uint id) const {
	int di = direction.vertical();
	return vswap(positions[numWgts][!di], positions[id + 1][di] - positions[id][di] - spacing, !di);
}

void Layout::deselectWidgets() const {
	if (isParentOf(World::scene()->getCapture()))
		World::scene()->setCapture(nullptr);
	if (isParentOf(World::scene()->getSelect()))
		World::scene()->deselect();
}

void Layout::deselectWidget(const Widget* wgt) const {
	if (World::scene()->getCapture() == wgt)
		World::scene()->setCapture(nullptr);
	if (World::scene()->getSelect() == wgt)
		World::scene()->deselect();
}

bool Layout::isParentOf(const Widget* wgt) const {
	if (!wgt || this == wgt)
		return wgt;
	for (Layout* box = wgt->getParent(); box; box = box->getParent())
		if (box == this)
			return true;
	return false;
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
	World::drawSys()->getRenderer()->synchTransfer();
}

// POPUP

Popup::Popup(const svec2& size, Children&& children, EventId cancelCall, EventId confirmCall, Widget* first, Color background, Direction dir, ushort space, bool pad) :
	RootLayout(size.x, std::move(children), dir, space, pad),
	bgColor(background),
	firstNavSelect(first),
	ccall(cancelCall),
	kcall(confirmCall),
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

Overlay::Overlay(const svec2& position, const svec2& size, const svec2& activationPos, const svec2& activationSize, Children&& children, Color background, Direction dir, ushort space, bool pad) :
	Popup(size, std::move(children), nullEvent, nullEvent, nullptr, background, dir, space, pad),
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

Context::Context(const svec2& position, const svec2& size, Children&& children, Widget* first, Widget* owner, Color background, EventId resize, Direction dir, ushort space, bool pad) :
	Popup(size, std::move(children), nullEvent, nullEvent, first, background, dir, space, pad),
	pos(position),
	resizeCall(resize)
{
	parent = reinterpret_cast<Layout*>(owner);	// parent shouldn't be in use anyway
}

void Context::onResize() {
	pushEvent(resizeCall, this);
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
	setLimits(vswap(positions[numWgts][hi], positions[numWgts][!hi] - spacing, hi), size(), !hi);
}

void ScrollArea::postWidgetsChange() {
	for (uint i = 0; i < numWgts; ++i)
		widgets[i]->onResize();
	setListPos(listPos);
	World::drawSys()->getRenderer()->synchTransfer();
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

void ScrollArea::navSelectNext(uint id, int mid, Direction dir) {
	Layout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, Direction dir) {
	Layout::navSelectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::scrollToSelected() {
	Widget* child = World::scene()->getSelect();
	for (; child->getParent() && child->getParent() != this; child = child->getParent());
	if (!child->getParent())
		return;

	uint di = direction.vertical();
	if (int cpos = child->position()[di], fpos = position()[di]; cpos < fpos)
		scrollToWidgetPos(child->getIndex());
	else if (cpos + child->size()[di] > fpos + size()[di])
		scrollToWidgetEnd(child->getIndex());
}

void ScrollArea::scrollToWidgetPos(uint id) {
	int di = direction.vertical();
	listPos[di] = std::min(wgtRPos(id), getListMax()[di]);
}

void ScrollArea::scrollToWidgetEnd(uint id) {
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

	uint id;
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

void ScrollArea::scrollToFollowing(uint id, bool prev) {
	if (id < numWgts) {
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

void ScrollArea::setWidgets(Children&& children) {
	deselectWidgets();
	initWidgets(std::move(children));
	postInit();
	setListPos(listPos);
	World::drawSys()->getRenderer()->synchTransfer();
	World::scene()->updateSelect();
}

Recti ScrollArea::frame() const {
	return parent ? rect().intersect(parent->frame()) : rect();
}

ivec2 ScrollArea::wgtPosition(uint id) const {
	return Layout::wgtPosition(id) - listPos;
}

ivec2 ScrollArea::wgtSize(uint id) const {
	int di = direction.vertical();
	return Layout::wgtSize(id) - vswap(barSize(size(), di), 0, !di);
}

int ScrollArea::wgtRPos(uint id) const {
	return positions[id][direction.vertical()];
}

int ScrollArea::wgtREnd(uint id) const {
	return positions[id + 1][direction.vertical()] - spacing;
}

uvec2 ScrollArea::visibleWidgets() const {
	uvec2 ival(0);
	if (!numWgts)	// nothing to draw
		return ival;

	int di = direction.vertical();
	ival.x = firstWidgetAt(listPos[di]);
	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = listPos[di] + size()[di]; ival.y < numWgts && wgtRPos(ival.y) <= end; ++ival.y);
	return ival;
}

uint ScrollArea::firstWidgetAt(int rpos) const {
	uint i;
	for (i = 0; i < numWgts && wgtREnd(i) < rpos; ++i);
	return i;
}

// TILE BOX

TileBox::TileBox(const Size& size, Children&& children, int childHeight, Direction dir, ushort space, bool pad) :
	ScrollArea(size, std::move(children), dir, space, pad),
	wheight(childHeight)
{}

void TileBox::calculateWidgetPositions() {
	positions[0] = ivec2(0);
	int hi = direction.horizontal();
	int wsiz = size()[hi] - Scrollable::barSizeVal;
	ivec2 pos(numWgts ? widgets[0]->sizeToPixAbs(widgets[0]->getRelSize(), wsiz) + spacing : 0, 0);
	for (uint i = 1; i < numWgts; ++i) {
		if (int end = pos.x + widgets[i]->sizeToPixAbs(widgets[i]->getRelSize(), wsiz); end > wsiz && positions[i - 1].y == pos.y) {
			pos = ivec2(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions[numWgts] = ivec2(wsiz, pos.y + wheight);
	setLimits(positions[numWgts], size(), !hi);
	positions[numWgts][!hi] += spacing;
}

void TileBox::navSelectNext(uint id, int mid, Direction dir) {
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

void TileBox::scanVertically(uint id, int mid, Direction dir) {
	if (int ypos = widgets[id]->position().y; dir.positive())
		while (++id < numWgts && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x + widgets[id]->size().x < mid));
	else
		while (--id < numWgts && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x > mid));
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanHorizontally(uint id, int mid, Direction dir) {
	for (uint mov = btom<uint>(dir.positive()); (id += mov) < numWgts && !widgets[id]->navSelectable(););
	if (id < numWgts && widgets[id]->center().y == mid)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(relSize.id, mid, dir);
}

void TileBox::scanFromStart(int mid, Direction dir) {
	uint id = 0;
	for (uint di = dir != Direction::down; id < numWgts && (!widgets[id]->navSelectable() || widgets[id]->position()[di] + widgets[id]->size()[di] < mid); ++id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, Direction dir) {
	uint id = numWgts - 1;
	for (uint di = dir != Direction::up; id < numWgts && (!widgets[id]->navSelectable() || widgets[id]->position()[di] > mid); --id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::navSelectIfInRange(uint id, int mid, Direction dir) {
	if (id < numWgts)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(relSize.id, mid, dir);
}

ivec2 TileBox::wgtSize(uint id) const {
	return ivec2(widgets[id]->getRelSize().pix, wheight);
}

int TileBox::wgtREnd(uint id) const {
	return positions[id].y + wheight;
}

// READER BOX

ReaderBox::ReaderBox(const Size& size, Direction dir, int8 zstep, ushort space, bool pad) :
	ScrollArea(size, Children(), dir, space, pad),
	zoomStep(zstep)
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
	double zoom = Settings::zoomValue(zoomStep);
	int maxSecs = 0;
	for (uint i = 0; i < numWgts; ++i)
		if (int rsiz = static_cast<const Picture*>(widgets[i])->getTex()->getRes()[hi]; rsiz > maxSecs)
			maxSecs = rsiz;
	maxSecs = std::max(int(double(maxSecs) * zoom), siz[hi]);

	// set position of each picture
	int rpos = 0;
	for (uint i = 0; i < numWgts; ++i) {
		ivec2 psz = dvec2(static_cast<Picture*>(widgets[i])->getTex()->getRes()) * zoom;
		positions[i] = vswap((maxSecs - psz[hi]) / 2, rpos, hi);
		rpos += psz[!hi] + spacing;
	}
	positions[numWgts] = vswap(maxSecs, rpos, hi);
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

void ReaderBox::setPictures(vector<pair<Cstring, Texture*>>& imgs, string_view startPic, bool fwd) {
	deselectWidgets();
	clearWidgets();

	switch (World::sets()->zoomType) {
	using enum Settings::Zoom;
	case first:
		if (!imgs.empty())
			if (uint res = imgs[0].second->getRes()[direction.horizontal()])
				zoomStep = zoomStepToFit(res);
		break;
	case largest: {
		int di = direction.horizontal();
		uint maxRes = 0;
		for (auto& [name, tex] : imgs)
			if (tex->getRes()[di] > maxRes)
				maxRes = tex->getRes()[di];
		if (maxRes)
			zoomStep = zoomStepToFit(maxRes);
	} }

	numWgts = imgs.size();
	widgets = std::make_unique_for_overwrite<Widget*[]>(numWgts);
	positions = std::make_unique_for_overwrite<ivec2[]>(numWgts + 1);
	picNames = std::make_unique<Cstring[]>(numWgts);
	bool dp = direction.positive();
	if (!dp)
		rng::reverse(imgs);

	for (uint i = 0; i < numWgts; ++i) {
		widgets[i] = new Picture(0, imgs[i].second);
		widgets[i]->setParent(this, i);
		picNames[i] = std::move(imgs[i].first);
	}
	startPicId = std::find(picNames.get(), picNames.get() + numWgts, startPic) - picNames.get();
	postInit();

	// scroll down to opened picture if it exists, otherwise start at beginning
	if (startPicId < numWgts) {
		if (dp)
			scrollToWidgetPos(startPicId);
		else
			scrollToWidgetEnd(startPicId);
	} else
		listPos = fwd == dp ? ivec2(0) : getListMax();
	centerList();
}

bool ReaderBox::showBar() const {
	return barRect().contains(World::winSys()->mousePos()) || getDraggingSlider();
}

void ReaderBox::setZoom(int8 step) {
	if (numWgts)
		setZoom([this](int8 zs) { zoomStep = zs; }, step);
	else
		zoomStep = step;
}

void ReaderBox::addZoom(int8 step) {
	if (step >= 0 ? zoomStep != Settings::zoomLimit : zoomStep != -Settings::zoomLimit) {
		if (numWgts)
			setZoom([this](int8 zs) { zoomStep += zs; }, step);
		else
			zoomStep += step;
	}
}

template <Invocable<int8> F>
void ReaderBox::setZoom(F zset, int8 step) {
	int vi = direction.vertical();
	ivec2 sh = size() / 2;
	ivec2 vpos = listPos + sh;
	uint id = firstWidgetAt(vpos[vi]);
	int preSpace = id * spacing;
	if (int ipos = wgtRPos(id); ipos > vpos[vi])
		preSpace -= spacing - (ipos - vpos[vi]);
	int totSpace = (numWgts - 1) * spacing;
	vec2 loc = vec2(vswap(vpos[!vi], vpos[vi] - preSpace, !vi)) / vec2(vswap(getListSize()[!vi], getListSize()[vi] - totSpace, !vi));
	zset(step);
	Layout::onResize();
	setListPos(ivec2(loc * vec2(vswap(getListSize()[!vi], getListSize()[vi] - totSpace, !vi))) + vswap(0, preSpace, !vi) - sh);
}

int8 ReaderBox::zoomStepToFit(uint res) const {
	double target = double(World::drawSys()->getViewRes()[direction.horizontal()]) / double(res);
	double zoom = std::floor(std::log(target) / std::log(Settings::zoomBase));
	return int8(std::clamp(zoom, double(-Settings::zoomLimit), double(Settings::zoomLimit)));
}

void ReaderBox::centerList() {
	int di = direction.horizontal();
	listPos[di] = getListMax()[di] / 2;
}

ivec2 ReaderBox::wgtSize(uint id) const {
	return dvec2(static_cast<Picture*>(widgets[id])->getTex()->getRes()) * Settings::zoomValue(zoomStep);
}
