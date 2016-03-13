#include "playlistEditor.h"
#include "engine/world.h"

PlaylistEditor::PlaylistEditor(string PLIST, bool SS) :
	showSongs(SS)
{
	LoadPlaylist(PLIST);
}

void PlaylistEditor::LoadPlaylist(string playlist) {
	pList = Filer::LoadPlaylist(playlist);
	selected = -1;
}

Playlist PlaylistEditor::getPlaylist() const {
	return pList;
}

