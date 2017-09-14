#pragma once

#include "utils/settings.h"

// handles the assets' data and contains general settings
class Library {
public:
	Library(const GeneralSettings& SETS=GeneralSettings(), string FONT="");

	void clear();

	FontSet& getFonts();

	void loadLanguage(const string& lang);
	string line(const string& line, ETextCase caseChange=ETextCase::first_upper) const;

	Mix_Chunk* sound(const string& name);
	Texture* texture(const string& name);

	const GeneralSettings& getSettings() const;
	void setLibraryPath(const string& dir);
	void setPlaylistsPath(const string& dir);

private:
	GeneralSettings sets;

	FontSet fonts;					// current font data
	map<string, string> lines;		// english, translated
	map<string, Mix_Chunk*> sounds;	// name, sound data
	map<string, Texture> texes;		// name, texture data
};
