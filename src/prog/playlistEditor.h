#pragma once

#include "utils/types.h"

class PlaylistEditor {
public:
	PlaylistEditor(string PLIST="", bool SS=true);

	Playlist getPlaylist() const;
	void LoadPlaylist(string playlist);

	void AddSong(string path="");
	void RenameSong(string path);
	void DelSong();
	void AddBook(string name="");
	void RenameBook(string name);
	void DelBook();

	bool showSongs;
	int selected;	// -1 means none selected
private:
	Playlist pList;
};
