#pragma once

#include "browser.h"
#include "playlistEditor.h"
#include "utils/widgets.h"
#include "kklib/sptr.h"

// handles the frontend
class Program {
public:
	// reader events
	void eventUp(float amt);
	void eventDown(float amt);
	void eventRight(float amt);
	void eventLeft(float amt);
	void eventPageUp();
	void eventPageDown();
	void eventZoomIn();
	void eventZoomOut();
	void eventZoomReset();
	void eventCenterView();
	void eventNextDir();
	void eventPrevDir();

	// player events
	void eventPlayPause();
	void eventNextSong();
	void eventPrevSong();
	void eventVolumeUp();
	void eventVolumeDown();
	void eventMute();

	// playlist editor events
	void eventAddPlaylistButtonClick();
	void eventDeletePlaylistButtonClick();
	void eventEditPlaylistButtonClick();
	void eventNewPlaylistOk();
	void eventNewPlaylistOk(const string& str);
	void eventSwitchButtonClick();
	void eventBrowseButtonClick();
	void eventAddSongBookButtonClick();
	void eventAddSongFileDirButtonClick();
	void eventDeleteSongBookButtonClick();
	void eventEditSongBookButtonClick();
	void eventSongBookRenameOk();
	void eventSongBookRenameOk(const string& str);
	void eventItemDoubleclicked(ListItem* item);
	void eventSaveButtonClick();
	void eventUpButtonClick();
	void eventAddBook(void* name);

	// menu events
	void eventOpenBookList();
	void eventOpenBrowser(void* path);
	void eventOpenReader(void* file);
	void eventOpenLastPage(void* book);
	void eventOpenPlaylistList();
	void eventOpenPlaylistEditor(void* playlist);
	void eventOpenSongBrowser(const string& dir);
	void eventOpenGeneralSettings();
	void eventOpenVideoSettings();
	void eventOpenAudioSettings();
	void eventOpenControlsSettings();
	void eventOk();
	void eventBack();

	// settings events
	void eventSwitchLanguage(const string& language);
	void eventSetLibraryPath(const string& dir);
	void eventSetPlaylistsPath(const string& dir);
	void eventSwitchFullscreen(bool on);
	void eventSetTheme(const string& theme);
	void eventSetFont(const string& font);
	void eventSetRenderer(const string& renderer);
	void eventSetMusicVolume(const string& mvol);
	void eventSetSoundVolume(const string& svol);
	void eventSetSongDelay(const string& sdelay);
	void eventSetScrollX(const string& scrollx);
	void eventSetScrollY(const string& scrolly);
	void eventSetDeadzone(const string& deadz);

	// other events
	void eventSelectionSet(void* box);
	void eventScreenMode();
	void eventFileDrop(char* file);
	
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

	void switchScene(EMenu newMenu, void* dat=nullptr);
	void switchScene(void* dat=nullptr) const;
	float modifySpeed(float value, float* axisFactor=nullptr, float normalFactor=1.f, float fastFactor=4.f, float slowFactor=0.5f);	// change scroll speed depending on pressed shortcuts
	vector<string> findFittingPlaylist(const string& book) const;	// find playlists for the book by using a picture file's path

	Popup* createPopupMessage(const string& msg);
	Popup* createPopupChoice(const string& msg, void (Program::*callb)());
	pair<Popup*, LineEditor*> createPopupText(const string& msg, const string& text, void (Program::*callt)(const string&), void (Program::*callb)());	// second return value is a pointer to the widget/item that is supposed to be captured (can be nullptr)
};
