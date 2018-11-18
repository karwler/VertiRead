#pragma once

#include "utils.h"

enum class Color : uint8 {
	background,
	normal,
	dark,
	light,
	select,
	text,
	texture,
	numColors
};

class Direction {
public:
	enum Dir : uint8 {
		up,
		down,
		left,
		right
	};

	Direction(Dir direction = Direction::up) :
		dir(direction)
	{}

	operator Dir() const { return dir; }

	bool vertical() const { return dir <= 1; }
	bool horizontal() const { return dir >= 2; }
	bool positive() const { return dir % 2; }
	bool negative() const { return !positive(); }

	string toString() const { return enumToStr(Default::directionNames, dir); }
	void set(const string& str) { dir = strToEnum<Dir>(Default::directionNames, str); }

private:
	Dir dir;
};

class Binding {
public:
	enum class Type : uint8 {
		enter,
		escape,
		up,
		down,
		left,
		right,
		scrollUp,
		scrollDown,
		scrollLeft,
		scrollRight,
		cursorUp,
		cursorDown,
		cursorLeft,
		cursorRight,
		centerView,
		scrollFast,
		scrollSlow,
		nextPage,
		prevPage,
		zoomIn,
		zoomOut,
		zoomReset,
		toStart,
		toEnd,
		nextDir,
		prevDir,
		fullscreen,
		refresh,
		numBindings
	};

	enum Assignment : uint8 {
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

	Binding();
	void setDefaultSelf(Type type);

	SDL_Scancode getKey() const { return key; }
	bool keyAssigned() const { return asg & ASG_KEY; }
	void clearAsgKey();
	void setKey(SDL_Scancode kkey);

	uint8 getJctID() const { return jctID; }
	bool jctAssigned() const;
	void clearAsgJct();

	bool jbuttonAssigned() const { return asg & ASG_JBUTTON; }
	void setJbutton(uint8 but);
	
	bool jaxisAssigned() const { return asg & (ASG_JAXIS_P | ASG_JAXIS_N); }
	bool jposAxisAssigned() const { return asg & ASG_JAXIS_P; }
	bool jnegAxisAssigned() const { return asg & ASG_JAXIS_N; }
	void setJaxis(uint8 axis, bool positive);

	uint8 getJhatVal() const { return jHatVal; }
	bool jhatAssigned() const { return asg & ASG_JHAT; }
	void setJhat(uint8 hat, uint8 val);

	uint8 getGctID() const { return gctID; }
	bool gctAssigned() const { return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N); }
	void clearAsgGct();

	SDL_GameControllerButton getGbutton() const { return static_cast<SDL_GameControllerButton>(gctID); }
	bool gbuttonAssigned() const { return asg & ASG_GBUTTON; }
	void setGbutton(SDL_GameControllerButton but);

	SDL_GameControllerAxis getGaxis() const { return static_cast<SDL_GameControllerAxis>(gctID); }
	bool gaxisAssigned() const { return asg & (ASG_GAXIS_P | ASG_GAXIS_N); }
	bool gposAxisAssigned() const { return asg & ASG_GAXIS_P; }
	bool gnegAxisAssigned() const { return asg & ASG_GAXIS_N; }
	void setGaxis(SDL_GameControllerAxis axis, bool positive);
	
	bool isAxis() const { return callAxis; }
	SBCall getBcall() const { return bcall; }
	void setBcall(SBCall call);
	SACall getAcall() const { return acall; }
	void setAcall(SACall call);

private:
	SDL_Scancode key;	// keybord key
	uint8 jctID;		// joystick control id
	uint8 jHatVal;		// joystick hat value
	uint8 gctID;		// gamepad control id
	Assignment asg;		// stores data for checking whether key and/or button/axis are assigned

	bool callAxis;
	union {
		SBCall bcall;
		SACall acall;
	};
};
inline Binding::Assignment operator~(Binding::Assignment a) { return static_cast<Binding::Assignment>(~static_cast<uint8>(a)); }
inline Binding::Assignment operator&(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline Binding::Assignment operator&=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline Binding::Assignment operator^(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline Binding::Assignment operator^=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline Binding::Assignment operator|(Binding::Assignment a, Binding::Assignment b) { return static_cast<Binding::Assignment>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
inline Binding::Assignment operator|=(Binding::Assignment& a, Binding::Assignment b) { return a = static_cast<Binding::Assignment>(static_cast<uint8>(a) | static_cast<uint8>(b)); }

class Settings {
public:
	Settings(bool maximized = Default::maximized, bool fullscreen = Default::fullscreen, const vec2i& resolution = Default::resolution, const Direction& direction = Direction::down, float zoom = Default::zoom, int spacing = Default::spacing, const string& theme="", const string& font = Default::font, const string& language = Default::language, const string& library = "", const string& renderer = "", const vec2f& speed = Default::scrollSpeed, int16 deadzone = Default::controllerDeadzone);

	const string& getTheme() const { return theme; }
	const string& setTheme(const string& name);
	const string& getFont() const { return font; }
	string setFont(const string& newFont);			// returns path to the font file, not the name
	const string& getLang() const { return lang; }
	const string& setLang(const string& language);
	const string& getDirLib() const { return dirLib; }
	const string& setDirLib(const string& drc);

	int getRendererIndex();
	static vector<string> getAvailibleRenderers();
	static string getRendererName(int id);

	string getResolutionString() const;
	string getScrollSpeedString() const;
	int getDeadzone() const { return deadzone; }
	void setDeadzone(int zone);

	bool maximized, fullscreen;
	Direction direction;
	float zoom;
	int spacing;
	vec2i resolution;
	string renderer;
	vec2f scrollSpeed;
private:
	int deadzone;
	string theme;
	string font;
	string lang;
	string dirLib;
};
