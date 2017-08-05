#pragma once

#include "browser.h"
#include "playlistEditor.h"
#include "utils/items.h"
#include "utils/objects.h"
#include "kklib/sptr.h"

class Program {
public:
	// reader events
	void Event_Up(float amt);
	void Event_Down(float amt);
	void Event_Right(float amt);
	void Event_Left(float amt);
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
	void Event_AddPlaylistButtonClick();
	void Event_DeletePlaylistButtonClick();
	void Event_EditPlaylistButtonClick();
	void Event_NewPlaylistOk();
	void Event_NewPlaylistOk(const string& str);
	void Event_SwitchButtonClick();
	void Event_BrowseButtonClick();
	void Event_AddSongBookButtonClick();
	void Event_AddSongFileDirButtonClick();
	void Event_DeleteSongBookButtonClick();
	void Event_EditSongBookButtonClick();
	void Event_SongBookRenameOk();
	void Event_SongBookRenameOk(const string& str);
	void Event_ItemDoubleclicked(ListItem* item);
	void Event_SaveButtonClick();
	void Event_UpButtonClick();
	void Event_AddBook(void* name);

	// menu events
	void Event_OpenBookList();
	void Event_OpenBrowser(void* path);
	void Event_OpenReader(void* file);
	void Event_OpenPlaylistList();
	void Event_OpenPlaylistEditor(void* playlist);
	void Event_OpenSongBrowser(const string& dir);
	void Event_OpenGeneralSettings();
	void Event_OpenVideoSettings();
	void Event_OpenAudioSettings();
	void Event_OpenControlsSettings();
	void Event_Ok();
	void Event_Back();

	// settings events
	void Event_SwitchLanguage(const string& language);
	void Event_SetLibraryPath(const string& dir);
	void Event_SetPlaylistsPath(const string& dir);
	void Event_SwitchFullscreen(bool on);
	void Event_SetTheme(const string& theme);
	void Event_SetFont(const string& font);
	void Event_SetRenderer(const string& renderer);
	void Event_SetMusicVolume(const string& mvol);
	void Event_SetSoundVolume(const string& svol);
	void Event_SetSongDelay(const string& sdelay);
	void Event_SetScrollX(const string& scrollx);
	void Event_SetScrollY(const string& scrolly);
	void Event_SetDeadzone(const string& deadz);

	// other events
	void Event_SelectionSet(void* box);
	void Event_ScreenMode();
	void FileDropEvent(char* file);
	
private:
	enum class EMenu : uint8 {
		books,
		browser,
		reader,
		playlists,
		plistEditor,
		songSearch,
		bookSearch,
		generalSets,
		videoSets,
		audioSets,
		controlsSets
	} curMenu;
	kk::sptr<Browser> browser;
	kk::sptr<PlaylistEditor> editor;

	void SwitchScene(EMenu newMenu, void* dat=nullptr);
	void SwitchScene(void* dat=nullptr) const;
	float ModifySpeed(float value, float* axisFactor=nullptr, float normalFactor=1.f, float fastFactor=4.f, float slowFactor=0.5f);
	Playlist FindFittingPlaylist(const string& picPath) const;

	Popup* CreatePopupMessage(const string& msg);
	Popup* CreatePopupChoice(const string& msg, void (Program::*callb)());
	pair<Popup*, LineEditor*> CreatePopupText(const string& msg, const string& text, void (Program::*callt)(const string&), void (Program::*callb)());
};
