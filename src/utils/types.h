#pragma once

#include "utils.h"

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

enum class EPopupType : uint8 {
	message,
	choice,
	text
};

// image related stuff

// texture surface data with path to the initial file
class Texture {
public:
	Texture();
	Texture(const string& FILE);

	void load(const string& path);
	void clear();

	vec2i getRes() const;
	string getFile() const;

	SDL_Texture* tex;
private:
	vec2i res;
	string file;
};

// texture with position and custom size
struct Image {
	Image(const vec2i& POS=0, Texture* TEX=nullptr, const vec2i& SIZ=0);

	SDL_Rect rect() const;

	vec2i pos, size;
	Texture* tex;
};

// text related stuff

// loads different font sizes from one family
class FontSet {
public:
	bool init(const string& FILE="");
	void clear();

	bool canRun() const;
	TTF_Font* getFont(int size);
	vec2i textSize(const string& text, int size);

private:
	string file;
	map<int, TTF_Font*> fonts;

	TTF_Font* addSize(int size);
};

// a string with position, size and color
struct Text {
	Text(const string& TXT="", const vec2i& POS=0, int HEI=50, EColor CLR=EColor::text);

	vec2i pos;
	int height;
	EColor color;
	string text;

	void setPosToRect(const SDL_Rect& rect, ETextAlign align);
	vec2i size() const;
};

// for editing text with caret
class TextEdit {
public:
	TextEdit(const string& TXT="", ETextType TYPE=ETextType::text, size_t CPOS=0);

	size_t getCaretPos() const;
	void setCaretPos(size_t pos);
	void moveCaret(bool right, bool loop=false);

	string getText() const;
	void setText(const string& str, bool resetCpos=true);
	void addText(const string& str);
	void delChar(bool current);

private:
	size_t cpos;	// caret position
	ETextType type;
	string text;

	void checkCaret();	// if caret is out of range, set it to max position
	void checkText();	// check if text is of the type specified. if not, remove not needed chars

	void cleanIntString(string& str);
	void cleanFloatString(string& str);
};

// input related stuff

struct ClickType {
	ClickType(uint8 BTN=0, uint8 NUM=0);

	uint8 button;
	uint8 clicks;
};

struct ClickStamp {
	ClickStamp(Widget* wgt=nullptr, uint8 BUT=0, const vec2i& POS=0);

	void reset();

	Widget* widget;
	uint8 button;
	vec2i mPos;
};

struct Controller {
	Controller();

	SDL_Joystick* joystick;			// for direct input
	SDL_GameController* gamepad;	// for xinput

	bool open(int id);
	void close();
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

	SDL_Scancode getKey() const;
	bool keyAssigned() const;
	void clearAsgKey();
	void setKey(SDL_Scancode KEY);

	uint8 getJctID() const;
	bool jctAssigned() const;
	void clearAsgJct();

	bool jbuttonAssigned() const;
	void setJbutton(uint8 BUT);

	bool jaxisAssigned() const;
	bool jposAxisAssigned() const;
	bool jnegAxisAssigned() const;
	void setJaxis(uint8 AXIS, bool positive);

	uint8 getJhatVal() const;
	bool jhatAssigned() const;
	void setJhat(uint8 HAT, uint8 VAL);

	uint8 getGctID() const;
	bool gctAssigned() const;
	void clearAsgGct();

	bool gbuttonAssigned() const;
	void gbutton(uint8 BUT);

	bool gaxisAssigned() const;
	bool gposAxisAssigned() const;
	bool gnegAxisAssigned() const;
	void setGaxis(uint8 AXIS, bool positive);

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

// some random types

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

struct IniLine {
	IniLine();
	IniLine(const string& ARG, const string& VAL);
	IniLine(const string& ARG, const string& KEY, const string& VAL);
	IniLine(const string& TIT);

	enum class Type : uint8 {
		av,		// argument, value, no key, not title
		akv,	// argument, key, value, no title
		title	// title, no everything else
	} type;
	string arg;	// argument, aka. the thing before the equal sign/brackets
	string key;	// the thing between the brackets (empty if there are no brackets)
	string val;	// value, aka. the thing after the equal sign

	string line() const;				// get the actual INI line
	void setVal(const string& ARG, const string& VAL);
	void setVal(const string& ARG, const string& KEY, const string& VAL);
	void setTitle(const string& TIT);
	bool setLine(const string& lin);	// returns false if not an INI line
	void clear();
};

struct Exception {
	Exception(const string& MSG="", int RV=-1);

	string message;
	int retval;

	void printMessage();
};

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
