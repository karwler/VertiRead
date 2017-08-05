#include "engine/world.h"

// reader events

void Program::Event_Up(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.y, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->ScrollList(-amt * factor);
	}
}

void Program::Event_Down(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.y, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->ScrollList(amt * factor);
	}
}

void Program::Event_Right(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.x, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->ScrollListX(amt * factor);
	}
}

void Program::Event_Left(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		float factor = 1.f;
		amt = ModifySpeed(amt * box->Zoom() * World::inputSys()->Settings().scrollSpeed.x, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->ScrollListX(-amt * factor);
	}
}

void Program::Event_PageUp() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		size_t i = box->VisibleItems().x;
		if (i != 0)
			i--;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_PageDown() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) {
		size_t i = box->VisibleItems().x;
		if (i != box->Pictures().size()-1)
			i++;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_ZoomIn() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject())) 
		box->MultZoom(Default::zoomFactor);
}

void Program::Event_ZoomOut() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		box->DivZoom(Default::zoomFactor);
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

// PLAYER EVENTS

void Program::Event_PlayPause() {
	World::audioSys()->PlayPauseMusic();
}

void Program::Event_NextSong() {
	World::audioSys()->NextSong();
	World::winSys()->SetRedrawNeeded();
}

void Program::Event_PrevSong() {
	World::audioSys()->PrevSong();
	World::winSys()->SetRedrawNeeded();
}

void Program::Event_VolumeUp() {
	World::audioSys()->MusicVolume(World::audioSys()->Settings().musicVolume + 8);
}

void Program::Event_VolumeDown() {
	World::audioSys()->MusicVolume(World::audioSys()->Settings().musicVolume - 8);
}

void Program::Event_Mute() {
	World::audioSys()->SongMuteSwitch();
}

// PLAYLIST EDITOR EVENTS

void Program::Event_AddPlaylistButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	pair<Popup*, LineEditor*> pt = CreatePopupText("New Playlist", "", &Program::Event_NewPlaylistOk, &Program::Event_NewPlaylistOk);
	World::scene()->SetPopup(pt.first, pt.second);
}

void Program::Event_DeletePlaylistButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (ListItem* item = World::scene()->SelectedButton()) {
		Filer::Remove(World::scene()->Settings().PlaylistPath() + item->label + ".ini");
		Event_OpenPlaylistList();
	}
}

void Program::Event_EditPlaylistButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (ListItem* item = World::scene()->SelectedButton())
		Event_OpenPlaylistEditor((void*)item->label.c_str());
}

void Program::Event_NewPlaylistOk() {
	for (const Object* it : World::scene()->getPopup()->objects)
		if (const LineEditor* obj = dynamic_cast<const LineEditor*>(it)) {
			Event_NewPlaylistOk(obj->Editor().Text());
			break;
		}
}

void Program::Event_NewPlaylistOk(const string& str) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (Filer::Exists(World::scene()->Settings().PlaylistPath() + str)) {
		World::audioSys()->PlaySound(Default::cueNameError);
		World::scene()->SetPopup(CreatePopupMessage("File already exists."));
	} else {
		Filer::SavePlaylist(Playlist(str));
		Event_OpenPlaylistList();
	}
}

void Program::Event_SwitchButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	editor->showSongs = !editor->showSongs;
	SwitchScene();
}

void Program::Event_BrowseButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (editor->showSongs)
		Event_OpenSongBrowser("");
	else
		SwitchScene(EMenu::bookSearch);
}

void Program::Event_AddSongBookButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (editor->showSongs)
		editor->AddSong("");
	else
		editor->AddBook("");
	SwitchScene();
}

void Program::Event_AddSongFileDirButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (ListItem* item = World::scene()->SelectedButton()) {
		if (browser->CurDir() == string(1, dsep))
			editor->AddSong(dsep+item->label);
		else
			editor->AddSong(browser->CurDir()+item->label);
		browser.clear();

		SwitchScene(EMenu::plistEditor);
	}
}

void Program::Event_DeleteSongBookButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (editor->showSongs)
		editor->DelSong();
	else
		editor->DelBook();
	SwitchScene();
}

void Program::Event_EditSongBookButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (ListItem* item = World::scene()->SelectedButton()) {
		pair<Popup*, LineEditor*> pt = CreatePopupText("Rename " + string(editor->showSongs ? "song" : "book"), item->getData(), &Program::Event_SongBookRenameOk, &Program::Event_SongBookRenameOk);
		World::scene()->SetPopup(pt.first, pt.second);
	}
}

void Program::Event_SongBookRenameOk() {
	for (const Object* it : World::scene()->getPopup()->objects)
		if (const LineEditor* obj = dynamic_cast<const LineEditor*>(it)) {
			Event_SongBookRenameOk(obj->Editor().Text());
			break;
		}
}

void Program::Event_SongBookRenameOk(const string& str) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (editor->showSongs)
		editor->RenameSong(str);
	else
		editor->RenameBook(str);
	SwitchScene();
}

void Program::Event_ItemDoubleclicked(ListItem* item) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	// does the same as Event_EditButtonClicked
	if (curMenu == EMenu::playlists)
		Event_OpenPlaylistEditor((void*)item->label.c_str());
	else if (curMenu == EMenu::plistEditor) {
		pair<Popup*, LineEditor*> pt = CreatePopupText("Rename " + string(editor->showSongs ? "song" : "book"), item->getData(), &Program::Event_SongBookRenameOk, &Program::Event_SongBookRenameOk);
		World::scene()->SetPopup(pt.first, pt.second);
	} else if (curMenu == EMenu::songSearch) {
#ifdef _WIN32
		if (browser->CurDir() == "\\")
			Event_OpenSongBrowser(item->label+dsep);
#else
		if (browser->CurDir() == "/" && Filer::FileType(dsep + item->label) == EFileType::dir)
			Event_OpenSongBrowser(item->label);
#endif
		else if (Filer::FileType(browser->CurDir() + item->label) == EFileType::dir)
			Event_OpenSongBrowser(item->label);
		else {
			if (browser->CurDir() == string(1, dsep))
				editor->AddSong(dsep+item->label);
			else
				editor->AddSong(browser->CurDir() + item->label);
			browser.clear();
			SwitchScene(EMenu::plistEditor);
		}
	} else if (curMenu == EMenu::bookSearch) {
		editor->AddBook(item->label);
		SwitchScene(EMenu::plistEditor);
	}
}

void Program::Event_SaveButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (editor)
		Filer::SavePlaylist(editor->getPlaylist());
}

void Program::Event_UpButtonClick() {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (browser->GoUp())
		SwitchScene();
	else
		Event_Back();
}

void Program::Event_AddBook(void* name) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	editor->AddBook(static_cast<char*>(name));
	SwitchScene(EMenu::plistEditor);
}

// MENU EVENTS

void Program::Event_OpenBookList() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::books);
}

void Program::Event_OpenBrowser(void* path) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	if (browser)
		browser->GoTo(string(static_cast<char*>(path)));
	else
		browser = new Browser(static_cast<char*>(path));
	SwitchScene(EMenu::browser);
}

void Program::Event_OpenReader(void* file) {
	World::audioSys()->PlaySound(Default::cueNameOpen);

	Playlist plist = FindFittingPlaylist(static_cast<char*>(file));
	if (!plist.name.empty() && World::audioSys())
		World::audioSys()->LoadPlaylist(plist);
	SwitchScene(EMenu::reader, file);
}

void Program::Event_OpenPlaylistList() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::playlists);
}

void Program::Event_OpenPlaylistEditor(void* playlist) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	editor = new PlaylistEditor(static_cast<char*>(playlist));
	SwitchScene(EMenu::plistEditor);
}

void Program::Event_OpenSongBrowser(const string& dir) {
	if (browser)
		browser->GoTo(dir); 
	else {
#ifdef _WIN32
		browser = new Browser("\\", std::getenv("UserProfile"));
#else
		browser = new Browser("/", std::getenv("HOME"));
#endif
	}
	SwitchScene(EMenu::songSearch);
}

void Program::Event_OpenGeneralSettings() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::generalSets);
}

void Program::Event_OpenVideoSettings() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::videoSets);
}

void Program::Event_OpenAudioSettings() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::audioSets);
}

void Program::Event_OpenControlsSettings() {
	World::audioSys()->PlaySound(Default::cueNameClick);
	SwitchScene(EMenu::controlsSets);
}

void Program::Event_Ok() {
	// idk. someday there might be something here
}

void Program::Event_Back() {
	World::audioSys()->PlaySound(Default::cueNameBack);

	if (World::scene()->getPopup())
		World::scene()->SetPopup(nullptr);
	else if (curMenu == EMenu::reader) {
		World::audioSys()->UnloadPlaylist();
		SwitchScene(EMenu::browser);
	} else if (curMenu == EMenu::browser) {
		if (browser->GoUp())
			SwitchScene(browser);
		else {
			browser.clear();
			SwitchScene(EMenu::books);
		}
	} else if (curMenu == EMenu::plistEditor) {
		editor.clear();
		SwitchScene(EMenu::playlists);
	} else if (curMenu == EMenu::songSearch) {
		browser.clear();
		SwitchScene(EMenu::plistEditor);
	} else if (curMenu == EMenu::bookSearch)
		SwitchScene(EMenu::plistEditor);
	else if (curMenu >= EMenu::generalSets)
		SwitchScene(EMenu::books);
	else
		World::engine()->Close();
}

// SETTINGS EVENTS

void Program::Event_SwitchLanguage(const string& language) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	
	World::library()->LoadLanguage(language);
	SwitchScene();
}

void Program::Event_SetLibraryPath(const string& dir) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::scene()->LibraryPath(dir);
}

void Program::Event_SetPlaylistsPath(const string& dir) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::scene()->PlaylistsPath(dir);
}

void Program::Event_SwitchFullscreen(bool on) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::winSys()->Fullscreen(on);
}

void Program::Event_SetTheme(const string& theme) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::winSys()->Theme(theme);
}

void Program::Event_SetFont(const string& font) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	World::winSys()->Font(font);
	SwitchScene();
}

void Program::Event_SetRenderer(const string& renderer) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	World::winSys()->Renderer(renderer);
	SwitchScene();
}

void Program::Event_SetMusicVolume(const string& mvol) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	World::audioSys()->MusicVolume(stoi(mvol));
	SwitchScene();
}

void Program::Event_SetSoundVolume(const string& svol) {
	World::audioSys()->PlaySound(Default::cueNameClick);

	World::audioSys()->SoundVolume(stoi(svol));
	SwitchScene();
}

void Program::Event_SetSongDelay(const string& sdelay) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::audioSys()->SongDelay(stof(sdelay));
}

void Program::Event_SetScrollX(const string& scrollx) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::inputSys()->ScrollSpeed(vec2f(stof(scrollx), World::inputSys()->Settings().scrollSpeed.y));
}

void Program::Event_SetScrollY(const string& scrolly) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::inputSys()->ScrollSpeed(vec2f(World::inputSys()->Settings().scrollSpeed.x, stof(scrolly)));
}

void Program::Event_SetDeadzone(const string& deadz) {
	World::audioSys()->PlaySound(Default::cueNameClick);
	World::inputSys()->Deadzone(stoi(deadz));
}

// OTHER EVENTS

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
		} else if (Filer::FileType(file) == EFileType::dir) {
			string libDir = World::scene()->Settings().LibraryPath();
			if (parentPath(file) == libDir) {
				editor->AddBook(filename(file));
				SwitchScene();
			} else if (file == libDir) {
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
	vector<Object*> objects;

	// little conveniences
	vec2i res = World::winSys()->Resolution();
	vec2i sizT(140, 40);

	if (curMenu == EMenu::books) {
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
	} else if (curMenu == EMenu::browser) {
		// back button
		objects = {new ButtonText(Object(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))};

		// list
		vector<ListItem*> items;
		for (string& it : browser->ListDirs())
			items.push_back(new ItemButton(it, "", &Program::Event_OpenBrowser));
		for (string& it : browser->ListFiles())
			items.push_back(new ItemButton(it, browser->CurDir()+it, &Program::Event_OpenReader));
		objects.push_back(new ListBox(Object(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
	} else if (curMenu == EMenu::reader) {
		// reader box
		string file = static_cast<char*>(dat);
		if (Filer::FileType(file) == EFileType::dir) {
			World::library()->LoadPics(Filer::GetPics(file));
			file.clear();
		}
		else 
			World::library()->LoadPics(Filer::GetPics(parentPath(file)));

		objects = {new ReaderBox(Object(0, 0, res, FIX_ANC | FIX_END, EColor::background), World::library()->Pictures(), file)};
	} else if (curMenu == EMenu::playlists) {
		// top buttons
		sizT = vec2i(80, 30);
		objects = {
			new ButtonText(Object(vec2i(0,         0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenBookList, World::library()->getLine("library")),
			new ButtonText(Object(vec2i(res.x/3,   0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::Event_OpenGeneralSettings, World::library()->getLine("settings")),
			new ButtonText(Object(vec2i(res.x/3*2, 0), -1, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::Event_Back, World::library()->getLine("exit")),

			new ButtonText(Object(vec2i(0,   60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_AddPlaylistButtonClick, World::library()->getLine("new")),
			new ButtonText(Object(vec2i(90,  60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_EditPlaylistButtonClick, World::library()->getLine("edit")),
			new ButtonText(Object(vec2i(180, 60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_DeletePlaylistButtonClick, World::library()->getLine("del"))
		};

		// playlist list
		TileBox* box = new TileBox(Object(vec2i(0, 100), -1, vec2i(res.x, res.y-100), FIX_ANC | FIX_END), {}, vec2i(400, 30));
		vector<ListItem*> tiles;
		for (string& it : Filer::ListDir(World::scene()->Settings().PlaylistPath(), FILTER_FILE, {".ini"}))
			tiles.push_back(new ListItem(delExt(it), box));
		box->Items(tiles);
		objects.push_back(box);
	} else if (curMenu == EMenu::plistEditor) {
		// option buttons
		sizT.x = 120;
		objects = {
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_BrowseButtonClick, World::library()->getLine("browse")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_AddSongBookButtonClick, World::library()->getLine("add")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_EditSongBookButtonClick, World::library()->getLine("edit")),
			new ButtonText(Object(vec2i(0, 200), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_DeleteSongBookButtonClick, World::library()->getLine("del")),
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
	} else if (curMenu == EMenu::songSearch) {
		// buttons
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_AddSongFileDirButtonClick, World::library()->getLine("add")),
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
	} else if (curMenu == EMenu::bookSearch) {
		// back button
		objects = {	new ButtonText(Object(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))	};

		// list
		vector<ListItem*> items;
		for (string& it : Filer::ListDir(World::scene()->Settings().DirLib(), FILTER_DIR))
			items.push_back(new ItemButton(it, "", &Program::Event_AddBook));
		objects.push_back(new ListBox(Object(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
	} else if (curMenu == EMenu::generalSets) {
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
	} else if (curMenu == EMenu::videoSets) {
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
			new Switchbox(box, World::library()->getLine("renderer"), getAvailibleRenderers(), World::winSys()->Settings().renderer, &Program::Event_SetRenderer),
		};
		box->Items(items);
		objects.push_back(box);
	} else if (curMenu == EMenu::audioSets) {
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
	} else if (curMenu == EMenu::controlsSets) {
		objects = {
			new ButtonText(Object(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenGeneralSettings, World::library()->getLine("general")),
			new ButtonText(Object(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenVideoSettings, World::library()->getLine("video")),
			new ButtonText(Object(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_OpenAudioSettings, World::library()->getLine("audio")),
			new ButtonText(Object(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::Event_Back, World::library()->getLine("back"))
		};

		ListBox* lbox = new ListBox(Object(vec2i(160, 0), -1, vec2i(res.x-160, 100), FIX_ANC | FIX_EX | FIX_H, EColor::background));
		vector<ListItem*> litems = {
			new LineEdit(lbox, World::library()->getLine("scroll speed")+" X", to_string(World::inputSys()->Settings().scrollSpeed.x), ETextType::floating, &Program::Event_SetScrollX),
			new LineEdit(lbox, World::library()->getLine("scroll speed")+" Y", to_string(World::inputSys()->Settings().scrollSpeed.y), ETextType::floating, &Program::Event_SetScrollY),
			new LineEdit(lbox, World::library()->getLine("deadzone"), to_string(World::inputSys()->Settings().deadzone), ETextType::integer, &Program::Event_SetDeadzone)
		};
		lbox->Items(litems);
		objects.push_back(lbox);

		TableBox* tbox = new TableBox(Object(vec2i(160, 110), -1, vec2i(res.x-160, res.y-110), FIX_ANC | FIX_END, EColor::background), {}, {0.31f, 0.23f, 0.23f, 0.23f});
		const map<string, Shortcut*>& smp = World::inputSys()->Settings().shortcuts;
		grid2<ListItem*> titems(smp.size(), 4);
		uint row = 0;
		for (map<string, Shortcut*>::const_iterator it=smp.begin(); it!=smp.end(); it++, row++) {
			titems.at(row, 0) = new ListItem(it->first, tbox);
			titems.at(row, 1) = new KeyGetter(tbox, KeyGetter::EAcceptType::keyboard, it->second);
			titems.at(row, 2) = new KeyGetter(tbox, KeyGetter::EAcceptType::joystick, it->second);
			titems.at(row, 3) = new KeyGetter(tbox, KeyGetter::EAcceptType::gamepad, it->second);
		}
		tbox->Items(titems);
		objects.push_back(tbox);
	}
	World::scene()->SwitchMenu(objects);
}

float Program::ModifySpeed(float value, float* axisFactor, float normalFactor, float fastFactor, float slowFactor) {
	if (World::inputSys()->isPressed(Default::shortcutFast, axisFactor))
		value *= fastFactor;
	else if (World::inputSys()->isPressed(Default::shortcutSlow, axisFactor))
		value *= slowFactor;
	else
		value *= normalFactor;
	return value * World::engine()->deltaSeconds();
}

Playlist Program::FindFittingPlaylist(const string& picPath) const {
	// get book name
	string name;
	size_t start = World::scene()->Settings().LibraryPath().length();
	for (size_t i=start; i!=picPath.length(); i++)
		if (picPath[i] == dsep) {
			name = picPath.substr(start, i-start);
			break;
		}
	if (name.empty())
		return Playlist();

	// check playlist files for matching book name
	for (string& file : Filer::ListDir(World::scene()->Settings().PlaylistPath(), FILTER_FILE)) {
		Playlist playlist = Filer::LoadPlaylist(delExt(file));
		if (playlist.songs.empty())
			continue;
		for (string& book : playlist.books)
			if (book == name)
				return playlist;
	}
	return Playlist();
}

Popup* Program::CreatePopupMessage(const string& msg) {
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(World::library()->Fonts()->TextSize(msg, 60).x, 120);

	vector<Object*> objs ={
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/2), FIX_SIZ), msg),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2), vec2i(size.x, size.y/2), FIX_SIZ), &Program::Event_Back, "Ok", ETextAlign::center)
	};
	return new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs);
}

Popup* Program::CreatePopupChoice(const string& msg, void(Program::*callb)()) {
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(World::library()->Fonts()->TextSize(msg, 60).x, 120);

	vector<Object*> objs ={
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), &Program::Event_Back, "Cancel", ETextAlign::center),
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	return new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs);
}

pair<Popup*, LineEditor*> Program::CreatePopupText(const string& msg, const string& text, void(Program::*callt)(const string&), void(Program::*callb)()) {
	vec2i res = World::winSys()->Resolution();
	vec2i size = vec2i(World::library()->Fonts()->TextSize(msg, 60).x, 180);

	LineEditor* editor = new LineEditor(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), text, ETextType::text, callt, &Program::Event_Back);
	vector<Object*> objs ={
		new Label(Object(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		editor,
		new ButtonText(Object(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), &Program::Event_Back, "Cancel", ETextAlign::center),
		new ButtonText(Object(res/2, vec2i(res.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	return make_pair(new Popup(Object(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), objs), editor);
}
