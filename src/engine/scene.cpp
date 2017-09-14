#include "world.h"
#include "filer.h"

Scene::Scene() :
	library(Filer::getGeneralSettings(), World::winSys()->getSettings().getFontpath()),
	focWidget(nullptr),
	dragSlider(false)
{}

Scene::~Scene() {
	setPopup(nullptr);
	clear(widgets);
}

void Scene::switchMenu(const vector<Widget*>& wgts) {
	// reset values and set widgets
	focWidget = nullptr;
	stamp.reset();
	dragSlider = false;

	setPopup(nullptr);	// resets capture and focwidget
	clear(widgets);
	widgets = wgts;

	// additional stuff
	updateFocWidget();
	World::winSys()->setRedrawNeeded();
}

void Scene::resizeMenu() {
	resizeWidgets(widgets);
	if (popup)
		resizeWidgets(popup->widgets);

	World::winSys()->setRedrawNeeded();
}

void Scene::resizeWidgets(vector<Widget*>& wgts) {
	for (Widget* wgt : wgts)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(wgt))
			box->setValues();
}

void Scene::tick(float dSec) {
	// widget ticks
	for (Widget* wgt : widgets)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(wgt))
			box->tick(dSec);

	if (popup)
		for (Widget* wgt : popup->widgets)
			if (ScrollArea* box = dynamic_cast<ScrollArea*>(wgt))
				box->tick(dSec);
}

void Scene::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	updateFocWidget();

	// call mouse move events
	for (Widget* wgt : popup ? popup->widgets : widgets)
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(wgt))
			box->onMouseMove(mPos);

	// handle scrolling
	if (ScrollArea* wgt = dynamic_cast<ScrollArea*>(stamp.widget)) {
		if (dragSlider)
			wgt->dragSlider(mPos.y);
		else {
			ReaderBox* box = dynamic_cast<ReaderBox*>(wgt);
			if (box && (InputSys::isPressedK(SDL_SCANCODE_LSHIFT) || InputSys::isPressedM(SDL_BUTTON_RIGHT)))
				box->scrollListX(-mMov.x);
			wgt->scrollList(-mMov.y);
		}
	}
}

void Scene::updateFocWidget() {
	vec2i mPos = World::inputSys()->mousePos();
	for (Widget* wgt : popup ? popup->widgets : widgets)
		if (inRect(wgt->rect(), mPos)) {
			focWidget = wgt;
			return;
		}
	focWidget = nullptr;	// nothing found
}

void Scene::onMouseDown(const vec2i& mPos, ClickType click) {
	if (ScrollArea* area = dynamic_cast<ScrollArea*>(focWidget))
		if (click.button == SDL_BUTTON_LEFT) {
			// get rid of scroll motion
			area->setMotion(0.f);

			// check slider click
			if (inRect(area->barRect(), mPos)) {
				dragSlider = true;
				if (mPos.y < area->sliderY() || mPos.y > area->sliderY() + area->getSliderH())	// if mouse outside of slider but inside bar
					area->dragSlider(mPos.y - area->getSliderH() /2);
				area->diffSliderMouseY = mPos.y - area->sliderY();	// get difference between mouse y and slider y
			}

			// handle double click
			if (click.clicks == 2 && focWidget == stamp.widget)
				if (ReaderBox* box = dynamic_cast<ReaderBox*>(area))
					checkReaderBoxDoubleClick(mPos, box);
		}

	// set click stamp
	if (stamp.button == 0)
		stamp = ClickStamp(focWidget, click.button, mPos);
	else {
		stamp.reset();
		dragSlider = false;
	}
}

void Scene::onMouseUp(const vec2i& mPos, ClickType click) {
	if (dragSlider) {
		// reset values
		static_cast<ScrollArea*>(stamp.widget)->diffSliderMouseY = 0;
		dragSlider = false;
	} else if (vec2f(mPos - stamp.mPos).len() > Default::clickThreshold) {
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.widget))	// if clicked widget is a scroll area, give it some scrolling motion
			box->setMotion(-World::inputSys()->getMouseMove().y);
	} else if (focWidget == stamp.widget) {	// if cursor hasn't moved too far check for item click
		if (Button* but = dynamic_cast<Button*>(focWidget))
			but->onClick(click);
		else if (LineEditor* edt = dynamic_cast<LineEditor*>(focWidget))
			edt->onClick(click);
		else if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(focWidget))
			checkScrollAreaItemsClick(mPos, box, click);
		else if (ReaderBox* box = dynamic_cast<ReaderBox*>(focWidget))
			checkReaderBoxClick(mPos, box, click);
	}
	stamp.reset();
}

void Scene::checkScrollAreaItemsClick(const vec2i& mPos, ScrollAreaItems* wgt, ClickType click) {
	SDL_Rect bg = wgt->rect();
	vec2t interval = wgt->visibleItems();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect rect = wgt->itemRect(i);
		cropRect(rect, bg);

		if (inRect(rect, mPos)) {
			wgt->item(i)->onClick(click);
			break;
		}
	}
}

void Scene::checkReaderBoxClick(const vec2i& mPos, ReaderBox* wgt, ClickType click) {
	if (wgt->showList()) {	// check list buttons
		for (Widget* it : wgt->getListWidgets())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->rect(), mPos)) {
					but->onClick(click);
					break;
				}
	} else if (wgt->showPlayer()) {	// check player buttons
		for (Widget* it : wgt->getPlayerWidgets())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->rect(), mPos)) {
					but->onClick(click);
					break;
				}
	}
}

void Scene::checkReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* wgt) {
	SDL_Rect bg = wgt->rect();
	vec2t interval = wgt->visiblePictures();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect rect = wgt->image(i).rect();
		cropRect(rect, bg);

		if (inRect(rect, mPos)) {
			wgt->setZoom(float(wgt->size().x) / float(wgt->texture(i)->getRes().x));
			break;
		}
	}
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(getFocWidget())) {
		box->scrollList(wMov.y*-Default::scrollFactorWheel);
		if (ReaderBox* rdr = dynamic_cast<ReaderBox*>(box))
			rdr->scrollListX(wMov.x*-Default::scrollFactorWheel);
	}
}

void Scene::onMouseLeave() {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.widget))	// if it's a scroll area, get rid of scrolling motion
		box->setMotion(0.f);
	stamp.reset();
	dragSlider = false;
}

Program& Scene::getProgram() {
	return program;
}

Library& Scene::getLibrary() {
	return library;
}

const vector<Widget*>& Scene::getWidgets() const {
	return widgets;
}

Widget* Scene::getFocWidget() {
	return focWidget;
}

bool Scene::isDraggingSlider(ScrollArea* wgt) const {
	return stamp.widget == wgt && dragSlider;
}

ListItem* Scene::selectedButton() {
	// check focused widget first
	if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(focWidget))
		if (box->selectedItem && box->selectedItem->selectable())
			return box->selectedItem;

	// if failed, look through all widgets
	for (Widget* wgt : widgets)
		if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(wgt))
			if (box->selectedItem && box->selectedItem->selectable())
				return box->selectedItem;

	// maybe popup :/
	if (popup)
		for (Widget* wgt : popup->widgets)
			if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(wgt))
				if (box->selectedItem && box->selectedItem->selectable())
					return box->selectedItem;

	return nullptr;	// nothing found
}

Popup* Scene::getPopup() {
	return popup;
}

void Scene::setPopup(Popup* newPopup, Capturer* capture) {
	popup = newPopup;
	World::inputSys()->setCaptured(capture);
	updateFocWidget();
}
