#include "engine/world.h"

// LAYOUT

Layout::Layout(const Size& SIZ, const vector<Widget*>& WGS, bool VRT, Selection SLT, int SPC) :
	Widget(SIZ),
	widgets(WGS),
	positions(WGS.size()+1),
	spacing(SPC),
	vertical(VRT),
	selection(SLT)
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
	float space = (vertical ? size().y : size().x) - (widgets.size()-1) * spacing;
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
	positions.back() = pos;

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

void Layout::selectWidget(Widget* wgt) {
	if (selection == Selection::one)
		selectSingle(wgt);
	else if (selection == Selection::any) {
		const uint8* state = SDL_GetKeyboardState(nullptr);
		if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
			selectAnother(wgt);
		else if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) {
			if (selected.size() == 1) {
				if ((*selected.begin())->getID() == wgt->getID())
					selected.erase(wgt);
				else {
					sizt start = ((*selected.begin())->getID() < wgt->getID()) ? (*selected.begin())->getID()+1 : wgt->getID();
					sizt end = ((*selected.begin())->getID() < wgt->getID()) ? wgt->getID()+1 : (*selected.begin())->getID();
					selected.insert(widgets.begin() + start, widgets.begin() + end);
				}
			} else
				selectAnother(wgt);
		} else
			selectSingle(wgt);
	}
}

void Layout::selectSingle(Widget* wgt) {
	if (selected.count(wgt))
		selected.clear();
	else {
		if (!selected.empty())
			selected.clear();
		selected.insert(wgt);
	}
}

void Layout::selectAnother(Widget* wgt) {
	if (selected.count(wgt))
		selected.erase(wgt);
	else
		selected.insert(wgt);
}

vec2i Layout::position() const {
	return parent ? parent->wgtPosition(pcID) : 0;
}

vec2i Layout::size() const {
	return parent ? parent->wgtSize(pcID) : World::winSys()->resolution();
}

SDL_Rect Layout::parentFrame() const {
	if (parent)
		return parent->frame();
	return World::drawSys()->viewport();
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

Popup::Popup(const vec2s& SIZ, const vector<Widget*>& WGS, bool VRT, Selection SLT, int SPC) :
	Layout(SIZ.x, WGS, VRT, SLT, SPC),
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

Overlay::Overlay(const vec2s& POS, const vec2s& SIZ, const vec2s& APS, const vec2s& ASZ, const vector<Widget*>& WGS, bool VRT, Selection SLT, int SPC) :
	Popup(SIZ, WGS, VRT, SLT, SPC),
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

ScrollArea::ScrollArea(const Size& SIZ, const vector<Widget*>& WGS, Selection SLT, int SPC) :
	Layout(SIZ, WGS, true, SLT, SPC),
	listPos(0),
	diffSliderMouseY(0),
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

	if (motion.x != 0.f || motion.y != 0.f) {
		onScroll(motion);
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
			int sp = sliderPos();
			int sh = sliderHeight();
			if (outRange(mPos.y, sp, sp + sh))	// if mouse outside of slider but inside bar
				setSlider(mPos.y - sh /2);
			diffSliderMouseY = mPos.y - sliderPos();	// get difference between mouse y and slider y
		}
	}
}

void ScrollArea::onDrag(const vec2i& mPos, const vec2i& mMov) {
	if (draggingSlider)
		setSlider(mPos.y - diffSliderMouseY);
	else
		onScroll(vec2i(0, -mMov.y));
}

void ScrollArea::onUndrag(uint8 mBut) {
	if (mBut == SDL_BUTTON_LEFT) {
		if (!(World::scene()->cursorInClickRange(InputSys::mousePos(), mBut) || draggingSlider))
			motion = -World::inputSys()->getMouseMove();

		draggingSlider = false;
		World::scene()->capture = nullptr;
	}
}

void ScrollArea::onScroll(const vec2i& wMov) {
	listPos = bringIn(listPos + wMov, vec2i(0), listLim());
}

void ScrollArea::throttleMotion(float& mot, float dSec) {
	dSec *= Default::scrollThrottle;
	mot = (mot > 0.f) ? mot - dSec : mot + dSec;
	if (std::abs(mot) < 1.f)
		mot = 0.f;
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

TileBox::TileBox(const Size& SIZ, const vector<Widget*>& WGS, Selection SLT, int WHT, int SPC) :
	ScrollArea(SIZ, WGS, SLT, SPC),
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
		} else  {
			positions[i] = pos;
			pos.x = end + spacing;
		}
	}
	positions.back() = pos;
	listPos = bringUnder(listPos, listLim());

	for (Widget* it : widgets)
		it->onResize();
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
	ScrollArea(SIZ, PICS, Selection::none, SPC),
	cursorFree(true),
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
		Picture* pic = static_cast<Picture*>(it);
		if (pic->getTex().getRes().x > maxWidth)
			maxWidth = pic->getTex().getRes().x;
	}

	// set position of each picture
	int ypos = 0;
	for (sizt i=0; i<widgets.size(); i++) {
		Picture* pic = static_cast<Picture*>(widgets[i]);
		positions[i] = vec2i((maxWidth - pic->getTex().getRes().x) / 2, ypos);
		ypos += pic->getTex().getRes().y;
	}
	positions.back() = vec2i(maxWidth, ypos);
	listPos = bringUnder(listPos, listLim());

	// for good measure?
	for (Widget* it : widgets)
		it->onResize();
}

void ReaderBox::tick(float dSec) {
	ScrollArea::tick(dSec);

	if (cursorFree && cursorTimer > 0.f) {
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
		if (static_cast<Picture*>(widgets[i])->getTex().getFile() == curPic) {
			listPos.y = positions[i].y;
			break;
		}
	centerListX();
}

void ReaderBox::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	Layout::onMouseMove(mPos, mMov);

	cursorFree = !inRect(mPos, barRect()) && !showBar() && World::scene()->capture != this && World::scene()->getFocusedScrollArea() != this;
	if (cursorTimer < Default::menuHideTimeout) {
		cursorTimer = Default::menuHideTimeout;
		SDL_ShowCursor(SDL_ENABLE);
	}
}

bool ReaderBox::showBar() const {
	return inRect(InputSys::mousePos(), barRect()) || draggingSlider;
}

void ReaderBox::centerListX() {
	listPos.x = ScrollArea::listLim().x / 2;
}

vec2i ReaderBox::wgtPosition(sizt id) const {
	vec2i wps = position() + vec2i(vec2f(positions[id] - listPos) * zoom);
	return vec2i(wps.x, wps.y + id * spacing);
}

vec2i ReaderBox::wgtSize(sizt id) const {
	return vec2f(static_cast<Picture*>(widgets[id])->getTex().getRes()) * zoom;
}

vec2i ReaderBox::listLim() const {
	vec2i lim = vec2f(ScrollArea::listLim()) * zoom;
	return vec2i(lim.x, lim.y + (widgets.size()-1) * spacing);
}
