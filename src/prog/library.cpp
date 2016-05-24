#include "library.h"
#include "engine/filer.h"

Library::Library(string FONT, const map<string, string>& TEXS, const map<string, string>& SNDS)
{
	if (!fs::exists(FONT))
		FONT = VideoSettings().FontPath();	// should give the default font
	fonts = new FontSet(FONT);

	for (const pair<string, string>& it : TEXS) {
		SDL_Surface* test = IMG_Load(it.second.c_str());	// add only valid textures
		if (test) {
			texes.insert(make_pair(it.first, Texture(it.second)));
			SDL_FreeSurface(test);
		}
	}

	for (const pair<string, string>& it : SNDS) {
		Mix_Chunk* test = Mix_LoadWAV(it.second.c_str());	// add only valid sound files
		if (test) {
			sounds.insert(it);
			Mix_FreeChunk(test);
		}
	}
}

Library::~Library() {
	for (const pair<string, Texture>& it : texes)
		SDL_FreeSurface(it.second.surface);
}

FontSet* Library::Fonts() const {
	return fonts;
}

string Library::getTexPath(const string& name) const {
	return texes.at(name).File();
}

Texture* Library::getTex(const string& name) const {
	return const_cast<Texture*>(&texes.at(name));
}

string Library::getSound(const string& name) const {
	return sounds.at(name);
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
