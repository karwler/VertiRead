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
	Texture(string FILE="");

	string File() const;
	vec2i Res() const;
	void LoadTex(string path);

	SDL_Texture* tex;
private:
	string file;
	vec2i res;
};

struct Image {
	Image(vec2i POS=0, Texture* TEX=nullptr, vec2i SIZ=0);
	Image(vec2i POS, string TEX, vec2i SIZ=0);

	SDL_Rect getRect() const;

	vec2i pos, size;
	Texture* texture;
};

class FontSet {
public:
	FontSet(string FILE="");
	~FontSet();

	bool CanRun() const;
	TTF_Font* Get(int size);
	vec2i TextSize(string text, int size);

private:
	string file;
	map<int, TTF_Font*> fonts;

	void AddSize(int size);
};

struct Text {
	Text(string TXT="", vec2i POS=0, int H=50, int HSCAL=0, EColor CLR=EColor::text);

	vec2i pos;
	int height;
	EColor color;
	string text;

	vec2i size() const;
};

class TextEdit {
public:
	TextEdit(string TXT="", int CPOS=0);

	int CursorPos() const;
	void SetCursor(int pos);
	void MoveCursor(int mov, bool loop=false);

	string getText() const;
	void Add(string str);
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
	Playlist(string NAME="", const vector<fs::path>& SGS={}, const vector<string>& BKS={});

	string name;
	vector<fs::path> songs;
	vector<string> books;
};

struct Directory {
	Directory(string NAME="", const vector<string>& DIRS={}, vector<string> FILS={});

	string name;
	vector<string> dirs;
	vector<string> files;
};

struct GeneralSettings {
	GeneralSettings(string LANG="english", string LIB="", string PST="");

	string language;
	string dirLib;
	string dirPlist;

	string libraryParh() const;		// returns full path to dirLib
	string playlistParh() const;	// same as above
};

struct VideoSettings {
	VideoSettings(bool VS=true, bool MAX=false, bool FSC=false, vec2i RES=vec2i(800, 600), string FNT="", string RNDR="");

	bool vsync;
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
	ControlsSettings(vec2f SSP=vec2f(4.f, 8.f), bool fillMissingBindings=true, const map<string, Shortcut>& SRTCS=map<string, Shortcut>(), const map<string, SDL_Scancode>& HLDS=map<string, SDL_Scancode>());

	vec2f scrollSpeed;
	map<string, Shortcut> shortcuts;
	map<string, SDL_Scancode> holders;

	void FillMissingBindings();
	static Shortcut GetDefaultShortcut(string name);
	static SDL_Scancode GetDefaultHolder(string name);
};

struct Exception {
	Exception(string MSG="", int RV=-1);

	string message;
	int retval;

	void Display();
};
