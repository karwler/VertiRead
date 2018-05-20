#include "engine/world.h"

// LAYOUT

Layout::Layout(const Size& SIZ, const vector<Widget*>& WGS, bool VRT, int SPC) :
	Widget(SIZ),
	widgets(WGS),
	positions(WGS.size()+1),
	spacing(SPC),
	vertical(VRT)
{
	for (sizt i=0; i<widgets.size(); i++)
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
	vec2i wsiz = size();
	float space = (vertical ? wsiz.y : wsiz.x) - (widgets.size()-1) * spacing;
	float total = 0;
	for (Widget* it : widgets) {
		if (it->getRelSize().usePix)
			space -= it->getRelSize().pix;
		else
			total += it->getRelSize().prc;
	}

	// calculate positions for each widget and set last poss element to end position of the last widget
	vec2i pos;
	for (sizt i=0; i<widgets.size(); i++) {
		positions[i] = pos;

		int siz = widgets[i]->getRelSize().usePix ? widgets[i]->getRelSize().pix : widgets[i]->getRelSize().prc * space / total;
		(vertical ? pos.y : pos.x) += siz + spacing;
	}
	positions.back() = vertical ? vec2i(wsiz.x, pos.y) : vec2i(pos.x, wsiz.y);

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

void Layout::selectNext(sizt id, int mid, uint8 dir) {
	if ((vertical && dir <= 1) || (!vertical && dir >= 2)) {
		bool fwd = dir % 2;
		if ((!fwd && id == 0) || (fwd && id >= widgets.size()-1)) {
			if (parent)
				parent->selectNext(pcID, mid, dir);
		} else
			scanSequential(id, mid, dir);
	} else if (parent)
		parent->selectNext(pcID, mid, dir);
}

void Layout::selectFrom(int mid, uint8 dir) {
	if ((vertical && dir <= 1) || (!vertical && dir >= 2))
		scanSequential((dir % 2) ? SIZE_MAX : widgets.size(), mid, dir);
	else
		scanPerpendicular(mid, dir);
}

void Layout::scanSequential(sizt id, int mid, uint8 dir) {
	int8 mov = (dir % 2) ? 1 : -1;
	while ((id += mov) < widgets.size() && !widgets[id]->selectable());
	if (id < widgets.size())
		selectWidget(id, mid, dir);
}

void Layout::scanPerpendicular(int mid, uint8 dir) {
	sizt id = 0;
	while (id < widgets.size() && (!widgets[id]->selectable() || ((dir <= 1) ? wgtPosition(id).x + wgtSize(id).x : wgtPosition(id).y + wgtSize(id).y) < mid))
		id++;

	if (id == widgets.size())
		while (--id > 0 && !widgets[id]->selectable());
	selectWidget(id, mid, dir);
}

void Layout::selectWidget(sizt id, int mid, uint8 dir) {
	if (Layout* lay = dynamic_cast<Layout*>(widgets[id]))
		lay->selectFrom(mid, dir);
	else if (dynamic_cast<Button*>(widgets[id]))
		World::scene()->select = widgets[id];
}

vec2i Layout::position() const {
	return parent ? parent->wgtPosition(pcID) : 0;
}

vec2i Layout::size() const {
	return parent ? parent->wgtSize(pcID) : World::winSys()->resolution();
}

SDL_Rect Layout::parentFrame() const {
	return parent ? parent->frame() : World::drawSys()->viewport();
}

vec2i Layout::wgtPosition(sizt id) const {
	return position() + positions[id];
}

vec2i Layout::wgtSize(sizt id) const {
	return vertical ? vec2i(size().x, positions[id+1].y - positions[id].y - spacing) : vec2i(positions[id+1].x - positions[id].x - spacing, size().y);
}

vec2i Layout::listSize() const {
	return positions.back() - spacing;
}

// POPUP

Popup::Popup(const vec2s& SIZ, const vector<Widget*>& WGS, bool VRT, int SPC) :
	Layout(SIZ.x, WGS, VRT, SPC),
	sizeY(SIZ.y)
{}

void Popup::drawSelf() {
	World::drawSys()->drawPopup(this);
}

vec2i Popup::position() const {
	return (World::winSys()->resolution() - size()) / 2;
}

vec2i Popup::size() const {
	vec2f res = World::winSys()->resolution();
	return vec2i(relSize.usePix ? relSize.pix : relSize.prc * res.x, sizeY.usePix ? sizeY.pix : sizeY.prc * res.y);
}

// OVERLAY

Overlay::Overlay(const vec2s& POS, const vec2s& SIZ, const vec2s& APS, const vec2s& ASZ, const vector<Widget*>& WGS, bool VRT, int SPC) :
	Popup(SIZ, WGS, VRT, SPC),
	pos(POS),
	actPos(APS),
	actSize(ASZ)
{}

vec2i Overlay::position() const {
	vec2f res = World::winSys()->resolution();
	return vec2i(pos.x.usePix ? pos.x.pix : pos.x.prc * res.x, pos.y.usePix ? pos.y.pix : pos.y.prc * res.y);
}

SDL_Rect Overlay::actRect() {
	vec2f res = World::winSys()->resolution();
	vec2i ps(actPos.x.usePix ? actPos.x.pix : actPos.x.prc * res.x, actPos.y.usePix ? actPos.y.pix : actPos.y.prc * res.y);
	vec2i sz(actSize.x.usePix ? actSize.x.pix : actSize.x.prc * res.x, actSize.y.usePix ? actSize.y.pix : actSize.y.prc * res.y);
	return {ps.x, ps.y, sz.x, sz.y};
}

// SCROLL AREA

ScrollArea::ScrollArea(const Size& SIZ, const vector<Widget*>& WGS, int SPC) :
	Layout(SIZ, WGS, true, SPC),
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
		onScroll(vec2i(0, motion));

		if (motion > 0.f) {
			motion -= Default::scrollThrottle * dSec;
			if (motion < 0.f)
				motion = 0.f;
		} else {
			motion += Default::scrollThrottle * dSec;
			if (motion > 0.f)
				motion = 0.f;
		}
	}
}

void ScrollArea::onHold(const vec2i& mPos, uint8 mBut) {
	motion = 0.f;	// get rid of scroll motion

	if (mBut == SDL_BUTTON_LEFT) {	// check scroll bar left click
		World::scene()->capture = this;

		draggingSlider = inRect(mPos, barRect());
		if (draggingSlider) {
			int sp = sliderPos();
			int sh = sliderHeight();
			if (outRange(mPos.y, sp, sp + sh))	// if mouse outside of slider but inside bar
				setSlider(mPos.y - sh /2);
			diffSliderMouse = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(const vec2i& mPos, const vec2i& mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouse);
	else if (InputSys::isPressedM(SDL_BUTTON_RIGHT))
		onScroll(-mMov);
	else
		onScroll(vec2i(0, -mMov.y));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!(World::scene()->cursorInClickRange(InputSys::mousePos(), mBut) || draggingSlider))
			motion = -World::inputSys()->getMouseMove().y;

		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(const vec2i& wMov) {
	listPos = bringIn(listPos + wMov, vec2i(0), listLim());
}

void ScrollArea::selectNext(sizt id, int mid, uint8 dir) {
	Layout::selectNext(id, mid, dir);
	scrollToSelected();
}

void ScrollArea::selectFrom(int mid, uint8 dir) {
	Layout::selectFrom(mid, dir);
	scrollToSelected();
}

void ScrollArea::scrollToSelected() {
	sizt cid = World::scene()->findSelectedID(this);
	if (cid >= widgets.size())
		return;

	int cpos = widgets[cid]->position().y;
	int fpos = position().y;
	if (cpos < fpos)
		scrollToWidgetPos(cid);
	else if (cpos + widgets[cid]->size().y > fpos + size().y)
		scrollToWidgetEnd(cid);
}

void ScrollArea::scrollToWidgetPos(sizt id) {
	listPos.y = positions[id].y;
}

void ScrollArea::scrollToWidgetEnd(sizt id) {
	listPos.y = positions[id].y + wgtSize(id).y - size().y;
}

void ScrollArea::setSlider(int ypos) {
	int lim = listLim().y;
	listPos.y = bringIn((ypos - position().y) * lim / sliderLim(), 0, lim);
}

int ScrollArea::barWidth() const {
	return (listSize().y > size().y) ? Default::sliderWidth : 0;
}

vec2i ScrollArea::listLim() const {
	vec2i wsiz = size();
	vec2i lsiz = listSize();
	return vec2i((wsiz.x < lsiz.x) ? lsiz.x - wsiz.x : 0, (wsiz.y < lsiz.y) ? lsiz.y - wsiz.y : 0);
}

int ScrollArea::sliderPos() const {
	int sizY = size().y;
	return (listSize().y > sizY) ? position().y + listPos.y * sliderLim() / listLim().y : position().y;
}

int ScrollArea::sliderHeight() const {
	int sizY = size().y;
	int lstH = listSize().y;
	return (sizY < lstH) ? sizY * sizY / lstH : sizY;
}

int ScrollArea::sliderLim() const {
	return size().y - sliderHeight();
}

vec2i ScrollArea::wgtPosition(sizt id) const {
	return Layout::wgtPosition(id) - listPos;
}

vec2i ScrollArea::wgtSize(sizt id) const {
	vec2i siz = Layout::wgtSize(id);
	return vec2i(siz.x - barWidth(), siz.y);
}

SDL_Rect ScrollArea::barRect() const {
	int bw = barWidth();
	vec2i pos = position();
	vec2i siz = size();
	return {pos.x+siz.x-bw, pos.y, bw, siz.y};
}

SDL_Rect ScrollArea::sliderRect() const {
	int bw = barWidth();
	return {position().x + size().x - bw, sliderPos(), bw, sliderHeight()};
}

vec2t ScrollArea::visibleWidgets() const {
	if (widgets.empty())	// nothing to draw
		return 0;

	sizt first = 1;	// start one too far cause it's checking the end of each widget
	while (first < widgets.size() && positions[first].y - spacing < listPos.y)
		first++;

	int end = listPos.y + size().y;
	sizt last = first;	// last is one greater than the actual last index
	while (last < widgets.size() && positions[last].y <= end)
		last++;

	return vec2t(first-1, last);	// correct first so it's the first element rather than it's end
}

// TILE BOX

TileBox::TileBox(const Size& SIZ, const vector<Widget*>& WGS, int WHT, int SPC) :
	ScrollArea(SIZ, WGS, SPC),
	wheight(WHT)
{}

void TileBox::onResize() {
	int sizX = size().x - Default::sliderWidth;
	vec2i pos;
	for (sizt i=0; i<widgets.size(); i++) {
		int end = pos.x + widgets[i]->getRelSize().pix;
		if (end > sizX) {
			pos = vec2i(0, pos.y + wheight + spacing);
			positions[i] = pos;
			pos.x += widgets[i]->getRelSize().pix + spacing;
		} else {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = vec2i(sizX, pos.y + wheight) + spacing;
	listPos = bringUnder(listPos, listLim());

	for (Widget* it : widgets)
		it->onResize();
}

void TileBox::selectNext(sizt id, int mid, uint8 dir) {
	if (dir <= 1)
		scanVertically(id, mid, dir);
	else
		scanHorizontally(id, mid, dir);
	scrollToSelected();
}

void TileBox::selectFrom(int mid, uint8 dir) {
	if (dir % 2)
		scanFromStart(mid, dir);
	else
		scanFromEnd(mid, dir);
	scrollToSelected();
}

void TileBox::scanVertically(sizt id, int mid, uint8 dir) {
	int ypos = widgets[id]->position().y;
	if (dir % 2)
		while (++id < widgets.size() && (!widgets[id]->selectable() || widgets[id]->position().y == ypos || widgets[id]->position().x + widgets[id]->size().x < mid));
	else
		while (--id < widgets.size() && (!widgets[id]->selectable() || widgets[id]->position().y == ypos || widgets[id]->position().x > mid));
	selectIfInRange(id, mid, dir);
}

void TileBox::scanHorizontally(sizt id, int mid, uint8 dir) {
	int8 mov = (dir % 2) ? 1 : -1;
	while ((id += mov) < widgets.size() && !widgets[id]->selectable());
	if (id < widgets.size() && widgets[id]->center().y == mid)
		selectWidget(id, mid, dir);
	else if (parent)
		parent->selectNext(pcID, mid, dir);
}

void TileBox::scanFromStart(int mid, uint8 dir) {
	sizt id = 0;
	while (id < widgets.size() && (!widgets[id]->selectable() || ((dir == 1) ? widgets[id]->position().x + widgets[id]->size().x : widgets[id]->position().y + widgets[id]->size().y) < mid))
		id++;
	selectIfInRange(id, mid, dir);
}

void TileBox::scanFromEnd(int mid, uint8 dir) {
	sizt id = widgets.size() - 1;
	while (id < widgets.size() && (!widgets[id]->selectable() || ((dir == 0) ? widgets[id]->position().x : widgets[id]->position().y) > mid))
		id--;
	selectIfInRange(id, mid, dir);
}

void TileBox::selectIfInRange(sizt id, int mid, uint8 dir) {
	if (id < widgets.size())
		selectWidget(id, mid, dir);
	else if (parent)
		parent->selectNext(pcID, mid, dir);
}

vec2i TileBox::wgtSize(sizt id) const {
	return vec2i(widgets[id]->getRelSize().pix, wheight);
}

vec2t TileBox::visibleWidgets() const {
	if (widgets.empty())	// nothing to draw
		return 0;

	sizt first = 0;
	while (first < widgets.size() && positions[first].y + wheight - spacing < listPos.y)
		first++;

	int end = listPos.y + size().y;
	sizt last = first + 1;
	while (last < widgets.size() && positions[last].y <= end)
		last++;

	return vec2t(first, last);
}

// READER BOX

ReaderBox::ReaderBox(const Size& SIZ, const vector<Widget*>& PICS, int SPC, float ZOOM) :
	ScrollArea(SIZ, PICS, SPC),
	countDown(true),
	cursorTimer(Default::menuHideTimeout),
	zoom(ZOOM)
{}

void ReaderBox::drawSelf() {
	World::drawSys()->drawReaderBox(this);
}

void ReaderBox::onResize() {
	// figure out the width of the list
	int maxWidth = size().x;
	for (Widget* it : widgets) {
		int width = float(static_cast<Picture*>(it)->getRes().x) * zoom;
		if (width > maxWidth)
			maxWidth = width;
	}

	// set position of each picture
	int ypos = 0;
	for (sizt i=0; i<widgets.size(); i++) {
		vec2i psz = vec2f(static_cast<Picture*>(widgets[i])->getRes()) * zoom;
		positions[i] = vec2i((maxWidth - psz.x) / 2, ypos);
		ypos += psz.y;
	}
	positions.back() = vec2i(maxWidth, ypos);
	listPos = bringUnder(listPos, listLim());

	// for good measure?
	for (Widget* it : widgets)
		it->onResize();
}

void ReaderBox::tick(float dSec) {
	ScrollArea::tick(dSec);

	if (countDown) {
		cursorTimer -= dSec;
		if (cursorTimer <= 0.f)
			SDL_ShowCursor(SDL_DISABLE);
	}
}

void ReaderBox::postInit() {
	Layout::postInit();

	// scroll down to opened picture
	string curPic = World::program()->getBrowser()->curFilepath();
	for (sizt i=0; i<widgets.size(); i++)
		if (static_cast<Picture*>(widgets[i])->getFile() == curPic) {
			listPos.y = positions[i].y;
			break;
		}
	centerListX();
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

void ReaderBox::centerListX() {
	listPos.x = listLim().x / 2;
}

vec2i ReaderBox::wgtPosition(sizt id) const {
	return position() + vec2i(positions[id].x, positions[id].y + id * spacing) - listPos;
}

vec2i ReaderBox::wgtSize(sizt id) const {
	return vec2f(static_cast<Picture*>(widgets[id])->getRes()) * zoom;
}

vec2i ReaderBox::listSize() const {
	const vec2i& end = positions.back();
	return vec2i(end.x, end.y + (widgets.size()-1) * spacing);
}
