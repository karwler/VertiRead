#include "engine/world.h"

// reader events

void Program::Event_Up() {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollList(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Down() {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollList(spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Left() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollListX(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Right() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollListX(spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_PageUp() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		int i = box->VisiblePictures().x;
		i -= (i == 0) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_PageDown() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		int i = box->VisiblePictures().x;
		i += (i == box->Pictures().size()-1) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_ZoomIn() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->AddZoom(0.2f);
}

void Program::Event_ZoomOut() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->AddZoom(-0.2f);
}

void Program::Event_ZoomReset() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->Zoom(1.f);
}

void Program::Event_CenterView() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->DragListX(0);
}

void Program::Event_NextDir() {
	if (curMenu == EMenu::reader)
		Event_OpenReader((void*)browser->GoNext().string().c_str());
}

void Program::Event_PrevDir() {
	if (curMenu == EMenu::reader)
		Event_OpenReader((void*)browser->GoPrev().string().c_str());
}

// PLAYER EVENTS

void Program::Event_PlayPause() {
	World::audioSys()->PlayPauseMusic();
}

void Program::Event_NextSong() {
	World::audioSys()->SwitchSong(1);
}

void Program::Event_PrevSong() {
	World::audioSys()->SwitchSong(-1);
}

void Program::Event_VolumeUp() {
	World::audioSys()->MusicVolume(World::audioSys()->Settings().musicVolume + 8);
}

void Program::Event_VolumeDown() {
	World::audioSys()->MusicVolume(World::audioSys()->Settings().musicVolume - 8);
}

void Program::Event_Mute() {
	World::audioSys()->MusicVolume(0);
}

// PLAYLIST EDITOR EVENTS

void Program::Event_SwitchButtonClick() {
	editor->showSongs = !editor->showSongs;

	SwitchScene(editor);
}

void Program::Event_AddButtonClick() {
	if (curMenu == EMenu::playlists)
		World::scene()->SetPopup(new PopupText("New Playlist"));
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->AddSong("");
		else
			editor->AddBook("");

		SwitchScene(editor);
	}
}

void Program::Event_DeleteButtonClick() {
	if (curMenu == EMenu::playlists) {
		if (ListItem* item = World::scene()->SelectedButton()) {
			fs::remove(World::scene()->Settings().playlistParh() + item->label);
			Event_OpenPlaylistList();
		}
	}
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->DelSong();
		else
			editor->DelBook();

		SwitchScene(editor);
	}
}

void Program::Event_EditButtonClick() {
	if (curMenu == EMenu::playlists) {
		if (ListItem* item = World::scene()->SelectedButton())
			Event_OpenPlaylistEditor((void*)item->label.c_str());
	}
	else if (curMenu == EMenu::plistEditor) {
		if (ListItem* item = World::scene()->SelectedButton())
			World::scene()->SetPopup(new PopupText("Rename", item->label));
	}
}

void Program::Event_ItemDoubleclicked(ListItem* item) {
	// does the same as Event_EditButtonClicked
	if (curMenu == EMenu::playlists)
		Event_OpenPlaylistEditor((void*)item->label.c_str());
	else if (curMenu == EMenu::plistEditor)
		World::scene()->SetPopup(new PopupText("Rename", item->label));
}

void Program::Event_SaveButtonClick() {
	if (editor)
		Filer::SavePlaylist(editor->getPlaylist());
}

// MENU EVENTS

void Program::Event_OpenBookList() {
	SwitchScene(EMenu::books);
}

void Program::Event_OpenBrowser(void* path) {
	if (browser)
		browser->GoTo(cstr(path));
	else
		browser = new Browser(cstr(path));

	SwitchScene(EMenu::browser, browser);
}

void Program::Event_OpenReader(void* file) {
	string filename = FindFittingPlaylist(cstr(file));
	if (!filename.empty())
		World::audioSys()->LoadPlaylist(Filer::LoadPlaylist(filename));

	SwitchScene(EMenu::reader, file);
}

void Program::Event_OpenPlaylistList() {
	SwitchScene(EMenu::playlists);
}

void Program::Event_OpenPlaylistEditor(void* playlist) {
	editor = new PlaylistEditor(cstr(playlist));

	SwitchScene(EMenu::plistEditor, editor);
}

void Program::Event_OpenGeneralSettings() {
	SwitchScene(EMenu::generalSets);
}

void Program::Event_OpenVideoSettings() {
	SwitchScene(EMenu::videoSets);
}

void Program::Event_OpenAudioSettings() {
	SwitchScene(EMenu::audioSets);
}

void Program::Event_OpenControlsSettings() {
	SwitchScene(EMenu::controlsSets);
}

void Program::Event_Back() {
	if (World::scene()->getPopup())
		World::scene()->SetPopup(nullptr);
	else if (World::inputSys()->CapturedObject())
		World::inputSys()->SetCapture(nullptr);
	else if (curMenu == EMenu::reader) {
		World::audioSys()->UnloadPlaylist();
		SwitchScene(EMenu::browser, browser);
	}
	else if (curMenu == EMenu::browser) {
		if (browser->GoUp())
			SwitchScene(browser);
		else {
			browser.reset();
			SwitchScene(EMenu::books);
		}
	}
	else if (curMenu >= EMenu::generalSets)
		SwitchScene(EMenu::books);
	else if (curMenu == EMenu::plistEditor) {
		editor.reset();
		SwitchScene(EMenu::playlists);
	}
	else
		World::engine->Close();
}

void Program::Event_Ok() {
	if (dynamic_cast<PopupMessage*>(World::scene()->getPopup()))
		World::scene()->SetPopup(nullptr);
	else if (curMenu == EMenu::playlists || curMenu == EMenu::plistEditor)
		Event_EditButtonClick();
}

// OTHER EVENTS

void Program::Event_TextCaptureOk(TextEdit* box) {
	if (curMenu == EMenu::playlists) {
		if (!fs::exists(World::scene()->Settings().playlistParh() + box->getText())) {
			Filer::SavePlaylist(Playlist(box->getText()));
			Event_OpenPlaylistList();
		}
		else
			World::scene()->SetPopup(new PopupMessage("File already exists."));
	}
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->RenameSong(box->getText());
		else
			editor->RenameBook(box->getText());
		SwitchScene(editor);
	}
	World::inputSys()->SetCapture(nullptr);
}

void Program::Event_KeyCaptureOk(SDL_Scancode key) {
	World::inputSys()->SetCapture(nullptr);
}

void Program::Event_SelectionSet(void* box) {
	if (editor)
		editor->selected = static_cast<ScrollArea*>(box)->SelectedItem();
}

void Program::Event_ScreenMode() {
	World::winSys()->Fullscreen(!World::winSys()->Settings().fullscreen);
}

EMenu Program::CurrentMenu() const {
	return curMenu;
}

void Program::SwitchScene(EMenu newMenu, void* dat) {
	curMenu = newMenu;
	SwitchScene(dat);
}

void Program::SwitchScene(void* dat) const {
	uint focObject = 0;
	vector<Object*> objects;

	// little conveniences
	vec2i res = World::winSys()->Resolution();
	vec2i posT(-1);
	vec2i sizT(140, 40);

	switch (curMenu) {
	case EMenu::books: {
		// top buttons
		objects = {
			new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenPlaylistList,"Playlists"),
			new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, "Settings"),
			new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, "Exit")
		};
		
		// book list
		vector<ListItem*> tiles;
		vector<fs::path> names = Filer::ListDir(World::scene()->Settings().libraryParh(), FILTER_DIR);
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
		World::library()->LoadPics(Filer::GetPicsFromDir(file.parent_path()));
		objects = { new ReaderBox(Object(0, 0, World::winSys()->Resolution(), FIX_POS | FIX_END, EColor::background), World::library()->Pictures(), file.string()) };
		focObject = objects.size()-1;
		break; }
	case EMenu::playlists: {
		// top buttons
		sizT = vec2i(80, 30);
		objects = {
			new ButtonText(Object(vec2i(0,         0), posT, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenBookList, "Library"),
			new ButtonText(Object(vec2i(res.x/3,   0), posT, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, "Settings"),
			new ButtonText(Object(vec2i(res.x/3*2, 0), posT, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, "Exit"),

			new ButtonText(Object(vec2i(0,   60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_AddButtonClick, "New"),
			new ButtonText(Object(vec2i(90,  60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_EditButtonClick, "Edit"),
			new ButtonText(Object(vec2i(180, 60), posT, sizT, FIX_POS | FIX_SIZ), &Program::Event_DeleteButtonClick, "Del")
		};

		// playlist list
		TileBox* box = new TileBox(Object(vec2i(0, 100), posT, vec2i(res.x, res.y-100), FIX_POS | FIX_END), {}, vec2i(400, 30));
		vector<ListItem*> tiles;
		vector<fs::path> names = Filer::ListDir(World::scene()->Settings().playlistParh(), FILTER_FILE);
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

		vec2i ancT(160, 0);
		sizT = vec2i(res.x-160, 30);
		EFix fixT = FIX_POS | FIX_EX | FIX_H;
		vector<Object*> items = {
			new LineEdit(Object(ancT, posT, sizT, fixT), "Library   ", World::scene()->Settings().dirLib),
			new LineEdit(Object(ancT, posT, sizT, fixT), "Playlists ", World::scene()->Settings().dirPlist)
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
	World::scene()->SwitchMenu(objects, focObject);
}

string Program::FindFittingPlaylist(const string& picPath) {
	// get book name
	string name;
	uint start = World::scene()->Settings().libraryParh().length();
	for (uint i=start; i!=picPath.length(); i++)
		if (picPath[i] == dsep[0]) {
			name = picPath.substr(start, i-start);
			break;
		}
	if (name.empty())
		return "";

	// check playlist files for matching book name
	vector<fs::path> files = Filer::ListDir(World::scene()->Settings().playlistParh(), FILTER_FILE);
	for (const fs::path& file : files) {
		Playlist playlist = Filer::LoadPlaylist(file.filename().string());
		for (const string& book : playlist.books)
			if (book == name)
				return file.filename().string();
	}
	return "";
}
