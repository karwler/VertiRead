#include "library.h"
#include "engine/filer.h"

Library::Library(GeneralSettings& GSET) :
	curGSets(GSET)
{}

void Library::Initialize(string FONT) {
	if (!Filer::Exists(FONT))
		FONT = VideoSettings().Fontpath();	// should give the default font

	LoadFont(FONT);
	LoadLanguage(curGSets.Lang());
	LoadSounds();
	LoadTextures();
}

void Library::Close() {
	ClearPics();
	ClearTextures();
	ClearSounds();
	fonts.Clear();
}

FontSet* Library::Fonts() {
	return &fonts;
}

void Library::LoadFont(const string& font) {
	fonts.Clear();
	fonts.Initialize(font);
}

string Library::getLine(const string& line, ETextCase caseChange) const {
	return modifyCase((lines.count(line) == 0) ? line : lines.at(line), caseChange);
}

void Library::LoadLanguage(const string& language) {
	curGSets.Lang(language);
	lines = Filer::GetLines(language);
}

Mix_Chunk* Library::getSound(const string& name) {
	return (sounds.count(name) == 0) ? nullptr : sounds[name];
}

void Library::LoadSounds() {
	ClearSounds();
	sounds = Filer::GetSounds();
}

void Library::ClearSounds() {
	for (const pair<string, Mix_Chunk*>& it : sounds)
		Mix_FreeChunk(it.second);
	sounds.clear();
}

Texture* Library::getTex(const string& name) {
	return (texes.count(name) == 0) ? nullptr : &texes[name];
}

void Library::LoadTextures() {
	ClearTextures();
	texes = Filer::GetTextures();
}

void Library::ClearTextures() {
	for (map<string, Texture>::iterator it=texes.begin(); it!=texes.end(); it++)
		it->second.Clear();
	texes.clear();
}

vector<Texture*> Library::Pictures() {
	vector<Texture*> txs(pics.size());
	for (size_t i=0; i!=pics.size(); i++)
		txs[i] = &pics[i];
	return txs;
}

void Library::LoadPics(const vector<string>& files) {
	ClearPics();
	for (const string& it : files) {
		Texture tx(it);
		if (!tx.Res().hasNull())
			pics.push_back(tx);
		else if (tx.surface)
			SDL_FreeSurface(tx.surface);
	}
}

void Library::ClearPics() {
	for (Texture& it : pics)
		it.Clear();
	pics.clear();
}
