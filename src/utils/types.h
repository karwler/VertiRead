#pragma once

#include "utils.h"

class Engine;
class AudioSys;
class InputSys;
class WinSys;

class Scene;
class Program;

class Button;
class ScrollArea;
class PopupChoice;
class Capturer;

enum class EColor : byte {
	background,
	rectangle,
	highlighted,
	darkened,
	text
};

class Texture {
public:
	Texture(const string& FILE="");

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
	Text(const string& TXT="", const vec2i& POS=0, int H=50, int HSCAL=0, EColor CLR=EColor::text);

	vec2i pos;
	int height;
	EColor color;
	string text;

	vec2i size() const;
};

class TextEdit {
public:
	TextEdit(const string& TXT="", int CPOS=0);

	int CursorPos() const;
	void SetCursor(int pos);
	void MoveCursor(int mov, bool loop=false);

	string getText() const;
	void Add(const string& str);
	void Delete(bool current);

private:
	int cpos;
	string text;
};

struct Shortcut {
	Shortcut(SDL_Scancode KEY=SDL_SCANCODE_RCTRL, void (Program::*CALL)()=nullptr);

	SDL_Scancode key;
	void (Program::*call)();
};

struct Playlist {
	Playlist(const string& NAME="", const vector<fs::path>& SGS={}, const vector<string>& BKS={});

	string name;
	vector<fs::path> songs;
	vector<string> books;
};

struct Directory {
	Directory(const string& NAME="", const vector<string>& DIRS={}, const vector<string>& FILS={});

	string name;
	vector<string> dirs;
	vector<string> files;
};

struct GeneralSettings {
	GeneralSettings(const string& LIB="", const string& PST="");

	string dirLib;
	string dirPlist;

	string libraryParh() const;		// returns full path to dirLib
	string playlistParh() const;	// same as above
};

struct VideoSettings {
	VideoSettings(bool MAX=false, bool FSC=false, const vec2i& RES=vec2i(800, 600), const string& FNT="", const string& RNDR="");

	bool maximized, fullscreen;
	vec2i resolution;
	string font;
	string renderer;
	map<EColor, vec4b> colors;

	string FontPath() const;	// returns absolute font path
	void SetDefaultColors();
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

struct Exception {
	Exception(const string& MSG="", int RV=-1);

	string message;
	int retval;

	void Display();
};
