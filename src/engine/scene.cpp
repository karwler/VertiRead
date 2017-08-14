#include "world.h"

Scene::Scene(const GeneralSettings& SETS) :
	library(Filer::getGeneralSettings()),
	focObject(nullptr),
	dragSlider(false)
{
	library.init(World::winSys()->getSettings().getFontpath());
}

Scene::~Scene() {
	setPopup(nullptr);
	clear(objects);
}

void Scene::switchMenu(const vector<Object*>& objs) {
	// reset values and set objects
	focObject = nullptr;
	stamp.reset();
	dragSlider = false;

	setPopup(nullptr);	// resets capture and focObject
	clear(objects);
	objects = objs;

	// additional stuff
	onMouseMove(InputSys::mousePos(), 0);
	World::winSys()->setRedrawNeeded();
}

void Scene::resizeMenu() {
	resizeObjects(objects);
	if (popup)
		resizeObjects(popup->objects);

	World::winSys()->setRedrawNeeded();
}

void Scene::resizeObjects(vector<Object*>& objs) {
	for (Object* obj : objs)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->setValues();
}

void Scene::tick(float dSec) {
	World::inputSys()->checkAxisShortcuts();	// handle keyhold

	// object ticks
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->tick(dSec);

	if (popup)
		for (Object* obj : popup->objects)
			if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
				box->tick(dSec);
}

void Scene::onMouseMove(const vec2i& mPos, const vec2i& mMov) {
	// set focused object and call mouse move events
	focObject = nullptr;
	for (Object* obj : popup ? popup->objects : objects) {
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
			box->onMouseMove(mPos);

		if (inRect(obj->rect(), mPos)) {
			focObject = obj;
			break;
		}
	}

	// handle scrolling
	if (ScrollArea* obj = dynamic_cast<ScrollArea*>(stamp.object)) {
		if (dragSlider)
			obj->dragSlider(mPos.y);
		else {
			ReaderBox* box = dynamic_cast<ReaderBox*>(obj);
			if (box && (InputSys::isPressedK(SDL_SCANCODE_LSHIFT) || InputSys::isPressedM(SDL_BUTTON_RIGHT)))
				box->scrollListX(-mMov.x);
			obj->scrollList(-mMov.y);
		}
	}
}

void Scene::onMouseDown(const vec2i& mPos, ClickType click) {
	if (ScrollArea* area = dynamic_cast<ScrollArea*>(focObject))
		if (click.button == SDL_BUTTON_LEFT) {
			// get rid of scroll motion and deselect all items
			area->setMotion(0.f);
			area->selectedItem = nullptr;
			World::winSys()->setRedrawNeeded();

			// check slider click
			if (inRect(area->barRect(), mPos)) {
				dragSlider = true;
				if (mPos.y < area->sliderY() || mPos.y > area->sliderY() + area->getSliderH())	// if mouse outside of slider but inside bar
					area->dragSlider(mPos.y - area->getSliderH() /2);
				area->diffSliderMouseY = mPos.y - area->sliderY();	// get difference between mouse y and slider y
			}

			// hangle double click
			if (click.clicks == 2 && focObject == stamp.object)
				if (ReaderBox* box = dynamic_cast<ReaderBox*>(area))
					checkReaderBoxDoubleClick(mPos, box);
		}

	// set click stamp
	if (stamp.button == 0)
		stamp = ClickStamp(focObject, click.button, mPos);
	else {
		stamp.reset();
		dragSlider = false;
	}
}

void Scene::onMouseUp(const vec2i& mPos, ClickType click) {
	if (dragSlider) {
		// reset values
		static_cast<ScrollArea*>(stamp.object)->diffSliderMouseY = 0;
		dragSlider = false;
	} else if (vec2f(mPos - stamp.mPos).len() > Default::clickThreshold) {
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.object))	// if clicked object is a scroll area, give it some scrolling motion
			box->setMotion(-World::inputSys()->mosueMove().y);
	} else if (focObject == stamp.object) {	// if cursor hasn't moved too far check for item click 
		if (Button* but = dynamic_cast<Button*>(focObject))
			but->onClick(click);
		else if (LineEditor* edt = dynamic_cast<LineEditor*>(focObject))
			edt->onClick(click);
		else if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(focObject))
			checkScrollAreaItemsClick(mPos, box, click);
		else if (ReaderBox* box = dynamic_cast<ReaderBox*>(focObject))
			checkReaderBoxClick(mPos, box, click);
	}
	stamp.reset();
}

void Scene::checkScrollAreaItemsClick(const vec2i& mPos, ScrollAreaItems* obj, ClickType click) {
	vec2t interval = obj->visibleItems();
	vector<ListItem*> items = obj->getItems();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->itemRect(i), mPos)) {
			items[i]->onClick(click);
			break;
		}
}

void Scene::checkReaderBoxClick(const vec2i& mPos, ReaderBox* obj, ClickType click) {
	if (obj->showList()) {	// check list buttons
		for (Object* it : obj->getListObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->rect(), mPos)) {
					but->onClick(click);
					break;
				}
	} else if (obj->showPlayer()) {	// check player buttons
		for (Object* it : obj->getPlayerObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->rect(), mPos)) {
					but->onClick(click);
					break;
				}
	}
}

void Scene::checkReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* obj) {
	vec2t interval = obj->visibleItems();
	const vector<Image>& pics = obj->getPictures();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->image(i, crop).rect();

		if (inRect(cropRect(rect, crop), mPos)) {
			obj->setZoom(float(obj->size().x) / float(pics[i].size.x));
			break;
		}
	}
}

void Scene::onMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(getFocObject())) {
		box->scrollList(wMov.y*-Default::scrollFactorWheel);
		if (ReaderBox* rdr = dynamic_cast<ReaderBox*>(box))
			rdr->scrollListX(wMov.x*-Default::scrollFactorWheel);
	}
}

void Scene::onMouseLeave() {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.object))	// if it's a scroll area, get rid of scrolling motion
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

const vector<Object*>& Scene::getObjects() const {
	return objects;
}

Object* Scene::getFocObject() {
	return focObject;
}

bool Scene::isDraggingSlider(ScrollArea* obj) const {
	return stamp.object == obj && dragSlider;
}

ListItem* Scene::selectedButton() {
	// check focused object first
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(focObject))
		if (box->selectedItem && box->selectedItem->selectable())
			return box->selectedItem;

	// if failed, look through all objects
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			if (box->selectedItem && box->selectedItem->selectable())
				return box->selectedItem;

	// maybe popup :/
	if (popup)
		for (Object* obj : popup->objects)
			if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
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
	onMouseMove(InputSys::mousePos(), 0);
}
