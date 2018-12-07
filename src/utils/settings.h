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

	Direction(Dir direction = Direction::up);

	operator Dir() const;

	bool vertical() const;
	bool horizontal() const;
	bool positive() const;
	bool negative() const;

	string toString() const;
	void set(const string& str);

private:
	Dir dir;
};

inline Direction::Direction(Dir direction) :
	dir(direction)
{}

inline Direction::operator Dir() const {
	return dir;
}

inline bool Direction::vertical() const {
	return dir <= 1;
}

inline bool Direction::horizontal() const {
	return dir >= 2;
}

inline bool Direction::positive() const {
	return dir % 2;
}

inline bool Direction::negative() const {
	return !positive();
}

inline string Direction::toString() const {
	return enumToStr(Default::directionNames, dir);
}

inline void Direction::set(const string& str) {
	dir = strToEnum<Dir>(Default::directionNames, str);
}

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
		refresh
	};

	enum Assignment {
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

	SDL_Scancode getKey() const;
	bool keyAssigned() const;
	void clearAsgKey();
	void setKey(SDL_Scancode kkey);

	uint8 getJctID() const;
	bool jctAssigned() const;
	void clearAsgJct();

	bool jbuttonAssigned() const;
	void setJbutton(uint8 but);
	
	bool jaxisAssigned() const;
	bool jposAxisAssigned() const;
	bool jnegAxisAssigned() const;
	void setJaxis(uint8 axis, bool positive);

	uint8 getJhatVal() const;
	bool jhatAssigned() const;
	void setJhat(uint8 hat, uint8 val);

	uint8 getGctID() const;
	bool gctAssigned() const;
	void clearAsgGct();

	SDL_GameControllerButton getGbutton() const;
	bool gbuttonAssigned() const;
	void setGbutton(SDL_GameControllerButton but);

	SDL_GameControllerAxis getGaxis() const;
	bool gaxisAssigned() const;
	bool gposAxisAssigned() const;
	bool gnegAxisAssigned() const;
	void setGaxis(SDL_GameControllerAxis axis, bool positive);
	
	bool isAxis() const;
	SBCall getBcall() const;
	void setBcall(SBCall call);
	SACall getAcall() const;
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

inline Binding::Assignment operator~(Binding::Assignment a) {
	return Binding::Assignment(~uint8(a));
}

inline Binding::Assignment operator&(Binding::Assignment a, Binding::Assignment b) {
	return Binding::Assignment(uint8(a) & uint8(b));
}

inline Binding::Assignment operator&=(Binding::Assignment& a, Binding::Assignment b) {
	return a = Binding::Assignment(uint8(a) & uint8(b));
}

inline Binding::Assignment operator^(Binding::Assignment a, Binding::Assignment b) {
	return Binding::Assignment(uint8(a) ^ uint8(b));
}

inline Binding::Assignment operator^=(Binding::Assignment& a, Binding::Assignment b) {
	return a = Binding::Assignment(uint8(a) ^ uint8(b));
}

inline Binding::Assignment operator|(Binding::Assignment a, Binding::Assignment b) {
	return Binding::Assignment(uint8(a) | uint8(b));
}

inline Binding::Assignment operator|=(Binding::Assignment& a, Binding::Assignment b) {
	return a = Binding::Assignment(uint8(a) | uint8(b));
}

inline Binding::Binding() :
	asg(ASG_NONE)
{}

inline SDL_Scancode Binding::getKey() const {
	return key;
}

inline bool Binding::keyAssigned() const {
	return asg & ASG_KEY;
}

inline void Binding::clearAsgKey() {
	asg &= ~ASG_KEY;
}

inline uint8 Binding::getJctID() const {
	return jctID;
}

inline bool Binding::jctAssigned() const {
	return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

inline void Binding::clearAsgJct() {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

inline bool Binding::jbuttonAssigned() const {
	return asg & ASG_JBUTTON;
}

inline bool Binding::jaxisAssigned() const {
	return asg & (ASG_JAXIS_P | ASG_JAXIS_N);
}

inline bool Binding::jposAxisAssigned() const {
	return asg & ASG_JAXIS_P;
}

inline bool Binding::jnegAxisAssigned() const {
	return asg & ASG_JAXIS_N;
}

inline uint8 Binding::getJhatVal() const {
	return jHatVal;
}

inline bool Binding::jhatAssigned() const {
	return asg & ASG_JHAT;
}

inline uint8 Binding::getGctID() const {
	return gctID;
}

inline bool Binding::gctAssigned() const {
	return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

inline void Binding::clearAsgGct() {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

inline SDL_GameControllerButton Binding::getGbutton() const {
	return SDL_GameControllerButton(gctID);
}

inline bool Binding::gbuttonAssigned() const {
	return asg & ASG_GBUTTON;
}

inline SDL_GameControllerAxis Binding::getGaxis() const {
	return SDL_GameControllerAxis(gctID);
}

inline bool Binding::gaxisAssigned() const {
	return asg & (ASG_GAXIS_P | ASG_GAXIS_N);
}

inline bool Binding::gposAxisAssigned() const {
	return asg & ASG_GAXIS_P;
}

inline bool Binding::gnegAxisAssigned() const {
	return asg & ASG_GAXIS_N;
}

inline bool Binding::isAxis() const {
	return callAxis;
}

inline SBCall Binding::getBcall() const {
	return bcall;
}

inline SACall Binding::getAcall() const {
	return acall;
}

class Settings {
public:
	Settings(bool maximized = Default::maximized, bool fullscreen = Default::fullscreen, const vec2i& resolution = Default::resolution, const Direction& direction = Direction::down, float zoom = Default::zoom, int spacing = Default::spacing, const string& theme="", const string& font = Default::font, const string& language = Default::language, const string& library = "", const string& renderer = "", const vec2f& speed = Default::scrollSpeed, int16 deadzone = Default::controllerDeadzone);

	const string& getTheme() const;
	const string& setTheme(const string& name);
	const string& getFont() const;
	string setFont(const string& newFont);			// returns path to the font file, not the name
	const string& getLang() const;
	const string& setLang(const string& language);
	const string& getDirLib() const;
	const string& setDirLib(const string& drc);

	int getRendererIndex();
	string getResolutionString() const;
	string getScrollSpeedString() const;
	int getDeadzone() const;
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

inline const string& Settings::getTheme() const {
	return theme;
}

inline const string& Settings::getFont() const {
	return font;
}

inline const string& Settings::getLang() const {
	return lang;
}

inline const string& Settings::getDirLib() const {
	return dirLib;
}

inline int Settings::getDeadzone() const {
	return deadzone;
}

inline string Settings::getResolutionString() const {
	return to_string(resolution.x) + ' ' + to_string(resolution.y);
}

inline string Settings::getScrollSpeedString() const {
	return trimZero(to_string(scrollSpeed.x)) + ' ' + trimZero(to_string(scrollSpeed.y));
}
