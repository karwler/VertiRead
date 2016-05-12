#include "engine/world.h"

// reader events

void Program::Event_Up() {
	ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject());
	if (box) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollList(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Down() {
	ScrollArea* box = dynamic_cast<ScrollArea*>(World::scene()->FocusedObject());
	if (box) {
		float spd = World::inputSys()->Settings().scrollSpeed.y;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollList(spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Left() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollListX(-spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_Right() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box) {
		float spd = World::inputSys()->Settings().scrollSpeed.x;
		spd *= World::inputSys()->isPressed("fast") ? 3.f : World::inputSys()->isPressed("slow") ? 0.5f : 1.f;
		box->ScrollListX(spd*World::engine->deltaSeconds()*100.f);
	}
}

void Program::Event_PageUp() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box) {
		int i = box->VisiblePictures().x;
		i -= (i == 0) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_PageDown() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box) {
		int i = box->VisiblePictures().x;
		i += (i == box->Pictures().size()-1) ? 0 : 1;
		box->ScrollList(box->getImage(i).pos.y);
	}
}

void Program::Event_ZoomIn() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box)
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(0.2f);
}

void Program::Event_ZoomOut() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box)
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(-0.2f);
}

void Program::Event_ZoomReset() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box)
		box->Zoom(1.f);
}

void Program::Event_CenterView() {
	ReaderBox* box = dynamic_cast<ReaderBox*>(World::scene()->FocusedObject());
	if (box)
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
			fs::remove(World::scene()->Settings().playlistParh() + item->label);
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
	if (World::scene()->getPopup())
		World::scene()->SetPopup(nullptr);
	else if (World::inputSys()->CapturedObject())
		World::inputSys()->SetCapture(nullptr);
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
		World::scene()->SwitchMenu(curMenu, editor);
	}
	World::inputSys()->SetCapture(nullptr);
}

void Program::Event_KeyCaptureOk(SDL_Scancode key) {
	cout << "key pressed" << SDL_GetScancodeName(key) << endl;
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
