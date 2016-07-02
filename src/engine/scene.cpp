#include "scene.h"
#include "world.h"

Scene::Scene(const GeneralSettings& SETS) :
	sets(SETS),
	program(new Program),
	objectHold(nullptr)
{
	Filer::CheckDirectories(sets);
	library = new Library(World::winSys()->Settings().Fontpath(), sets);
}

Scene::~Scene() {
	clear(objects);
}

void Scene::SwitchMenu(const vector<Object*>& objs, uint focObj) {
	// reset values
	focObject = focObj;
	objectHold = nullptr;

	// reset objects
	popup.reset();
	clear(objects);
	objects = objs;
	
	World::engine()->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->SetValues();

	World::engine()->SetRedrawNeeded();
}

void Scene::Tick() {
	// handle keyhold
	if (World::inputSys()->isPressed("up"))
		program->Event_Up();
	else if (World::inputSys()->isPressed("down"))
		program->Event_Down();
	else if (World::inputSys()->isPressed("left"))
		program->Event_Left();
	else if (World::inputSys()->isPressed("right"))
		program->Event_Right();

	// handle mousemove
	if (objectHold) {
		ReaderBox* box = dynamic_cast<ReaderBox*>(objectHold);
		if (box && !box->sliderFocused) {
			if (InputSys::isPressed(SDL_SCANCODE_LSHIFT) || InputSys::isPressed(SDL_BUTTON_RIGHT))
				box->ScrollListX(-World::inputSys()->mouseMove().x);
			objectHold->ScrollList(-World::inputSys()->mouseMove().y);
		}
		else
			objectHold->DragSlider(InputSys::mousePos().y);
	}

	// object ticks
	for (Object* it : objects) {
		if (ReaderBox* box = dynamic_cast<ReaderBox*>(it))
			box->Tick();
		else if (Popup* box = dynamic_cast<Popup*>(it))
			box->Tick();
	}
}

void Scene::OnMouseDown(EClick clickType) {
	// first check if there's a popup window
	if (popup)
		CheckPopupClick();
	else
		CheckObjectsClick(clickType);
}

void Scene::CheckObjectsClick(EClick clickType) {
	for (Object* obj : objects) {
		if (!inRect(obj->getRect(), InputSys::mousePos()))	// skip if mouse isn't over object
			continue;

		if (Button* but = dynamic_cast<Button*>(obj)) {
			if (clickType == EClick::left)
				but->OnClick();
			break;
		}
		else if (LineEditor* edt = dynamic_cast<LineEditor*>(obj)) {
			edt->OnClick(clickType);
			break;
		}
		else if (ScrollArea* area = dynamic_cast<ScrollArea*>(obj)) {
			if (clickType == EClick::left) {
				area->selectedItem = nullptr;		// deselect all items
				World::engine()->SetRedrawNeeded();
			}

			if (CheckSliderClick(area))			// first check if slider is clicked
				break;
			else if (ListBox* box = dynamic_cast<ListBox*>(area)) {
				CheckListBoxClick(box, clickType);
				break;
			}
			else if (TileBox* box = dynamic_cast<TileBox*>(area)) {
				CheckTileBoxClick(box, clickType);
				break;
			}
			else if (ReaderBox* box = dynamic_cast<ReaderBox*>(area)) {
				CheckReaderBoxClick(box, clickType);
				break;
			}
		}

	}
}

void Scene::CheckPopupClick() {
	if (!inRect(popup->getRect(), InputSys::mousePos()))
		return;

	if (PopupChoice* box = dynamic_cast<PopupChoice*>(popup.get()))
		CheckPopupChoiceClick(box);
	else if (PopupMessage* box = dynamic_cast<PopupMessage*>(popup.get()))
		CheckPopupSimpleClick(box);
}

bool Scene::CheckSliderClick(ScrollArea* obj) {
	vec2i mPos = InputSys::mousePos();
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

void Scene::CheckListBoxClick(ListBox* obj, EClick clickType) {
	vec2i interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), InputSys::mousePos())) {
			items[i]->OnClick(clickType);
			break;
		}
}

void Scene::CheckTileBoxClick(TileBox* obj, EClick clickType) {
	vec2i interval = obj->VisibleItems();
	const vector<ListItem*>& items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), InputSys::mousePos())) {
			items[i]->OnClick(clickType);
			break;
		}
}

void Scene::CheckReaderBoxClick(ReaderBox* obj, EClick clickType) {
	vec2i mPos = InputSys::mousePos();
	if (obj->showList()) {		// check list buttons
		for (ButtonImage& but : obj->ListButtons())
			if (inRect(but.getRect(), mPos)) {
				but.OnClick();
				break;
			}
	}
	else if (obj->showPlayer()) {	// check player buttons
		for (ButtonImage& but : obj->PlayerButtons())
			if (inRect(but.getRect(), mPos)) {
				but.OnClick();
				break;
			}
	}
	else if (clickType == EClick::left_double) {
		vec2i interval = obj->VisiblePictures();
		const vector<Image>& pics = obj->Pictures();
		for (int i=interval.x; i<=interval.y; i++) {
			SDL_Rect crop;
			SDL_Rect rect = obj->getImage(i, &crop).getRect();

			if (inRect(cropRect(rect, crop), mPos)) {
				obj->Zoom(float(obj->Size().x) / float(pics[i].size.x));
				break;
			}
		}
	}
	else {
		vec2i pos = obj->Pos();
		vec2i size = obj->Size();
		if (inRect({pos.x, pos.y, size.x-obj->BarW(), size.y}, mPos))		// init list mouse drag
			objectHold = obj;
	}
}

void Scene::CheckPopupSimpleClick(PopupMessage* obj) {
	if (inRect(obj->CancelButton(), InputSys::mousePos()))
		SetPopup(nullptr);
}

void Scene::CheckPopupChoiceClick(PopupChoice* obj) {
	if (inRect(obj->OkButton(), InputSys::mousePos())) {
		if (PopupText* poptext = dynamic_cast<PopupText*>(obj))
			program->Event_TextCaptureOk(poptext->LEdit()->Editor()->Text());
	}
	else if (inRect(obj->CancelButton(), InputSys::mousePos()))
		SetPopup(nullptr);
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

void Scene::OnMouseWheel(int ymov) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject()))
		box->ScrollList(ymov*-20);
}

GeneralSettings Scene::Settings() const {
	return sets;
}

void Scene::LibraryPath(const string& dir) {
	sets.DirLib(dir);
}

void Scene::PlaylistsPath(const string& dir) {
	sets.DirPlist(dir);
}

Program* Scene::getProgram() const {
	return program;
}

Library* Scene::getLibrary() const {
	return library;
}

vector<Object*> Scene::Objects() const {
	vector<Object*> objs = objects;	// return objects
	objs.push_back(popup);			// append popup
	return objs;
}

Object* Scene::FocusedObject() const {
	return popup ? popup : objects[focObject];
}

ListItem* Scene::SelectedButton() const {
	// check focused object first
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject()))
		if (box->selectedItem && box->selectedItem->selectable())
			return box->selectedItem;

	// if failed, look through all objects
	for (Object* obj : objects) {
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			if (box->selectedItem && box->selectedItem->selectable())
				return box->selectedItem;
	}
	return nullptr;	// nothing found
}

Popup* Scene::getPopup() const {
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
