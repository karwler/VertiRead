#include "program.h"
#include "engine/world.h"

void Program::Event_Up() {
	if (dynamic_cast<ScrollArea*>(World::scene()->FocusedObject()))
		static_cast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(-30);
}

void Program::Event_Down() {
	if (dynamic_cast<ScrollArea*>(World::scene()->FocusedObject()))
		static_cast<ScrollArea*>(World::scene()->FocusedObject())->ScrollList(30);
}

void Program::Event_Left() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(-15);
}

void Program::Event_Right() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->ScrollListX(15);
}

void Program::Event_ZoomIn() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(0.2f);
}

void Program::Event_ZoomOut() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->AddZoom(-0.2f);
}

void Program::Event_ZoomReset() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->Zoom(1.f);
}

void Program::Event_CenterView() {
	if (dynamic_cast<ReaderBox*>(World::scene()->FocusedObject()))
		static_cast<ReaderBox*>(World::scene()->FocusedObject())->DragListX(0);
}

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
	World::audioSys()->setMusicVolume(World::audioSys()->Settings().musicVolume + 8);
}

void Program::Event_VolumeDown() {
	World::audioSys()->setMusicVolume(World::audioSys()->Settings().musicVolume - 8);
}

void Program::Event_ScreenMode() {
	World::winSys()->setFullscreen(!World::winSys()->Settings().fullscreen);
}

void Program::Event_OpenBookList() {
	curMenu = EMenu::books;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenBrowser(string path) {
	curMenu = EMenu::browser;
	if (browser)
		browser->GoTo(path);
	else
		browser = new Browser(path);
	World::scene()->SwitchMenu(curMenu, (void*) browser);
}

void Program::Event_OpenReader(string dir) {
	curMenu = EMenu::reader;
	World::scene()->SwitchMenu(curMenu, (void*)dir.c_str());
}

void Program::Event_OpenPlaylistList() {
	curMenu = EMenu::playlists;
	World::scene()->SwitchMenu(curMenu);
}

void Program::Event_OpenPlaylistEditor(string playlist) {
	curMenu = EMenu::plistEditor;
	editor = new PlaylistEditor(playlist);
	World::scene()->SwitchMenu(curMenu, (void*)editor);
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
		World::scene()->SwitchMenu(curMenu = EMenu::browser, (void*)browser);
	else if (curMenu == EMenu::browser) {
		if (browser->GoUp())
			World::scene()->SwitchMenu(curMenu, (void*)browser);
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

EMenu Program::CurrentMenu() const {
	return curMenu;
}
