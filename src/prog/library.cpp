#include "library.h"
#include "engine/filer.h"

Library::Library(string FONT, string& LANG) :
	curLanguage(LANG)
{
	if (!fs::exists(FONT))
		FONT = VideoSettings().FontPath();	// should give the default font
	if (!fs::exists(Filer::dirLangs()+curLanguage+".ini"))
		curLanguage = GeneralSettings().language;	// same as above
	
	LoadFont(FONT);
	LoadLanguage(curLanguage);
	LoadSounds();
	LoadTextures();
}

Library::~Library() {
	ClearPics();
	ClearTextures();
	ClearSounds();
}

FontSet* Library::Fonts() const {
	return fonts;
}

void Library::LoadFont(const string& font) {
	fonts = new FontSet(font);
}

string Library::getLine(const string& line, ETextCase caseChange) const {
	string str = (lines.count(line) == 0) ? line : lines.at(line);
	switch (caseChange) {
	case ETextCase::first_upper:
		if (!str.empty())
			str[0] = toupper(str[0]);
		break;
	case ETextCase::all_upper:
		std::transform(str.begin(), str.end(), str.begin(), toupper);
		break;
	case ETextCase::all_lower:
		std::transform(str.begin(), str.end(), str.begin(), tolower);
	}
	return str;
}

void Library::LoadLanguage(const string& language) {
	curLanguage = language;
	lines = Filer::GetLines(language);
}

Mix_Chunk* Library::getSound(const string& name) const {
	return sounds.at(name);
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
	return &texes.at(name);
}

void Library::LoadTextures() {
	ClearTextures();
	texes = Filer::GetTextures();
}

void Library::ClearTextures() {
	for (const pair<string, Texture>& it : texes)
		SDL_FreeSurface(it.second.surface);
	texes.clear();
}

vector<Texture*> Library::Pictures() {
	vector<Texture*> txs(pics.size());
	for (uint i=0; i!=pics.size(); i++)
		txs[i] = &pics[i];
	return txs;
}

void Library::LoadPics(const vector<string>& files) {
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
		SDL_FreeSurface(it.surface);
	pics.clear();
}
