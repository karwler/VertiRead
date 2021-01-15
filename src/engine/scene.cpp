#include "world.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* wgt, ScrollArea* sarea, ivec2 cursPos) :
	widget(wgt),
	area(sarea),
	mPos(cursPos)
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
	if (context)
		context->tick(dSec);
}

void Scene::onMouseMove(ivec2 mPos, ivec2 mMov) {
	updateSelect(mPos);
	if (capture)
		capture->onDrag(mPos, mMov);

	layout->onMouseMove(mPos, mMov);
	if (popup)
		popup->onMouseMove(mPos, mMov);
	if (overlay)
		overlay->onMouseMove(mPos, mMov);
	if (context)
		context->onMouseMove(mPos, mMov);
}

void Scene::onMouseDown(ivec2 mPos, uint8 mBut, uint8 mCnt) {
	if (LabelEdit* box = dynamic_cast<LabelEdit*>(capture); !popup && box && box->unfocusConfirm)	// confirm entered text if such a thing exists and it wants to, unless it's in a popup (that thing handles itself)
		box->confirm();
	if (KeyGetter* box = dynamic_cast<KeyGetter*>(capture)) {	// cancel key getting process if necessary
		box->restoreText();
		capture = nullptr;
	}
	if (context && !context->rect().contain(mPos))
		setContext(nullptr);

	updateSelect(mPos);	// update in case selection has changed through keys while cursor remained at the old position
	if (uint8 bid = mBut - 1; mCnt == 2 && select && stamps[bid].widget == select && cursorInClickRange(mPos, mBut) && select->hasDoubleclick())
		select->onDoubleClick(mPos, mBut);
	else {
		stamps[mBut-1] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[bid].area)	// area goes first so widget can overwrite it's capture
			stamps[bid].area->onHold(mPos, mBut);
		if (stamps[bid].widget != stamps[bid].area)
			stamps[bid].widget->onHold(mPos, mBut);
	}
}

void Scene::onMouseUp(ivec2 mPos, uint8 mBut, uint8 mCnt) {
	if (capture)
		capture->onUndrag(mBut);
	if (select && stamps[mBut-1].widget == select && cursorInClickRange(mPos, mBut) && (mCnt != 2 || !select->hasDoubleclick()))
		select->onClick(mPos, mBut);
}

void Scene::onMouseWheel(ivec2 wMov) {
	if (ScrollArea* box = getSelectedScrollArea()) {
		box->onScroll(wMov * scrollFactorWheel);
		updateSelect(mousePos());
	} else if (select)
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
	World::state()->onResize();
	layout->onResize();
	if (popup)
		popup->onResize();
	if (overlay)
		overlay->onResize();
	if (context)
		context->onResize();
}

void Scene::resetLayouts() {
	// clear scene
	World::drawSys()->clearFonts();
	onMouseLeave();	// reset stamps
	select = nullptr;
	capture = nullptr;
	popup.reset();
	context.reset();

	// set up new widgets
	layout = World::state()->createLayout();
	overlay = World::state()->createOverlay();

	layout->postInit();
	if (overlay)
		overlay->postInit();
	World::inputSys()->simulateMouseMove();
}

void Scene::setPopup(uptr<Popup>&& newPopup, Widget* newCapture) {
	if (context)
		setContext(nullptr);
	if (popup = std::move(newPopup); popup)
		popup->postInit();
	if (capture = newCapture; capture)
		capture->onClick(mousePos(), SDL_BUTTON_LEFT);
	if (!World::inputSys()->mouseLast)
		select = capture ? capture : popup ? popup->firstNavSelect : nullptr;
	updateSelect();
}

void Scene::setContext(uptr<Context>&& newContext) {
	if (context && context->owner() && !World::inputSys()->mouseLast)
		select = context->owner();
	if (context = std::move(newContext); context)
		context->postInit();
	if (!World::inputSys()->mouseLast)
		select = context ? context->firstNavSelect : nullptr;
	updateSelect();
}

void Scene::updateSelect() {
	if (World::inputSys()->mouseLast)
		updateSelect(mousePos());
}

Widget* Scene::getSelected(ivec2 mPos) {
	if (!World::drawSys()->viewport().contain(mPos))
		return nullptr;

	Layout* box = layout.get();
	if (context && context->rect().contain(mPos))
		box = context.get();
	else if (popup)
		box = popup.get();
	else if (overlayFocused(mPos))
		box = overlay.get();

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

bool Scene::overlayFocused(ivec2 mPos) {
	if (overlay)
		return overlay->on = overlay->on ? overlay->rect().contain(mPos) : overlay->actRect().contain(mPos);
	return false;
}

void Scene::selectFirst() {
	if (context)
		select = context->firstNavSelect;
	else if (popup)
		select = popup->firstNavSelect;
	else {
		Layout* next = dynamic_cast<Layout*>(select);
		Layout* lay = next ? next : layout.get();
		for (sizet id = 0; lay;) {
			if (id >= lay->getWidgets().size()) {
				id = lay->getIndex() + 1;
				lay = lay->getParent();
			} else if (next = dynamic_cast<Layout*>(lay->getWidget(id)); next)
				lay = next;
			else if (lay->getWidget(id)->navSelectable()) {
				select = lay->getWidget(0);
				break;
			} else
				id++;
		}
	}
}

sizet Scene::findSelectedID(Layout* box) {
	Widget* child = select;
	while (child->getParent() && child->getParent() != box)
		child = child->getParent();
	return child->getParent() ? child->getIndex() : SIZE_MAX;
}

bool Scene::cursorDisableable() {
	Layout* parent = dynamic_cast<Layout*>(select);
	if (select && !parent)
		parent = select->getParent();

	while (parent->getParent())
		parent = parent->getParent();
	return parent == layout.get();
}
