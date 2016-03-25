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
	Clear(objects);
	focObject = 0;
	objectHold = nullptr;
	vec2i res = World::winSys()->Resolution();


	switch (newMenu) {
	case EMenu::books: {
		objects.push_back(new ButtonText(Object(vec2i(0,         0), vec2i(res.x/3-10, 50), FIX_SY | FIX_EY), &Program::Event_OpenPlaylistList, "Playlists"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3,   0), vec2i(res.x/3-10, 50), FIX_SY | FIX_EY), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3*2, 0), vec2i(res.x/3,    50), FIX_SY | FIX_EY), &Program::Event_Back, "Exit"));
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirLib(), FILTER_DIR);
		for (fs::path& it : names)
			tiles.push_back(TileItem(it.filename().string(), it.string(), &Program::Event_OpenBrowser));
		objects.push_back(new TileBox(Object(vec2i(0, 60), vec2i(res.x, res.y-60), FIX_SX | FIX_SY), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::browser: {
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(90, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Back"));
		vector<ListItem*> items;
		Browser* browser = static_cast<Browser*>(dat);
		for (fs::path& it : browser->ListDirs())
			items.push_back(new BrowserButton(30, it.filename().string(), "", &Program::Event_OpenBrowser));
		for (fs::path& it : browser->ListFiles())
			items.push_back(new BrowserButton(30, it.filename().string(), it.parent_path().string(), &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(100, 0), vec2i(res.x-100, res.y), FIX_SX | FIX_SY, EColor::background), items));
		focObject = objects.size() - 1;
		break; }
	case EMenu::reader:
		objects.push_back(new ReaderBox(Filer::GetPicsFromDir(cstr(dat))));
		focObject = objects.size() - 1;
		break;
	case EMenu::playlists: {
		objects.push_back(new ButtonText(Object(vec2i(0,         0), vec2i(res.x/3-10, 50), FIX_SY | FIX_EY), &Program::Event_OpenBookList, "Library"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3,   0), vec2i(res.x/3-10, 50), FIX_SY | FIX_EY), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x/3*2, 0), vec2i(res.x/3,    50), FIX_SY | FIX_EY), &Program::Event_Back, "Exit"));
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirPlist(), FILTER_FILE);
		for (fs::path& it : names)
			tiles.push_back(TileItem(removeExtension(it.filename()).string(), it.string(), &Program::Event_OpenPlaylistEditor));
		objects.push_back(new TileBox(Object(vec2i(0, 60), vec2i(res.x, res.y-60), FIX_SX | FIX_SY), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::plistEditor: {
		PlaylistEditor* editor = static_cast<PlaylistEditor*>(dat);
		vector<ListItem*> items;
		if (editor->showSongs) {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Books"));
			for (fs::path& it : editor->GetPlaylist().songs)
				items.push_back(new ListItem(30, it.filename().string()));
		}
		else {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Songs"));
			for (string& it : editor->GetPlaylist().books)
				items.push_back(new ListItem(30, it));
		}
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Add"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Del"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Edit"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Save"));
		objects.push_back(new ButtonText(Object(vec2i(0, 200), vec2i(100, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Cancel"));
		objects.push_back(new ListBox(Object(vec2i(110, 0), vec2i(res.x-110, res.y), FIX_SX | FIX_SY, EColor::background), items));
		break; }
	case EMenu::generalSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Back"));
		break;
	case EMenu::videoSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Back"));
		break;
	case EMenu::audioSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Back"));
		break;
	case EMenu::controlsSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0),   vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 50),  vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 100), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 150), vec2i(140, 40), FIX_SX | FIX_SY | FIX_EX | FIX_EY), &Program::Event_Back, "Back"));
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

	for (Object* it : objects)
		if (it->isA<ReaderBox>())
			static_cast<ReaderBox*>(it)->Tick();
}

void Scene::OnMouseDown() {
	for (Object* obj : objects) {
		if (obj->isA<Button>()) {
			if (CheckButtonClick(static_cast<Button*>(obj)))
				break;
		}
		else if (obj->isA<ScrollArea>()) {
			if (CheckSliderClick(static_cast<ScrollArea*>(obj)))
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

bool Scene::CheckButtonClick(Button* obj) {
	if (inRect(obj->getRect(), InputSys::mousePos())) {
		obj->OnClick();
		return true;
	}
	return false;
}

bool Scene::CheckSliderClick(ScrollArea* obj) {
	vec2i mPos = InputSys::mousePos();
	if (inRect(obj->Bar(), mPos)) {
		objectHold = obj;
		if (mPos.y < obj->SliderY() || mPos.y > obj->SliderY() + obj->SliderH())
			obj->DragSlider(mPos.y - obj->SliderH() / 2);
		obj->diffSliderMouseY = mPos.y - obj->SliderY();
		if (obj->isA<ReaderBox>()) {
			ReaderBox* rbox = static_cast<ReaderBox*>(obj);
			rbox->sliderFocused = rbox->showSlider();		// set slider focused only if it's displayed
		}
		return true;
	}
	return false;
}

bool Scene::CheckListBoxClick(ListBox* obj) {
	vector<ListItem*> items = obj->Items();
	int posY = obj->Pos().y + obj->ListY();
	for (uint i = 0; i != items.size(); i++) {
		int itemMax = posY + items[i]->height;
		if (inRect({obj->Pos().x, posY, obj->Size().x - obj->barW, itemMax}, InputSys::mousePos())) {
			items[i]->OnClick();
			return true;
		}
		posY = itemMax;
	}
	return false;
}

bool Scene::CheckTileBoxClick(TileBox* obj) {
	const vector<TileItem>& items = obj->Items();
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
	if (inRect(obj->getRect(), InputSys::mousePos())) {	// needs ui subtraction
		objectHold = obj;
		return true;
	}
	return false;
}

void Scene::OnMouseUp() {
	if (objectHold) {
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
