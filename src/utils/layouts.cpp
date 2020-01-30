#include "engine/world.h"

// LAYOUT

Layout::Layout(const Size& relSize, vector<Widget*>&& children, Direction direction, Select select, int spacing, Layout* parent, sizet id) :
	Widget(relSize, parent, id),
	spacing(spacing),
	selection(select),
	direction(direction)
{
	initWidgets(std::move(children));
}

Layout::~Layout() {
	clear(widgets);
}

void Layout::drawSelf() const {
	for (Widget* it : widgets)
		it->drawSelf();
}

void Layout::onResize() {
	// get amount of space for widgets with prc and get sum of widget's prc
	sizet vi = direction.vertical();
	vec2i wsiz = size();
	float space = float(wsiz[vi] - int(widgets.size()-1) * spacing);
	float total = 0;
	for (Widget* it : widgets) {
		if (it->getRelSize().usePix)
			space -= it->getRelSize().pix;
		else
			total += it->getRelSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	vec2i pos(0);
	for (sizet i = 0; i < widgets.size(); i++) {
		positions[i] = pos;
		pos[vi] += (widgets[i]->getRelSize().usePix ? widgets[i]->getRelSize().pix : int(widgets[i]->getRelSize().prc * space / total)) + spacing;
	}
	positions.back() = vec2i::swap(wsiz[!vi], pos[vi], !vi);

	// do the same for children
	for (Widget* it : widgets)
		it->onResize();
}

void Layout::tick(float dSec) {
	for (Widget* it : widgets)
		it->tick(dSec);
}

void Layout::postInit() {
	onResize();
	for (Widget* it : widgets)
		it->postInit();
}

void Layout::onMouseMove(vec2i mPos, vec2i mMov) {
	for (Widget* it : widgets)
		it->onMouseMove(mPos, mMov);
}

bool Layout::navSelectable() const {
	return !widgets.empty();
}

void Layout::navSelectNext(sizet id, int mid, Direction dir) {
	if (dir.vertical() == direction.vertical()) {
		if (dir.positive() ? id >= widgets.size()-1 : id == 0) {
			if (parent)
				parent->navSelectNext(pcID, mid, dir);
		} else
			scanSequential(id, mid, dir);
	} else if (parent)
		parent->navSelectNext(pcID, mid, dir);
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
}

void Layout::scanPerpendicular(int mid, Direction dir) {
	sizet id = 0;
	for (uint hori = dir.horizontal(); id < widgets.size() && (!widgets[id]->navSelectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)); id++);

	if (id == widgets.size())
		while (--id > 0 && !widgets[id]->navSelectable());
	navSelectWidget(id, mid, dir);
}

void Layout::navSelectWidget(sizet id, int mid, Direction dir) {
	if (Layout* lay = dynamic_cast<Layout*>(widgets[id]))
		lay->navSelectFrom(mid, dir);
	else if (widgets[id]->navSelectable())
		World::scene()->select = widgets[id];
}

void Layout::initWidgets(vector<Widget*>&& wgts) {
	clear(widgets);
	widgets = std::move(wgts);
	positions.resize(widgets.size() + 1);

	if (direction.negative())
		std::reverse(widgets.begin(), widgets.end());
	for (sizet i = 0; i < widgets.size(); i++)
		widgets[i]->setParent(this, i);
}

void Layout::setWidgets(vector<Widget*>&& wgts) {
	initWidgets(std::move(wgts));
	postInit();
}

void Layout::replaceWidget(sizet id, Widget* widget) {
	delete widgets[id];
	widgets[id] = widget;
	widget->setParent(this, id);
	postInit();
}

void Layout::deleteWidget(sizet id) {
	delete widgets[id];
	widgets.erase(widgets.begin() + pdift(id));
	positions.pop_back();

	for (sizet i = id; i < widgets.size(); i++)
		widgets[i]->setParent(this, i);
	postInit();
}

vec2i Layout::wgtPosition(sizet id) const {
	return position() + positions[id];
}

vec2i Layout::wgtSize(sizet id) const {
	sizet di = direction.vertical();
	return vec2i::swap(size()[!di], positions[id+1][di] - positions[id][di] - spacing, !di);
}

vec2i Layout::listSize() const {
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
			else if (vec2t lims = findMinMaxSelectedID(); outRange(id, lims.b, lims.t))
				selected.insert(widgets.begin() + pdift(id < lims.b ? id : lims.t+1), widgets.begin() + pdift((id < lims.b) ? lims.t : id+1));
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

vec2t Layout::findMinMaxSelectedID() const {
	vec2t idm = (*selected.begin())->getID();
	for (uset<Widget*>::const_iterator it = std::next(selected.begin()); it != selected.end(); it++) {
		if ((*it)->getID() < idm.b)
			idm.b = (*it)->getID();
		else if ((*it)->getID() > idm.t)
			idm.t = (*it)->getID();
	}
	return idm;
}

// ROOT LAYOUT

RootLayout::RootLayout(const Size& relSize, vector<Widget*>&& children, Direction direction, Select select, int spacing) :
	Layout(relSize, std::move(children), direction, select, spacing)
{}

vec2i RootLayout::position() const {
	return 0;
}

vec2i RootLayout::size() const {
	return World::drawSys()->viewport().size();
}

Rect RootLayout::frame() const {
	return World::drawSys()->viewport();
}

// POPUP

Popup::Popup(const vec2s& relSize, vector<Widget*>&& children, Direction direction, int spacing) :
	RootLayout(relSize.x, std::move(children), direction, Select::none, spacing),
	sizeY(relSize.y)
{}

void Popup::drawSelf() const {
	World::drawSys()->drawPopup(this);
}

vec2i Popup::position() const {
	return (World::drawSys()->viewport().size() - size()) / 2;
}

vec2i Popup::size() const {
	vec2f res = World::drawSys()->viewport().size();
	return vec2i(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

// OVERLAY

Overlay::Overlay(const vec2s& position, const vec2s& relSize, const vec2s& activationPos, const vec2s& activationSize, vector<Widget*>&& children, Direction direction, int spacing) :
	Popup(relSize, std::move(children), direction, spacing),
	on(false),
	pos(position),
	actPos(activationPos),
	actSize(activationSize)
{}

vec2i Overlay::position() const {
	vec2f res = World::drawSys()->viewport().size();
	return vec2i(pos.x.usePix ? pos.x.pix : pos.x.prc * res.x, pos.y.usePix ? pos.y.pix : pos.y.prc * res.y);
}

Rect Overlay::actRect() const {
	vec2f res = World::drawSys()->viewport().size();
	return Rect(actPos.x.usePix ? actPos.x.pix : int(actPos.x.prc * res.x), actPos.y.usePix ? actPos.y.pix : int(actPos.y.prc * res.y), actSize.x.usePix ? actSize.x.pix : int(actSize.x.prc * res.x), actSize.y.usePix ? actSize.y.pix : int(actSize.y.prc * res.y));
}

// SCROLL AREA

ScrollArea::ScrollArea(const Size& relSize, vector<Widget*>&& children, Direction direction, Select select, int spacing, Layout* parent, sizet id) :
	Layout(relSize, std::move(children), direction, select, spacing, parent, id),
	draggingSlider(false),
	listPos(0),
	motion(0.f),
	diffSliderMouse(0)
{}

void ScrollArea::drawSelf() const {
	World::drawSys()->drawScrollArea(this);
}

void ScrollArea::onResize() {
	Layout::onResize();
	listPos = listPos.min(listLim());
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);

	if (motion != 0.f) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
	}
}

void ScrollArea::postInit() {
	Layout::postInit();
	listPos = direction.positive() ? 0 : listLim();
}

void ScrollArea::onHold(vec2i mPos, uint8 mBut) {
	motion = 0.f;	// get rid of scroll motion

	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->capture = this;
		if (draggingSlider = barRect().contain(mPos)) {
			sizet di = direction.vertical();
			if (int sp = sliderPos(), ss = sliderSize(); outRange(mPos[di], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[di] - ss /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(vec2i mPos, vec2i mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT))
		moveListPos(-mMov);
	else
		moveListPos(mMov * vec2i::swap(0, -1, direction.horizontal()));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!World::scene()->cursorInClickRange(mousePos(), mBut) && !draggingSlider)
			motion = World::inputSys()->getMouseMove() * vec2i::swap(0, -1, direction.horizontal());

		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(vec2i wMov) {
	moveListPos(wMov.swap(direction.horizontal()));
	motion = 0.f;
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
	sizet di = direction.vertical();
	listPos[di] = std::min(wgtRPos(id), listLim()[di]);
}

void ScrollArea::scrollToWidgetEnd(sizet id) {
	sizet di = direction.vertical();
	listPos[di] = std::max(wgtREnd(id) - size()[di], 0);
}

bool ScrollArea::scrollToNext() {
	bool dp = direction.positive();
	if (sizet dv = direction.vertical(); dp ? listPos[dv] >= listLim()[dv] : listPos[dv] <= 0)
		return false;

	scrollToFollowing(dp ? visibleWidgets().b + 1 : visibleWidgets().t - 2, false);
	return true;
}

bool ScrollArea::scrollToPrevious() {
	bool dp = direction.positive();
	sizet dv = direction.vertical();
	if (dp ? listPos[dv] <= 0 : listPos[dv] >= listLim()[dv])
		return false;

	sizet id;
	if (dp) {
		id = visibleWidgets().b;
		if (listPos[dv] <= wgtRPos(id))
			id--;
	} else {
		id = visibleWidgets().t - 1;
		if (listPos[dv] + size()[dv] >= wgtREnd(id))
			id++;
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
	motion = 0.f;
}

void ScrollArea::scrollToLimit(bool start) {
	sizet di = direction.vertical();
	listPos[di] = direction.positive() == start ? 0 : listLim()[di];
	motion = 0.f;
}

Rect ScrollArea::frame() const {
	return parent ? rect().intersect(parent->frame()) : rect();
}

vec2i ScrollArea::wgtPosition(sizet id) const {
	return Layout::wgtPosition(id) - listPos;
}

vec2i ScrollArea::wgtSize(sizet id) const {
	return Layout::wgtSize(id) - vec2i::swap(barSize(), 0, direction.horizontal());
}

void ScrollArea::setSlider(int spos) {
	sizet di = direction.vertical();
	int lim = listLim()[di];
	listPos[di] = std::clamp((spos - position()[di]) * lim / sliderLim(), 0, lim);
}

int ScrollArea::barSize() const {
	sizet di = direction.vertical();
	return listSize()[di] > size()[di] ? Slider::barSize : 0;
}

vec2i ScrollArea::listLim() const {
	vec2i wsiz = size(), lsiz = listSize();
	return vec2i(wsiz.x < lsiz.x ? lsiz.x - wsiz.x : 0, wsiz.y < lsiz.y ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::wgtRPos(sizet id) const {
	return positions[id][direction.vertical()];
}

int ScrollArea::wgtREnd(sizet id) const {
	return positions[id+1][direction.vertical()] - spacing;
}

int ScrollArea::sliderPos() const {
	sizet di = direction.vertical();
	return listSize()[di] > size()[di] ? position()[di] + listPos[di] * sliderLim() / listLim()[di] : position()[di];
}

int ScrollArea::sliderSize() const {
	sizet di = direction.vertical();
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

Rect ScrollArea::barRect() const {
	int bs = barSize();
	vec2i pos = position();
	vec2i siz = size();
	return direction.vertical() ? Rect(pos.x + siz.x - bs, pos.y, bs, siz.y) : Rect(pos.x, pos.y + siz.y - bs, siz.x, bs);
}

Rect ScrollArea::sliderRect() const {
	int bs = barSize();
	return direction.vertical() ? Rect(position().x + size().x - bs, sliderPos(), bs, sliderSize()) : Rect(sliderPos(), position().y + size().y - bs, sliderSize(), bs);
}

vec2t ScrollArea::visibleWidgets() const {
	vec2t ival(0);
	if (widgets.empty())	// nothing to draw
		return ival;

	sizet di = direction.vertical();
	for (; ival.b < widgets.size() && wgtREnd(ival.b) < listPos[di]; ival.b++);

	ival.t = ival.b + 1;	// last is one greater than the actual last index
	for (int end = listPos[di] + size()[di]; ival.t < widgets.size() && wgtRPos(ival.t) <= end; ival.t++);
	return ival;
}

// TILE BOX

TileBox::TileBox(const Size& relSize, vector<Widget*>&& children, int childHeight, Direction direction, Select select, int spacing, Layout* parent, sizet id) :
	ScrollArea(relSize, std::move(children), direction, select, spacing, parent, id),
	wheight(childHeight)
{}

void TileBox::onResize() {
	int wsiz = size()[direction.horizontal()] - Slider::barSize;
	vec2i pos(0);
	for (sizet i = 0; i < widgets.size(); i++) {
		if (int end = pos.x + widgets[i]->getRelSize().pix; end > wsiz && i && positions[i-1].y == pos.y) {
			pos = vec2i(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = vec2i(wsiz, pos.y + wheight) + spacing;
	listPos = listPos.min(listLim());

	for (Widget* it : widgets)
		it->onResize();
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
		parent->navSelectNext(pcID, mid, dir);
}

void TileBox::scanFromStart(int mid, Direction dir) {
	sizet id = 0;
	for (uint di = dir != Direction::down; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] + widgets[id]->size()[di] < mid); id++);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, Direction dir) {
	sizet id = widgets.size() - 1;
	for (uint di = dir != Direction::up; id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] > mid); id--);
	navSelectIfInRange(id, mid, dir);
}

void TileBox::navSelectIfInRange(sizet id, int mid, Direction dir) {
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

vec2i TileBox::wgtSize(sizet id) const {
	return vec2i(widgets[id]->getRelSize().pix, wheight);
}

int TileBox::wgtREnd(sizet id) const {
	return positions[id].y + wheight;
}

// READER BOX

ReaderBox::ReaderBox(const Size& relSize, vector<Texture>&& imgs, Direction direction, float zoom, int spacing, Layout* parent, sizet id) :
	ScrollArea(relSize, {}, direction, Select::none, spacing, parent, id),
	cursorTimer(menuHideTimeout),
	zoom(zoom),
	countDown(true)
{
	initWidgets(std::move(imgs));
}

ReaderBox::~ReaderBox() {
	clearTexVec(pics);
}

void ReaderBox::drawSelf() const {
	World::drawSys()->drawReaderBox(this);
}

void ReaderBox::onResize() {
	// figure out the width of the list
	sizet hi = direction.horizontal();
	int maxRSiz = size()[hi];
	for (const Texture& it : pics)
		if (int rsiz = int(float(it.res()[hi]) * zoom); rsiz > maxRSiz)
			maxRSiz = rsiz;

	// set position of each picture
	int rpos = 0;
	for (sizet i = 0; i < widgets.size(); i++) {
		vec2i psz = vec2f(pics[i].res()) * zoom;
		positions[i] = vec2i::swap((maxRSiz - psz[hi]) / 2, rpos, hi);
		rpos += psz[!hi];
	}
	positions.back() = vec2i::swap(maxRSiz, rpos, hi);
	listPos = listPos.min(listLim());

	// for good measure?
	for (Widget* it : widgets)
		it->onResize();
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
	if (size_t id = sizet(std::find_if(pics.begin(), pics.end(), [](const Texture& it) -> bool { return it.name == World::browser()->getCurFile(); }) - pics.begin()); id < pics.size()) {
		if (direction.positive())
			scrollToWidgetPos(id);
		else
			scrollToWidgetEnd(id);
	}
	centerList();
}

void ReaderBox::onMouseMove(vec2i mPos, vec2i mMov) {
	Layout::onMouseMove(mPos, mMov);

	countDown = World::scene()->cursorDisableable() && rect().contain(mPos) && !showBar() && World::scene()->capture != this && cursorTimer > 0.f;
	if (cursorTimer < menuHideTimeout) {
		cursorTimer = menuHideTimeout;
		SDL_ShowCursor(SDL_ENABLE);
	}
}

void ReaderBox::initWidgets(vector<Texture>&& imgs) {
	clearTexVec(pics);
	clear(widgets);

	pics = std::move(imgs);
	widgets.resize(pics.size());
	positions.resize(pics.size()+1);

	if (direction.negative())
		std::reverse(pics.begin(), pics.end());
	for (sizet i = 0; i < pics.size(); i++)
		widgets[i] = new Picture(0, false, pics[i].tex, 0, this, i);
}

void ReaderBox::setWidgets(vector<Texture>&& imgs) {
	initWidgets(std::move(imgs));
	postInit();
}

void ReaderBox::setZoom(float factor) {
	vec2i sh = size() / 2;
	zoom *= factor;
	listPos = (vec2i(vec2f(listPos + sh) * factor) - sh).clamp(0, listLim());
	onResize();
}

void ReaderBox::centerList() {
	sizet di = direction.horizontal();
	listPos[di] = listLim()[di] / 2;
}

vec2i ReaderBox::wgtPosition(sizet id) const {
	return position() + positions[id] + vec2i::swap(0, int(id) * spacing, direction.horizontal()) - listPos;
}

vec2i ReaderBox::wgtSize(sizet id) const {
	return vec2f(pics[id].res()) * zoom;
}

vec2i ReaderBox::listSize() const {
	return positions.back() + vec2i::swap(0, int(widgets.size()-1) * spacing, direction.horizontal());
}

int ReaderBox::wgtRPos(sizet id) const {
	return positions[id][direction.vertical()] + int(id) * spacing;
}

int ReaderBox::wgtREnd(sizet id) const {
	return positions[id+1][direction.vertical()] + int(id) * spacing;
}
