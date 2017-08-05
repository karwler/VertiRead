#pragma once

#include "types.h"
#include "prog/defaults.h"

class GeneralSettings {
public:
	GeneralSettings(const string& LANG=Default::language, const string& LIB="", const string& PST="");

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
	VideoSettings(bool MAX=false, bool FSC=false, const vec2i& RES=Default::resolution, const string& FNT=Default::font, const string& RNDR="");

	string Font() const;
	string Fontpath() const;
	void SetFont(const string& newFont);
	void SetDefaultTheme();
	
	static map<EColor, vec4c> GetDefaultColors();
	static vec4c GetDefaultColor(EColor color);

	int GetRenderDriverIndex();

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
	ControlsSettings(const vec2f& SSP=Default::scrollSpeed, int16 DDZ=Default::controllerDeadzone, bool fillMissingBindings=true, const map<string, Shortcut*>& CTS=map<string, Shortcut*>());

	vec2f scrollSpeed;
	int16 deadzone;
	map<string, Shortcut*> shortcuts;

	void FillMissingBindings();
	static Shortcut* GetDefaultShortcut(const string& name);	// returns nullptr if none found
};
