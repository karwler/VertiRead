#include "library.h"
#include "engine/filer.h"

Library::Library(const GeneralSettings& SETS, string FONT) :
	sets(SETS)
{
	Filer::checkDirectories(sets);

	if (!Filer::fileExists(FONT))
		FONT = VideoSettings().getFontpath();	// should give the default font
	fonts.init(FONT);

	lines = Filer::getLines(SETS.getLang());
	sounds = Filer::getSounds();
}

void Library::clear() {
	for (map<string, Texture>::iterator it=texes.begin(); it!=texes.end(); it++)
		it->second.clear();
	texes.clear();

	for (const pair<string, Mix_Chunk*>& it : sounds)
		Mix_FreeChunk(it.second);
	sounds.clear();

	fonts.clear();
}

FontSet& Library::getFonts() {
	return fonts;
}

void Library::loadLanguage(const string& lang) {
	sets.setLang(lang);
	lines = Filer::getLines(lang);
}

string Library::line(const string& line, ETextCase caseChange) const {
	return modifyCase((lines.count(line) == 0) ? line : lines.at(line), caseChange);
}

Mix_Chunk* Library::sound(const string& name) {
	return (sounds.count(name) == 0) ? nullptr : sounds[name];
}

Texture* Library::texture(const string& name) {
	if (texes.count(name) == 0) {
		Texture tex(Filer::dirTexs + name);
		if (!tex.tex)
			return nullptr;
		texes.insert(make_pair(name, tex));
	}
	return &texes[name];
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
