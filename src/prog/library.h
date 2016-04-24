#pragma once

#include "utils/types.h"

class Library {
public:
	Library(string FONT="", const map<string, string>& TEXS={}, const map<string, string>& SNDS={});
	~Library();

	FontSet* Fonts() const;
	string getTexPath(string name) const;
	Texture* getTex(string name) const;
	string getSound(string name) const;

	vector<Texture*> Pictures();
	void LoadPics(const vector<string>& files);
	void ClearPics();

private:
	kptr<FontSet> fonts;
	map<string, Texture> texes;
	map<string, string> sounds;
	vector<Texture> pics;
};
