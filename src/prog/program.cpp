#include "engine/world.h"
#include "engine/filer.h"

// READER EVENTS

void Program::eventUp(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->getFocWidget())) {
		float factor = 1.f;
		amt = modifySpeed(amt * box->getZoom() * World::inputSys()->getSettings().scrollSpeed.y, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->scrollList(-amt * factor);
	}
}

void Program::eventDown(float amt) {
	if (ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->getFocWidget())) {
		float factor = 1.f;
		amt = modifySpeed(amt * box->getZoom() * World::inputSys()->getSettings().scrollSpeed.y, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->scrollList(amt * factor);
	}
}

void Program::eventRight(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget())) {
		float factor = 1.f;
		amt = modifySpeed(amt * box->getZoom() * World::inputSys()->getSettings().scrollSpeed.x, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->scrollListX(amt * factor);
	}
}

void Program::eventLeft(float amt) {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget())) {
		float factor = 1.f;
		amt = modifySpeed(amt * box->getZoom() * World::inputSys()->getSettings().scrollSpeed.x, &factor, Default::normalScrollFactor, Default::scrollFactorFast, Default::scrollFactorSlow);
		box->scrollListX(-amt * factor);
	}
}

void Program::eventPageUp() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget())) {
		size_t i = box->visiblePictures().x;
		if (i != 0)
			i--;
		box->scrollList(box->image(i).pos.y);
	}
}

void Program::eventPageDown() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget())) {
		size_t i = box->visiblePictures().x;
		if (i != box->numPics()-1)
			i++;
		box->scrollList(box->image(i).pos.y);
	}
}

void Program::eventZoomIn() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget())) 
		box->multZoom(Default::zoomFactor);
}

void Program::eventZoomOut() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget()))
		box->divZoom(Default::zoomFactor);
}

void Program::eventZoomReset() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget()))
		box->setZoom(1.f);
}

void Program::eventCenterView() {
	if (ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->getFocWidget()))
		box->dragListX(0);
}

void Program::eventNextDir() {
	if (curMenu == EMenu::reader)
		eventOpenReader(const_cast<char*>(browser->goNext().c_str()));
}

void Program::eventPrevDir() {
	if (curMenu == EMenu::reader)
		eventOpenReader(const_cast<char*>(browser->goPrev().c_str()));
}

// PLAYER EVENTS

void Program::eventPlayPause() {
	World::audioSys()->playPauseMusic();
}

void Program::eventNextSong() {
	World::audioSys()->nextSong();
	World::winSys()->setRedrawNeeded();
}

void Program::eventPrevSong() {
	World::audioSys()->prevSong();
	World::winSys()->setRedrawNeeded();
}

void Program::eventVolumeUp() {
	World::audioSys()->setMusicVolume(World::audioSys()->getSettings().musicVolume + 8);
}

void Program::eventVolumeDown() {
	World::audioSys()->setMusicVolume(World::audioSys()->getSettings().musicVolume - 8);
}

void Program::eventMute() {
	World::audioSys()->songMuteSwitch();
}

// PLAYLIST EDITOR EVENTS

void Program::eventAddPlaylistButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	pair<Popup*, LineEditor*> pt = createPopupText("New Playlist", "", &Program::eventNewPlaylistOk, &Program::eventNewPlaylistOk);
	World::scene()->setPopup(pt.first, pt.second);
}

void Program::eventDeletePlaylistButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (ListItem* item = World::scene()->selectedButton()) {
		Filer::remove(World::library()->getSettings().playlistPath() + item->label + ".ini");
		eventOpenPlaylistList();
	}
}

void Program::eventEditPlaylistButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (ListItem* item = World::scene()->selectedButton())
		eventOpenPlaylistEditor(const_cast<char*>(item->label.c_str()));
}

void Program::eventNewPlaylistOk() {
	for (Widget* it : World::scene()->getPopup()->widgets)
		if (LineEditor* wgt = dynamic_cast<LineEditor*>(it)) {
			eventNewPlaylistOk(wgt->getEditor().getText());
			break;
		}
}

void Program::eventNewPlaylistOk(const string& str) {
	World::audioSys()->playSound(Default::cueNameClick);

	if (Filer::fileExists(World::library()->getSettings().playlistPath() + str)) {
		World::audioSys()->playSound(Default::cueNameError);
		World::scene()->setPopup(createPopupMessage("File already exists."));
	} else {
		Filer::savePlaylist(Playlist(str));
		eventOpenPlaylistList();
	}
}

void Program::eventSwitchButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	editor->setShowSongs(!editor->getShowSongs());
	switchScene();
}

void Program::eventBrowseButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (editor->getShowSongs())
		eventOpenSongBrowser("");
	else
		switchScene(EMenu::bookSearch);
}

void Program::eventAddSongBookButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (editor->getShowSongs())
		editor->addSong("");
	else
		editor->addBook("");
	switchScene();
}

void Program::eventAddSongFileDirButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (ListItem* item = World::scene()->selectedButton()) {
		if (browser->getCurDir() == string(1, dsep))
			editor->addSong(dsep+item->label);
		else
			editor->addSong(browser->getCurDir()+item->label);
		browser.clear();

		switchScene(EMenu::plistEditor);
	}
}

void Program::eventDeleteSongBookButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (editor->getShowSongs())
		editor->delSong();
	else
		editor->delBook();
	switchScene();
}

void Program::eventEditSongBookButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (ListItem* item = World::scene()->selectedButton()) {
		pair<Popup*, LineEditor*> pt = createPopupText("Rename " + string(editor->getShowSongs() ? "song" : "book"), item->getData(), &Program::eventSongBookRenameOk, &Program::eventSongBookRenameOk);
		World::scene()->setPopup(pt.first, pt.second);
	}
}

void Program::eventSongBookRenameOk() {
	for (Widget* it : World::scene()->getPopup()->widgets)
		if (LineEditor* wgt = dynamic_cast<LineEditor*>(it)) {
			eventSongBookRenameOk(wgt->getEditor().getText());
			break;
		}
}

void Program::eventSongBookRenameOk(const string& str) {
	World::audioSys()->playSound(Default::cueNameClick);

	if (editor->getShowSongs())
		editor->renameSong(str);
	else
		editor->renameBook(str);
	switchScene();
}

void Program::eventItemDoubleclicked(ListItem* item) {
	World::audioSys()->playSound(Default::cueNameClick);

	// does the same as Event_EditButtonClicked
	if (curMenu == EMenu::playlists)
		eventOpenPlaylistEditor(const_cast<char*>(item->label.c_str()));
	else if (curMenu == EMenu::plistEditor) {
		pair<Popup*, LineEditor*> pt = createPopupText("Rename " + string(editor->getShowSongs() ? "song" : "book"), item->getData(), &Program::eventSongBookRenameOk, &Program::eventSongBookRenameOk);
		World::scene()->setPopup(pt.first, pt.second);
	} else if (curMenu == EMenu::songSearch) {
#ifdef _WIN32
		if (browser->getCurDir() == "\\")
			eventOpenSongBrowser(item->label+dsep);
#else
		if (browser->getCurDir() == "/" && Filer::fileType(dsep + item->label) == FTYPE_DIR)
			eventOpenSongBrowser(item->label);
#endif
		else if (Filer::fileType(browser->getCurDir() + item->label) == FTYPE_DIR)
			eventOpenSongBrowser(item->label);
		else {
			if (browser->getCurDir() == string(1, dsep))
				editor->addSong(dsep+item->label);
			else
				editor->addSong(browser->getCurDir() + item->label);
			browser.clear();
			switchScene(EMenu::plistEditor);
		}
	} else if (curMenu == EMenu::bookSearch) {
		editor->addBook(item->label);
		switchScene(EMenu::plistEditor);
	}
}

void Program::eventSaveButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (editor)
		Filer::savePlaylist(editor->getPlaylist());
}

void Program::eventUpButtonClick() {
	World::audioSys()->playSound(Default::cueNameClick);

	if (browser->goUp())
		switchScene();
	else
		eventBack();
}

void Program::eventAddBook(void* name) {
	World::audioSys()->playSound(Default::cueNameClick);

	editor->addBook(static_cast<char*>(name));
	switchScene(EMenu::plistEditor);
}

// MENU EVENTS

void Program::eventOpenBookList() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::books);
}

void Program::eventOpenBrowser(void* path) {
	World::audioSys()->playSound(Default::cueNameClick);

	if (browser)
		browser->goTo(string(static_cast<char*>(path)));
	else
		browser = new Browser(static_cast<char*>(path));
	switchScene(EMenu::browser);
}

void Program::eventOpenReader(void* file) {
	World::audioSys()->playSound(Default::cueNameOpen);

	vector<string> songs = findFittingPlaylist(getBook(static_cast<char*>(file)));
	if (!songs.empty())
		World::audioSys()->setPlaylist(songs);
	switchScene(EMenu::reader, file);
}

void Program::eventOpenLastPage(void* book) {
	string file = Filer::getLastPage(string(static_cast<char*>(book)).substr(World::library()->getSettings().libraryPath().length()));
	if (file.empty())
		eventOpenBrowser(book);
	else
		eventOpenReader(const_cast<char*>(file.c_str()));
}

void Program::eventOpenPlaylistList() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::playlists);
}

void Program::eventOpenPlaylistEditor(void* playlist) {
	World::audioSys()->playSound(Default::cueNameClick);

	editor = new PlaylistEditor(static_cast<char*>(playlist));
	switchScene(EMenu::plistEditor);
}

void Program::eventOpenSongBrowser(const string& dir) {
	if (browser)
		browser->goTo(dir); 
	else {
#ifdef _WIN32
		browser = new Browser("\\", std::getenv("UserProfile"));
#else
		browser = new Browser("/", std::getenv("HOME"));
#endif
	}
	switchScene(EMenu::songSearch);
}

void Program::eventOpenGeneralSettings() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::generalSets);
}

void Program::eventOpenVideoSettings() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::videoSets);
}

void Program::eventOpenAudioSettings() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::audioSets);
}

void Program::eventOpenControlsSettings() {
	World::audioSys()->playSound(Default::cueNameClick);
	switchScene(EMenu::controlsSets);
}

void Program::eventOk() {
	// idk. someday there might be something here
}

void Program::eventBack() {
	World::audioSys()->playSound(Default::cueNameBack);

	if (World::scene()->getPopup())
		World::scene()->setPopup(nullptr);
	else if (curMenu == EMenu::reader) {
		World::audioSys()->unloadPlaylist();
		switchScene(EMenu::browser);
	} else if (curMenu == EMenu::browser) {
		if (browser->goUp())
			switchScene(browser);
		else {
			browser.clear();
			switchScene(EMenu::books);
		}
	} else if (curMenu == EMenu::plistEditor) {
		editor.clear();
		switchScene(EMenu::playlists);
	} else if (curMenu == EMenu::songSearch) {
		browser.clear();
		switchScene(EMenu::plistEditor);
	} else if (curMenu == EMenu::bookSearch)
		switchScene(EMenu::plistEditor);
	else if (curMenu >= EMenu::generalSets)
		switchScene(EMenu::books);
	else
		World::base()->close();
}

// SETTINGS EVENTS

void Program::eventSwitchLanguage(const string& language) {
	World::audioSys()->playSound(Default::cueNameClick);
	
	World::library()->loadLanguage(language);
	switchScene();
}

void Program::eventSetLibraryPath(const string& dir) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::library()->setLibraryPath(dir);
}

void Program::eventSetPlaylistsPath(const string& dir) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::library()->setPlaylistsPath(dir);
}

void Program::eventSwitchFullscreen(bool on) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::winSys()->setFullscreen(on);
}

void Program::eventSetTheme(const string& theme) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::winSys()->setTheme(theme);
}

void Program::eventSetFont(const string& font) {
	World::audioSys()->playSound(Default::cueNameClick);

	World::winSys()->setFont(font);
	switchScene();
}

void Program::eventSetRenderer(const string& renderer) {
	World::audioSys()->playSound(Default::cueNameClick);

	World::winSys()->setRenderer(renderer);
	switchScene();
}

void Program::eventSetMusicVolume(const string& mvol) {
	World::audioSys()->playSound(Default::cueNameClick);

	World::audioSys()->setMusicVolume(stoi(mvol));
	switchScene();
}

void Program::eventSetSoundVolume(const string& svol) {
	World::audioSys()->playSound(Default::cueNameClick);

	World::audioSys()->setSoundVolume(stoi(svol));
	switchScene();
}

void Program::eventSetSongDelay(const string& sdelay) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::audioSys()->setSongDelay(stof(sdelay));
}

void Program::eventSetScrollX(const string& scrollx) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::inputSys()->setScrollSpeed(vec2f(stof(scrollx), World::inputSys()->getSettings().scrollSpeed.y));
}

void Program::eventSetScrollY(const string& scrolly) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::inputSys()->setScrollSpeed(vec2f(World::inputSys()->getSettings().scrollSpeed.x, stof(scrolly)));
}

void Program::eventSetDeadzone(const string& deadz) {
	World::audioSys()->playSound(Default::cueNameClick);
	World::inputSys()->setDeadzone(stoi(deadz));
}

// OTHER EVENTS

void Program::eventSelectionSet(void* box) {
	if (editor)
		editor->selected = static_cast<ScrollAreaItems*>(box)->getSelectedItem();
}

void Program::eventScreenMode() {
	World::winSys()->setFullscreen(!World::winSys()->getSettings().fullscreen);
}

void Program::eventFileDrop(char* file) {
	if (curMenu == EMenu::plistEditor) {
		if (editor->getShowSongs()) {
			editor->addSong(file);
			switchScene();
		} else if (Filer::fileType(file) == FTYPE_DIR) {
			string libDir = World::library()->getSettings().libraryPath();
			if (parentPath(file) == libDir) {
				editor->addBook(filename(file));
				switchScene();
			} else if (file == libDir) {
				string path = appendDsep(file);
				for (string& it : Filer::listDir(path, FTYPE_DIR))
					editor->addBook(filename(path+it));
				switchScene();
			}
		}
	}
}

void Program::switchScene(EMenu newMenu, void* dat) {
	curMenu = newMenu;
	switchScene(dat);
}

void Program::switchScene(void* dat) const {
	// set stuff up
	World::library()->clear();
	vector<Widget*> widgets;
	vec2i res = World::winSys()->resolution();
	vec2i sizT(140, 40);

	if (curMenu == EMenu::books) {
		// top buttons
		widgets = {
			new ButtonText(Widget(vec2i(0,         0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::eventOpenPlaylistList, World::library()->line("playlists")),
			new ButtonText(Widget(vec2i(res.x/3,   0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::eventOpenGeneralSettings, World::library()->line("settings")),
			new ButtonText(Widget(vec2i(res.x/3*2, 0), -1, vec2i(res.x/3.f,  50), FIX_Y | FIX_H), &Program::eventBack, World::library()->line("exit"))
		};
		
		// book list
		string libPath = World::library()->getSettings().libraryPath();
		vector<ListItem*> tiles;
		for (string& it : Filer::listDir(libPath, FTYPE_DIR))
			tiles.push_back(new ItemButton(it, libPath+it, &Program::eventOpenBrowser, &Program::eventOpenLastPage));
		widgets.push_back(new TileBox(Widget(vec2i(0, 60), -1, vec2i(res.x, res.y-60), FIX_ANC | FIX_END, EColor::background), tiles, vec2i(350, 30)));
	} else if (curMenu == EMenu::browser) {
		// back button
		widgets = {new ButtonText(Widget(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))};

		// list
		vector<ListItem*> items;
		for (string& it : browser->listDirs())
			items.push_back(new ItemButton(it, "", &Program::eventOpenBrowser));
		for (string& it : browser->listFiles())
			items.push_back(new ItemButton(it, browser->getCurDir()+it, &Program::eventOpenReader));
		widgets.push_back(new ListBox(Widget(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
	} else if (curMenu == EMenu::reader) {
		// reader box
		string file = static_cast<char*>(dat);
		string dir;
		if (Filer::fileType(file) == FTYPE_DIR) {
			dir = file;
			file.clear();
		} else
			dir = parentPath(file);

		widgets = {new ReaderBox(Widget(0, 0, res, FIX_ANC | FIX_END, EColor::background), dir, file)};
	} else if (curMenu == EMenu::playlists) {
		// top buttons
		sizT = vec2i(80, 30);
		widgets = {
			new ButtonText(Widget(vec2i(0,         0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::eventOpenBookList, World::library()->line("library")),
			new ButtonText(Widget(vec2i(res.x/3,   0), -1, vec2i(res.x/3.1f, 50), FIX_Y | FIX_H), &Program::eventOpenGeneralSettings, World::library()->line("settings")),
			new ButtonText(Widget(vec2i(res.x/3*2, 0), -1, vec2i(res.x/3,    50), FIX_Y | FIX_H), &Program::eventBack, World::library()->line("exit")),

			new ButtonText(Widget(vec2i(0,   60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventAddPlaylistButtonClick, World::library()->line("new")),
			new ButtonText(Widget(vec2i(90,  60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventEditPlaylistButtonClick, World::library()->line("edit")),
			new ButtonText(Widget(vec2i(180, 60), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventDeletePlaylistButtonClick, World::library()->line("del"))
		};

		// playlist list
		TileBox* box = new TileBox(Widget(vec2i(0, 100), -1, vec2i(res.x, res.y-100), FIX_ANC | FIX_END, EColor::background), {}, vec2i(400, 30));
		vector<ListItem*> tiles;
		for (string& it : Filer::listDir(World::library()->getSettings().playlistPath(), FTYPE_FILE, {".ini"}))
			tiles.push_back(new ListItem(delExt(it), box));
		box->setItems(tiles);
		widgets.push_back(box);
	} else if (curMenu == EMenu::plistEditor) {
		// option buttons
		sizT.x = 120;
		widgets = {
			new ButtonText(Widget(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBrowseButtonClick, World::library()->line("browse")),
			new ButtonText(Widget(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventAddSongBookButtonClick, World::library()->line("add")),
			new ButtonText(Widget(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventEditSongBookButtonClick, World::library()->line("edit")),
			new ButtonText(Widget(vec2i(0, 200), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventDeleteSongBookButtonClick, World::library()->line("del")),
			new ButtonText(Widget(vec2i(0, 250), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventSaveButtonClick, World::library()->line("save")),
			new ButtonText(Widget(vec2i(0, 300), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("close"))
		};

		// playlist list
		ListBox* box = new ListBox(Widget(vec2i(sizT.x+10, 0), -1, vec2i(res.x-110, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items;
		if (editor->getShowSongs()) {
			widgets.push_back(new ButtonText(Widget(vec2i(0, 0), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventSwitchButtonClick, World::library()->line("books")));
			for (const string& it : editor->getPlaylist().songs)
				items.push_back(new ItemButton(filename(it), it, &Program::eventSelectionSet, nullptr, box));
		} else {
			widgets.push_back(new ButtonText(Widget(vec2i(0, 0), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventSwitchButtonClick, World::library()->line("songs")));
			for (const string& it : editor->getPlaylist().books)
				items.push_back(new ItemButton(it, "", &Program::eventSelectionSet, nullptr, box));
		}
		box->setItems(items);
		widgets.push_back(box);
	} else if (curMenu == EMenu::songSearch) {
		// buttons
		widgets = {
			new ButtonText(Widget(vec2i(0, 0),   -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::eventAddSongFileDirButtonClick, World::library()->line("add")),
			new ButtonText(Widget(vec2i(0, 50),  -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::eventUpButtonClick, World::library()->line("up")),
			new ButtonText(Widget(vec2i(0, 100), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))
		};

		// list
		ListBox* box = new ListBox(Widget(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items;
		for (string& it : browser->listDirs())
			items.push_back(new ListItem(it, box));
		for (string& it : browser->listFiles())
			items.push_back(new ListItem(it, box));
		box->setItems(items);
		widgets.push_back(box);
	} else if (curMenu == EMenu::bookSearch) {
		// back button
		widgets = {	new ButtonText(Widget(vec2i(0, 0), -1, vec2i(90, 40), FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))	};

		// list
		vector<ListItem*> items;
		for (string& it : Filer::listDir(World::library()->getSettings().getDirLib(), FTYPE_DIR))
			items.push_back(new ItemButton(it, "", &Program::eventAddBook));
		widgets.push_back(new ListBox(Widget(vec2i(100, 0), -1, vec2i(res.x-100, res.y), FIX_ANC | FIX_END, EColor::background), items));
	} else if (curMenu == EMenu::generalSets) {
		widgets = {
			new ButtonText(Widget(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenVideoSettings, World::library()->line("video")),
			new ButtonText(Widget(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenAudioSettings, World::library()->line("audio")),
			new ButtonText(Widget(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenControlsSettings, World::library()->line("controls")),
			new ButtonText(Widget(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))
		};

		ListBox* box = new ListBox(Widget(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new Switchbox(box, World::library()->line("language"), Filer::getAvailibleLanguages(), World::library()->getSettings().getLang(), &Program::eventSwitchLanguage),
			new LineEdit(box, World::library()->line("library"), World::library()->getSettings().getDirLib(), ETextType::text, &Program::eventSetLibraryPath),
			new LineEdit(box, World::library()->line("playlists"), World::library()->getSettings().getDirPlist(), ETextType::text, &Program::eventSetPlaylistsPath)
		};
		box->setItems(items);
		widgets.push_back(box);
	} else if (curMenu == EMenu::videoSets) {
		widgets = {
			new ButtonText(Widget(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenGeneralSettings, World::library()->line("general")),
			new ButtonText(Widget(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenAudioSettings, World::library()->line("audio")),
			new ButtonText(Widget(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenControlsSettings, World::library()->line("controls")),
			new ButtonText(Widget(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))
		};

		ListBox* box = new ListBox(Widget(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new Checkbox(box, World::library()->line("fullscreen"), World::winSys()->getSettings().fullscreen, &Program::eventSwitchFullscreen),
			new Switchbox(box, World::library()->line("theme"), Filer::getAvailibleThemes(), World::winSys()->getSettings().theme, &Program::eventSetTheme),
			new LineEdit(box, World::library()->line("font"), World::winSys()->getSettings().getFont(), ETextType::text, &Program::eventSetFont),
			new Switchbox(box, World::library()->line("renderer"), getAvailibleRenderers(), World::winSys()->getSettings().renderer, &Program::eventSetRenderer),
		};
		box->setItems(items);
		widgets.push_back(box);
	} else if (curMenu == EMenu::audioSets) {
		widgets = {
			new ButtonText(Widget(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenGeneralSettings, World::library()->line("general")),
			new ButtonText(Widget(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenVideoSettings, World::library()->line("video")),
			new ButtonText(Widget(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenControlsSettings, World::library()->line("controls")),
			new ButtonText(Widget(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))
		};

		ListBox* box = new ListBox(Widget(vec2i(160, 0), -1, vec2i(res.x-160, res.y), FIX_ANC | FIX_END, EColor::background));
		vector<ListItem*> items = {
			new LineEdit(box, World::library()->line("music vol"), to_string(World::audioSys()->getSettings().musicVolume), ETextType::integer, &Program::eventSetMusicVolume),
			new LineEdit(box, World::library()->line("sound vol"), to_string(World::audioSys()->getSettings().soundVolume), ETextType::integer, &Program::eventSetSoundVolume),
			new LineEdit(box, World::library()->line("song delay"), to_string(World::audioSys()->getSettings().songDelay), ETextType::floating, &Program::eventSetSongDelay)
		};
		box->setItems(items);
		widgets.push_back(box);
	} else if (curMenu == EMenu::controlsSets) {
		widgets = {
			new ButtonText(Widget(vec2i(0, 0),   -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenGeneralSettings, World::library()->line("general")),
			new ButtonText(Widget(vec2i(0, 50),  -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenVideoSettings, World::library()->line("video")),
			new ButtonText(Widget(vec2i(0, 100), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventOpenAudioSettings, World::library()->line("audio")),
			new ButtonText(Widget(vec2i(0, 150), -1, sizT, FIX_ANC | FIX_SIZ), &Program::eventBack, World::library()->line("back"))
		};

		ListBox* lbox = new ListBox(Widget(vec2i(160, 0), -1, vec2i(res.x-160, 100), FIX_ANC | FIX_EX | FIX_H, EColor::background));
		vector<ListItem*> litems = {
			new LineEdit(lbox, World::library()->line("scroll speed")+" X", to_string(World::inputSys()->getSettings().scrollSpeed.x), ETextType::floating, &Program::eventSetScrollX),
			new LineEdit(lbox, World::library()->line("scroll speed")+" Y", to_string(World::inputSys()->getSettings().scrollSpeed.y), ETextType::floating, &Program::eventSetScrollY),
			new LineEdit(lbox, World::library()->line("deadzone"), to_string(World::inputSys()->getSettings().deadzone), ETextType::integer, &Program::eventSetDeadzone)
		};
		lbox->setItems(litems);
		widgets.push_back(lbox);

		TableBox* tbox = new TableBox(Widget(vec2i(160, 110), -1, vec2i(res.x-160, res.y-110), FIX_ANC | FIX_END, EColor::background), {}, {0.31f, 0.23f, 0.23f, 0.23f});
		const map<string, Shortcut*>& smp = World::inputSys()->getSettings().shortcuts;
		grid2<ListItem*> titems(smp.size(), 4);
		uint row = 0;
		for (map<string, Shortcut*>::const_iterator it=smp.begin(); it!=smp.end(); it++, row++) {
			titems.at(row, 0) = new ListItem(it->first, tbox);
			titems.at(row, 1) = new KeyGetter(tbox, KeyGetter::EAcceptType::keyboard, it->second);
			titems.at(row, 2) = new KeyGetter(tbox, KeyGetter::EAcceptType::joystick, it->second);
			titems.at(row, 3) = new KeyGetter(tbox, KeyGetter::EAcceptType::gamepad, it->second);
		}
		tbox->setItems(titems);
		widgets.push_back(tbox);
	}
	World::scene()->switchMenu(widgets);
}

float Program::modifySpeed(float value, float* axisFactor, float normalFactor, float fastFactor, float slowFactor) {
	if (World::inputSys()->isPressed(Default::shortcutFast, axisFactor))
		value *= fastFactor;
	else if (World::inputSys()->isPressed(Default::shortcutSlow, axisFactor))
		value *= slowFactor;
	else
		value *= normalFactor;
	return value * World::base()->getDSec();
}

vector<string> Program::findFittingPlaylist(const string& book) const {
	// check playlist files for matching book name
	vector<string> songs;
	for (string& file : Filer::listDir(World::library()->getSettings().playlistPath(), FTYPE_FILE)) {
		Playlist playlist = Filer::getPlaylist(delExt(file));
		if (playlist.songs.empty())
			continue;

		// merge songs from all playlists with that book name
		for (string& it : playlist.books)
			if (it == book) {
				size_t old = songs.size();
				songs.resize(songs.size() + playlist.songs.size());
				for (size_t i=old; i!=songs.size(); i++)
					songs[i] = playlist.songPath(i-old);
				break;
			}
	}
	return songs;
}

Popup* Program::createPopupMessage(const string& msg) {
	vec2i res = World::winSys()->resolution();
	vec2i size = vec2i(World::library()->getFonts().textSize(msg, 60).x, 120);

	vector<Widget*> wgts ={
		new Label(Widget(res/2, res/2-size/2, vec2i(size.x, size.y/2), FIX_SIZ), msg),
		new ButtonText(Widget(res/2, vec2i(res.x/2-size.x/2, res.y/2), vec2i(size.x, size.y/2), FIX_SIZ), &Program::eventBack, "Ok", ETextAlign::center)
	};
	return new Popup(Widget(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), wgts);
}

Popup* Program::createPopupChoice(const string& msg, void(Program::*callb)()) {
	vec2i res = World::winSys()->resolution();
	vec2i size = vec2i(World::library()->getFonts().textSize(msg, 60).x, 120);

	vector<Widget*> wgts ={
		new Label(Widget(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		new ButtonText(Widget(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), &Program::eventBack, "Cancel", ETextAlign::center),
		new ButtonText(Widget(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	return new Popup(Widget(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), wgts);
}

pair<Popup*, LineEditor*> Program::createPopupText(const string& msg, const string& text, void(Program::*callt)(const string&), void(Program::*callb)()) {
	vec2i res = World::winSys()->resolution();
	vec2i size = vec2i(World::library()->getFonts().textSize(msg, 60).x, 180);

	LineEditor* editor = new LineEditor(Widget(res/2, vec2i(res.x/2-size.x/2, res.y/2-size.y/6), vec2i(size.x, size.y/3), FIX_SIZ), text, ETextType::text, callt, &Program::eventBack);
	vector<Widget*> wgts ={
		new Label(Widget(res/2, res/2-size/2, vec2i(size.x, size.y/3), FIX_SIZ), msg),
		editor,
		new ButtonText(Widget(res/2, vec2i(res.x/2-size.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), &Program::eventBack, "Cancel", ETextAlign::center),
		new ButtonText(Widget(res/2, vec2i(res.x/2, res.y/2+size.y/6), vec2i(size.x/2, size.y/3), FIX_SIZ), callb, "Ok", ETextAlign::center)
	};
	return make_pair(new Popup(Widget(res/2, res/2-size/2, size, FIX_SIZ, EColor::background), wgts), editor);
}
