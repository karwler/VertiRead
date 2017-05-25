#pragma once

#include "utils.h"
#include "kklib/vec2.h"
#include "kklib/vec3.h"
#include "kklib/vec4.h"

// enumerations

enum class EColor : uint8 {
	background,
	rectangle,
	highlighted,
	darkened,
	text
};

enum class ETextAlign : uint8 {
	left,
	center,
	right
};

enum class ETextType : uint8 {
	text,
	integer,
	floating
};

enum class ETextCase : uint8 {
	no_change,
	first_upper,
	all_upper,
	all_lower
};

enum class EClick : uint8 {
	left,
	left_double,
	right
};

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
	bool Initialize(const string& FILE="");
	void Clear();

	bool CanRun() const;
	TTF_Font* Get(int size);
	vec2i TextSize(const string& text, int size);

private:
	string file;
	map<int, TTF_Font*> fonts;

	TTF_Font* AddSize(int size);
};

struct Text {
	Text(const string& TXT="", const vec2i& POS=0, int H=50, EColor CLR=EColor::text, int HSCAL=8);

	vec2i pos;
	int height;
	EColor color;
	string text;

	void SetPosToRect(const SDL_Rect& rect, ETextAlign align, int offset=5);
	vec2i size() const;
};

class TextEdit {
public:
	TextEdit(const string& TXT="", ETextType TYPE=ETextType::text, size_t CPOS=0);

	size_t CursorPos() const;
	void SetCursor(size_t pos);
	void MoveCursor(bool right, bool loop=false);

	string Text() const;
	void Text(const string& str, bool resetCpos=true);
	void Add(const string& str);
	void Delete(bool current);

private:
	size_t cpos;
	ETextType type;
	string text;

	void CheckCaret();
	void CheckText();

	void CleanIntString(string& str);
	void CleanFloatString(string& str);
};

// some random types

template <typename T>
struct idsel {
	idsel() : id(0), sl(false) {}
	idsel(T ID) : id(ID), sl(true) {}

	T id;
	bool sl;

	idsel& operator=(T ID) {
		id = ID;
		sl = true;
		return *this;
	}
	idsel& operator=(bool SL) {
		sl = SL;
		return *this;
	}
};
using btsel = idsel<size_t>;

struct Controller {
	Controller();

	SDL_Joystick* joystick;
	SDL_GameController* gamepad;

	bool Open(int id);
	void Close();
};

class Shortcut {
public:
	enum EAssgnment : uint8 {
		ASG_NONE	= 0x0,
		ASG_KEY     = 0x1,
		ASG_JBUTTON = 0x2,
		ASG_JHAT    = 0x4,
		ASG_JAXIS_P = 0x8,	// use only positive values
		ASG_JAXIS_N = 0x10,	// use only negative values
		ASG_GBUTTON = 0x20,
		ASG_GAXIS_P = 0x40,
		ASG_GAXIS_N = 0x80
	};

	Shortcut();
	virtual ~Shortcut();

	SDL_Scancode Key() const;
	bool KeyAssigned() const;
	void ClearAsgKey();
	void Key(SDL_Scancode KEY);

	uint8 JctID() const;
	bool JctAssigned() const;
	void ClearAsgJct();

	bool JButtonAssigned() const;
	void JButton(uint8 BUT);

	bool JAxisAssigned() const;
	bool JPosAxisAssigned() const;
	bool JNegAxisAssigned() const;
	void JAxis(uint8 AXIS, bool positive);

	uint8 JHatVal() const;
	bool JHatAssigned() const;
	void JHat(uint8 HAT, uint8 VAL);

	uint8 GctID() const;
	bool GctAssigned() const;
	void ClearAsgGct();

	bool GButtonAssigned() const;
	void GButton(uint8 BUT);

	bool GAxisAssigned() const;
	bool GPosAxisAssigned() const;
	bool GNegAxisAssigned() const;
	void GAxis(uint8 AXIS, bool positive);


private:
	EAssgnment asg;		// stores data for checking whether key and/or button/axis are assigned
	SDL_Scancode key;	// keybord key
	uint8 jctID;		// joystick control ID
	uint8 jHatVal;		// joystick hat value
	uint8 gctID;		// gamepad control ID
};
Shortcut::EAssgnment operator~(Shortcut::EAssgnment a);
Shortcut::EAssgnment operator&(Shortcut::EAssgnment a, Shortcut::EAssgnment b);
Shortcut::EAssgnment operator&=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b);
Shortcut::EAssgnment operator^(Shortcut::EAssgnment a, Shortcut::EAssgnment b);
Shortcut::EAssgnment operator^=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b);
Shortcut::EAssgnment operator|(Shortcut::EAssgnment a, Shortcut::EAssgnment b);
Shortcut::EAssgnment operator|=(Shortcut::EAssgnment& a, Shortcut::EAssgnment b);

class ShortcutKey : public Shortcut {	// keys are handled by the event system
public:
	ShortcutKey(void (Program::*CALL)()=nullptr);
	virtual ~ShortcutKey();

	void (Program::*call)();
};

class ShortcutAxis : public Shortcut {	// axes are handled in the scene's tick function
public:
	ShortcutAxis(void (Program::*CALL)(float)=nullptr);
	virtual ~ShortcutAxis();

	void (Program::*call)(float);
};

struct Playlist {
	Playlist(const string& NAME="", const vector<string>& SGS={}, const vector<string>& BKS={});

	string name;
	vector<string> songs;
	vector<string> books;

	string songPath(size_t id) const;	// returns song's full path if original path is relative
};

struct Directory {
	Directory(const string& NAME="", const vector<string>& DIRS={}, const vector<string>& FILS= {});

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
