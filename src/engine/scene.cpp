#include "world.h"

Scene::Scene(const GeneralSettings& SETS) :
	sets(SETS),
	library(sets),
	focObject(nullptr),
	dragSlider(false)
{
	Filer::CheckDirectories(sets);
	library.Initialize(World::winSys()->Settings().Fontpath());
}

Scene::~Scene() {
	SetPopup(nullptr);
	clear(objects);
}

void Scene::SwitchMenu(const vector<Object*>& objs) {
	// reset values and set objects
	focObject = nullptr;
	stamp.Reset();
	dragSlider = false;

	SetPopup(nullptr);	// resets capture and focObject
	clear(objects);
	objects = objs;

	// additional stuff
	OnMouseMove(InputSys::mousePos(), 0);
	World::winSys()->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	ResizeObjects(objects);
	if (popup)
		ResizeObjects(popup->objects);

	World::winSys()->SetRedrawNeeded();
}

void Scene::ResizeObjects(vector<Object*>& objs) {
	for (Object* obj : objs)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->SetValues();
}

void Scene::Tick(float dSec) {
	World::inputSys()->CheckAxisShortcuts();	// handle keyhold

	// object ticks
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->Tick(dSec);

	if (popup)
		for (Object* obj : popup->objects)
			if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
				box->Tick(dSec);
}

void Scene::OnMouseMove(const vec2i& mPos, const vec2i& mMov) {
	// set focused object and call mouse move events
	focObject = nullptr;
	for (Object* obj : popup ? popup->objects : objects) {
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
			box->OnMouseMove(mPos);

		if (inRect(obj->getRect(), mPos)) {
			focObject = obj;
			break;
		}
	}

	// handle scrolling
	if (ScrollArea* obj = dynamic_cast<ScrollArea*>(stamp.object)) {
		if (dragSlider)
			obj->DragSlider(mPos.y);
		else {
			ReaderBox* box = dynamic_cast<ReaderBox*>(obj);
			if (box && (InputSys::isPressedK(SDL_SCANCODE_LSHIFT) || InputSys::isPressedM(SDL_BUTTON_RIGHT)))
				box->ScrollListX(-mMov.x);
			obj->ScrollList(-mMov.y);
		}
	}
}

void Scene::OnMouseDown(const vec2i& mPos, ClickType click) {
	if (ScrollArea* area = dynamic_cast<ScrollArea*>(focObject))
		if (click.button == SDL_BUTTON_LEFT) {
			// get rid of scroll motion and deselect all items
			area->SetMotion(0.f);
			area->selectedItem = nullptr;
			World::winSys()->SetRedrawNeeded();

			// check slider click
			if (inRect(area->Bar(), mPos)) {
				dragSlider = true;
				if (mPos.y < area->SliderY() || mPos.y > area->SliderY() + area->SliderH())	// if mouse outside of slider but inside bar
					area->DragSlider(mPos.y - area->SliderH() /2);
				area->diffSliderMouseY = mPos.y - area->SliderY();	// get difference between mouse y and slider y
			}

			// hangle double click
			if (click.clicks == 2 && focObject == stamp.object)
				if (ReaderBox* box = dynamic_cast<ReaderBox*>(area))
					CheckReaderBoxDoubleClick(mPos, box);
		}

	// set click stamp
	if (stamp.button == 0)
		stamp = ClickStamp(focObject, click.button, mPos);
	else {
		stamp.Reset();
		dragSlider = false;
	}
}

void Scene::OnMouseUp(const vec2i& mPos, ClickType click) {
	if (dragSlider) {
		// reset values
		static_cast<ScrollArea*>(stamp.object)->diffSliderMouseY = 0;
		dragSlider = false;
	} else if (vec2f(mPos - stamp.mPos).len() > Default::clickThreshold) {
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.object))
			box->SetMotion(-World::inputSys()->mosueMove().y);
	} else if (focObject == stamp.object) {	// if cursor hasn't moved too far check for item click 
		if (Button* but = dynamic_cast<Button*>(focObject))
			but->OnClick(click);
		else if (LineEditor* edt = dynamic_cast<LineEditor*>(focObject))
			edt->OnClick(click);
		else if (ListBox* box = dynamic_cast<ListBox*>(focObject))
			CheckListBoxClick(mPos, box, click);
		else if (TableBox* box = dynamic_cast<TableBox*>(focObject))
			CheckTableBoxClick(mPos, box, click);
		else if (TileBox* box = dynamic_cast<TileBox*>(focObject))
			CheckTileBoxClick(mPos, box, click);
		else if (ReaderBox* box = dynamic_cast<ReaderBox*>(focObject))
			CheckReaderBoxClick(mPos, box, click);
	}
	stamp.Reset();
}

void Scene::CheckListBoxClick(const vec2i& mPos, ListBox* obj, ClickType click) {
	vec2t interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(click);
			break;
		}
}

void Scene::CheckTableBoxClick(const vec2i& mPos, TableBox* obj, ClickType click) {
	vec2t interval = obj->VisibleItems();
	const grid2<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(click);
			break;
		}
}

void Scene::CheckTileBoxClick(const vec2i& mPos, TileBox* obj, ClickType click) {
	vec2t interval = obj->VisibleItems();
	const vector<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(click);
			break;
		}
}

void Scene::CheckReaderBoxClick(const vec2i& mPos, ReaderBox* obj, ClickType click) {
	if (obj->showList()) {			// check list buttons
		for (Object* it : obj->ListObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick(click);
					break;
				}
	} else if (obj->showPlayer()) {	// check player buttons
		for (Object* it : obj->PlayerObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick(click);
					break;
				}
	}
}

void Scene::CheckReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* obj) {
	vec2t interval = obj->VisibleItems();
	const vector<Image>& pics = obj->Pictures();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->getImage(i, crop).getRect();

		if (inRect(cropRect(rect, crop), mPos)) {
			obj->Zoom(float(obj->Size().x) / float(pics[i].size.x));
			break;
		}
	}
}

void Scene::OnMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject())) {
		box->ScrollList(wMov.y*-Default::scrollFactorWheel);
		if (ReaderBox* rdr = dynamic_cast<ReaderBox*>(box))
			rdr->ScrollListX(wMov.x*-Default::scrollFactorWheel);
	}
}

void Scene::OnMouseLeave() {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(stamp.object))
		box->SetMotion(0.f);
	stamp.Reset();
	dragSlider = false;
}

const GeneralSettings& Scene::Settings() const {
	return sets;
}

void Scene::LibraryPath(const string& dir) {
	sets.DirLib(dir);
}

void Scene::PlaylistsPath(const string& dir) {
	sets.DirPlist(dir);
}

Program* Scene::getProgram() {
	return &program;
}

Library* Scene::getLibrary() {
	return &library;
}

const vector<Object*>& Scene::Objects() const {
	return objects;
}

Object* Scene::FocusedObject() {
	return focObject;
}

bool Scene::isDraggingSlider(ScrollArea* obj) const {
	return stamp.object == obj && dragSlider;
}

ListItem* Scene::SelectedButton() {
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

void Scene::SetPopup(Popup* newPopup, Capturer* capture) {
	popup = newPopup;
	World::inputSys()->SetCapture(capture);
	OnMouseMove(InputSys::mousePos(), 0);
}
