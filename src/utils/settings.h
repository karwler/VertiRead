#pragma once

#include "types.h"
#include "prog/defaults.h"

class GeneralSettings {
public:
	GeneralSettings(const string& LANG=DEFAULT_LANGUAGE, const string& LIB="", const string& PST="");

	string Lang() const;
	void Lang(const string& language);

	string DirLib() const;
	string LibraryPath() const;		// returns full path to dirLib
	void DirLib(const string& dir);

	string DirPlist() const;
	string PlaylistPath() const;	// same as above
	void DirPlist(const string& dir);

private:
	string lang;					// this one has to be all lower case
	string dirLib;
	string dirPlist;
};

class VideoSettings {
public:
	VideoSettings(bool MAX=false, bool FSC=false, const vec2i& RES=DEFAULT_RESOLUTION, const string& FNT=DEFAULT_FONT, const string& RNDR="");

	string Font() const;
	string Fontpath() const;
	void SetFont(const string& newFont);
	void SetDefaultTheme();
	
	static map<EColor, vec4b> GetDefaultColors();
	static vec4b GetDefaultColor(EColor color);

	bool maximized, fullscreen;
	vec2i resolution;
	string renderer;
	string theme;
	map<EColor, vec4b> colors;

private:
	string font;
	string fontpath;	// stores absolute path to font
};

struct AudioSettings {
	AudioSettings(int MV=DEFAULT_MUSIC_VOL, int SV=DEFAULT_SOUND_VOL, float SD=DEFAULT_SONG_DELAY);

	int musicVolume;
	int soundVolume;
	float songDelay;
};

struct ControlsSettings {
	ControlsSettings(const vec2f& SSP=DEFAULT_SCROLL_SPEED, bool fillMissingBindings=true, const map<string, Shortcut*>& CTS=map<string, Shortcut*>());

	vec2f scrollSpeed;
	map<string, Shortcut*> shortcuts;

	void FillMissingBindings();
	static Shortcut* GetDefaultShortcut(const string& name);	// returns nullptr if none found
};
