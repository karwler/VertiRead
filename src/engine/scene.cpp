#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* WGT, ScrollArea* ARE, const vec2i& POS) :
	widget(WGT),
	area(ARE),
	mPos(POS)
{}

// SCENE

Scene::Scene() :
	stamps(SDL_BUTTON_X2+1),
	capture(nullptr)
{}

Scene::~Scene() {
	clear(overlays);
}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
	for (Overlay* it : overlays)
		it->tick(dSec);
}

void Scene::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	setFocused(mPos);

	if (capture)
		capture->onDrag(mPos, mMov);

	// call mouse move events
	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
	for (Overlay* it : overlays)
		it->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (!popup)	// confirm entered text if such a thing exists unless it's in a popup (that thing handles itself)
		if (LineEdit* box = dynamic_cast<LineEdit*>(capture))
			box->confirm();

	if (mCnt == 1) {
		stamps[mBut] = ClickStamp(focused.back(), getFocusedScrollArea(), mPos);
		if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
			stamps[mBut].area->onHold(mPos, mBut);
		if (stamps[mBut].widget != stamps[mBut].area)
			stamps[mBut].widget->onHold(mPos, mBut);
	} else if (mCnt == 2 && stamps[mBut].widget == focused.back() && cursorInClickRange(mPos, mBut))
		focused.back()->onDoubleClick(mPos, mBut);
}

void Scene::onMouseUp(const vec2i& mPos, uint8 mBut) {
	if (capture)
		capture->onUndrag(mBut);

	if (stamps[mBut].widget == focused.back() && cursorInClickRange(mPos, mBut))
		stamps[mBut].widget->onClick(mPos, mBut);
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = getFocusedScrollArea())
		box->onScroll(wMov * Default::scrollFactorWheel);
	else
		focused.back()->onScroll(wMov * Default::scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps) {
		it.widget = nullptr;
		it.area = nullptr;
	}
}

void Scene::onText(const char* str) {
	capture->onText(str);	// text input should only run if line edit is being captured, therefore a cast check isn't necessary
}

void Scene::onResize() {
	layout->onResize();
	if (popup)
		popup->onResize();
	for (Overlay* it : overlays)
		it->onResize();
}

void Scene::resetLayouts() {
	// clear scene
	World::drawSys()->clearFonts();
	onMouseLeave();	// reset stamps
	focused.clear();
	capture = nullptr;
	popup.reset();
	clear(overlays);

	// set up new widgets
	layout.reset(World::state()->createLayout());
	overlays = World::state()->createOverlays();

	layout->postInit();
	for (Overlay* it : overlays)
		it->postInit();
	onMouseMove(InputSys::mousePos(), World::inputSys()->getMouseMove());
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(InputSys::mousePos(), SDL_BUTTON_LEFT);

	setFocused(InputSys::mousePos());
}

void Scene::setFocused(const vec2i& mPos) {
	Layout* lay;
	if (popup)
		lay = popup.get();
	else {
		lay = findFocusedOverlay(mPos);
		if (!lay)
			lay = layout.get();
	}
	focused.resize(1);
	focused[0] = lay;
	setFocusedElement(mPos, lay);
}

void Scene::setFocusedElement(const vec2i& mPos, Layout* box) {
	SDL_Rect frame = box->frame();
	for (Widget* it : box->getWidgets())
		if (inRect(mPos, overlapRect(it->rect(), frame))) {
			focused.push_back(it);
			if (Layout* lay = dynamic_cast<Layout*>(it))
				setFocusedElement(mPos, lay);
			break;
		}
}

Overlay* Scene::findFocusedOverlay(const vec2i& mPos) {
	for (Overlay* it : overlays) {
		if (it->on) {
			if (inRect(mPos, it->rect()))
				return it;
			else
				it->on = false;
		} else if (inRect(mPos, it->actRect())) {
			it->on = true;
			return it;
		}
	}
	return nullptr;
}

ScrollArea* Scene::getFocusedScrollArea() const {
	for (Widget* it : focused)
		if (ScrollArea* sa = dynamic_cast<ScrollArea*>(it))
			return sa;
	return nullptr;
}

bool Scene::cursorInClickRange(const vec2i& mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= Default::clickThreshold;
}
