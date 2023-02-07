#include "layouts.h"
#include "engine/scene.h"
#include "engine/drawSys.h"
#include "engine/inputSys.h"
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
	for (sizet i = 0; i < widgets.size(); ++i)
		switch (const Size& siz = widgets[i]->getRelSize(); siz.mod) {
		case Size::rela:
			total += siz.prc;
			break;
		case Size::pixv:
			pixSizes[i] = siz.pix;
			space -= std::min(pixSizes[i], space);
			break;
		case Size::calc:
			pixSizes[i] = siz(widgets[i]);
			space -= std::min(pixSizes[i], space);
		}

	// calculate positions for each widget and set last poss element to end position of the last widget
	ivec2 pos(pad);
	for (sizet i = 0; i < widgets.size(); ++i) {
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

void Layout::navSelectNext(sizet id, int mid, Direction dir) {
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

void Layout::scanSequential(sizet id, int mid, Direction dir) {
	for (sizet mov = btom<sizet>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->navSelectable(););
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	sizet id = 0;
	for (uint hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->navSelectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); ++id);
	if (id == widgets.size())
		while (--id < widgets.size() && !widgets[id]->navSelectable());

	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void Layout::navSelectWidget(sizet id, int mid, Direction dir) {
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
		std::reverse(widgets.begin(), widgets.end());
	for (sizet i = 0; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
}

void Layout::clearWidgets() {
	for (Widget* it : widgets)
		delete it;
	widgets.clear();
}

void Layout::setWidgets(vector<Widget*>&& wgts) {
	bool updateSelect = anyWidgetsNavSelected(this);
	initWidgets(std::move(wgts));
	postInit();
	if (updateSelect) {
		World::scene()->select = nullptr;
		World::scene()->updateSelect();
	}
}

bool Layout::anyWidgetsNavSelected(Layout* box) {
	for (Widget* it : box->widgets) {
		if (World::scene()->select == it)
			return true;
		if (Layout* lay = dynamic_cast<Layout*>(it); lay && anyWidgetsNavSelected(lay))
			return true;
	}
	return false;
}

void Layout::replaceWidget(sizet id, Widget* widget) {
	bool updateSelect = World::scene()->select == widgets[id];
	delete widgets[id];
	widgets[id] = widget;
	widget->setParent(this, id);
	postInit();
	if (updateSelect)
		World::scene()->select = widgets[id];
}

void Layout::deleteWidget(sizet id) {
	bool updateSelect = World::scene()->select == widgets[id];
	delete widgets[id];
	widgets.erase(widgets.begin() + pdift(id));
	positions.pop_back();

	for (sizet i = id; i < widgets.size(); ++i)
		widgets[i]->setParent(this, i);
	postInit();
	if (updateSelect) {
		World::scene()->select = nullptr;
		World::scene()->updateSelect();
	}
}

ivec2 Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

ivec2 Layout::wgtSize(sizet id) const {
	int di = direction.vertical();
	return vswap((size() - (margin ? spacing * 2 : 0))[!di], positions[id + 1][di] - positions[id][di] - spacing, !di);
}

ivec2 Layout::listSize() const {
	return positions.back() - spacing;
}

void Layout::selectWidget(sizet id) {
	switch (selection) {
	case Select::one:
		selected = { widgets[id] };
		break;
	case Select::any:
		if (const uint8* keys = SDL_GetKeyboardState(nullptr); keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
			if (selected.empty())
				selected.insert(widgets[id]);
			else if (mvec2 lims = findMinMaxSelectedID(); outRange(id, lims.x, lims.y))
				selected.insert(widgets.begin() + pdift(id < lims.x ? id : lims.y+1), widgets.begin() + pdift((id < lims.x) ? lims.y : id+1));
			else
				selected.erase(widgets[id]);
		} else if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
			if (selected.count(widgets[id]))
				selected.erase(widgets[id]);
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
}

// POPUP

Popup::Popup(const svec2& size, vector<Widget*>&& children, Widget* first, Color background, Direction dir, int space, bool pad) :
	RootLayout(size.x, std::move(children), dir, Select::none, space, pad),
	bgColor(background),
	firstNavSelect(first),
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
	Popup(size, std::move(children), nullptr, background, dir, space, pad),
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
	Popup(size, std::move(children), first, background, dir, space, pad),
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
	listPos = glm::min(listPos, listLim());
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);

	if (motion.x != 0.f || motion.y != 0.f) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
		World::scene()->updateSelect();
	}
}

void ScrollArea::postInit() {
	Layout::postInit();
	listPos = direction.positive() ? ivec2(0) : listLim();
}

void ScrollArea::onHold(ivec2 mPos, uint8 mBut) {
	motion = vec2(0.f);	// get rid of scroll motion

	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->setCapture(this);
		if (draggingSlider = barRect().contains(mPos); draggingSlider) {
			int di = direction.vertical();
			if (int sp = sliderPos(), ss = sliderSize(); outRange(mPos[di], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[di] - ss /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(ivec2 mPos, ivec2 mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))
		moveListPos(-mMov);
	else
		moveListPos(mMov * vswap(0, -1, direction.horizontal()));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(World::winSys()->mousePos(), mBut) && !draggingSlider)
			motion = World::inputSys()->getMouseMove() * vswap(0, -1, direction.horizontal());

		draggingSlider = false;
		World::scene()->setCapture(nullptr);
	}
}

void ScrollArea::onScroll(ivec2 wMov) {
	moveListPos(vswap(wMov.x, wMov.y, direction.horizontal()));
	motion = vec2(0.f);
}

void ScrollArea::navSelectNext(sizet id, int mid, Direction dir) {
	Layout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, Direction dir) {
	Layout::navSelectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::scrollToSelected() {
	sizet cid = World::scene()->findSelectedID(this);
	if (cid >= widgets.size())
		return;

	uint di = direction.vertical();
	if (int cpos = widgets[cid]->position()[di], fpos = position()[di]; cpos < fpos)
		scrollToWidgetPos(cid);
	else if (cpos + widgets[cid]->size()[di] > fpos + size()[di])
		scrollToWidgetEnd(cid);
}

void ScrollArea::scrollToWidgetPos(sizet id) {
	int di = direction.vertical();
	listPos[di] = std::min(wgtRPos(id), listLim()[di]);
}

void ScrollArea::scrollToWidgetEnd(sizet id) {
	int di = direction.vertical();
	listPos[di] = std::max(wgtREnd(id) - size()[di], 0);
}

bool ScrollArea::scrollToNext() {
	bool dp = direction.positive();
	if (int dv = direction.vertical(); dp ? listPos[dv] >= listLim()[dv] : listPos[dv] <= 0)
		return false;

	scrollToFollowing(dp ? visibleWidgets().x + 1 : visibleWidgets().y - 2, false);
	return true;
}

bool ScrollArea::scrollToPrevious() {
	bool dp = direction.positive();
	int dv = direction.vertical();
	if (dp ? listPos[dv] <= 0 : listPos[dv] >= listLim()[dv])
		return false;

	sizet id;
	if (dp) {
		id = visibleWidgets().x;
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

void ScrollArea::scrollToFollowing(sizet id, bool prev) {
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
	listPos[di] = direction.positive() == start ? 0 : listLim()[di];
	motion = vec2(0.f);
}

Recti ScrollArea::frame() const {
	return parent ? rect().intersect(parent->frame()) : rect();
}

ivec2 ScrollArea::wgtPosition(sizet id) const {
	return Layout::wgtPosition(id) - listPos;
}

ivec2 ScrollArea::wgtSize(sizet id) const {
	return Layout::wgtSize(id) - vswap(barSize(), 0, direction.horizontal());
}

void ScrollArea::setSlider(int spos) {
	int di = direction.vertical();
	int lim = listLim()[di];
	listPos[di] = std::clamp((spos - position()[di]) * lim / sliderLim(), 0, lim);
}

int ScrollArea::barSize() const {
	int di = direction.vertical();
	return listSize()[di] > size()[di] ? Slider::barSize : 0;
}

ivec2 ScrollArea::listLim() const {
	ivec2 wsiz = size(), lsiz = listSize();
	return ivec2(wsiz.x < lsiz.x ? lsiz.x - wsiz.x : 0, wsiz.y < lsiz.y ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::wgtRPos(sizet id) const {
	return positions[id][direction.vertical()];
}

int ScrollArea::wgtREnd(sizet id) const {
	return positions[id + 1][direction.vertical()] - spacing;
}

int ScrollArea::sliderPos() const {
	int di = direction.vertical();
	return listSize()[di] > size()[di] ? position()[di] + listPos[di] * sliderLim() / listLim()[di] : position()[di];
}

int ScrollArea::sliderSize() const {
	int di = direction.vertical();
	int siz = size()[di], lts = listSize()[di];
	return siz < lts ? siz * siz / lts : siz;
}

void ScrollArea::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		mov -= scrollThrottle * dSec;
		if (mov < 0.f)
			mov = 0.f;
	} else {
		mov += scrollThrottle * dSec;
		if (mov > 0.f)
			mov = 0.f;
	}
}

Recti ScrollArea::barRect() const {
	int bs = barSize();
	ivec2 pos = position();
	ivec2 siz = size();
	return direction.vertical() ? Recti(pos.x + siz.x - bs, pos.y, bs, siz.y) : Recti(pos.x, pos.y + siz.y - bs, siz.x, bs);
}

Recti ScrollArea::sliderRect() const {
	int bs = barSize();
	return direction.vertical() ? Recti(position().x + size().x - bs, sliderPos(), bs, sliderSize()) : Recti(sliderPos(), position().y + size().y - bs, sliderSize(), bs);
}

mvec2 ScrollArea::visibleWidgets() const {
	mvec2 ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	int di = direction.vertical();
	for (; ival.x < widgets.size() && wgtREnd(ival.x) < listPos[di]; ++ival.x);

	ival.y = ival.x + 1;	// last is one greater than the actual last index
	for (int end = listPos[di] + size()[di]; ival.y < widgets.size() && wgtRPos(ival.y) <= end; ++ival.y);
	return ival;
}

// TILE BOX

TileBox::TileBox(const Size& size, vector<Widget*>&& children, int childHeight, Direction dir, Select select, int space, bool pad) :
	ScrollArea(size, std::move(children), dir, select, space, pad),
	wheight(childHeight)
{}

void TileBox::calculateWidgetPositions() {
	positions[0] = ivec2(0);
	int wsiz = size()[direction.horizontal()] - Slider::barSize;
	ivec2 pos(!widgets.empty() ? widgets[0]->sizeToPixAbs(widgets[0]->getRelSize(), wsiz) + spacing : 0, 0);
	for (sizet i = 1; i < widgets.size(); ++i) {
		if (int end = pos.x + widgets[i]->sizeToPixAbs(widgets[i]->getRelSize(), wsiz); end > wsiz && positions[i - 1].y == pos.y) {
			pos = ivec2(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = ivec2(wsiz, pos.y + wheight) + spacing;
}

void TileBox::navSelectNext(sizet id, int mid, Direction dir) {
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

void TileBox::scanVertically(sizet id, int mid, Direction dir) {
	if (int ypos = widgets[id]->position().y; dir.positive())
		while (++id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x + widgets[id]->size().x < mid));
	else
		while (--id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x > mid));
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanHorizontally(sizet id, int mid, Direction dir) {
	for (sizet mov = btom<sizet>(dir.positive()); (id += mov) < widgets.size() && !widgets[id]->navSelectable(););
	if (id < widgets.size() && widgets[id]->center().y == mid)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

void TileBox::scanFromStart(int mid, Direction dir) {
	sizet id = 0;
	for (uint di = dir != Direction::down; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] + widgets[id]->size()[di] < mid); ++id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, Direction dir) {
	sizet id = widgets.size() - 1;
	for (uint di = dir != Direction::up; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] > mid); --id);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::navSelectIfInRange(sizet id, int mid, Direction dir) {
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(index, mid, dir);
}

ivec2 TileBox::wgtSize(sizet id) const {
	return ivec2(widgets[id]->getRelSize().pix, wheight);
}

int TileBox::wgtREnd(sizet id) const {
	return positions[id].y + wheight;
}

// READER BOX

ReaderBox::ReaderBox(const Size& size, Direction dir, float fzoom, int space, bool pad) :
	ScrollArea(size, {}, dir, Select::none, space, pad),
	zoom(fzoom)
{}

ReaderBox::~ReaderBox() {
	for (auto& [name, tex] : pics)
		World::drawSys()->freeTexture(tex);
}

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

void ReaderBox::postInit() {
	ScrollArea::postInit();

	// scroll down to opened picture if it exists, otherwise start at beginning
	string file = World::browser()->getCurFile().u8string();
	if (size_t id = std::find_if(pics.begin(), pics.end(), [&file](const pair<string, Texture*>& it) -> bool { return it.first == file; }) - pics.begin(); id < pics.size()) {
		if (direction.positive())
			scrollToWidgetPos(id);
		else
			scrollToWidgetEnd(id);
	}
	centerList();
}

void ReaderBox::calculateWidgetPositions() {
	// figure out the width of the list
	int hi = direction.horizontal();
	int maxRSiz = size()[hi];
	for (const pair<string, Texture*>& it : pics)
		if (int rsiz = int(float(it.second->getRes()[hi]) * zoom); rsiz > maxRSiz)
			maxRSiz = rsiz;

	// set position of each picture
	int rpos = 0;
	for (sizet i = 0; i < widgets.size(); ++i) {
		ivec2 psz = vec2(pics[i].second->getRes()) * zoom;
		positions[i] = vswap((maxRSiz - psz[hi]) / 2, rpos, hi);
		rpos += psz[!hi];
	}
	positions.back() = vswap(maxRSiz, rpos, hi);
}

void ReaderBox::onMouseMove(ivec2 mPos, ivec2 mMov) {
	Layout::onMouseMove(mPos, mMov);

	countDown = World::scene()->getSelectedScrollArea() == this && !showBar() && World::scene()->getCapture() != this && cursorTimer > 0.f;
	if (cursorTimer < menuHideTimeout) {
		cursorTimer = menuHideTimeout;
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void ReaderBox::setWidgets(vector<pair<string, Texture*>>&& imgs) {
	clearWidgets();
	pics = std::move(imgs);
	widgets.resize(pics.size());
	positions.resize(pics.size()+1);

	if (direction.negative())
		std::reverse(pics.begin(), pics.end());
	for (sizet i = 0; i < pics.size(); ++i) {
		widgets[i] = new Picture(0, false, pics[i].second, 0);
		widgets[i]->setParent(this, i);
	}
	postInit();
}

bool ReaderBox::showBar() const {
	return barRect().contains(World::winSys()->mousePos()) || draggingSlider;
}

void ReaderBox::setZoom(float factor) {
	ivec2 sh = size() / 2;
	zoom *= factor;
	listPos = glm::clamp(ivec2(vec2(listPos + sh) * factor) - sh, ivec2(0), listLim());
	onResize();
}

void ReaderBox::centerList() {
	int di = direction.horizontal();
	listPos[di] = listLim()[di] / 2;
}

ivec2 ReaderBox::wgtPosition(sizet id) const {
	return position() + positions[id] + vswap(0, int(id) * spacing, direction.horizontal()) - listPos;
}

ivec2 ReaderBox::wgtSize(sizet id) const {
	return vec2(pics[id].second->getRes()) * zoom;
}

ivec2 ReaderBox::listSize() const {
	return positions.back() + vswap(0, int(widgets.size() - 1) * spacing, direction.horizontal());
}

int ReaderBox::wgtRPos(sizet id) const {
	return positions[id][direction.vertical()] + id * spacing;
}

int ReaderBox::wgtREnd(sizet id) const {
	return positions[id + 1][direction.vertical()] + id * spacing;
}
