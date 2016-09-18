#pragma once

#include "utils/settings.h"

class Library {
public:
	Library(GeneralSettings& GSET);

	void Initialize(string FONT);
	void Close();

	FontSet* Fonts();
	void LoadFont(const string& font);

	string getLine(const string& line, ETextCase caseChange=ETextCase::first_upper) const;
	void LoadLanguage(const string& language);

	Mix_Chunk* getSound(const string& name);
	void LoadSounds();
	void ClearSounds();

	Texture* getTex(const string& name);
	void LoadTextures();
	void ClearTextures();

	vector<Texture*> Pictures();
	void LoadPics(const vector<string>& files);
	void ClearPics();

private:
	GeneralSettings& curGSets;
	FontSet fonts;
	map<string, string> lines;		// english, translated
	map<string, Mix_Chunk*> sounds;	// name, path
	map<string, Texture> texes;		// name, texture data
	vector<Texture> pics;
};
