#pragma once

#include "utils.h"

class Scene;
class Program;
class Button;

typedef void (Program::*progEFunc)();

enum class EMenu : byte {
	books,
	browser,
	reader,
	playlists,
	plistEditor,
	generalSets,
	videoSets,
	audioSets,
	controlsSets
};

enum class EColor : byte {
	background,
	rectangle,
	highlighted,
	darkened,
	text
};

struct Image {
	Image(vec2i POS=vec2i(), vec2i SIZ=vec2i(), string TEXN="");

	vec2i pos, size;
	string texname;
};

struct Text {
	Text(string TXT = "", vec2i POS=vec2i(), int SIZE=72, EColor CLR = EColor::text);

	int size;
	vec2i pos;
	EColor color;
	string text;
};

class Shortcut {
public:
	Shortcut(string NAME="", bool setDefaultKey=true, const vector<SDL_Keysym>& KEYS=vector<SDL_Keysym>());

	string Name() const;
	bool SetName(string sname, bool setDefaultKey=true);
	progEFunc Call() const;

	vector<SDL_Keysym> keys;

private:
	string name;
	void (Program::*call)();
};

struct Playlist {
	Playlist(string NAME = "", const vector<fs::path>& SGS = vector<fs::path>(), const vector<string>& BKS = vector<string>());

	string name;
	vector<fs::path> songs;
	vector<string> books;
};

struct Directory {
	Directory(string NAME = "", const vector<string>& DIRS = vector<string>(), vector<string> FILS = vector<string>());

	string name;
	vector<string> dirs;
	vector<string> files;
};

struct GeneralSettings {
	GeneralSettings();
};

struct VideoSettings {
	VideoSettings(bool VS=true, bool MAX=false, bool FSC=false, vec2i RES = vec2i(800, 600), string FONT="", string RNDR="");

	bool vsync;
	bool maximized, fullscreen;
	vec2i resolution;
	string font;
	string renderer;
	map<EColor, vec4b> colors;

	void SetDefaultColors();
};

struct AudioSettings {
	AudioSettings(int MV=128, int SV=90, float SD=0.5f);

	int musicVolume;
	int soundVolume;
	float songDelay;
};

struct ControlsSettings {
	ControlsSettings(bool fillMissingBindings=false, const vector<Shortcut>& SRTCS = vector<Shortcut>());

	vector<Shortcut> shortcuts;

	void FillMissingBindings();
	Shortcut* shortcut(string name);
};
