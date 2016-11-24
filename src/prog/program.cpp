#include "engine/world.h"

// reader events

void Program::Event_Up(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.y, &factor, normalScrollFactor, fastScrollFactor, slowScrollFactor);
		box->ScrollList(-amt * factor);
	}
}

void Program::Event_Down(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.y, &factor, normalScrollFactor, fastScrollFactor, slowScrollFactor);
		box->ScrollList(amt * factor);
	}
}

void Program::Event_Right(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.x, &factor, normalScrollFactor, fastScrollFactor, slowScrollFactor);
		box->ScrollListX(amt * factor);
	}
}

void Program::Event_Left(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.x, &factor, normalScrollFactor, fastScrollFactor, slowScrollFactor);
		box->ScrollListX(-amt * factor);
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
		size_t i = box->VisiblePictures().x;
		i += (i == box->Pictures().size()-1) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_ZoomIn() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) 
		box->MultZoom(zoomFactor);
}

void Program::Event_ZoomOut() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->DivZoom(zoomFactor);
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
		Event_OpenReader((void*)browser->GoNext().c_str());
}

void Program::Event_PrevDir() {
	if (curMenu == EMenu::reader)
		Event_OpenReader((void*)browser->GoPrev().c_str());
}

void Program::Event_CursorUp(float amt) {
	float factor = 1.f;
	amt = ModifySpeed(amt, &factor, normalMDragFactor, fastMDragFactor, slowMDragFactor);
	World::winSys()->MoveMouse(World::inputSys()->mousePos() - vec2i(0, std::ceil(amt * factor)));
}

void Program::Event_CursorDown(float amt) {
	float factor = 1.f;
	amt = ModifySpeed(amt, &factor, normalMDragFactor, fastMDragFactor, slowMDragFactor);
	World::winSys()->MoveMouse(World::inputSys()->mousePos() + vec2i(0, std::ceil(amt * factor)));
}

void Program::Event_CursorRight(float amt) {
	float factor = 1.f;
	amt = ModifySpeed(amt, &factor, normalMDragFactor, fastMDragFactor, slowMDragFactor);
	World::winSys()->MoveMouse(World::inputSys()->mousePos() + vec2i(std::ceil(amt * factor), 0));
}

void Program::Event_CursorLeft(float amt) {
	float factor = 1.f;
	amt = ModifySpeed(amt, &factor, normalMDragFactor, fastMDragFactor, slowMDragFactor);
	World::winSys()->MoveMouse(World::inputSys()->mousePos() - vec2i(std::ceil(amt * factor), 0));
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
	World::PlaySound("click");

	editor->showSongs = !editor->showSongs;
	SwitchScene();
}

void Program::Event_BrowseButtonClick() {
	World::PlaySound("click");

	if (editor->showSongs)
		Event_OpenSongBrowser("");
	else
		SwitchScene(EMenu::bookSearch);
}

void Program::Event_AddButtonClick() {
	World::PlaySound("click");

	if (curMenu == EMenu::playlists)
		World::scene()->SetPopup(new PopupText("New Playlist"));
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->AddSong("");
		else
			editor->AddBook("");

		SwitchScene();
	}
	else if (curMenu == EMenu::songSearch) {
		if (ListItem* item = World::scene()->SelectedButton()) {
			if (browser->CurDir() == string(1, dsep))
				editor->AddSong(dsep+item->label);
			else
				editor->AddSong(browser->CurDir()+item->label);
			browser.reset();

			SwitchScene(EMenu::plistEditor);
		}
	}
}

void Program::Event_DeleteButtonClick() {
	World::PlaySound("click");

	if (curMenu == EMenu::playlists) {
		if (ListItem* item = World::scene()->SelectedButton()) {
			Filer::Remove(World::scene()->Settings().PlaylistPath()+item->label);
			Event_OpenPlaylistList();
		}
	}
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->DelSong();
		else
			editor->DelBook();

		SwitchScene();
	}
}

void Program::Event_EditButtonClick() {
	World::PlaySound("click");

	if (curMenu == EMenu::playlists) {
		if (ListItem* item = World::scene()->SelectedButton())
			Event_OpenPlaylistEditor((void*)item->label.c_str());
	}
	else if (curMenu == EMenu::plistEditor) {
		if (ListItem* item = World::scene()->SelectedButton())
			World::scene()->SetPopup(new PopupText("Rename", item->getData()));
	}
}

void Program::Event_ItemDoubleclicked(ListItem* item) {
	World::PlaySound("click");

	// does the same as Event_EditButtonClicked
	if (curMenu == EMenu::playlists)
		Event_OpenPlaylistEditor((void*)item->label.c_str());
	else if (curMenu == EMenu::plistEditor)
		World::scene()->SetPopup(new PopupText("Rename", item->getData()));
	else if (curMenu == EMenu::songSearch) {
#ifdef _WIN32
		if (browser->CurDir() == "\\")
			Event_OpenSongBrowser(item->label+dsep);
#else
		if (browser->CurDir() == "/" && Filer::FileType(dsep + item->label) == EFileType::dir)
			Event_OpenSongBrowser(dsep+item->label);
#endif
		else if (Filer::FileType(browser->CurDir() + item->label) == EFileType::dir)
			Event_OpenSongBrowser(item->label);
		else {
			if (browser->CurDir() == string(1, dsep))
				editor->AddSong(dsep+item->label);
			else
				editor->AddSong(browser->CurDir() + item->label);
			browser.reset();
			SwitchScene(EMenu::plistEditor);
		}
	}
	else if (curMenu == EMenu::bookSearch) {
		editor->AddBook(item->label);
		SwitchScene(EMenu::plistEditor);
	}
}

void Program::Event_SaveButtonClick() {
	World::PlaySound("click");

	if (editor)
		Filer::SavePlaylist(editor->getPlaylist());
}

void Program::Event_UpButtonClick() {
	World::PlaySound("click");

	if (browser->GoUp())
		SwitchScene();
	else
		Event_Back();
}

void Program::Event_AddBook(void* name) {
	World::PlaySound("click");

	editor->AddBook((const char*)name);
	SwitchScene(EMenu::plistEditor);
}

// MENU EVENTS

void Program::Event_OpenBookList() {
	World::PlaySound("click");
	SwitchScene(EMenu::books);
}

void Program::Event_OpenBrowser(void* path) {
	World::PlaySound("click");
	
	if (browser)
		browser->GoTo(string((const char*)path));
	else
		browser = new Browser((const char*)path);
	
	SwitchScene(EMenu::browser);
}

void Program::Event_OpenReader(void* file) {
	World::PlaySound("open");

	string filename = FindFittingPlaylist((const char*)file);
	if (!filename.empty())
		World::audioSys()->LoadPlaylist(Filer::LoadPlaylist(filename));
	
	SwitchScene(EMenu::reader, file);
}

void Program::Event_OpenPlaylistList() {
	SwitchScene(EMenu::playlists);
	World::PlaySound("click");
}

void Program::Event_OpenPlaylistEditor(void* playlist) {
	World::PlaySound("click");

	editor = new PlaylistEditor((const char*)playlist);
	SwitchScene(EMenu::plistEditor);
}

void Program::Event_OpenSongBrowser(const string& dir) {
	if (browser)
		browser->GoTo(dir); 
	else {
#ifdef _WIN32
		browser = new Browser("\\", getenv("UserProfile"));
#else
		browser = new Browser("/", getenv("HOME"));
#endif
	}
	SwitchScene(EMenu::songSearch);
}

void Program::Event_OpenGeneralSettings() {
	World::PlaySound("click");
	SwitchScene(EMenu::generalSets);
}

void Program::Event_OpenVideoSettings() {
	World::PlaySound("click");
	SwitchScene(EMenu::videoSets);
}

void Program::Event_OpenAudioSettings() {
	World::PlaySound("click");
	SwitchScene(EMenu::audioSets);
}

void Program::Event_OpenControlsSettings() {
	World::PlaySound("click");
	SwitchScene(EMenu::controlsSets);
}

void Program::Event_Ok() {
	if (dynamic_cast<PopupMessage*>(World::scene()->getPopup())) {
		World::PlaySound("click");
		World::scene()->SetPopup(nullptr);
	}
	else
		World::scene()->OnMouseDown(EClick::left, false);	// simulate mouse single left click
}

void Program::Event_Back() {
	World::PlaySound("back");

	if (World::scene()->getPopup())
		World::scene()->SetPopup(nullptr);
	else if (curMenu == EMenu::reader) {
		World::audioSys()->UnloadPlaylist();
		SwitchScene(EMenu::browser);
	}
	else if (curMenu == EMenu::browser) {
		if (browser->GoUp())
			SwitchScene(browser);
		else {
			browser.reset();
			SwitchScene(EMenu::books);
		}
	}
	else if (curMenu == EMenu::plistEditor) {
		editor.reset();
		SwitchScene(EMenu::playlists);
	}
	else if (curMenu == EMenu::songSearch) {
		browser.reset();
		SwitchScene(EMenu::plistEditor);
	}
	else if (curMenu == EMenu::bookSearch)
		SwitchScene(EMenu::plistEditor);
	else if (curMenu >= EMenu::generalSets)
		SwitchScene(EMenu::books);
	else
		World::engine()->Close();
}

// SETTINGS EVENTS

void Program::Event_SwitchLanguage(const string& language) {
	World::PlaySound("click");

	World::library()->LoadLanguage(language);
	SwitchScene();
}

void Program::Event_SetLibraryPath(const string& dir) {
	World::PlaySound("click");
	World::scene()->LibraryPath(dir);
}

void Program::Event_SetPlaylistsPath(const string& dir) {
	World::PlaySound("click");
	World::scene()->PlaylistsPath(dir);
}

void Program::Event_SwitchFullscreen(bool on) {
	World::PlaySound("click");
	World::winSys()->Fullscreen(on);
}

void Program::Event_SetTheme(const string& theme) {
	World::PlaySound("click");
	World::winSys()->Theme(theme);
}

void Program::Event_SetFont(const string& font) {
	World::PlaySound("click");

	World::winSys()->Font(font);
	SwitchScene();
}

void Program::Event_SetRenderer(const string& renderer) {
	World::PlaySound("click");

	World::winSys()->Renderer(renderer);
	SwitchScene();
}

void Program::Event_SetMusicVolume(const string& mvol) {
	World::PlaySound("click");

	World::audioSys()->MusicVolume(stoi(mvol));
	SwitchScene();
}

void Program::Event_SetSoundVolume(const string& svol) {
	World::PlaySound("click");

	World::audioSys()->SoundVolume(stoi(svol));
	SwitchScene();
}

void Program::Event_SetSongDelay(const string& sdelay) {
	World::PlaySound("click");
	World::audioSys()->SongDelay(stof(sdelay));
}

void Program::Event_SetScrollX(const string& scrollx) {
	World::PlaySound("click");
	World::inputSys()->ScrollSpeed(vec2f(stof(scrollx), World::inputSys()->Settings().scrollSpeed.y));
}

void Program::Event_SetScrollY(const string& scrolly) {
	World::PlaySound("click");
	World::inputSys()->ScrollSpeed(vec2f(World::inputSys()->Settings().scrollSpeed.x, stof(scrolly)));
}

void Program::Event_SetDeadzone(const string& deadz) {
	World::PlaySound("click");
	World::inputSys()->Deadzone(stoi(deadz));
}

// OTHER EVENTS

void Program::Event_TextCaptureOk(const string& str) {
	if (curMenu == EMenu::playlists) {
		if (!Filer::Exists(World::scene()->Settings().PlaylistPath() + str)) {
			Filer::SavePlaylist(Playlist(str));
			Event_OpenPlaylistList();
		}
		else {
			World::PlaySound("error");
			World::scene()->SetPopup(new PopupMessage("File already exists."));
		}
	}
	else if (curMenu == EMenu::plistEditor) {
		World::PlaySound("click");
		if (editor->showSongs)
			editor->RenameSong(str);
		else
			editor->RenameBook(str);
		SwitchScene();
	}
}

void Program::Event_SelectionSet(void* box) {
	if (editor)
		editor->selected = static_cast<ScrollArea*>(box)->SelectedItem();
}

void Program::Event_ScreenMode() {
	World::winSys()->Fullscreen(!World::winSys()->Settings().fullscreen);
}

void Program::FileDropEvent(char* file) {
	if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs) {
			editor->AddSong(file);
			SwitchScene();
		}
		else if (Filer::FileType(file) == EFileType::dir) {
			string libDir = World::scene()->Settings().LibraryPath();
			if (parentPath(file) == libDir) {
				editor->AddBook(filename(file));
				SwitchScene();
			}
			else if (file == libDir) {
				string path = appendDsep(file);
				for (string& it : Filer::ListDir(path, FILTER_DIR))
					editor->AddBook(filename(path+it));
				SwitchScene();
			}
		}
	}
}

void Program::SwitchScene(EMenu newMenu, void* dat) {
	curMenu = newMenu;
	SwitchScene(dat);
}

void Program::SwitchScene(void* dat) const {
	// clean up library
	World::library()->Fonts()->Clear();
	World::library()->ClearPics();

	// prepare necessary variables
	size_t focObject = 0;
	vector<Object*> objects;

	// little conveniences
	vec2i res = World::winSys()->Resolution();
	vec2i sizT(140, 40);

	switch (curMenu) {
	case EMenu::books: {
		// top buttons
		objects = {
			new ButtonText(Object(vec2i(0,         0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenPlaylistList, World::library()->getLine("playlists")),
			new ButtonText(Object(vec2i(res.x/3,   0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, World::library()->getLine("settings")),
			new ButtonText(Object(vec2i(res.x/3*2, 0), -1, vec2i(res.x/3.f,  50), FIX_Y | FIX_H), &Program::Event_Back, World::library()->getLine("exit"))
		};
		
		// book list
		string libPath = World::scene()->Settings().LibraryPath();
		vector<ListItem*> tiles;
		for (string& it : Filer::ListDir(libPath, FILTER_DIR))
			tiles.push_back(new ItemButton(it, libPath+it, &Program::Event_OpenBrowser));
		objects.push_back(new TileBox(Object(vec2i(0, 60), -1, vec2i(res.x, res.y-60), FIX_ANC | FIX_END), tiles, vec2i(400, 30)));

		focObject = objects.size()-1;
		break; }
	case EMenu::browser: {
		// back button
		objects = { new ButtonText(Object(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back")) };

		// list
		vector<ListItem*> items;
		for (string& it : browser->ListDirs())
			items.push_back(new ItemButton(it, "", &Program::Event_OpenBrowser));
		for (string& it : browser->ListFiles())
			items.push_back(new ItemButton(it, browser->CurDir()+it, &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::reader: {
		// reader box
		string file = (const char*)dat;
		if (Filer::FileType(file) == EFileType::dir) {
			World::library()->LoadPics(Filer::GetPics(file));
			file.clear();
		}
		else 
			World::library()->LoadPics(Filer::GetPics(parentPath(file)));
		
		objects = { new ReaderBox(Object(0, 0, World::winSys()->Resolution(), FIX_ANC | FIX_END, EColor::background), World::library()->Pictures(), file) };
		focObject = objects.size()-1;
		break; }
	case EMenu::playlists: {
		// top buttons
		sizT = vec2i(80, 30);
		objects = {
			new ButtonText(Object(vec2i(0,         0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenBookList, World::library()->getLine("library")),
			new ButtonText(Object(vec2i(res.x/3,   0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, World::library()->getLine("settings")),
			new ButtonText(Object(vec2i(res.x/3*2, 0), -1, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, World::library()->getLine("exit")),

			new ButtonText(Object(vec2i(0,   60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_AddButtonClick, World::library()->getLine("new")),
			new ButtonText(Object(vec2i(90,  60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_EditButtonClick, World::library()->getLine("edit")),
			new ButtonText(Object(vec2i(180, 60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_DeleteButtonClick, World::library()->getLine("del"))
		};

		// playlist list
		TileBox* box = new TileBox(Object(vec2i(0, 100), -1, vec2i(res.x, res.y-100), FIX_ANC | FIX_END), {}, vec2i(400, 30));
		vector<ListItem*> tiles;
		for (string& it : Filer::ListDir(World::scene()->Settings().PlaylistPath(), FILTER_FILE))
			tiles.push_back(new ListItem(it, box));
		box->Items(tiles);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::plistEditor: {
		// option buttons
		sizT.x = 120;
		objects = {
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_BrowseButtonClick, World::library()->getLine("browse")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_AddButtonClick, World::library()->getLine("add")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_EditButtonClick, World::library()->getLine("edit")),
			new ButtonText(Object(vec2i(0, 200), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_DeleteButtonClick, World::library()->getLine("del")),
			new ButtonText(Object(vec2i(0, 250), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_SaveButtonClick, World::library()->getLine("save")),
			new ButtonText(Object(vec2i(0, 300), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("close"))
		};

		// playlist list
		ListBox* box = new ListBox(Object(vec2i(sizT.x+10, 0), -1, vec2i(res.x-110, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items;
		if (editor->showSongs) {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_SwitchButtonClick, World::library()->getLine("books")));
			for (string& it : editor->getPlaylist().songs)
				items.push_back(new ItemButton(filename(it), it, &Program::Event_SelectionSet, box));
		}
		else {
			objects.push_back(new ButtonText(Object(vec2i(0, 0), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_SwitchButtonClick, World::library()->getLine("songs")));
			for (string& it : editor->getPlaylist().books)
				items.push_back(new ItemButton(it, "", &Program::Event_SelectionSet, box));
		}
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::songSearch: {
		// buttons
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_AddButtonClick, World::library()->getLine("add")),
			new ButtonText(Object(vec2i(0, 50),  -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_UpButtonClick, World::library()->getLine("up")),
			new ButtonText(Object(vec2i(0, 100), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		// list
		ListBox* box = new ListBox(Object(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items;
		for (string& it : browser->ListDirs())
			items.push_back(new ListItem(it, box));
		for (string& it : browser->ListFiles())
			items.push_back(new ListItem(it, box));
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::bookSearch: {
		// back button
		objects = {	new ButtonText(Object(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))	};

		// list
		vector<ListItem*> items;
		for (string& it : Filer::ListDir(World::scene()->Settings().DirLib(), FILTER_DIR))
			items.push_back(new ItemButton(it, "", &Program::Event_AddBook));
		objects.push_back(new ListBox(Object(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
		focObject = objects.size()-1;
		break; }
	case EMenu::generalSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenVideoSettings, World::library()->getLine("video")),
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenAudioSettings, World::library()->getLine("audio")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenControlsSettings, World::library()->getLine("controls")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		ListBox* box = new ListBox(Object(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new Switchbox(box, World::library()->getLine("language"), Filer::GetAvailibleLanguages(), World::scene()->Settings().Lang(), &Program::Event_SwitchLanguage),
			new LineEdit(box, World::library()->getLine("library"), World::scene()->Settings().DirLib(), ETextType::text, &Program::Event_SetLibraryPath),
			new LineEdit(box, World::library()->getLine("playlists"), World::scene()->Settings().DirPlist(), ETextType::text, &Program::Event_SetPlaylistsPath)
		};
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::videoSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenGeneralSettings, World::library()->getLine("general")),
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenAudioSettings, World::library()->getLine("audio")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenControlsSettings, World::library()->getLine("controls")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		ListBox* box = new ListBox(Object(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new Checkbox(box, World::library()->getLine("fullscreen"), World::winSys()->Settings().fullscreen, &Program::Event_SwitchFullscreen),
			new Switchbox(box, World::library()->getLine("theme"), Filer::GetAvailibleThemes(), World::winSys()->Settings().theme, &Program::Event_SetTheme),
			new LineEdit(box, World::library()->getLine("font"), World::winSys()->Settings().Font(), ETextType::text, &Program::Event_SetFont),
			new Switchbox(box, World::library()->getLine("renderer"), getAvailibleRenderers(true), World::winSys()->Settings().renderer, &Program::Event_SetRenderer),
		};
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::audioSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenGeneralSettings, World::library()->getLine("general")),
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenVideoSettings, World::library()->getLine("video")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenControlsSettings, World::library()->getLine("controls")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		ListBox* box = new ListBox(Object(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new LineEdit(box, World::library()->getLine("music vol"), to_string(World::audioSys()->Settings().musicVolume), ETextType::integer, &Program::Event_SetMusicVolume),
			new LineEdit(box, World::library()->getLine("sound vol"), to_string(World::audioSys()->Settings().soundVolume), ETextType::integer, &Program::Event_SetSoundVolume),
			new LineEdit(box, World::library()->getLine("song delay"), to_string(World::audioSys()->Settings().songDelay), ETextType::floating, &Program::Event_SetSongDelay)
		};
		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		break; }
	case EMenu::controlsSets: {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenGeneralSettings, World::library()->getLine("general")),
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenVideoSettings, World::library()->getLine("video")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenAudioSettings, World::library()->getLine("audio")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		ListBox* box = new ListBox(Object(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new LineEdit(box, World::library()->getLine("scroll speed")+" X", to_string(World::inputSys()->Settings().scrollSpeed.x), ETextType::floating, &Program::Event_SetScrollX),
			new LineEdit(box, World::library()->getLine("scroll speed")+" Y", to_string(World::inputSys()->Settings().scrollSpeed.y), ETextType::floating, &Program::Event_SetScrollY),
			new LineEdit(box, World::library()->getLine("deadzone"), to_string(World::inputSys()->Settings().deadzone), ETextType::integer, &Program::Event_SetDeadzone)
		};
		for (const pair<string, Shortcut*>& it : World::inputSys()->Settings().shortcuts)
			items.push_back(new KeyGetter(box, it.first, World::inputSys()->GetShortcut(it.first)));

		box->Items(items);
		objects.push_back(box);
		focObject = objects.size()-1;
		}
	}
	World::scene()->SwitchMenu(objects, focObject);
}

float Program::ModifySpeed(float value, float* axisFactor, float normalFactor, float fastFactor, float slowFactor) {
	if (World::inputSys()->isPressed(SHORTCUT_FAST, axisFactor))
		value *= fastFactor;
	else if (World::inputSys()->isPressed(SHORTCUT_SLOW, axisFactor))
		value *= slowFactor;
	else
		value *= normalFactor;
	return value * World::engine()->deltaSeconds();
}

string Program::FindFittingPlaylist(const string& picPath) {
	// get book name
	string name;
	size_t start = World::scene()->Settings().LibraryPath().length();
	for (size_t i=start; i!=picPath.length(); i++)
		if (picPath[i] == dsep) {
			name = picPath.substr(start, i-start);
			break;
		}
	if (name.empty())
		return "";

	// check playlist files for matching book name
	for (string& file : Filer::ListDir(World::scene()->Settings().PlaylistPath(), FILTER_FILE)) {
		Playlist playlist = Filer::LoadPlaylist(file);
		for (string& book : playlist.books)
			if (book == name)
				return playlist.name;
	}
	return "";
}
