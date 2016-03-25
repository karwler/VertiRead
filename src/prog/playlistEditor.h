#pragma once

#include "utils/types.h"

class PlaylistEditor {
public:
	PlaylistEditor(string PLIST="", bool SS=true);

	void LoadPlaylist(string playlist);
	Playlist GetPlaylist() const;

	bool showSongs;
private:
	Playlist pList;
	int selected;
};
