#include "library.h"
#include "engine/filer.h"

Library::Library(string FONT, const map<string, string>& TEXS, const map<string, string>& SNDS)
{
	if (!fs::exists(FONT))
		FONT = VideoSettings().font;	// should give the default font
	fonts = new FontSet(FONT);

	for (const pair<string, string>& it : TEXS)
		texes.insert(make_pair(it.first, Texture(it.second)));
	for (const pair<string, string>& it : SNDS)
		sounds.insert(it);
}

Library::~Library() {
	for (const pair<string, Texture>& it : texes)
		SDL_DestroyTexture(it.second.tex);
}

FontSet* Library::Fonts() const {
	return fonts;
}

Texture* Library::getTex(string name) const {
	return cCast<Texture*>(&texes.at(name));
}

string* Library::getSound(string name) const {
	return cCast<string*>(&sounds.at(name));
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
		else if (tx.tex)
			SDL_DestroyTexture(tx.tex);
	}
}

void Library::ClearPics() {
	for (Texture& it : pics)
		SDL_DestroyTexture(it.tex);
	pics.clear();
}
