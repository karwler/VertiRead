#include "playlistEditor.h"
#include "engine/filer.h"

PlaylistEditor::PlaylistEditor(const string& PLIST, bool SS) :
	showSongs(SS)
{
	LoadPlaylist(PLIST);	// if playlist couldn't be loaded, a blank playlist will be set up
}

Playlist PlaylistEditor::getPlaylist() const {
	return pList;
}

void PlaylistEditor::LoadPlaylist(const string& playlist) {
	pList = Filer::LoadPlaylist(playlist);
	selected = -1;
}

void PlaylistEditor::AddSong(const string& path) {
	if (fs::is_directory(path)) {
		for (fs::path& it : Filer::ListDirRecursively(path, FILTER_FILE))
			pList.songs.push_back(it.string());
	}
	else
		pList.songs.push_back(path);

	selected = pList.songs.size()-1;
}

void PlaylistEditor::RenameSong(const string& path) {
	pList.songs[selected] = path;
}

void PlaylistEditor::DelSong() {
	if (selected != -1) {
		pList.songs.erase(pList.songs.begin()+selected);
		selected = -1;
	}
}

void PlaylistEditor::AddBook(const string& name) {
	pList.books.push_back(name);
	selected = pList.books.size()-1;
}

void PlaylistEditor::RenameBook(const string& name) {
	pList.books[selected] = name;
}

void PlaylistEditor::DelBook() {
	if (selected != -1) {
		pList.books.erase(pList.books.begin()+selected);
		selected = -1;
	}
}

