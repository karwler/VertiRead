#pragma once

#include "utils/types.h"

// logic for editing playlists
class PlaylistEditor {
public:
	PlaylistEditor(const string& PLIST="", bool SS=true);

	const Playlist& getPlaylist() const;
	void loadPlaylist(const string& playlist);

	void addSong(string path);
	void renameSong(const string& path);
	void delSong();
	void addBook(const string& name);
	void renameBook(const string& name);
	void delBook();

	bool showSongs;	// show song list or book list
	btsel selected;	// currently selected item
private:
	Playlist pList;	// the thing we're working on
};
