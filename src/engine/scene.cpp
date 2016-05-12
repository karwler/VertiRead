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
	Clear(objects);
}

void Scene::SwitchMenu(EMenu newMenu, void* dat) {
	// reset values
	Clear(objects);
	popup.reset();
	focObject = 0;
	objectHold = nullptr;

	// little conveniences
	vec2i res = World::winSys()->Resolution();
	vec2i posT(-1, -1);
	vec2i sizT(140, 40);

	switch (newMenu) {
	case EMenu::books: {
		// top buttons
		objects = {
			new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3-10, 50), FIX_Y | FIX_H), &Program::Event_OpenPlaylistList, "Playlists"),
			new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3-10, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, "Settings"),
			new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, "Exit")
		};

		// book list
		vector<ListItem*> tiles;
		vector<fs::path> names = Filer::ListDir(sets.libraryParh(), FILTER_DIR);
		for (fs::path& it : names)
			tiles.push_back(new ItemButton(it.filename().string(), it.string(), &Program::Event_OpenBrowser));
		objects.push_back(new TileBox(Object(vec2i(0, 60), posT, vec2i(res.x, res.y-60), FIX_POS | FIX_END), tiles, vec2i(400, 30)));
		focObject = objects.size()-1;
		break; }
	case EMenu::browser: {
		// back button
		objects = { new ButtonText(Object(vec2i(0, 0), posT, vec2i(90, 40), FIX_POS | FIX_SIZ), &Program::Event_Back, "Back") };

		// list
		vector<ListItem*> items;
		Browser* browser = static_cast<Browser*>(dat);
		for (fs::path& it : browser->ListDirs())
			items.push_back(new ItemButton(it.filename().string(), "", &Program::Event_OpenBrowser));
		for (fs::path& it : browser->ListFiles())
			items.push_back(new ItemButton(it.filename().string(), it.string(), &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(100, 0), posT, vec2i(res.x-100, res.y), FIX_POS | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::reader: {
		// reader box
		fs::path file = cstr(dat);
		library->LoadPics(Filer::GetPicsFromDir(file.parent_path()));
		objects = { new ReaderBox(library->Pictures(), file.string()) };
		focObject = objects.size()-1;
		break; }
	case EMenu::playlists: {
		// top buttons
		sizT = vec2i(80, 30);
		objects = {
			new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3-10, 50), FIX_Y | FIX_H), &Program::Event_OpenBookList, "Library"),
			new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3-10, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, "Settings"),
			new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, "Exit"),

			new ButtonText(Object(vec2i(0,   60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_AddButtonClick, "New"),
			new ButtonText(Object(vec2i(90,  60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_EditButtonClick, "Edit"),
			new ButtonText(Object(vec2i(180, 60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_DeleteButtonClick, "Del")
		};

		// playlist list
		TileBox* box = new TileBox(Object(vec2i(0, 100), posT, vec2i(res.x, res.y-100), FIX_POS | FIX_END), {}, vec2i(400, 30));
		vector<ListItem*> tiles;
		vector<fs::path> names = Filer::ListDir(sets.playlistParh(), FILTER_FILE);
		for (fs::path& it : names)
			tiles.push_back(new ListItem(it.filename().string(), box));
		box->Items(tiles);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::plistEditor: {
		// option buttons
		sizT.x = 100;
		objects = {
			new ButtonText(Object(vec2i(0, 50),  posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_AddButtonClick, "Add"),
			new ButtonText(Object(vec2i(0, 100), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_EditButtonClick, "Edit"),
			new ButtonText(Object(vec2i(0, 150), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_DeleteButtonClick, "Del"),
			new ButtonText(Object(vec2i(0, 200), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_SaveButtonClick, "Save"),
			new ButtonText(Object(vec2i(0, 250), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_Back, "Close")
		};

		// playlist list
		ListBox* box = new ListBox(Object(vec2i(110, 0), posT, vec2i(res.x-110, res.y), FIX_POS | FIX_END, EColor::background));
		vector<ListItem*> items;
		PlaylistEditor* editor = static_cast<PlaylistEditor*>(dat);
		if (editor->showSongs) {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_SwitchButtonClick, "Books"));
			for (fs::path& it : editor->getPlaylist().songs)
				items.push_back(new ItemButton(it.filename().string(), "", &Program::Event_SelectionSet, box));
		}
		else {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_SwitchButtonClick, "Songs"));
			for (string& it : editor->getPlaylist().books)
				items.push_back(new ItemButton(it, "", &Program::Event_SelectionSet, box));
		}
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::generalSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenVideoSettings, "Video"),
			new ButtonText(Object(vec2i(0, 50),  posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenAudioSettings, "Audio"),
			new ButtonText(Object(vec2i(0, 100), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenControlsSettings, "Controls"),
			new ButtonText(Object(vec2i(0, 150), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_Back, "Back")
		};

		vector<Object*> items = {
			// something
		};
		objects.push_back(new ObjectBox(Object(vec2i(160, 0), posT, vec2i(res.x-160, res.y), FIX_POS | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::videoSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenGeneralSettings, "General"),
			new ButtonText(Object(vec2i(0, 50),  posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenAudioSettings, "Audio"),
			new ButtonText(Object(vec2i(0, 100), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenControlsSettings, "Controls"),
			new ButtonText(Object(vec2i(0, 150), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_Back, "Back")
		};

		vector<Object*> items = {
			// something
		};
		objects.push_back(new ObjectBox(Object(vec2i(160, 0), posT, vec2i(res.x-160, res.y), FIX_POS | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::audioSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenGeneralSettings, "General"),
			new ButtonText(Object(vec2i(0, 50),  posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenVideoSettings, "Video"),
			new ButtonText(Object(vec2i(0, 100), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_OpenControlsSettings, "Controls"),
			new ButtonText(Object(vec2i(0, 150), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_Back, "Back")
		};

		vector<Object*> items = {
			// something
		};
		objects.push_back(new ObjectBox(Object(vec2i(160, 0), posT, vec2i(res.x-160, res.y), FIX_POS | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::controlsSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   posT, sizT, FIX_ALL), &Program::Event_OpenGeneralSettings, "General"),
			new ButtonText(Object(vec2i(0, 50),  posT, sizT, FIX_ALL), &Program::Event_OpenVideoSettings, "Video"),
			new ButtonText(Object(vec2i(0, 100), posT, sizT, FIX_ALL), &Program::Event_OpenAudioSettings, "Audio"),
			new ButtonText(Object(vec2i(0, 150), posT, sizT, FIX_ALL), &Program::Event_Back, "Back")
		};

		vector<Object*> items = {
			// something
		};
		objects.push_back(new ObjectBox(Object(vec2i(160, 0), posT, vec2i(res.x-160, res.y), FIX_POS | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		}
	}
	World::engine->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	// for some reason the next three lines crash which is why they've been temporarily moved to the WindowSystem::DrawObjects function
	for (Object* obj : objects) {
		ScrollArea* box = dynamic_cast<ScrollArea*>(obj);
		if (box)
			box->SetValues();
	}
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
		if (dynamic_cast<ReaderBox*>(it))
			static_cast<ReaderBox*>(it)->Tick();
		else if (dynamic_cast<Popup*>(it))
			static_cast<Popup*>(it)->Tick();
	}
}

void Scene::OnMouseDown(bool doubleclick) {
	// check if there's a popup window
	if (popup) {
		if (!inRect(popup->getRect(), InputSys::mousePos()))
			return;

		if (dynamic_cast<PopupChoice*>(popup.get())) {
			if (CheckPopupChoiceClick(static_cast<PopupChoice*>(popup.get())))
				return;
		}
		else if (dynamic_cast<PopupMessage*>(popup.get())) {
			if (CheckPopupSimpleClick(static_cast<PopupMessage*>(popup.get())))
				return;
		}
		return;
	}

	// check other objects
	for (Object* obj : objects) {
		if (!inRect(obj->getRect(), InputSys::mousePos()))	// skip if mouse isn't over object
			continue;

		if (dynamic_cast<Button*>(obj)) {
			dynamic_cast<Button*>(obj)->OnClick();
			break;
		}
		else if (dynamic_cast<ScrollArea*>(obj)) {
			static_cast<ScrollArea*>(obj)->selectedItem = nullptr;	// deselect all items

			if (CheckSliderClick(static_cast<ScrollArea*>(obj)))	// first check if slider is clicked
				break;
			else if (dynamic_cast<ListBox*>(obj)) {
				if (CheckListBoxClick(static_cast<ListBox*>(obj), doubleclick))
					break;
			}
			else if (dynamic_cast<TileBox*>(obj)) {
				if (CheckTileBoxClick(static_cast<TileBox*>(obj), doubleclick))
					break;
			}
			else if (dynamic_cast<ReaderBox*>(obj)) {
				if (CheckReaderBoxClick(static_cast<ReaderBox*>(obj), doubleclick))
					break;
			}
		}

	}
}

bool Scene::CheckSliderClick(ScrollArea* obj) {
	vec2i mPos = InputSys::mousePos();
	if (inRect(obj->Bar(), mPos)) {
		objectHold = obj;
		if (mPos.y < obj->SliderY() || mPos.y > obj->SliderY() + obj->SliderH())	// if mouse outside of slider but inside bar
			obj->DragSlider(mPos.y - obj->SliderH() /2);
		obj->diffSliderMouseY = mPos.y - obj->SliderY();	// get difference between mouse y and slider y

		ReaderBox* box = dynamic_cast<ReaderBox*>(obj);
		if (box)
			box->sliderFocused = true;
		return true;
	}
	return false;
}

bool Scene::CheckListBoxClick(ListBox* obj, bool doubleclick) {
	vec2i interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop);
		if (inRect(rect, InputSys::mousePos())) {
			items[i]->OnClick(doubleclick);
			return true;
		}
	}
	return false;
}

bool Scene::CheckTileBoxClick(TileBox* obj, bool doubleclick) {
	vec2i interval = obj->VisibleItems();
	const vector<ListItem*>& items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++)
		if (inRect(obj->ItemRect(i), InputSys::mousePos())) {
			items[i]->OnClick(doubleclick);
			return true;
		}
	return false;
}

bool Scene::CheckReaderBoxClick(ReaderBox* obj, bool doubleclick) {
	vec2i mPos = InputSys::mousePos();
	if (obj->showList())	// check list buttons
		for (ButtonImage& but : obj->ListButtons())
			if (inRect(but.getRect(), mPos)) {
				but.OnClick();
				return true;
			}
	if (obj->showPlayer())	// check player buttons
		for (ButtonImage& but : obj->PlayerButtons())
			if (inRect(but.getRect(), mPos)) {
				but.OnClick();
				return true;
			}

	if (doubleclick) {
		vec2i interval = obj->VisiblePictures();
		const vector<Image>& pics = obj->Pictures();
		for (int i=interval.x; i<=interval.y; i++) {
			SDL_Rect rect = pics[i].getRect();
			if (inRect(rect, mPos)) {
				obj->Zoom(float(obj->Size().x) / float(pics[i].size.x));
				return true;
			}
		}
	}
	else {
		vec2i pos = obj->Pos();
		vec2i size = obj->Size();
		if (inRect({pos.x, pos.y, size.x-obj->barW, size.y}, mPos)) {	// init list mouse drag
			objectHold = obj;
			return true;
		}
	}
	return false;
}

bool Scene::CheckPopupSimpleClick(PopupMessage* obj) {
	if (inRect(obj->CancelButton(), InputSys::mousePos())) {
		SetPopup(nullptr);
		return true;
	}
	return false;
}

bool Scene::CheckPopupChoiceClick(PopupChoice* obj) {
	if (inRect(obj->OkButton(), InputSys::mousePos())) {
		PopupText* poptext = dynamic_cast<PopupText*>(obj);
		if (poptext)
			program->Event_TextCaptureOk(poptext->Line()->Editor());
	}
	else if (inRect(obj->CancelButton(), InputSys::mousePos()))
		SetPopup(nullptr);
	else
		return false;
	return true;
}

void Scene::OnMouseUp() {
	if (objectHold) {
		// reset values
		ReaderBox* box = dynamic_cast<ReaderBox*>(objectHold);
		if (box)
			box->sliderFocused = false;

		objectHold->diffSliderMouseY = 0;
		objectHold = nullptr;
	}
}

void Scene::OnMouseWheel(int ymov) {
	ScrollArea* box = dynamic_cast<ScrollArea*>(FocusedObject());
	if (box)
		box->ScrollList(ymov*-20);
}

const GeneralSettings&Scene::Settings() const {
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
	ScrollArea* sabox = dynamic_cast<ScrollArea*>(FocusedObject());
	if (sabox)
		if (sabox->selectedItem && sabox->selectedItem->selectable())
			return sabox->selectedItem;

	// if failed, look through all objects
	for (Object* obj : objects) {
		ScrollArea* box = dynamic_cast<ScrollArea*>(obj);
		if (box)
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

	PopupText* poptext = dynamic_cast<PopupText*>(box);
	if (poptext)
		World::inputSys()->SetCapture(poptext->Line());
	else
		World::inputSys()->SetCapture(nullptr);

	World::engine->SetRedrawNeeded();
}
