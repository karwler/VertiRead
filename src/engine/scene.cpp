#include "scene.h"
#include "world.h"

Scene::Scene(const GeneralSettings& SETS) :
	sets(SETS),
	program(new Program),
	library(new Library(World::winSys()->Settings().FontPath(), Filer::GetTextures(), Filer::GetSounds())),
	objectHold(nullptr)
{
	Filer::CheckDirectories(sets);
}

Scene::~Scene() {
	clear(objects);
}

void Scene::SwitchMenu(const vector<Object*>& objs, uint focObj) {
	// reset values
	popup.reset();
	focObject = focObj;
	objectHold = nullptr;

	// reset objects
	clear(objects);
	objects = objs;
	
	World::engine->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	for (Object* obj : objects)
		if (ScrollArea* box = dynamic_cast<ScrollArea*>(obj))
			box->SetValues();

	World::engine->SetRedrawNeeded();
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
			if (InputSys::isPressed(SDL_SCANCODE_LSHIFT))
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

void Scene::OnMouseDown(bool doubleclick) {
	// first check if there's a popup window
	if (popup)
		CheckPopupClick(popup);
	else
		CheckObjectsClick(objects, doubleclick);
}

void Scene::CheckObjectsClick(const vector<Object*>& objs, bool doubleclick) {
	for (Object* obj : objects) {
		if (!inRect(obj->getRect(), InputSys::mousePos()))	// skip if mouse isn't over object
			continue;

		if (Button* but = dynamic_cast<Button*>(obj)) {
			but->OnClick();
			break;
		}
		else if (Capturer* cap = dynamic_cast<Capturer*>(obj)) {
			cap->OnClick();
			break;
		}
		else if (ScrollArea* area = dynamic_cast<ScrollArea*>(obj)) {
			area->selectedItem = nullptr;	// deselect all items

			if (CheckSliderClick(area))		// first check if slider is clicked
				break;
			else if (ListBox* box = dynamic_cast<ListBox*>(area)) {
				CheckListBoxClick(box, doubleclick);
				break;
			}
			else if (TileBox* box = dynamic_cast<TileBox*>(area)) {
				CheckTileBoxClick(box, doubleclick);
				break;
			}
			else if (ReaderBox* box = dynamic_cast<ReaderBox*>(area)) {
				CheckReaderBoxClick(box, doubleclick);
				break;
			}
		}

	}
}

void Scene::CheckPopupClick(Popup* obj) {
	if (!inRect(popup->getRect(), InputSys::mousePos()))
		return;

	if (PopupChoice* box = dynamic_cast<PopupChoice*>(obj))
		CheckPopupChoiceClick(box);
	else if (PopupMessage* box = dynamic_cast<PopupMessage*>(obj))
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

void Scene::CheckListBoxClick(ListBox* obj, bool doubleclick) {
	vec2i interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), InputSys::mousePos())) {
			items[i]->OnClick(doubleclick);
			break;
		}
}

void Scene::CheckTileBoxClick(TileBox* obj, bool doubleclick) {
	vec2i interval = obj->VisibleItems();
	const vector<ListItem*>& items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), InputSys::mousePos())) {
			items[i]->OnClick(doubleclick);
			break;
		}
}

void Scene::CheckReaderBoxClick(ReaderBox* obj, bool doubleclick) {
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
	else if (doubleclick) {
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
		if (inRect({pos.x, pos.y, size.x-obj->barW, size.y}, mPos))		// init list mouse drag
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
			program->Event_TextCaptureOk(poptext->LEdit()->Editor());
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

const GeneralSettings& Scene::Settings() const {
	return sets;
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
	popup = box;

	if (PopupText* poptext = dynamic_cast<PopupText*>(box))
		World::inputSys()->SetCapture(poptext->LEdit());
	else
		World::inputSys()->SetCapture(nullptr);

	World::engine->SetRedrawNeeded();
}
