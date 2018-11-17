#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* widget, ScrollArea* area, const vec2i& mPos) :
	widget(widget),
	area(area),
	mPos(mPos)
{}

// SCENE

Scene::Scene() :
	select(nullptr),
	capture(nullptr),
	layout(new Layout(1.f, {})),	// dummy layout in case a function gets called preemptively
	stamps(SDL_BUTTON_X2 + 1)
{}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
	if (overlay)
		overlay->tick(dSec);
}

void Scene::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	setSelected(mPos, topLayout(mPos));

	if (capture)
		capture->onDrag(mPos, mMov);

	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
	if (overlay)
		overlay->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(const vec2i& mPos, uint8 mBut, uint8 mCnt) {
	if (!popup)	// confirm entered text if such a thing exists unless it's in a popup (that thing handles itself)
		if (LineEdit* box = dynamic_cast<LineEdit*>(capture))
			box->confirm();
	
	setSelected(mPos, topLayout(mPos));	// update in case selection has changed through keys while cursor remained at the old position
	if (mCnt == 1) {
		stamps[mBut] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[mBut].area)	// area goes first so widget can overwrite it's capture
			stamps[mBut].area->onHold(mPos, mBut);
		if (stamps[mBut].widget != stamps[mBut].area)
			stamps[mBut].widget->onHold(mPos, mBut);
	} else if (mCnt == 2 && stamps[mBut].widget == select && cursorInClickRange(mPos, mBut))
		select->onDoubleClick(mPos, mBut);
}

void Scene::onMouseUp(const vec2i& mPos, uint8 mBut) {
	if (capture)
		capture->onUndrag(mBut);

	if (select && stamps[mBut].widget == select && cursorInClickRange(mPos, mBut))
		stamps[mBut].widget->onClick(mPos, mBut);
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * Default::scrollFactorWheel);
	else if (select)
		select->onScroll(wMov * Default::scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps) {
		it.widget = nullptr;
		it.area = nullptr;
	}
}

void Scene::onResize() {
	layout->onResize();
	if (popup)
		popup->onResize();
	if (overlay)
		overlay->onResize();
}

void Scene::resetLayouts() {
	// clear scene
	World::drawSys()->clearFonts();
	onMouseLeave();	// reset stamps
	select = nullptr;
	capture = nullptr;
	popup.reset();

	// set up new widgets
	layout.reset(World::state()->createLayout());
	overlay.reset(World::state()->createOverlay());

	layout->postInit();
	if (overlay)
		overlay->postInit();
	onMouseMove(InputSys::mousePos(), 0);
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(InputSys::mousePos(), SDL_BUTTON_LEFT);
	onMouseMove(InputSys::mousePos(), 0);
}

void Scene::setSelected(const vec2i& mPos, Layout* box) {
	SDL_Rect frame = box->frame();
	for (Widget* it : box->getWidgets())
		if (inRect(mPos, overlapRect(it->rect(), frame))) {
			if (Layout* lay = dynamic_cast<Layout*>(it))
				setSelected(mPos, lay);
			else
				select = dynamic_cast<Button*>(it) ? it : box;
			return;
		}
	select = box;
}

ScrollArea* Scene::getSelectedScrollArea() const {
	Layout* parent = dynamic_cast<Layout*>(select);
	if (select && !parent)
		parent = select->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

bool Scene::overlayFocused(const vec2i& mPos) {
	if (!overlay)
		return false;

	if (overlay->on) {
		if (inRect(mPos, overlay->rect()))
			return true;
		overlay->on = false;
		return false;
	}
	if (inRect(mPos, overlay->actRect())) {
		overlay->on = true;
		return true;
	}
	return false;
}

void Scene::selectFirst() {
	if (popup || overlay)
		return;

	Layout* lay;
	if (Layout* next = dynamic_cast<Layout*>(select))
		lay = next;
	else
		lay = layout.get();

	for (sizt id = 0; lay;) {
		if (id >= lay->getWidgets().size()) {
			id = lay->getID() + 1;
			lay = lay->getParent();
		} else if (Layout* next = dynamic_cast<Layout*>(lay->getWidget(id)))
			lay = next;
		else if (lay->getWidget(id)->navSelectable()) {
			select = lay->getWidget(0);
			break;
		} else
			id++;
	}
}

sizt Scene::findSelectedID(Layout* box) {
	Widget* child = select;
	while (child->getParent() && child->getParent() != box)
		child = child->getParent();
	return child->getParent() ? child->getID() : SIZE_MAX;
}

bool Scene::cursorDisableable() {
	Layout* parent = dynamic_cast<Layout*>(select);
	if (select && !parent)
		parent = select->getParent();

	while (parent->getParent())
		parent = parent->getParent();
	return parent == layout.get();
}

bool Scene::cursorInClickRange(const vec2i& mPos, uint8 mBut) {
	return vec2f(mPos - stamps[mBut].mPos).length() <= Default::clickThreshold;
}

Layout* Scene::topLayout(const vec2i& mPos) {
	return popup ? popup.get() : overlayFocused(mPos) ? overlay.get() : layout.get();
}
