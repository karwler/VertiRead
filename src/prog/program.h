#pragma once

#include "browser.h"
#include "playlistEditor.h"

class Program {
public:
	// reader events
	void Event_Up();
	void Event_Down();
	void Event_Left();
	void Event_Right();
	void Event_PageUp();
	void Event_PageDown();
	void Event_ZoomIn();
	void Event_ZoomOut();
	void Event_ZoomReset();
	void Event_CenterView();
	void Event_NextDir();
	void Event_PrevDir();

	// player events
	void Event_PlayPause();
	void Event_NextSong();
	void Event_PrevSong();
	void Event_VolumeUp();
	void Event_VolumeDown();
	void Event_Mute();

	// playlist editor events
	void Event_AddButtonClick();
	void Event_DeleteButtonClick();
	void Event_EditButtonClick();
	void Event_SaveButtonClick();

	// menu events
	void Event_OpenBookList();
	void Event_OpenBrowser(void* path);
	void Event_OpenReader(void* file);
	void Event_OpenPlaylistList();
	void Event_OpenPlaylistEditor(void* playlist);
	void Event_OpenGeneralSettings();
	void Event_OpenVideoSettings();
	void Event_OpenAudioSettings();
	void Event_OpenControlsSettings();
	void Event_Back();

	// other events
	void Event_SelectionSet(void* box);
	void Event_ScreenMode();

	EMenu CurrentMenu() const;
	
private:
	EMenu curMenu;
	kptr<Browser> browser;
	kptr<PlaylistEditor> editor;
};
