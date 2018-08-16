#include "engine/world.h"

// LAYOUT

Layout::Layout(const Size& relSize, const vector<Widget*>& children, const Direction& direction, Select select, int spacing, Layout* parent, sizt id) :
	Widget(relSize, parent, id),
	widgets(children),
	positions(children.size() + 1),
	direction(direction),
	selection(select),
	spacing(spacing)
{
	if (direction.negative())
		std::reverse(widgets.begin(), widgets.end());
	for (sizt i = 0; i < widgets.size(); i++)
		widgets[i]->setParent(this, i);
}

Layout::~Layout() {
	for (Widget* it : widgets)
		delete it;
}

void Layout::drawSelf() {
	for (Widget* it : widgets)
		it->drawSelf();
}

void Layout::onResize() {
	// get amount of space for widgets with prc and get sum of widget's prc
	bool vert = direction.vertical();
	vec2i wsiz = size();
	float space = wsiz[vert] - (widgets.size()-1) * spacing;
	float total = 0;
	for (Widget* it : widgets) {
		if (it->getRelSize().usePix)
			space -= it->getRelSize().pix;
		else
			total += it->getRelSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	vec2i pos;
	for (sizt i = 0; i < widgets.size(); i++) {
		positions[i] = pos;
		pos[vert] += (widgets[i]->getRelSize().usePix ? widgets[i]->getRelSize().pix : widgets[i]->getRelSize().prc * space / total) + spacing;
	}
	positions.back() = vec2i(wsiz[!vert], pos[vert], !vert);

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

void Layout::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	for (Widget* it : widgets)
		it->onMouseMove(mPos, mMov);
}

void Layout::navSelectNext(sizt id, int mid, const Direction& dir) {
	if (dir.vertical() == direction.vertical()) {
		bool fwd = dir.positive();
		if ((!fwd && id == 0) || (fwd && id >= widgets.size()-1)) {
			if (parent)
				parent->navSelectNext(pcID, mid, dir);
		} else
			scanSequential(id, mid, dir);
	} else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

void Layout::navSelectFrom(int mid, const Direction& dir) {
	if (dir.vertical() == direction.vertical())
		scanSequential(dir.positive() ? widgets.size() : SIZE_MAX, mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::scanSequential(sizt id, int mid, const Direction& dir) {
	int8 mov = dir.positive() ? 1 : -1;
	while ((id += mov) < widgets.size() && !widgets[id]->navSelectable());
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
}

void Layout::scanPerpendicular(int mid, const Direction& dir) {
	int8 hori = dir.horizontal();
	sizt id = 0;
	while (id < widgets.size() && (!widgets[id]->navSelectable() || (wgtPosition(id)[hori] + wgtSize(id)[hori] < mid)))
		id++;

	if (id == widgets.size())
		while (--id > 0 && !widgets[id]->navSelectable());
	navSelectWidget(id, mid, dir);
}

void Layout::navSelectWidget(sizt id, int mid, const Direction& dir) {
	if (Layout* lay = dynamic_cast<Layout*>(widgets[id]))
		lay->navSelectFrom(mid, dir);
	else if (dynamic_cast<Button*>(widgets[id]))
		World::scene()->select = widgets[id];
}

vec2i Layout::position() const {
	return parent ? parent->wgtPosition(pcID) : 0;
}

vec2i Layout::size() const {
	return parent ? parent->wgtSize(pcID) : World::drawSys()->viewSize();
}

SDL_Rect Layout::frame() const {
	return parent ? parent->frame() : World::drawSys()->viewport();
}

vec2i Layout::wgtPosition(sizt id) const {
	return position() + positions[id];
}

vec2i Layout::wgtSize(sizt id) const {
	bool vert = direction.vertical();
	return vec2i(size()[!vert], positions[id+1][vert] - positions[id][vert] - spacing, !vert);
}

vec2i Layout::listSize() const {
	return positions.back() - spacing;
}

void Layout::selectWidget(sizt id) {
	if (selection == Select::one)
		selectSingleWidget(id);
	else if (selection == Select::any) {
		const uint8* keys = SDL_GetKeyboardState(nullptr);
		if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
			if (selected.size()) {
				vec2t lims = findMinMaxSelectedID();
				if (outRange(id, lims.l, lims.u))
					selected.insert(widgets.begin() + (id < lims.l ? id : lims.u+1), widgets.begin() + ((id < lims.l) ? lims.u : id+1));
				else
					selected.erase(widgets[id]);
			} else
				selected.insert(widgets[id]);
		} else if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) {
			if (selected.count(widgets[id]))
				selected.erase(widgets[id]);
			else
				selected.insert(widgets[id]);
		} else
			selectSingleWidget(id);
	}
}

void Layout::selectSingleWidget(sizt id) {
	if (selected.size()) {
		selected.clear();
		selected.insert(widgets[id]);
	} else
		selected.insert(widgets[id]);
}

vec2t Layout::findMinMaxSelectedID() const {
	vec2t idm = (*selected.begin())->getID();
	for (uset<Widget*>::const_iterator it = std::next(selected.begin()); it != selected.end(); it++) {
		if ((*it)->getID() < idm.l)
			idm.l = (*it)->getID();
		else if ((*it)->getID() > idm.u)
			idm.u = (*it)->getID();
	}
	return idm;
}

// POPUP

Popup::Popup(const vec2s& relSize, const vector<Widget*>& children, const Direction& direction, int spacing) :
	Layout(relSize.x, children, direction, Select::none, spacing, nullptr, SIZE_MAX),
	sizeY(relSize.y)
{}

void Popup::drawSelf() {
	World::drawSys()->drawPopup(this);
}

vec2i Popup::position() const {
	return (World::drawSys()->viewSize() - size()) / 2;
}

vec2i Popup::size() const {
	vec2f res = World::drawSys()->viewSize();
	return vec2i(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

SDL_Rect Popup::frame() const {
	return World::drawSys()->viewport();
}

// OVERLAY

Overlay::Overlay(const vec2s& position, const vec2s& relSize, const vec2s& activationPos, const vec2s& activationSize, const vector<Widget*>& children, const Direction& direction, int spacing) :
	Popup(relSize, children, direction, spacing),
	pos(position),
	actPos(activationPos),
	actSize(activationSize),
	on(false)
{}

vec2i Overlay::position() const {
	vec2f res = World::drawSys()->viewSize();
	return vec2i(pos.x.usePix ? pos.x.pix : pos.x.prc * res.x, pos.y.usePix ? pos.y.pix : pos.y.prc * res.y);
}

SDL_Rect Overlay::actRect() const {
	vec2f res = World::drawSys()->viewSize();
	vec2i ps(actPos.x.usePix ? actPos.x.pix : actPos.x.prc * res.x, actPos.y.usePix ? actPos.y.pix : actPos.y.prc * res.y);
	vec2i sz(actSize.x.usePix ? actSize.x.pix : actSize.x.prc * res.x, actSize.y.usePix ? actSize.y.pix : actSize.y.prc * res.y);
	return {ps.x, ps.y, sz.x, sz.y};
}

// SCROLL AREA

ScrollArea::ScrollArea(const Size& relSize, const vector<Widget*>& children, const Direction& direction, Select select, int spacing, Layout* parent, sizt id) :
	Layout(relSize, children, direction, select, spacing, parent, id),
	listPos(0),
	motion(0.f),
	diffSliderMouse(0),
	draggingSlider(false)
{}

void ScrollArea::drawSelf() {
	World::drawSys()->drawScrollArea(this);
}

void ScrollArea::onResize() {
	Layout::onResize();
	listPos = bringUnder(listPos, listLim());
}

void ScrollArea::tick(float dSec) {
	Layout::tick(dSec);

	if (motion != 0.f) {
		moveListPos(motion);
		throttleMotion(motion.x, dSec);
		throttleMotion(motion.y, dSec);
	}
}

void ScrollArea::onHold(const vec2i& mPos, uint8 mBut) {
	motion = 0.f;	// get rid of scroll motion

	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->capture = this;

		draggingSlider = inRect(mPos, barRect());
		if (draggingSlider) {
			int8 di = direction.vertical();
			int sp = sliderPos();
			int ss = sliderSize();
			if (outRange(mPos[di], sp, sp + ss))	// if mouse outside of slider but inside bar
				setSlider(mPos[di] - ss /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(const vec2i& mPos, const vec2i& mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (InputSys::isPressedM(SDL_BUTTON_RIGHT))
		moveListPos(-mMov);
	else
		moveListPos(mMov * vec2i(0, -1, direction.horizontal()));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!(World::scene()->cursorInClickRange(InputSys::mousePos(), mBut) || draggingSlider))
			motion = World::inputSys()->getMouseMove() * vec2i(0, -1, direction.horizontal());

		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(const vec2i& wMov) {
	moveListPos(wMov.swap(direction.horizontal()));
}

void ScrollArea::moveListPos(const vec2i& mov) {
	listPos = bringIn(listPos + mov, vec2i(0), listLim());
}

void ScrollArea::navSelectNext(sizt id, int mid, const Direction& dir) {
	Layout::navSelectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::navSelectFrom(int mid, const Direction& dir) {
	Layout::navSelectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::scrollToSelected() {
	sizt cid = World::scene()->findSelectedID(this);
	if (cid >= widgets.size())
		return;

	int8 di = direction.vertical();
	int cpos = widgets[cid]->position()[di];
	int fpos = position()[di];
	if (cpos < fpos)
		scrollToWidgetPos(cid);
	else if (cpos + widgets[cid]->size()[di] > fpos + size()[di])
		scrollToWidgetEnd(cid);
}

void ScrollArea::scrollToWidgetPos(sizt id) {
	listPos[direction.vertical()] = wgtRPos(id);
}

void ScrollArea::scrollToWidgetEnd(sizt id) {
	int8 di = direction.vertical();
	listPos[di] = wgtREnd(id) - size()[di];
}

void ScrollArea::setSlider(int spos) {
	int8 di = direction.vertical();
	int lim = listLim()[di];
	listPos[di] = bringIn((spos - position()[di]) * lim / sliderLim(), 0, lim);
}

int ScrollArea::barSize() const {
	int8 di = direction.vertical();
	return listSize()[di] > size()[di] ? Default::sbarSize : 0;
}

vec2i ScrollArea::listLim() const {
	vec2i wsiz = size();
	vec2i lsiz = listSize();
	return vec2i(wsiz.x < lsiz.x ? lsiz.x - wsiz.x : 0, wsiz.y < lsiz.y ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::sliderPos() const {
	int8 di = direction.vertical();
	return listSize()[di] > size()[di] ? position()[di] + listPos[di] * sliderLim() / listLim()[di] : position()[di];
}

int ScrollArea::sliderSize() const {
	int8 di = direction.vertical();
	int siz = size()[di];
	int lts = listSize()[di];
	return siz < lts ? siz * siz / lts : siz;
}

int ScrollArea::sliderLim() const {
	return size()[direction.vertical()] - sliderSize();
}

void ScrollArea::throttleMotion(float& mov, float dSec) {
	if (mov > 0.f) {
		mov -= Default::scrollThrottle * dSec;
		if (mov < 0.f)
			mov = 0.f;
	} else {
		mov += Default::scrollThrottle * dSec;
		if (mov > 0.f)
			mov = 0.f;
	}
}

SDL_Rect ScrollArea::frame() const {
	return parent ? overlapRect(rect(), parent->frame()) : rect();
}

vec2i ScrollArea::wgtPosition(sizt id) const {
	return Layout::wgtPosition(id) - listPos;
}

vec2i ScrollArea::wgtSize(sizt id) const {
	return Layout::wgtSize(id) - vec2i(barSize(), 0, direction.horizontal());
}

SDL_Rect ScrollArea::barRect() const {
	int bs = barSize();
	vec2i pos = position();
	vec2i siz = size();
	return direction.vertical() ? SDL_Rect({pos.x + siz.x - bs, pos.y, bs, siz.y}) : SDL_Rect({pos.x, pos.y + siz.y - bs, siz.x, bs});
}

SDL_Rect ScrollArea::sliderRect() const {
	int bs = barSize();
	return direction.vertical() ? SDL_Rect({position().x + size().x - bs, sliderPos(), bs, sliderSize()}) : SDL_Rect({sliderPos(), position().y + size().y - bs, sliderSize(), bs});
}

vec2t ScrollArea::visibleWidgets() const {
	vec2t ival;
	if (widgets.empty())	// nothing to draw
		return ival;

	int8 di = direction.vertical();
	while (ival.l < widgets.size() && wgtREnd(ival.l) < listPos[di])
		ival.l++;

	int end = listPos[di] + size()[di];
	ival.u = ival.l + 1;	// last is one greater than the actual last index
	while (ival.u < widgets.size() && wgtRPos(ival.u) <= end)
		ival.u++;
	return ival;
}

int ScrollArea::wgtRPos(sizt id) const {
	return positions[id][direction.vertical()];
}

int ScrollArea::wgtREnd(sizt id) const {
	return positions[id+1][direction.vertical()] - spacing;
}

// TILE BOX

TileBox::TileBox(const Size& relSize, const vector<Widget*>& children, int childHeight, const Direction& direction, Select select, int spacing, Layout* parent, sizt id) :
	ScrollArea(relSize, children, direction, select, spacing, parent, id),
	wheight(childHeight)
{}

void TileBox::onResize() {
	bool vert = direction.vertical();
	int wsiz = size()[!vert] - Default::sbarSize;
	vec2i pos;
	for (sizt i = 0; i < widgets.size(); i++) {
		int end = pos.x + widgets[i]->getRelSize().pix;
		if (end > wsiz) {
			pos = vec2i(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = vec2i(wsiz, pos.y + wheight) + spacing;
	listPos = bringUnder(listPos, listLim());

	for (Widget* it : widgets)
		it->onResize();
}

void TileBox::navSelectNext(sizt id, int mid, const Direction& dir) {
	if (dir.vertical())
		scanVertically(id, mid, dir);
	else
		scanHorizontally(id, mid, dir);
	scrollToSelected();
}

void TileBox::navSelectFrom(int mid, const Direction& dir) {
	if (dir.positive())
		scanFromStart(mid, dir);
	else
		scanFromEnd(mid, dir);
	scrollToSelected();
}

void TileBox::scanVertically(sizt id, int mid, const Direction& dir) {
	int ypos = widgets[id]->position().y;
	if (dir.positive())
		while (++id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x + widgets[id]->size().x < mid));
	else
		while (--id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position().y == ypos || widgets[id]->position().x > mid));
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanHorizontally(sizt id, int mid, const Direction& dir) {
	int8 mov = dir.positive() ? 1 : -1;
	while ((id += mov) < widgets.size() && !widgets[id]->navSelectable());
	if (id < widgets.size() && widgets[id]->center().y == mid)
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

void TileBox::scanFromStart(int mid, const Direction& dir) {
	int8 di = dir != Direction::down;
	sizt id = 0;
	while (id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] + widgets[id]->size()[di] < mid))
		id++;
	navSelectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, const Direction& dir) {
	int8 di = dir != Direction::up;
	sizt id = widgets.size() - 1;
	while (id < widgets.size() && (!widgets[id]->navSelectable() || widgets[id]->position()[di] > mid))
		id--;
	navSelectIfInRange(id, mid, dir);
}

void TileBox::navSelectIfInRange(sizt id, int mid, const Direction& dir) {
	if (id < widgets.size())
		navSelectWidget(id, mid, dir);
	else if (parent)
		parent->navSelectNext(pcID, mid, dir);
}

vec2i TileBox::wgtSize(sizt id) const {
	return vec2i(widgets[id]->getRelSize().pix, wheight);
}

int TileBox::wgtREnd(sizt id) const {
	return positions[id].y + wheight;
}

// READER BOX

ReaderBox::ReaderBox(const Size& relSize, const Direction& direction, int spacing, Layout* parent, sizt id) :
	ScrollArea(relSize, {}, direction, Select::none, spacing, parent, id),
	countDown(true),
	cursorTimer(Default::menuHideTimeout),
	zoom(1.f)
{
	pics = World::program()->getBrowser()->getCurType() == Browser::FileType::archive ? World::drawSys()->loadTexturesArchive(World::program()->getBrowser()->curFilepath()) : World::drawSys()->loadTexturesDirectory(World::program()->getBrowser()->getCurDir());
	widgets.resize(pics.size());
	positions.resize(pics.size()+1);

	if (direction.negative())
		std::reverse(pics.begin(), pics.end());
	for (sizt i = 0; i < pics.size(); i++)
		widgets[i] = new Button(0, nullptr, nullptr, nullptr, pics[i].second, false, 0, this, i);
}

ReaderBox::~ReaderBox() {
	for (pair<string, SDL_Texture*>& it : pics)
		SDL_DestroyTexture(it.second);
}

void ReaderBox::drawSelf() {
	World::drawSys()->drawReaderBox(this);
}

void ReaderBox::onResize() {
	// figure out the width of the list
	bool hori = direction.horizontal();
	int maxRSiz = size()[hori];
	for (sizt i = 0; i < pics.size(); i++) {
		int rsiz = float(texRes(i)[hori]) * zoom;
		if (rsiz > maxRSiz)
			maxRSiz = rsiz;
	}

	// set position of each picture
	int rpos = 0;
	for (sizt i = 0; i < widgets.size(); i++) {
		vec2i psz = vec2f(texRes(i)) * zoom;
		positions[i] = vec2i((maxRSiz - psz[hori]) / 2, rpos, hori);
		rpos += psz[!hori];
	}
	positions.back() = vec2i(maxRSiz, rpos, hori);
	listPos = bringUnder(listPos, listLim());

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
	Layout::postInit();

	// scroll down to opened picture
	if (World::program()->getBrowser()->getCurType() == Browser::FileType::picture)
		for (sizt i = 0; i < widgets.size(); i++)
			if (pics[i].first == World::program()->getBrowser()->getCurFile()) {
				scrollToWidgetPos(i);
				break;
			}
	centerList();
}

void ReaderBox::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	Layout::onMouseMove(mPos, mMov);

	countDown = World::scene()->cursorDisableable() && inRect(mPos, rect()) && !showBar() && World::scene()->capture != this && cursorTimer > 0.f;
	if (cursorTimer < Default::menuHideTimeout) {
		cursorTimer = Default::menuHideTimeout;
		SDL_ShowCursor(SDL_ENABLE);
	}
}

bool ReaderBox::showBar() const {
	return inRect(InputSys::mousePos(), barRect()) || draggingSlider;
}

void ReaderBox::setZoom(float factor) {
	vec2i sh = size() / 2;
	zoom *= factor;
	listPos = bringIn(vec2i(vec2f(listPos + sh) * factor) - sh, vec2i(0), listLim());
	onResize();
}

void ReaderBox::centerList() {
	int8 di = direction.horizontal();
	listPos[di] = listLim()[di] / 2;
}

vec2i ReaderBox::wgtPosition(sizt id) const {
	return position() + positions[id] + vec2i(0, id * spacing, direction.horizontal()) - listPos;
}

vec2i ReaderBox::wgtSize(sizt id) const {
	return vec2f(texRes(id)) * zoom;
}

vec2i ReaderBox::listSize() const {
	return positions.back() + vec2i(0, (widgets.size()-1) * spacing, direction.horizontal());
}

int ReaderBox::wgtRPos(sizt id) const {
	return positions[id][direction.vertical()] + id * spacing;
}

int ReaderBox::wgtREnd(sizt id) const {
	return positions[id+1][direction.vertical()] + id * spacing;
}

vec2i ReaderBox::texRes(sizt id) const {
	vec2i res;
	SDL_QueryTexture(pics[id].second, nullptr, nullptr, &res.x, &res.y);
	return res;
}
