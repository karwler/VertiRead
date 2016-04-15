#include "engine/world.h"

// reader events

void Program::Event_Up() {
	if (dCast<ScrollArea*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 3.f : InputSys::isPressed(SDL_SCANCODE_LALT) ? 0.5f : 1.f;
		sCast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Down() {
	if (dCast<ScrollArea*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 3.f : InputSys::isPressed(SDL_SCANCODE_LALT) ? 0.5f : 1.f;
		sCast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Left() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 3.f : InputSys::isPressed(SDL_SCANCODE_LALT) ? 0.5f : 1.f;
		sCast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Right() {
	if (dCast<ReaderBox*>(World::scene()->FocusedObject())) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= InputSys::isPressed(SDL_SCANCODE_LSHIFT) ? 3.f : InputSys::isPressed(SDL_SCANCODE_LALT) ? 0.5f : 1.f;
		sCast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(spd*World::engine->deltaSeconds()*100.f);
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

void Program::Event_SwitchButtonClick() {
	editor->showSongs = !editor->showSongs;
	World::scene()->SwitchMenu(curMenu, editor);
}

void Program::Event_AddButtonClick() {
	if (curMenu == EMenu::playlists)
		World::scene()->SetPopup(new PopupText("New Playlist"));
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->AddSong();
		else
			editor->AddBook();
		World::scene()->SwitchMenu(curMenu, editor);
	}
}

void Program::Event_DeleteButtonClick() {
	if (curMenu == EMenu::playlists) {
		ListItem* item = World::scene()->SelectedButton();
		if (item) {
			fs::remove(Filer::dirPlist() + item->label);
			Event_OpenPlaylistList();
		}
	}
	else if (curMenu == EMenu::plistEditor) {
		if (editor->showSongs)
			editor->DelSong();
		else
			editor->DelBook();
		World::scene()->SwitchMenu(curMenu, editor);
	}
}

void Program::Event_EditButtonClick() {
	if (curMenu == EMenu::playlists) {
		ListItem* item = World::scene()->SelectedButton();
		if (item)
			Event_OpenPlaylistEditor((void*)item->label.c_str());
	}
	else if (curMenu == EMenu::plistEditor) {
		ListItem* item = World::scene()->SelectedButton();
		if (item)
			World::scene()->SetPopup(new PopupText("Rename", item->label));
	}
}

void Program::Event_SaveButtonClick() {
	if (editor)
		Filer::SavePlaylist(editor->getPlaylist());
}

// MENU EVENTS

void Program::Event_OpenBookList() {
	World::scene()->SwitchMenu(curMenu = EMenu::books);
}

void Program::Event_OpenBrowser(void* path) {
	if (browser)
		browser->GoTo(cstr(path));
	else
		browser = new Browser(cstr(path));
	World::scene()->SwitchMenu(curMenu = EMenu::browser, browser);
}

void Program::Event_OpenReader(void* file) {
	World::scene()->SwitchMenu(curMenu = EMenu::reader, (void*)string(cstr(file)).c_str());
}

void Program::Event_OpenPlaylistList() {
	World::scene()->SwitchMenu(curMenu = EMenu::playlists);
}

void Program::Event_OpenPlaylistEditor(void* playlist) {
	editor = new PlaylistEditor(cstr(playlist));
	World::scene()->SwitchMenu(curMenu = EMenu::plistEditor, editor);
}

void Program::Event_OpenGeneralSettings() {
	World::scene()->SwitchMenu(curMenu = EMenu::generalSets);
}

void Program::Event_OpenVideoSettings() {
	World::scene()->SwitchMenu(curMenu = EMenu::videoSets);
}

void Program::Event_OpenAudioSettings() {
	World::scene()->SwitchMenu(curMenu = EMenu::audioSets);
}

void Program::Event_OpenControlsSettings() {
	World::scene()->SwitchMenu(curMenu = EMenu::controlsSets);
}

void Program::Event_Back() {
	if (World::scene()->ShowingPopup())
		Event_PopupCancel();
	else if (curMenu == EMenu::reader)
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
	else if (curMenu == EMenu::plistEditor) {
		editor.reset();
		World::scene()->SwitchMenu(curMenu = EMenu::playlists);
	}
	else
		World::engine->Close();
}

// OTHER EVENTS

void Program::Event_PopupCancel() {
	World::scene()->SetPopup(nullptr);
}

void Program::Event_PopupOk(PopupChoice* box) {
	if (dCast<PopupText*>(box))
		Event_TextEditConfirmed(sCast<PopupText*>(box)->Line());
}

void Program::Event_TextEditConfirmed(TextEdit* box) {
	if (curMenu == EMenu::playlists) {
		if (!fs::exists(Filer::dirPlist() + box->getText())) {
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
		World::scene()->SwitchMenu(curMenu, editor);
	}
	World::inputSys()->SetCaptureText(nullptr);
}

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
