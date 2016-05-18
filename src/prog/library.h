#pragma once

#include "utils/types.h"

class Library {
public:
	Library(string FONT="", const map<string, string>& TEXS=map<string, string>(), const map<string, string>& SNDS=map<string, string>());
	~Library();

	FontSet* Fonts() const;
	string getTexPath(const string& name) const;
	Texture* getTex(const string& name) const;
	string getSound(const string& name) const;

	vector<Texture*> Pictures();
	void LoadPics(const vector<string>& files);
	void ClearPics();

private:
	kptr<FontSet> fonts;
	map<string, Texture> texes;
	map<string, string> sounds;
	vector<Texture> pics;
};
