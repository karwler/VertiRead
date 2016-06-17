#pragma once

#include "utils.h"

// enums and flags

enum class EColor : byte {
	background,
	rectangle,
	highlighted,
	darkened,
	text
};

enum class ETextType : byte {
	text,
	integer,
	floating
};

enum class ETextCase : byte {
	no_change,
	first_upper,
	all_upper,
	all_lower
};

enum class EClick : byte {
	left,
	left_double,
	right
};

enum EDirFilter : byte {
	FILTER_ALL  = 0,
	FILTER_FILE = 1,
	FILTER_DIR  = 2,
	FILTER_LINK = 4
};
EDirFilter operator|(EDirFilter a, EDirFilter b);

// image related stuff

class Texture {
public:
	Texture(const string& FILE="");
	Texture(const string& FILE, SDL_Surface* SURF);

	string File() const;
	vec2i Res() const;
	void LoadSurface(const string& path);

	SDL_Surface* surface;
private:
	string file;
};

struct Image {
	Image(const vec2i& POS=0, Texture* TEX=nullptr, const vec2i& SIZ=0);
	Image(const vec2i& POS, const string& TEX, const vec2i& SIZ=0);

	SDL_Rect getRect() const;

	vec2i pos, size;
	Texture* texture;
};

// text related stuff

class FontSet {
public:
	FontSet(const string& FILE="");
	~FontSet();

	bool CanRun() const;
	TTF_Font* Get(int size);
	vec2i TextSize(const string& text, int size);

private:
	string file;
	map<int, TTF_Font*> fonts;

	void AddSize(int size);
};

struct Text {
	Text(const string& TXT="", const vec2i& POS=0, int H=50, int HSCAL=8, EColor CLR=EColor::text);

	vec2i pos;
	int height;
	EColor color;
	string text;

	vec2i size() const;
};

class TextEdit {
public:
	TextEdit(const string& TXT="", ETextType TYP=ETextType::text, int CPOS=0);

	int CursorPos() const;
	void SetCursor(int pos);
	void MoveCursor(int mov, bool loop=false);

	string Text() const;
	void Text(const string& str, bool resetCpos=true);
	void Add(const string& str);
	void Delete(bool current);

private:
	int cpos;
	ETextType type;
	string text;

	void CheckCaret();
	void CheckText();
};

// some random types

struct Shortcut {
	Shortcut(SDL_Scancode KEY=SDL_SCANCODE_RCTRL, void (Program::*CALL)()=nullptr);

	SDL_Scancode key;
	void (Program::*call)();
};

struct Playlist {
	Playlist(const string& NAME="", const vector<fs::path>& SGS= {}, const vector<string>& BKS= {});

	string name;
	vector<fs::path> songs;
	vector<string> books;

	string songPath(uint id) const;	// returns song full path if original path is relative
};

struct Directory {
	Directory(const string& NAME="", const vector<string>& DIRS= {}, const vector<string>& FILS= {});

	string name;
	vector<string> dirs;
	vector<string> files;
};

struct Exception {
	Exception(const string& MSG="", int RV=-1);

	string message;
	int retval;

	void Display();
};

// settings structs

class GeneralSettings {
public:
	GeneralSettings(const string& LANG="", const string& LIB="", const string& PST="");

	string Lang() const;
	void Lang(const string& language);

	string DirLib() const;
	string LibraryParh() const;		// returns full path to dirLib
	void DirLib(const string& dir);

	string DirPlist() const;
	string PlaylistParh() const;	// same as above
	void DirPlist(const string& dir);

private:
	string lang;					// this one has to be all lower case
	string dirLib;
	string dirPlist;
};

class VideoSettings {
public:
	VideoSettings(bool MAX=false, bool FSC=false, const vec2i& RES=vec2i(800, 600), const string& FNT="Arial", const string& RNDR="");

	string Font() const;
	string Fontpath() const;
	void SetFont(const string& newFont);
	void SetDefaultColors();

	bool maximized, fullscreen;
	vec2i resolution;
	string renderer;
	map<EColor, vec4b> colors;

private:
	string font;
	string fontpath;	// stores absolute path to font
};

struct AudioSettings {
	AudioSettings(int MV=128, int SV=90, float SD=0.5f);

	int musicVolume;
	int soundVolume;
	float songDelay;
};

struct ControlsSettings {
	ControlsSettings(const vec2f& SSP=vec2f(4.f, 8.f), bool fillMissingBindings=true, const map<string, Shortcut>& SRTCS=map<string, Shortcut>(), const map<string, SDL_Scancode>& HLDS=map<string, SDL_Scancode>());

	vec2f scrollSpeed;
	map<string, Shortcut> shortcuts;
	map<string, SDL_Scancode> holders;

	void FillMissingBindings();
	static Shortcut GetDefaultShortcut(const string& name);
	static SDL_Scancode GetDefaultHolder(const string& name);
};
