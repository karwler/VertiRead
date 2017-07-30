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
	DelPopup();
	clear(objects);
}

void Scene::SwitchMenu(const vector<Object*>& objs) {
	// reset and set objects
	objectHold = nullptr;
	DelPopup();			// resets capture and focObject
	clear(objects);
	objects = objs;

	// additional stuff
	OnMouseMove(World::inputSys()->mousePos(), 0);
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
	for (Object* it : objects)
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(it))
			box->Tick(dSec);
}

void Scene::OnMouseMove(const vec2i& mPos, const vec2i& mMov) {
	// set focused object and call mouse move events;
	focObject.sl = false;
	if (popup)
		MouseMoveObjectOverCheck(popup->objects, mPos);
	else
		MouseMoveObjectOverCheck(objects, mPos);

	// handle scrolling
	if (objectHold) {
		ReaderBox* box = dynamic_cast<ReaderBox*>(objectHold);
		if (box && !box->sliderFocused) {
			if (InputSys::isPressedK(SDL_SCANCODE_LSHIFT) || InputSys::isPressedM(SDL_BUTTON_RIGHT))
				box->ScrollListX(-mMov.x);
			objectHold->ScrollList(-mMov.y);
		} else
			objectHold->DragSlider(mPos.y);
	}
}

void Scene::MouseMoveObjectOverCheck(vector<Object*>& objs, const vec2i& mPos) {
	for (size_t i=0; i!=objs.size(); i++) {
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(objs[i]))
			box->OnMouseMove(mPos);

		if (inRect(objs[i]->getRect(), mPos)) {
			focObject = i;
			break;
		}
	}
}

void Scene::OnMouseDown(const vec2i& mPos, EClick clickType, bool handleHold) {
	Object* obj = FocusedObject();
	if (Button* but = dynamic_cast<Button*>(obj)) {
		if (clickType == EClick::left || clickType == EClick::left_double)
			but->OnClick();
	} else if (LineEditor* edt = dynamic_cast<LineEditor*>(obj))
		edt->OnClick(clickType);
	else if (ScrollArea* area = dynamic_cast<ScrollArea*>(obj)) {
		if (clickType == EClick::left || clickType == EClick::left_double) {
			area->selectedItem = nullptr;		// deselect all items
			World::winSys()->SetRedrawNeeded();
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
	if (obj->showList()) {			// check list buttons
		for (Object* it : obj->ListObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick();
					break;
				}
	} else if (obj->showPlayer()) {	// check player buttons
		for (Object* it : obj->PlayerObjects())
			if (Button* but = dynamic_cast<Button*>(it))
				if (inRect(but->getRect(), mPos)) {
					but->OnClick();
					break;
				}
	} else if (clickType == EClick::left_double) {
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
	} else if (handleHold) {
		vec2i pos = obj->Pos();
		vec2i size = obj->Size();
		if (inRect({pos.x, pos.y, size.x-obj->BarW(), size.y}, mPos))		// init list mouse drag
			objectHold = obj;
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

const vector<Object*>& Scene::Objects() const {
	return objects;
}

Object* Scene::FocusedObject() {
	if (focObject.sl)
		return popup ? popup->objects[focObject.id] : objects[focObject.id];
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

	// maybe popup :/
	if (popup)
		for (Object* obj : popup->objects)
			if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
				if (box->selectedItem && box->selectedItem->selectable())
					return box->selectedItem;

	return nullptr;	// nothing found
}

const Popup* Scene::getPopup() const {
	return popup;
}

void Scene::SetPopupMessage(const string& msg) {
	DelPopup();
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(library.Fonts()->TextSize(msg, 60).x, 120);

	vector<Object*> objs = {
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/2), FIX_SIZ), msg),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2), vec2i(size.x, size.y/2), FIX_SIZ), &Program::Event_Back, "Ok", ETextAlign::center)
	};
	popup = new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs);
}

void Scene::SetPopupChoice(const string& msg, void (Program::*callb)()) {
	DelPopup();
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(library.Fonts()->TextSize(msg, 60).x, 120);

	vector<Object*> objs = {
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), &Program::Event_Back, "Cancel", ETextAlign::center),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	popup = new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs);
}

void Scene::SetPopupText(const string& msg, const string& text, void (Program::*callt)(const string&), void (Program::*callb)()) {
	DelPopup();
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(library.Fonts()->TextSize(msg, 60).x, 180);

	LineEditor* editor = new LineEditor(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), text, ETextType::text, callt, &Program::Event_Back);
	vector<Object*> objs = {
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		editor,
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), &Program::Event_Back, "Cancel", ETextAlign::center),
		new ButtonText(Object(res/2, vec2i(res.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	popup = new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs);
	World::inputSys()->SetCapture(editor);
}

void Scene::DelPopup() {
	popup.clear();
	World::inputSys()->SetCapture(nullptr);
	OnMouseMove(World::inputSys()->mousePos(), 0);
}
