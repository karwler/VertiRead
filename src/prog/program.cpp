#include "engine/world.h"

// reader events

void Program::Event_Up() {
	if (dCast<ScrollArea*>(World::scene()->FocusedObject())) {
		int amt = InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 100 : InputSys::isPressed(SDL_SCANCODE_LCTRL) ? 25 : 50;
		sCast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(-amt);
	}
}

void Program::Event_Down() {
	if (dCast<ScrollArea*>(World::scene()->FocusedObject())) {
		int amt = InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 100 : InputSys::isPressed(SDL_SCANCODE_LCTRL) ? 25 : 50;
		sCast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(amt);
	}
}

void Program::Event_Left() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		int amt = InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 40 : InputSys::isPressed(SDL_SCANCODE_LCTRL) ? 10 : 20;
		sCast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(-amt);
	}
}

void Program::Event_Right() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		int amt = InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 40 : InputSys::isPressed(SDL_SCANCODE_LCTRL) ? 10 : 20;
		sCast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(amt);
	}
}

void Program::Event_PageUp() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		ReaderBox* box = sCast<ReaderBox*>(World::scene()->FocusedObject());
		int i = box->VisiblePictures().x;
		i -= (i == 0) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_PageDown() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		ReaderBox* box = sCast<ReaderBox*>(World::scene()->FocusedObject());
		int i = box->VisiblePictures().x;
		i += (i == box->Pictures().size()-1) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_ZoomIn() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject()))
		sCast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(0.2f);
}

void Program::Event_ZoomOut() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject()))
		sCast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(-0.2f);
}

void Program::Event_ZoomReset() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject()))
		sCast<ReaderBox*>(World::scene()->FocusedObject())->Zoom(1.f);
}

void Program::Event_CenterView() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject()))
		sCast<ReaderBox*>(World::scene()->FocusedObject())->DragListX(0);
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

void Program::Event_AddButtonClick() {
	if (editor) {
		if (editor->showSongs)
			editor->AddSong();
		else
			editor->AddBook();
	}
	else {
		Filer::SavePlaylist(Playlist("new_playlist"));
		Event_OpenPlaylistList();
	}
}

void Program::Event_DeleteButtonClick() {
	if (editor) {
		if (editor->showSongs)
			editor->DelSong();
		else
			editor->DelBook();
	}
	else {
		ListItem* item = World::scene()->SelectedButton();
		if (item) {
			fs::remove(Filer::dirPlist() + item->label);
			Event_OpenPlaylistList();
		}
	}
}

void Program::Event_EditButtonClick() {
	ListItem* item = World::scene()->SelectedButton();
	if (item)
		Event_OpenPlaylistEditor((void*)item->label.c_str());
}

void Program::Event_SaveButtonClick() {
	if (editor)
		Filer::SavePlaylist(editor->getPlaylist());
}

// MENU EVENTS

void Program::Event_OpenBookList() {
	curMenu = EMenu::books;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenBrowser(void* path) {
	curMenu = EMenu::browser;
	if (browser)
		browser->GoTo(cstr(path));
	else
		browser = new Browser(cstr(path));
	World::scene()->SwitchMenu(curMenu, browser);
}

void Program::Event_OpenReader(void* file) {
	curMenu = EMenu::reader;
	World::scene()->SwitchMenu(curMenu, (void*)string(cstr(file)).c_str());
}

void Program::Event_OpenPlaylistList() {
	curMenu = EMenu::playlists;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenPlaylistEditor(void* playlist) {
	curMenu = EMenu::plistEditor;
	editor = new PlaylistEditor(cstr(playlist));
	World::scene()->SwitchMenu(curMenu, editor);
}

void Program::Event_OpenGeneralSettings() {
	curMenu = EMenu::generalSets;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenVideoSettings() {
	curMenu = EMenu::videoSets;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenAudioSettings() {
	curMenu = EMenu::audioSets;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenControlsSettings() {
	curMenu = EMenu::controlsSets;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_Back() {
	if (curMenu == EMenu::reader)
		World::scene()->SwitchMenu(curMenu = EMenu::browser, browser);
	else if (curMenu == EMenu::browser) {
		if (browser->GoUp())
			World::scene()->SwitchMenu(curMenu, browser);
		else {
			browser.reset();
			World::scene()->SwitchMenu(curMenu = EMenu::books);
		}
	}
	else if (curMenu >= EMenu::generalSets)
		World::scene()->SwitchMenu(curMenu = EMenu::books);
	else if (curMenu == EMenu::plistEditor)
		World::scene()->SwitchMenu(curMenu = EMenu::playlists);
	else
		World::engine->Close();
}

// OTHER EVENTS

void Program::Event_SelectionSet(void* box) {
	if (editor)
		editor->selected = sCast<ScrollArea*>(box)->SelectedItem();
}

void Program::Event_ScreenMode() {
	World::winSys()->Fullscreen(!World::winSys()->Settings().fullscreen);
}

EMenu Program::CurrentMenu() const {
	return curMenu;
}
