#pragma once

#include "utils/types.h"

class PlaylistEditor {
public:
	PlaylistEditor(const string& PLIST="", bool SS=true);

	Playlist getPlaylist() const;
	void LoadPlaylist(const string& playlist);

	void AddSong(string path);
	void RenameSong(const string& path);
	void DelSong();
	void AddBook(const string& name);
	void RenameBook(const string& name);
	void DelBook();

	bool showSongs;
	btsel selected;
private:
	Playlist pList;
};
