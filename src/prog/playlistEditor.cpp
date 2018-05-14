#include "engine/world.h"

PlaylistEditor::PlaylistEditor(const string& PLIST, bool SS) :
	showSongs(SS)
{
	loadPlaylist(PLIST);	// if playlist couldn't be loaded, a blank playlist will be set up
}

void PlaylistEditor::loadPlaylist(const string& playlist) {
	pList = Filer::getPlaylist(playlist);
}

bool PlaylistEditor::add(const string& str) {
	return showSongs ? addSong(str) : addBook(str);
}

bool PlaylistEditor::addSong(const string& path) {
	if (Filer::fileType(path) == FTYPE_DIR) {
		for (string& it : Filer::listDirRecursively(appendDsep(path)))
			if (isSong(it))
				pList.songs.push_back(it);
		return true;
	} else if (isSong(path)) {
		pList.songs.push_back(path);
		return true;
	}
	return false;
}

bool PlaylistEditor::addBook(const string& name) {
	if (isBook(name)) {
		pList.books.insert(name);
		return true;
	}
	return false;
}

bool PlaylistEditor::rename(const string& old, const string& str) {
	if (showSongs) {
		if (isSong(str)) {
			*std::find(pList.songs.begin(), pList.songs.end(), old) = str;
			return true;
		}
	} else if (isBook(str)) {
		pList.books.erase(old);
		pList.books.insert(str);
		return true;
	}
	return false;
}

void PlaylistEditor::del(const string& name) {
	if (showSongs)
		pList.songs.erase(std::find(pList.songs.begin(), pList.songs.end(), name));
	else
		pList.books.erase(name);
}

bool PlaylistEditor::isSong(const string& file) {
	if (Mix_Music* snd = Mix_LoadMUS(file.c_str())) {
		Mix_FreeMusic(snd);
		return true;
	}
	return false;
}

bool PlaylistEditor::isBook(const string& name) {
	vector<string> books = Filer::listDir(World::winSys()->sets.getDirLib(), FTYPE_DIR);
	return std::find(books.begin(), books.end(), name) != books.end();
}
