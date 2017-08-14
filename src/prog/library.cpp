#include "library.h"
#include "engine/filer.h"

Library::Library(const GeneralSettings& SETS) :
	sets(SETS)
{
	Filer::checkDirectories(sets);
}

void Library::init(string FONT) {
	if (!Filer::fileExists(FONT))
		FONT = VideoSettings().getFontpath();	// should give the default font

	loadFont(FONT);
	loadLanguage(sets.getLang());
	loadSounds();
	loadTextures();
}

void Library::close() {
	clearPics();
	clearTextures();
	clearSounds();
	fonts.clear();
}

FontSet& Library::getFonts() {
	return fonts;
}

void Library::loadFont(const string& font) {
	fonts.clear();
	fonts.Initialize(font);
}

string Library::line(const string& line, ETextCase caseChange) const {
	return modifyCase((lines.count(line) == 0) ? line : lines.at(line), caseChange);
}

void Library::loadLanguage(const string& language) {
	sets.setLang(language);
	lines = Filer::getLines(language);
}

Mix_Chunk* Library::sound(const string& name) {
	return (sounds.count(name) == 0) ? nullptr : sounds[name];
}

void Library::loadSounds() {
	clearSounds();
	sounds = Filer::getSounds();
}

void Library::clearSounds() {
	for (const pair<string, Mix_Chunk*>& it : sounds)
		Mix_FreeChunk(it.second);
	sounds.clear();
}

Texture* Library::texture(const string& name) {
	return (texes.count(name) == 0) ? nullptr : &texes[name];
}

void Library::loadTextures() {
	clearTextures();
	texes = Filer::getTextures();
}

void Library::clearTextures() {
	for (map<string, Texture>::iterator it=texes.begin(); it!=texes.end(); it++)
		it->second.clear();
	texes.clear();
}

vector<Texture*> Library::getPictures() {
	vector<Texture*> txs(pics.size());
	for (size_t i=0; i!=pics.size(); i++)
		txs[i] = &pics[i];
	return txs;
}

void Library::loadPics(const vector<string>& files) {
	clearPics();
	for (const string& it : files) {
		Texture tx(it);
		if (!tx.resolution().hasNull())
			pics.push_back(tx);
		else if (tx.surface)
			SDL_FreeSurface(tx.surface);
	}
}

void Library::clearPics() {
	for (Texture& it : pics)
		it.clear();
	pics.clear();
}

const GeneralSettings& Library::getSettings() const {
	return sets;
}

void Library::setLibraryPath(const string& dir) {
	sets.setDirLib(dir);
}

void Library::setPlaylistsPath(const string& dir) {
	sets.setDirPlist(dir);
}
