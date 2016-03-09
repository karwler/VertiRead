#include "scene.h"
#include "world.h"

Scene::Scene() :
	program(new Program),
	library(new Library),
	sliderHold(nullptr)
{}

Scene::~Scene() {
	Clear(objects);
}

void Scene::SwitchMenu(EMenu newMenu, void* dat) {
	Clear(objects);
	focObject = 0;
	sliderHold = nullptr;
	vec2i res = World::winSys()->Resolution();
	switch (newMenu) {
	case EMenu::books: {
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(res.x / 3 - 10, 50)), &Program::Event_OpenPlaylistList, "Playlists"));
		objects.push_back(new ButtonText(Object(vec2i(res.x / 3, 0), vec2i(res.x / 3 - 10, 50)), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x / 3 * 2, 0), vec2i(res.x / 3, 50)), &Program::Event_Back, "Exit"));
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirLib(), FILTER_DIR);
		for (fs::path& it : names)
			tiles.push_back(TileItem(it.filename().string(), it.string(), &Program::Event_OpenBrowser));
		objects.push_back(new TileBox(Object(vec2i(0, 60), vec2i(res.x, res.y-60)), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::browser: {
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(110, 50)), &Program::Event_Back, "Back"));
		vector<ListItem*> items;
		Browser* browser = (Browser*)dat;
		for (fs::path& it : browser->ListDirs())
			items.push_back(new BrowserButton(30, it.filename().string(), "", &Program::Event_OpenBrowser));
		for (fs::path& it : browser->ListFiles())
			items.push_back(new BrowserButton(30, it.filename().string(), "", &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(120, 0), vec2i(res.x-120, res.y), EColor::background), items));
		focObject = objects.size() - 1;
		break; }
	case EMenu::reader:
		break;
	case EMenu::playlists: {
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(res.x / 3 - 10, 50)), &Program::Event_OpenBookList, "Library"));
		objects.push_back(new ButtonText(Object(vec2i(res.x / 3, 0), vec2i(res.x / 3 - 10, 50)), &Program::Event_OpenGeneralSettings, "Settings"));
		objects.push_back(new ButtonText(Object(vec2i(res.x / 3 * 2, 0), vec2i(res.x / 3, 50)), &Program::Event_Back, "Exit"));
		vector<TileItem> tiles;
		vector<fs::path> names = Filer::ListDir(Filer::dirPlist(), FILTER_FILE);
		for (fs::path& it : names)
			tiles.push_back(TileItem(removeExtension(it.filename()).string(), it.string(), &Program::Event_OpenPlaylistEditor));
		objects.push_back(new TileBox(Object(vec2i(0, 60), vec2i(res.x, res.y - 60)), tiles, vec2i(400, 30)));
		focObject = objects.size() - 1;
		break; }
	case EMenu::plistEditor:
		break;
	case EMenu::generalSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(200, 50)), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 60), vec2i(200, 50)), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 120), vec2i(200, 50)), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 180), vec2i(200, 50)), &Program::Event_Back, "Back"));
		break;
	case EMenu::videoSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(200, 50)), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 60), vec2i(200, 50)), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 120), vec2i(200, 50)), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 180), vec2i(200, 50)), &Program::Event_Back, "Back"));
		break;
	case EMenu::audioSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(200, 50)), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 60), vec2i(200, 50)), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 120), vec2i(200, 50)), &Program::Event_OpenControlsSettings, "Controls"));
		objects.push_back(new ButtonText(Object(vec2i(0, 180), vec2i(200, 50)), &Program::Event_Back, "Back"));
		break;
	case EMenu::controlsSets:
		objects.push_back(new ButtonText(Object(vec2i(0, 0), vec2i(200, 50)), &Program::Event_OpenGeneralSettings, "General"));
		objects.push_back(new ButtonText(Object(vec2i(0, 60), vec2i(200, 50)), &Program::Event_OpenVideoSettings, "Video"));
		objects.push_back(new ButtonText(Object(vec2i(0, 120), vec2i(200, 50)), &Program::Event_OpenAudioSettings, "Audio"));
		objects.push_back(new ButtonText(Object(vec2i(0, 180), vec2i(200, 50)), &Program::Event_Back, "Back"));
	}
	World::winSys()->DrawScene();
}

void Scene::OnMouseDown() {
	for (Object* obj : objects) {
		if (dynamic_cast<Button*>(obj)) {
			if (CheckButtonClick(static_cast<Button*>(obj)))
				break;
		}
		else if (dynamic_cast<ScrollArea*>(obj)) {
			if (CheckSliderClick(static_cast<ScrollArea*>(obj)))
				break;
			else if (dynamic_cast<ListBox*>(obj)) {
				if (CheckListBoxClick(static_cast<ListBox*>(obj)))
					break;
			}
			else if (dynamic_cast<TileBox*>(obj)) {
				if (CheckTileBoxClick(static_cast<TileBox*>(obj)))
					break;
			}
		}
	}
}

bool Scene::CheckButtonClick(Button* obj) {
	if (inRect({ obj->pos.x, obj->pos.y, obj->Size().x, obj->Size().y }, InputSys::mousePos())) {
		obj->OnClick();
		return true;
	}
	return false;
}

bool Scene::CheckSliderClick(ScrollArea* obj) {
	vec2i mPos = InputSys::mousePos();
	if (inRect(obj->Bar(), mPos)) {
		sliderHold = obj;
		if (mPos.y < obj->SliderY() || mPos.y > obj->SliderY() + obj->SliderH())
			obj->DragSlider(mPos.y - obj->SliderH() / 2);
		obj->diffSliderMouseY = mPos.y - obj->SliderY();
		return true;
	}
	return false;
}

bool Scene::CheckListBoxClick(ListBox* obj) {
	vector<ListItem*> items = obj->Items();
	int posY = obj->pos.y + obj->listY();
	for (uint i = 0; i != items.size(); i++) {
		int itemMax = posY + items[i]->height;
		if (inRect({ obj->pos.x, posY, obj->Size().x - obj->barW, itemMax }, InputSys::mousePos())) {
			items[i]->OnClick();
			return true;
		}
		posY = itemMax;
	}
	return false;
}

bool Scene::CheckTileBoxClick(TileBox* obj) {
	vector<TileItem>& items = obj->Items();
	for (uint i = 0; i != items.size(); i++) {
		int row = i / obj->TilesPerRow();
		int posX = (i - row * obj->TilesPerRow()) * (obj->TileSize().x + obj->Spacing()) + obj->pos.x;
		int posY = row * (obj->TileSize().y + obj->Spacing()) + obj->pos.y;

		if (inRect({ posX, posY, obj->TileSize().x, obj->TileSize().y }, InputSys::mousePos())) {
			items[i].OnClick();
			return true;
		}
	}
	return false;
}

void Scene::OnMouseUp() {
	if (sliderHold) {
		sliderHold->diffSliderMouseY = 0;
		sliderHold = nullptr;
	}
}

void Scene::OnMouseDrag() {
	if (sliderHold)
		sliderHold->DragSlider(InputSys::mousePos().y);
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
