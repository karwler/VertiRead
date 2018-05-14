#pragma once

#include "utils/settings.h"

// logic for editing playlists
class PlaylistEditor {
public:
	PlaylistEditor(const string& PLIST="", bool SS=true);

	const Playlist& getPlaylist() const { return pList; }
	void loadPlaylist(const string& playlist);

	bool add(const string& str);
	bool addSong(const string& path);
	bool addBook(const string& name);
	bool rename(const string& old, const string& str);
	void del(const string& name);

	bool showSongs;	// show song list or book list
private:
	Playlist pList;	// the thing we're working on

	bool isSong(const string& file);
	bool isBook(const string& name);
};
