#include "engine/world.h"

// BOOKS

Program::Program() :
	state(new ProgBooks)
{}

void Program::eventOpenBookList(Button* but) {
	setState(new ProgBooks);
}

void Program::eventOpenBrowser(Button* but) {
	browser.reset(new Browser(appendDsep(World::winSys()->sets.getDirLib()) + static_cast<Label*>(but)->getText()));
	setState(new ProgBrowser);
}

void Program::eventBrowserGoUp(Button* but) {
	if (browser->goUp())
		World::scene()->resetLayouts();
	else {
		browser.reset();
		setState(new ProgBooks);
	}
}

void Program::eventBrowserGoIn(Button* but) {
	if (browser->goIn(static_cast<Label*>(but)->getText()))
		World::scene()->resetLayouts();
}

void Program::eventOpenReader(Button* but) {
	startReader(static_cast<Label*>(but)->getText());
}

void Program::eventOpenLastPage(Button* but) {
	const string& book = static_cast<Label*>(but)->getText();
	string file = Filer::getLastPage(book);
	if (file.empty())
		eventOpenBrowser(but);
	else {
		browser.reset(new Browser(appendDsep(World::winSys()->sets.getDirLib()) + book, parentPath(file)));
		if (!startReader(filename(file)))
			eventOpenBrowser(but);
	}
}

// READER

void Program::eventZoomIn(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->zoom *= Default::zoomFactor;
}

void Program::eventZoomOut(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->zoom /= Default::zoomFactor;
}

void Program::eventZoomReset(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->zoom = 1.f;
}

void Program::eventCenterView(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->centerListX();
}

void Program::eventNextDir(Button* but) {
	browser->goNext();
	browser->selectFirstPicture();
	World::scene()->resetLayouts();
}

void Program::eventPrevDir(Button* but) {
	browser->goPrev();
	browser->selectFirstPicture();
	World::scene()->resetLayouts();
}

void Program::eventPlayPause(Button* but) {
	if (player)
		player->playPauseMusic();
}

void Program::eventNextSong(Button* but) {
	if (player)
		player->nextSong();
}

void Program::eventPrevSong(Button* but) {
	if (player)
		player->prevSong();
}

void Program::eventVolumeUp(Button* but) {
	if (player)
		player->setVolume(World::winSys()->sets.getVolume() + Default::volumeStep);
}

void Program::eventVolumeDown(Button* but) {
	if (player)
		player->setVolume(World::winSys()->sets.getVolume() - Default::volumeStep);
}

void Program::eventMute(Button* but) {
	if (player)
		player->songMuteSwitch();
}

void Program::eventExitReader(Button* but) {
	player.reset();
	SDL_ShowCursor(SDL_ENABLE);
	setState(new ProgBrowser);
}

bool Program::startReader(const string& picname) {
	if (!browser->selectPicture(picname))
		return false;

	player.reset(new Player);
	if (!player->init(getBook(browser->getCurDir() + picname)))
		player.reset();

	setState(new ProgReader);
	return true;
}

// PLAYLISTS

void Program::eventOpenPlaylistList(Button* but) {
	setState(new ProgPlaylists);
}

void Program::eventAddPlaylist(Button* but) {
	World::scene()->setPopup(ProgState::createPopupTextInput("New Playlist", "", &Program::eventAddPlaylistOk));
}

void Program::eventAddPlaylistOk(Button* but) {
	const string& name = static_cast<LineEdit*>(World::scene()->getPopup()->getWidget(1))->getText();
	if (Filer::fileType(appendDsep(World::winSys()->sets.getDirPlist()) + name))
		World::scene()->setPopup(ProgState::createPopupMessage("File already exists."));
	else {
		Filer::savePlaylist(Playlist(name));
		World::scene()->resetLayouts();
	}
}

void Program::eventEditPlaylist(Button* but) {
	const uset<Widget*>& sel = static_cast<Layout*>(World::scene()->getLayout()->getWidget(2))->getSelected();
	if (!sel.empty()) {
		editor.reset(new PlaylistEditor(static_cast<Label*>(*sel.begin())->getText()));
		setState(new ProgEditor);
	}
}

void Program::eventRenamePlaylist(Button* but) {
	const uset<Widget*>& sel = static_cast<Layout*>(World::scene()->getLayout()->getWidget(2))->getSelected();
	if (!sel.empty())
		World::scene()->setPopup(ProgState::createPopupTextInput("New Name", static_cast<Label*>(*sel.begin())->getText(), &Program::eventRenamePlaylistOk));
}

void Program::eventRenamePlaylistOk(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(World::scene()->getPopup()->getWidget(1));
	string dir = appendDsep(World::winSys()->sets.getDirPlist());
	if (Filer::rename(dir + le->getOldText() + ".ini", dir + le->getText() + ".ini"))
		World::scene()->resetLayouts();
	else
		World::scene()->setPopup(ProgState::createPopupMessage("Failed to rename file."));
}

void Program::eventDeletePlaylist(Button* but) {
	sizt num = static_cast<Layout*>(World::scene()->getLayout()->getWidget(2))->getSelected().size();
	if (num > 0)
		World::scene()->setPopup(ProgState::createPopupChoice("Are you sure?", &Program::eventDeletePlaylistOk));
}

void Program::eventDeletePlaylistOk(Button* but) {
	for (Widget* it : static_cast<Layout*>(World::scene()->getLayout()->getWidget(2))->getSelected())
		Filer::remove(appendDsep(World::winSys()->sets.getDirPlist()) + static_cast<Label*>(it)->getText() + ".ini");
	World::scene()->resetLayouts();
}

void Program::eventExitPlaylistEditor(Button* but) {
	editor.reset();
	setState(new ProgPlaylists);
}

void Program::eventSwitchSB(Button* but) {
	editor->showSongs = !editor->showSongs;
	World::scene()->resetLayouts();
}

void Program::eventBrowseSB(Button* but) {
	if (editor->showSongs) {
#ifdef _WIN32
		browser.reset(new Browser("\\", std::getenv("UserProfile")));
#else
		browser.reset(new Browser("/", std::getenv("HOME")));
#endif
		setState(new ProgSearchSongs);
	} else
		setState(new ProgSearchBooks);
}

void Program::eventAddSB(Button* but) {
	World::scene()->setPopup(ProgState::createPopupTextInput("Add " + string(editor->showSongs ? "Song" : "Book"), "", &Program::eventAddSBOk));
}

void Program::eventAddSBOk(Button* but) {
	if (editor->add(static_cast<LineEdit*>(World::scene()->getPopup()->getWidget(1))->getText()))
		World::scene()->resetLayouts();
	else
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
}

void Program::eventEditSB(Button* but) {
	const uset<Widget*>& sel = static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getSelected();
	if (!sel.empty())
		World::scene()->setPopup(ProgState::createPopupTextInput("Rename " + string(editor->showSongs ? "Song" : "Book"), static_cast<Label*>(*sel.begin())->getText(), &Program::eventEditSBOk));
}

void Program::eventEditSBOk(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(World::scene()->getPopup()->getWidget(1));
	if (editor->rename(le->getOldText(), le->getText()))
		World::scene()->resetLayouts();
	else
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid name."));
}

void Program::eventDeleteSB(Button* but) {
	for (Widget* it : static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getSelected())
		editor->del(static_cast<Label*>(it)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSavePlaylist(Button* but) {
	Filer::savePlaylist(editor->getPlaylist());
	eventExitPlaylistEditor();
}

void Program::eventAddSongFD(Button* but) {
	string fails;
	for (Widget* it : static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getSelected()) {
		const string& text = static_cast<Label*>(it)->getText();
		if (!editor->addSong(browser->getCurDir() + text))
			fails.append(text + ", ");
	}

	eventExitSongBrowser();
	if (!fails.empty()) {
		fails.erase(fails.size()-2);
		World::scene()->setPopup(ProgState::createPopupMessage("Failed to add " + fails + '.'));
	}
}

void Program::eventSongBrowserGoUp(Button* but) {
	if (browser->goUp())
		World::scene()->resetLayouts();
	else
		eventExitSongBrowser();
}

void Program::eventExitSongBrowser(Button* but) {
	browser.reset();
	setState(new ProgEditor);
}

void Program::eventAddBook(Button* but) {
	editor->addBook(static_cast<Label*>(but)->getText());
	eventExitBookBrowser();
}

void Program::eventExitBookBrowser(Button* but) {
	setState(new ProgEditor);
}

// SETTINGS

void Program::eventOpenSettings(Button* but) {
	setState(new ProgSettings);
}

void Program::eventSwitchLanguage(Button* but) {
	World::drawSys()->setLanguage(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetLibraryPath(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	if (World::winSys()->sets.setDirLib(le->getText())) {
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
		le->setText(World::winSys()->sets.getDirLib());
	}
}

void Program::eventSetPlaylistsPath(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	if (World::winSys()->sets.setDirPlist(le->getText())) {
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
		le->setText(World::winSys()->sets.getDirPlist());
	}
}

void Program::eventSetVolumeSL(Button* but) {
	World::winSys()->sets.setVolume(static_cast<Slider*>(but)->getVal());
	static_cast<LineEdit*>(but->getParent()->getWidget(2))->setText(ntos(World::winSys()->sets.getVolume()));
}

void Program::eventSetVolumeLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	World::winSys()->sets.setVolume(stoi(le->getText()));
	le->setText(ntos(World::winSys()->sets.getVolume()));	// set text again in case the volume was out of range
	static_cast<Slider*>(but->getParent()->getWidget(1))->setVal(World::winSys()->sets.getVolume());
}

void Program::eventSwitchFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
}

void Program::eventSetTheme(Button* but) {
	World::drawSys()->setTheme(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetFont(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	World::drawSys()->setFont(le->getText());
	World::scene()->resetLayouts();
	if (World::winSys()->sets.getFont() != le->getText())
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid font."));
}

void Program::eventSetRenderer(Button* but) {
	World::winSys()->setRenderer(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetScrollSpeed(Button* but) {
	World::winSys()->sets.setScrollSpeed(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::winSys()->sets.setDeadzone(static_cast<Slider*>(but)->getVal());
	static_cast<LineEdit*>(but->getParent()->getWidget(2))->setText(ntos(World::winSys()->sets.getDeadzone()));	// update line edit
}

void Program::eventSetDeadzoneLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	World::winSys()->sets.setDeadzone(stoi(le->getText()));
	le->setText(ntos(World::winSys()->sets.getDeadzone()));	// set text again in case the volume was out of range
	static_cast<Slider*>(but->getParent()->getWidget(1))->setVal(World::winSys()->sets.getDeadzone());	// update slider

}

// OTHER

void Program::eventClosePopup(Button* but) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button* but) {
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state->eventClosing();
	state.reset(newState);
	World::scene()->resetLayouts();
}
