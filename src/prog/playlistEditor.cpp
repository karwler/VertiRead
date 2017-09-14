#include "playlistEditor.h"
#include "engine/filer.h"

PlaylistEditor::PlaylistEditor(const string& PLIST, bool SS) :
	showSongs(SS)
{
	loadPlaylist(PLIST);	// if playlist couldn't be loaded, a blank playlist will be set up
}

const Playlist& PlaylistEditor::getPlaylist() const {
	return pList;
}

void PlaylistEditor::loadPlaylist(const string& playlist) {
	pList = Filer::getPlaylist(playlist);
	selected = false;
}

bool PlaylistEditor::getShowSongs() const {
	return showSongs;
}

void PlaylistEditor::setShowSongs(bool show) {
	showSongs = show;
	selected = false;
}

void PlaylistEditor::addSong(string path) {
	if (Filer::fileType(path) == FTYPE_DIR) {
		path = appendDsep(path);
		for (string& it : Filer::listDirRecursively(path))
			pList.songs.push_back(path+it);
	} else
		pList.songs.push_back(path);

	selected = pList.songs.size()-1;
}

void PlaylistEditor::renameSong(const string& path) {
	pList.songs[selected.id] = path;
}

void PlaylistEditor::delSong() {
	if (selected.sl) {
		pList.songs.erase(pList.songs.begin()+selected.id);
		selected = false;
	}
}

void PlaylistEditor::addBook(const string& name) {
	pList.books.push_back(name);
	selected = pList.books.size()-1;
}

void PlaylistEditor::renameBook(const string& name) {
	pList.books[selected.id] = name;
}

void PlaylistEditor::delBook() {
	if (selected.sl) {
		pList.books.erase(pList.books.begin()+selected.id);
		selected = false;
	}
}

