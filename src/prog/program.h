#pragma once

#include "browser.h"
#include "playlistEditor.h"

class Program {
public:
	void Event_Up();
	void Event_Down();
	void Event_Left();
	void Event_Right();
	void Event_ZoomIn();
	void Event_ZoomOut();
	void Event_ZoomReset();
	void Event_CenterView();
	void Event_PlayPause();
	void Event_NextSong();
	void Event_PrevSong();
	void Event_VolumeUp();
	void Event_VolumeDown();
	void Event_ScreenMode();
	void Event_OpenBookList();
	void Event_OpenBrowser(string path);
	void Event_OpenReader(string dir);
	void Event_OpenPlaylistList();
	void Event_OpenPlaylistEditor(string playlist);
	void Event_OpenGeneralSettings();
	void Event_OpenVideoSettings();
	void Event_OpenAudioSettings();
	void Event_OpenControlsSettings();
	void Event_Back();

	EMenu CurrentMenu() const;
	
private:
	EMenu curMenu;
	kptr<Browser> browser;
	kptr<PlaylistEditor> editor;
};
