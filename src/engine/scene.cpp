#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* widget, ScrollArea* area, vec2i mPos) :
	widget(widget),
	area(area),
	mPos(mPos)
{}

// SCENE

Scene::Scene() :
	select(nullptr),
	capture(nullptr)
{}

void Scene::tick(float dSec) {
	layout->tick(dSec);
	if (popup)
		popup->tick(dSec);
	if (overlay)
		overlay->tick(dSec);
}

void Scene::onMouseMove(vec2i mPos, vec2i mMov) {
	select = getSelected(mPos, topLayout(mPos));
	if (capture)
		capture->onDrag(mPos, mMov);

	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
	if (overlay)
		overlay->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(vec2i mPos, uint8 mBut, uint8 mCnt) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box && box->unfocusConfirm)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	if (KeyGetter* box = dynamic_cast<KeyGetter*>(capture)) {	// cancel key getting process if necessary
		box->restoreText();
		capture = nullptr;
	}

	select = getSelected(mPos, topLayout(mPos));	// update in case selection has changed through keys while cursor remained at the old position
	if (uint8 bid = mBut - 1; mCnt == 1) {
		stamps[mBut-1] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[bid].area)	// area goes first so widget can overwrite it's capture
			stamps[bid].area->onHold(mPos, mBut);
		if (stamps[bid].widget != stamps[bid].area)
			stamps[bid].widget->onHold(mPos, mBut);
	} else if (mCnt == 2 && select && stamps[bid].widget == select && cursorInClickRange(mPos, mBut))
		select->onDoubleClick(mPos, mBut);
}

void Scene::onMouseUp(vec2i mPos, uint8 mBut, uint8 mCnt) {
	if (capture && mCnt == 1)
		capture->onUndrag(mBut);
	if (select && stamps[mBut-1].widget == select && cursorInClickRange(mPos, mBut) && !(mCnt != 1 && select->hasDoubleclick()))
		select->onClick(mPos, mBut);
}

void Scene::onMouseWheel(vec2i wMov) {
	if (ScrollArea* box = getSelectedScrollArea())
		box->onScroll(wMov * scrollFactorWheel);
	else if (select)
		select->onScroll(wMov * scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps)
		it.widget = it.area = nullptr;
}

void Scene::onText(const char* str) {
	if (capture)
		capture->onText(str);
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
	onMouseMove(mousePos(), 0);
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	popup.reset(newPopup);
	if (popup)
		popup->postInit();

	capture = newCapture;
	if (capture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	onMouseMove(mousePos(), 0);
}

Widget* Scene::getSelected(vec2i mPos, Layout* box) {
	for (;;) {
		Rect frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contain(mPos); }); it != box->getWidgets().end()) {
			if (Layout* lay = dynamic_cast<Layout*>(*it))
				box = lay;
			else
				return (*it)->navSelectable() ? *it : box;
		} else
			return box;
	}
}

ScrollArea* Scene::getSelectedScrollArea() const {
	Layout* parent = dynamic_cast<Layout*>(select);
	if (select && !parent)
		parent = select->getParent();

	while (parent && !dynamic_cast<ScrollArea*>(parent))
		parent = parent->getParent();
	return dynamic_cast<ScrollArea*>(parent);
}

bool Scene::overlayFocused(vec2i mPos) {
	if (!overlay)
		return false;

	if (overlay->on)
		return overlay->rect().contain(mPos) ? true : overlay->on = false;
	return overlay->actRect().contain(mPos) ? overlay->on = true : false;
}

void Scene::selectFirst() {
	if (popup || overlay)
		return;

	Layout* next = dynamic_cast<Layout*>(select);
	Layout* lay = next ? next : layout.get();
	for (sizet id = 0; lay;) {
		if (id >= lay->getWidgets().size()) {
			id = lay->getID() + 1;
			lay = lay->getParent();
		} else if (next = dynamic_cast<Layout*>(lay->getWidget(id)))
			lay = next;
		else if (lay->getWidget(id)->navSelectable()) {
			select = lay->getWidget(0);
			break;
		} else
			id++;
	}
}

sizet Scene::findSelectedID(Layout* box) {
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
