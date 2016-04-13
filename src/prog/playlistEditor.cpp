#include "playlistEditor.h"
#include "engine/filer.h"

PlaylistEditor::PlaylistEditor(string PLIST, bool SS) :
	showSongs(SS)
{
	LoadPlaylist(PLIST);	// if playlist couldn't be loaded,a blank playlist will be set up
}

Playlist PlaylistEditor::getPlaylist() const {
	return pList;
}

void PlaylistEditor::LoadPlaylist(string playlist) {
	pList = Filer::LoadPlaylist(playlist);
	selected = -1;
}

void PlaylistEditor::Rename(string name) {
	pList.name = name;
}

void PlaylistEditor::AddSong(string path) {
	pList.songs.push_back(path);
	selected = pList.songs.size()-1;
}

void PlaylistEditor::DelSong() {
	if (selected >= pList.songs.size())	// just in case
		selected = -1;
	else {
		pList.songs.erase(pList.songs.begin()+selected);
		selected = -1;
	}
}

void PlaylistEditor::AddBook(string name) {
	pList.books.push_back(name);
	selected = pList.books.size()-1;
}

void PlaylistEditor::DelBook() {
	if (selected >= pList.books.size())
		selected = -1;
	else {
		pList.books.erase(pList.books.begin()+selected);
		selected = -1;
	}
}

