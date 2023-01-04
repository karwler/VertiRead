#include "scene.h"
#include "drawSys.h"
#include "inputSys.h"
#include "world.h"
#include "prog/progs.h"
#include "utils/layouts.h"

// CLICK STAMP

ClickStamp::ClickStamp(Widget* wgt, ScrollArea* sarea, ivec2 cursPos) :
	widget(wgt),
	area(sarea),
	mPos(cursPos)
{}

// SCENE

Scene::~Scene() {
	delete layout;
	delete popup;
	delete overlay;
	delete context;
}

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
	if (context && !context->rect().contains(mPos))
		setContext(nullptr);

	updateSelect(mPos);	// update in case selection has changed through keys while cursor remained at the old position
	if (uint8 bid = mBut - 1; mCnt == 2 && select && stamps[bid].widget == select && cursorInClickRange(mPos, mBut) && select->hasDoubleclick())
		select->onDoubleClick(mPos, mBut);
	else {
		stamps[mBut - 1] = ClickStamp(select, getSelectedScrollArea(), mPos);
		if (stamps[bid].area)	// area goes first so widget can overwrite it's capture
			stamps[bid].area->onHold(mPos, mBut);
		if (stamps[bid].widget != stamps[bid].area)
			stamps[bid].widget->onHold(mPos, mBut);
	}
}

void Scene::onMouseUp(ivec2 mPos, uint8 mBut, uint8 mCnt) {
	if (capture)
		capture->onUndrag(mBut);
	if (select && stamps[mBut - 1].widget == select && cursorInClickRange(mPos, mBut) && (mCnt != 2 || !select->hasDoubleclick()))
		select->onClick(mPos, mBut);
}

void Scene::onMouseWheel(ivec2 mPos, ivec2 wMov) {
	if (ScrollArea* box = getSelectedScrollArea()) {
		box->onScroll(wMov * scrollFactorWheel);
		updateSelect(mPos);
	} else if (select)
		select->onScroll(wMov * scrollFactorWheel);
}

void Scene::onMouseLeave() {
	for (ClickStamp& it : stamps)
		it.widget = it.area = nullptr;
	select = nullptr;
}

void Scene::onCompose(string_view str) {
	if (capture) {
		capture->onCompose(str, captureLen);
		captureLen = str.length();
	}
}

void Scene::onText(string_view str) {
	if (capture) {
		capture->onText(str, captureLen);
		captureLen = 0;
	}
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

void Scene::onDisplayChange() {
	layout->onDisplayChange();
	if (popup)
		popup->onDisplayChange();
	if (overlay)
		overlay->onDisplayChange();
}

void Scene::resetLayouts() {
	clearLayouts();
	setLayouts();
}

void Scene::clearLayouts() {
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	World::drawSys()->clearFonts();
#endif
	onMouseLeave();	// reset stamps
	select = nullptr;
	capture = nullptr;
	delete layout;
	delete popup;
	popup = nullptr;
	delete overlay;
	delete context;
	context = nullptr;
}

void Scene::setLayouts() {
	layout = World::state()->createLayout();
	overlay = World::state()->createOverlay();
	layout->postInit();
	if (overlay)
		overlay->postInit();
	World::inputSys()->simulateMouseMove();
}

void Scene::setCapture(Widget* inter) {
	capture = inter;
	captureLen = 0;
}

void Scene::setPopup(Popup* newPopup, Widget* newCapture) {
	if (context)
		setContext(nullptr);
	delete popup;
	if (popup = newPopup; popup)
		popup->postInit();
	if (capture = newCapture; capture)
		capture->onClick(World::winSys()->mousePos(), SDL_BUTTON_LEFT);
	if (!World::inputSys()->mouseWin)
		select = capture ? capture : popup ? popup->firstNavSelect : nullptr;
	updateSelect();
}

void Scene::setContext(Context* newContext) {
	if (context && context->owner() && !World::inputSys()->mouseWin)
		select = context->owner();
	delete context;
	if (context = newContext; context)
		context->postInit();
	if (!World::inputSys()->mouseWin)
		select = context ? context->firstNavSelect : nullptr;
	updateSelect();
}

void Scene::updateSelect() {
	if (World::inputSys()->mouseWin)
		updateSelect(World::winSys()->mousePos());
}

Widget* Scene::getSelected(ivec2 mPos) {
	if (!Recti(ivec2(0), World::drawSys()->getViewRes()).contains(mPos))
		return nullptr;

	Layout* box = layout;
	if (context && context->rect().contains(mPos))
		box = context;
	else if (popup)
		box = popup;
	else if (overlayFocused(mPos))
		box = overlay;

	if (World::sets()->gpuSelecting) {
		Widget* wgt = World::drawSys()->getSelectedWidget(box, mPos);
		return wgt ? wgt->navSelectable() ? wgt : wgt->getParent() : nullptr;
	}

	for (;;) {
		Recti frame = box->frame();
		if (vector<Widget*>::const_iterator it = std::find_if(box->getWidgets().begin(), box->getWidgets().end(), [&frame, &mPos](const Widget* wi) -> bool { return wi->rect().intersect(frame).contains(mPos); }); it != box->getWidgets().end()) {
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

bool Scene::overlayFocused(ivec2 mPos) const {
	if (overlay)
		return overlay->on = overlay->on ? overlay->rect().contains(mPos) : overlay->actRect().contains(mPos);
	return false;
}

void Scene::selectFirst() {
	if (context)
		select = context->firstNavSelect;
	else if (popup)
		select = popup->firstNavSelect;
	else {
		Layout* next = dynamic_cast<Layout*>(select);
		Layout* lay = next ? next : layout;
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
				++id;
		}
	}
}

sizet Scene::findSelectedID(Layout* box) const {
	Widget* child = select;
	while (child->getParent() && child->getParent() != box)
		child = child->getParent();
	return child->getParent() ? child->getIndex() : SIZE_MAX;
}
