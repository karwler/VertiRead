#include "world.h"

Scene::Scene(const GeneralSettings& SETS) :
	sets(SETS),
	library(sets),
	objectHold(nullptr)
{
	Filer::CheckDirectories(sets);
	library.Initialize(World::winSys()->Settings().Fontpath());
}

Scene::~Scene() {
	clear(objects);
}

void Scene::SwitchMenu(const vector<Object*>& objs) {
	// reset values
	objectHold = nullptr;
	World::inputSys()->SetCapture(nullptr);

	// reset objects
	popup.clear();
	clear(objects);
	objects = objs;

	OnMouseMove(World::inputSys()->mousePos(), 0);
	World::engine()->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->SetValues();

	World::engine()->SetRedrawNeeded();
}

void Scene::Tick(float dSec) {
	World::inputSys()->CheckAxisShortcuts();	// handle keyhold

	// object ticks
	for (Object* it : objects) {
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(it))
			box->Tick(dSec);
		else if (Popup* box = dynamic_cast<Popup*>(it))
			box->Tick(dSec);
	}
}

void Scene::OnMouseMove(const vec2i& mPos, const vec2i& mMov) {
	// set focused object and call mouse move events
	focObject.sl = false;
	for (size_t i=0; i!=objects.size(); i++) {
		if (inRect(objects[i]->getRect(), mPos))
			focObject = i;

		if (ReaderBox* box = dynamic_cast<ReaderBox*>(objects[i]))
			box->OnMouseMove(mPos);
	}

	// handle scrolling
	if (objectHold) {
		ReaderBox* box = dynamic_cast<ReaderBox*>(objectHold);
		if (box && !box->sliderFocused) {
			if (InputSys::isPressedK(SDL_SCANCODE_LSHIFT) || InputSys::isPressedM(SDL_BUTTON_RIGHT))
				box->ScrollListX(-mMov.x);
			objectHold->ScrollList(-mMov.y);
		}
		else
			objectHold->DragSlider(mPos.y);
	}
}

void Scene::OnMouseDown(const vec2i& mPos, EClick clickType, bool handleHold) {
	// first check if there's a popup window
	if (popup)
		CheckPopupClick(mPos);
	else
		CheckObjectsClick(mPos, clickType, handleHold);
}

void Scene::CheckObjectsClick(const vec2i& mPos, EClick clickType, bool handleHold) {
	Object* obj = FocusedObject();
	if (Button* but = dynamic_cast<Button*>(obj)) {
		if (clickType == EClick::left || clickType == EClick::left_double)
			but->OnClick();
	}
	else if (LineEditor* edt = dynamic_cast<LineEditor*>(obj))
		edt->OnClick(clickType);
	else if (ScrollArea* area = dynamic_cast<ScrollArea*>(obj)) {
		if (clickType == EClick::left || clickType == EClick::left_double) {
			area->selectedItem = nullptr;		// deselect all items
			World::engine()->SetRedrawNeeded();
		}

		if (handleHold && CheckSliderClick(mPos, area))	// first check if slider is clicked
			return;
		if (ListBox* box = dynamic_cast<ListBox*>(area))
			CheckListBoxClick(mPos, box, clickType);
		else if (TableBox* box = dynamic_cast<TableBox*>(area))
			CheckTableBoxClick(mPos, box, clickType);
		else if (TileBox* box = dynamic_cast<TileBox*>(area))
			CheckTileBoxClick(mPos, box, clickType);
		else if (ReaderBox* box = dynamic_cast<ReaderBox*>(area))
			CheckReaderBoxClick(mPos, box, clickType, handleHold);
	}
}

void Scene::CheckPopupClick(const vec2i& mPos) {
	if (!inRect(popup->getRect(), mPos))
		return;

	if (PopupChoice* box = dynamic_cast<PopupChoice*>(popup.get()))
		CheckPopupChoiceClick(mPos, box);
	else if (PopupMessage* box = dynamic_cast<PopupMessage*>(popup.get()))
		CheckPopupSimpleClick(mPos, box);
}

bool Scene::CheckSliderClick(const vec2i& mPos, ScrollArea* obj) {
	if (inRect(obj->Bar(), mPos)) {
		objectHold = obj;
		if (mPos.y < obj->SliderY() || mPos.y > obj->SliderY() + obj->SliderH())	// if mouse outside of slider but inside bar
			obj->DragSlider(mPos.y - obj->SliderH() /2);
		obj->diffSliderMouseY = mPos.y - obj->SliderY();	// get difference between mouse y and slider y

		if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
			box->sliderFocused = true;
		return true;
	}
	return false;
}

void Scene::CheckListBoxClick(const vec2i& mPos, ListBox* obj, EClick clickType) {
	vec2t interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(clickType);
			break;
		}
}

void Scene::CheckTableBoxClick(const vec2i& mPos, TableBox* obj, EClick clickType) {
	vec2t interval = obj->VisibleItems();
	const grid2<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(clickType);
			break;
		}
}

void Scene::CheckTileBoxClick(const vec2i& mPos, TileBox* obj, EClick clickType) {
	vec2t interval = obj->VisibleItems();
	const vector<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), mPos)) {
			items[i]->OnClick(clickType);
			break;
		}
}

void Scene::CheckReaderBoxClick(const vec2i& mPos, ReaderBox* obj, EClick clickType, bool handleHold) {
	if (obj->showList()) {		// check list buttons
		for (Object* it : obj->ListObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick();
					break;
				}
	}
	else if (obj->showPlayer()) {	// check player buttons
		for (Object* it : obj->PlayerObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick();
					break;
				}
	}
	else if (clickType == EClick::left_double) {
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
	else if (handleHold) {
		vec2i pos = obj->Pos();
		vec2i size = obj->Size();
		if (inRect({pos.x, pos.y, size.x-obj->BarW(), size.y}, mPos))		// init list mouse drag
			objectHold = obj;
	}
}

void Scene::CheckPopupSimpleClick(const vec2i& mPos, PopupMessage* obj) {
	if (inRect(obj->CancelButton(), mPos))
		SetPopup(nullptr);
}

void Scene::CheckPopupChoiceClick(const vec2i& mPos, PopupChoice* obj) {
	if (inRect(obj->OkButton(), mPos)) {
		if (PopupText* poptext = dynamic_cast<PopupText*>(obj))
			program.Event_TextCaptureOk(poptext->LEdit()->Editor()->Text());
	}
	else if (inRect(obj->CancelButton(), mPos)) {
		World::audioSys()->PlaySound("back");
		SetPopup(nullptr);
	}
}

void Scene::OnMouseUp() {
	if (objectHold) {
		// reset values
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(objectHold))
			box->sliderFocused = false;

		objectHold->diffSliderMouseY = 0;
		objectHold = nullptr;
	}
}

void Scene::OnMouseWheel(const vec2i& wMov) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject())) {
		box->ScrollList(wMov.y*-10);
		if (ReaderBox* rdr = dynamic_cast<ReaderBox*>(box))
			rdr->ScrollListX(wMov.x*-10);
	}
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

vector<Object*> Scene::Objects() {
	vector<Object*> objs = objects;	// return objects
	objs.push_back(popup);			// append popup
	return objs;
}

Object* Scene::FocusedObject() {
	if (popup)
		return popup;
	if (focObject.sl)
		return objects[focObject.id];
	return nullptr;
}

ListItem* Scene::SelectedButton() {
	// check focused object first
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject()))
		if (box->selectedItem && box->selectedItem->selectable())
			return box->selectedItem;

	// if failed, look through all objects
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			if (box->selectedItem && box->selectedItem->selectable())
				return box->selectedItem;

	return nullptr;	// nothing found
}

Popup* Scene::getPopup() {
	return popup;
}

void Scene::SetPopup(Popup* box) {
	if (PopupText* poptext = dynamic_cast<PopupText*>(box))
		World::inputSys()->SetCapture(poptext->LEdit());
	else
		World::inputSys()->SetCapture(nullptr);
	popup = box;

	World::engine()->SetRedrawNeeded();
}
