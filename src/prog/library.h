#pragma once

#include "utils/types.h"

class Library {
public:
	Library(string FONT="", const map<string, string>& TEXS=map<string, string>(), const map<string, string>& SNDS=map<string, string>(), string LANG="");
	~Library();

	FontSet* Fonts() const;
	string getTexPath(string name) const;
	Texture* getTex(string name) const;
	string getSound(string name) const;
	string getLine(string name) const;

	void LoadLanguage(string language);

	vector<Texture*> Pictures();
	void LoadPics(const vector<string>& files);
	void ClearPics();

private:
	kptr<FontSet> fonts;
	map<string, Texture> texes;
	map<string, string> sounds;
	map<string, string> lines;
	vector<Texture> pics;
};
