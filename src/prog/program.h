#pragma once

#include "browser.h"
#include "playlistEditor.h"
#include "player.h"
#include "progs.h"

// handles the frontend
class Program {
public:
	Program();

	// books
	void eventOpenBookList(Button* but=nullptr);
	void eventOpenBrowser(Button* but);
	void eventBrowserGoUp(Button* but=nullptr);
	void eventBrowserGoIn(Button* but);
	void eventOpenReader(Button* but);
	void eventOpenLastPage(Button* but);

	// reader
	void eventZoomIn(Button* but=nullptr);
	void eventZoomOut(Button* but=nullptr);
	void eventZoomReset(Button* but=nullptr);
	void eventCenterView(Button* but=nullptr);
	void eventNextDir(Button* but=nullptr);
	void eventPrevDir(Button* but=nullptr);
	void eventPlayPause(Button* but=nullptr);
	void eventNextSong(Button* but=nullptr);
	void eventPrevSong(Button* but=nullptr);
	void eventVolumeUp(Button* but=nullptr);
	void eventVolumeDown(Button* but=nullptr);
	void eventMute(Button* but=nullptr);
	void eventExitReader(Button* but=nullptr);

	// playlists
	void eventOpenPlaylistList(Button* but=nullptr);
	void eventAddPlaylist(Button* but=nullptr);
	void eventAddPlaylistOk(Button* but=nullptr);
	void eventEditPlaylist(Button* but=nullptr);
	void eventRenamePlaylist(Button* but=nullptr);
	void eventRenamePlaylistOk(Button* but=nullptr);
	void eventDeletePlaylist(Button* but=nullptr);
	void eventDeletePlaylistOk(Button* but=nullptr);
	void eventExitPlaylistEditor(Button* but=nullptr);

	void eventSwitchSB(Button* but=nullptr);
	void eventBrowseSB(Button* but=nullptr);
	void eventAddSB(Button* but=nullptr);
	void eventAddSBOk(Button* but=nullptr);
	void eventEditSB(Button* but=nullptr);
	void eventEditSBOk(Button* but=nullptr);
	void eventDeleteSB(Button* but=nullptr);
	void eventSavePlaylist(Button* but=nullptr);

	void eventAddSongFD(Button* but=nullptr);
	void eventSongBrowserGoUp(Button* but=nullptr);
	void eventExitSongBrowser(Button* but=nullptr);
	void eventAddBook(Button* but);
	void eventExitBookBrowser(Button* but=nullptr);

	// settings
	void eventOpenSettings(Button* but=nullptr);
	void eventSwitchLanguage(Button* but);
	void eventSetLibraryPath(Button* but);
	void eventSetPlaylistsPath(Button* but);
	void eventSetVolumeSL(Button* but);
	void eventSetVolumeLE(Button* but);
	void eventSwitchFullscreen(Button* but);
	void eventSetTheme(Button* but);
	void eventSetFont(Button* but);
	void eventSetRenderer(Button* but);
	void eventSetScrollSpeed(Button* but);
	void eventSetDeadzoneSL(Button* but);
	void eventSetDeadzoneLE(Button* but);

	// other
	void eventClosePopup(Button* but=nullptr);
	void eventExit(Button* but=nullptr);
	
	ProgState* getState() { return state.get(); }
	Browser* getBrowser() { return browser.get(); }
	PlaylistEditor* getEditor() { return editor.get(); }
	Player* getPlayer() { return player.get(); }

private:
	uptr<ProgState> state;
	uptr<Browser> browser;
	uptr<PlaylistEditor> editor;
	uptr<Player> player;

	void setState(ProgState* newState);
	bool startReader(const string& picname);
};
