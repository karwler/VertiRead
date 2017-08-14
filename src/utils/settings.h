#pragma once

#include "types.h"
#include "prog/defaults.h"

class GeneralSettings {
public:
	GeneralSettings(const string& LANG=Default::language, const string& LIB="", const string& PST="");

	string getLang() const;
	void setLang(const string& language);

	string getDirLib() const;
	string libraryPath() const;		// returns full path to dirLib
	void setDirLib(const string& dir);

	string getDirPlist() const;
	string playlistPath() const;	// same as above
	void setDirPlist(const string& dir);

private:
	string lang;	// this one has to be all lower case
	string dirLib;
	string dirPlist;
};

class VideoSettings {
public:
	VideoSettings(bool MAX=Default::maximized, bool FSC=Default::fullscreen, const vec2i& RES=Default::resolution, const string& FNT=Default::font, const string& RNDR="");

	string getFont() const;
	string getFontpath() const;
	void setFont(const string& newFont);
	
	void setDefaultTheme();
	static map<EColor, vec4c> getDefaultColors();
	static vec4c getDefaultColor(EColor color);

	int getRenderDriverIndex();

	bool maximized, fullscreen;
	vec2i resolution;
	string renderer;
	string theme;
	map<EColor, vec4c> colors;

private:
	string font;
	string fontpath;	// stores absolute path to font
};

struct AudioSettings {
	AudioSettings(int MV=Default::volumeMusic, int SV=Default::volumeSound, float SD=Default::songDelay);

	int musicVolume;
	int soundVolume;
	float songDelay;
};

struct ControlsSettings {
	ControlsSettings(const vec2f& SSP=Default::scrollSpeed, int16 DDZ=Default::controllerDeadzone, bool fillMissing=true, const map<string, Shortcut*>& CTS=map<string, Shortcut*>());

	vec2f scrollSpeed;
	int16 deadzone;
	map<string, Shortcut*> shortcuts;

	void fillMissingBindings();
	static Shortcut* getDefaultShortcut(const string& name);	// returns nullptr if none found
};
