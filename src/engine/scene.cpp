#include "scene.h"
#include "world.h"

Scene::Scene() :
	program(new Program),
	library(new Library),
	objectHold(nullptr)
{}

Scene::~Scene() {
	Clear(objects);
}

void Scene::SwitchMenu(EMenu newMenu, void* dat) {
	// reset values
	Clear(objects);
	focObject = 0;
	objectHold = nullptr;

	// little conveniences
	vec2i res = World::winSys()->Resolution();
	vec2i posT(-1, -1);
	vec2i sizT(140, 40);

	switch (newMenu) {
	case EMenu::books: {
		// top buttons
		objects.push_back(new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3-10, 50), true, true, false, true), &Program::Event_OpenPlaylistList, "Playlists"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3-10, 50), true, true, false, true), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), true, true, false, true), &Program::Event_Back, "Exit"));

		// book list
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirLib(), FILTER_DIR);
		for (fs::path& it : names)
			tiles.push_back(TileItem(it.filename().string(), it.string(), &Program::Event_OpenBrowser));
		objects.push_back(new TileBox(Object(vec2i(0, 60), posT, vec2i(res.x, res.y-60), true, true), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::browser: {
		// back button
		objects.push_back(new ButtonText(Object(vec2i(0, 0), posT, vec2i(90, 40), true, true, true, true), &Program::Event_Back, "Back"));

		// list
		vector<ListItem*> items;
		Browser* browser = static_cast<Browser*>(dat);
		for (fs::path& it : browser->ListDirs())
			items.push_back(new BrowserButton(30, it.filename().string(), "", &Program::Event_OpenBrowser));
		for (fs::path& it : browser->ListFiles())
			items.push_back(new BrowserButton(30, it.filename().string(), it.parent_path().string(), &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(100, 0), posT, vec2i(res.x-100, res.y), true, true, false, false, EColor::background), items));
		focObject = objects.size() - 1;
		break; }
	case EMenu::reader:
		// reader box
		objects.push_back(new ReaderBox(Filer::GetPicsFromDir(cstr(dat))));
		focObject = objects.size() - 1;
		break;
	case EMenu::playlists: {
		// top buttons
		objects.push_back(new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3-10, 50), true, true, false, true), &Program::Event_OpenBookList, "Library"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3-10, 50), true, true, false, true), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), true, true, false, true), &Program::Event_Back, "Exit"));

		// playlist list
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirPlist(), FILTER_FILE);
		for (fs::path& it : names)
			tiles.push_back(TileItem(removeExtension(it.filename()).string(), it.string(), &Program::Event_OpenPlaylistEditor));
		objects.push_back(new TileBox(Object(vec2i(0, 60), posT, vec2i(res.x, res.y-60), true, true), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::plistEditor: {
		// option buttons
		sizT.x = 100;
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   posT, sizT, true, true, true, true), &Program::Event_Back, "Add"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  posT, sizT, true, true, true, true), &Program::Event_Back, "Del"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), posT, sizT, true, true, true, true), &Program::Event_Back, "Edit"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), posT, sizT, true, true, true, true), &Program::Event_Back, "Save"));
		objects.push_back(new ButtonText(Object(vec2i(0, 200), posT, sizT, true, true, true, true), &Program::Event_Back, "Cancel"));

		// playlist list
		vector<ListItem*> items;
		PlaylistEditor* editor = static_cast<PlaylistEditor*>(dat);
		if (editor->showSongs) {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), posT, sizT, true, true, true, true), &Program::Event_Back, "Books"));
			for (fs::path& it : editor->GetPlaylist().songs)
				items.push_back(new ListItem(30, it.filename().string()));
		}
		else {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), posT, sizT, true, true, true, true), &Program::Event_Back, "Songs"));
			for (string& it : editor->GetPlaylist().books)
				items.push_back(new ListItem(30, it));
		}
		objects.push_back(new ListBox(Object(vec2i(110, 0), posT, vec2i(res.x-110, res.y), true, true, false, false, EColor::background), items));
		break; }
	case EMenu::generalSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   posT, sizT, true, true, true, true), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  posT, sizT, true, true, true, true), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), posT, sizT, true, true, true, true), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), posT, sizT, true, true, true, true), &Program::Event_Back, "Back"));
		break;
	case EMenu::videoSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   posT, sizT, true, true, true, true), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  posT, sizT, true, true, true, true), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), posT, sizT, true, true, true, true), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), posT, sizT, true, true, true, true), &Program::Event_Back, "Back"));
		break;
	case EMenu::audioSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   posT, sizT, true, true, true, true), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  posT, sizT, true, true, true, true), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), posT, sizT, true, true, true, true), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), posT, sizT, true, true, true, true), &Program::Event_Back, "Back"));
		break;
	case EMenu::controlsSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   posT, sizT, true, true, true, true), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  posT, sizT, true, true, true, true), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), posT, sizT, true, true, true, true), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), posT, sizT, true, true, true, true), &Program::Event_Back, "Back"));
	}
	World::engine->SetRedrawNeeded();
}

void Scene::ResizeMenu() {
	for (Object* obj : objects) {
		if (obj->isA<ScrollArea>())
			static_cast<ScrollArea*>(obj)->SetValues();
	}
	World::engine->SetRedrawNeeded();
}

void Scene::Tick() {
	// handle keyhold
	if (InputSys::isPressed(SDL_SCANCODE_UP))
		program->Event_Up();
	else if (InputSys::isPressed(SDL_SCANCODE_DOWN))
		program->Event_Down();
	else if (InputSys::isPressed(SDL_SCANCODE_LEFT))
		program->Event_Left();
	else if (InputSys::isPressed(SDL_SCANCODE_RIGHT))
		program->Event_Right();

	// handle mousemove
	if (objectHold) {
		if (objectHold->isA<ReaderBox>() && !static_cast<ReaderBox*>(objectHold)->sliderFocused) {
			if (InputSys::isPressed(SDL_SCANCODE_LCTRL))
				static_cast<ReaderBox*>(objectHold)->ScrollListX(-World::inputSys()->mouseMove().x);
			objectHold->ScrollList(-World::inputSys()->mouseMove().y);
		}
		else
			objectHold->DragSlider(InputSys::mousePos().y);
	}

	// object ticks
	for (Object* it : objects)
		if (it->isA<ReaderBox>())
			static_cast<ReaderBox*>(it)->Tick();
}

void Scene::OnMouseDown() {
	for (Object* obj : objects) {
		if (!inRect(obj->getRect(), InputSys::mousePos()))	// skip if mouse isn't over object
			continue;

		if (obj->isA<Button>()) {
			static_cast<Button*>(obj)->OnClick();
			break;
		}
		else if (obj->isA<ScrollArea>()) {
			if (CheckSliderClick(static_cast<ScrollArea*>(obj)))	// first check if slider is clicked
				break;
			else if (obj->isA<ListBox>()) {
				if (CheckListBoxClick(static_cast<ListBox*>(obj)))
					break;
			}
			else if (obj->isA<TileBox>()) {
				if (CheckTileBoxClick(static_cast<TileBox*>(obj)))
					break;
			}
			else if (obj->isA<ReaderBox>()) {
				if (CheckReaderBoxClick(static_cast<ReaderBox*>(obj)))
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
		if (obj->isA<ReaderBox>())
			static_cast<ReaderBox*>(obj)->sliderFocused = true;
		return true;
	}
	return false;
}

bool Scene::CheckListBoxClick(ListBox* obj) {
	int posY = obj->Pos().y - obj->ListY();
	for (ListItem* it : obj->Items()) {
		if (inRect({obj->Pos().x, posY, obj->Size().x - obj->barW, it->height}, InputSys::mousePos())) {
			it->OnClick();
			return true;
		}
		posY += it->height + obj->Spacing();
	}
	return false;
}

bool Scene::CheckTileBoxClick(TileBox* obj) {
	vector<TileItem>& items = obj->Items();
	for (uint i = 0; i != items.size(); i++) {
		int row = i / obj->TilesPerRow();
		int posX = (i - row * obj->TilesPerRow()) * (obj->TileSize().x + obj->Spacing()) + obj->Pos().x;
		int posY = row * (obj->TileSize().y + obj->Spacing()) + obj->Pos().y;

		if (inRect({ posX, posY, obj->TileSize().x, obj->TileSize().y }, InputSys::mousePos())) {
			items[i].OnClick();
			return true;
		}
	}
	return false;
}

bool Scene::CheckReaderBoxClick(ReaderBox* obj) {
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
	if (inRect({obj->Pos().x, obj->Pos().y, obj->Size().x-obj->barW, obj->Size().y}, mPos)) {	// init list mouse drag
		objectHold = obj;
		return true;
	}
	return false;
}

void Scene::OnMouseUp() {
	if (objectHold) {
		// reset values
		if (objectHold->isA<ReaderBox>())
			static_cast<ReaderBox*>(objectHold)->sliderFocused = false;
		objectHold->diffSliderMouseY = 0;
		objectHold = nullptr;
	}
}

void Scene::OnMouseWheel(int ymov) {
	if (FocusedObject()->isA<ScrollArea>())
		static_cast<ScrollArea*>(FocusedObject())->ScrollList(ymov*-20);
}

Program* Scene::getProgram() const {
	return program;
}

const vector<Object*>& Scene::Objects() const {
	return objects;
}

Object* Scene::FocusedObject() const {
	return (objects[focObject]) ? objects[focObject] : nullptr;
}
