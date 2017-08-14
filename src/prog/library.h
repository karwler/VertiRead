#pragma once

#include "utils/settings.h"

// handles the assets' data and contains general settings
class Library {
public:
	Library(const GeneralSettings& SETS=GeneralSettings());

	void init(string FONT);
	void close();

	FontSet& getFonts();
	void loadFont(const string& font);

	string line(const string& line, ETextCase caseChange=ETextCase::first_upper) const;
	void loadLanguage(const string& language);

	Mix_Chunk* sound(const string& name);
	void loadSounds();
	void clearSounds();

	Texture* texture(const string& name);
	void loadTextures();
	void clearTextures();

	vector<Texture*> getPictures();
	void loadPics(const vector<string>& files);
	void clearPics();

	const GeneralSettings& getSettings() const;
	void setLibraryPath(const string& dir);
	void setPlaylistsPath(const string& dir);

private:
	GeneralSettings sets;

	FontSet fonts;					// current font data)
	map<string, string> lines;		// english, translated
	map<string, Mix_Chunk*> sounds;	// name, sound data
	map<string, Texture> texes;		// name, texture data
	vector<Texture> pics;			// pictures for ReaderBox
};
