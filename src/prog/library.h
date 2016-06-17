#pragma once

#include "utils/types.h"

class Library {
public:
	Library(string FONT, GeneralSettings& GSET);
	~Library();

	FontSet* Fonts() const;
	void LoadFont(const string& font);

	string getLine(const string& line, ETextCase caseChange=ETextCase::first_upper) const;
	void LoadLanguage(const string& language);

	Mix_Chunk* getSound(const string& name) const;
	void LoadSounds();
	void ClearSounds();

	Texture* getTex(const string& name);
	void LoadTextures();
	void ClearTextures();

	vector<Texture*> Pictures();
	void LoadPics(const vector<string>& files);
	void ClearPics();

private:
	kptr<FontSet> fonts;
	GeneralSettings& curGSets;
	map<string, string> lines;		// english, translated
	map<string, Mix_Chunk*> sounds;	// name, path
	map<string, Texture> texes;		// name, texture data
	vector<Texture> pics;
};
