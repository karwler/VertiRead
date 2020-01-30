#pragma once

#include "utils.h"

enum class Color : uint8 {
	background,
	normal,
	dark,
	light,
	select,
	text,
	texture
};

class Direction {
public:
	enum Dir : uint8 {
		up,
		down,
		left,
		right
	};
	static constexpr array<const char*, right+1> names = {
		"Up",
		"Down",
		"Left",
		"Right"
	};

private:
	Dir dir;

public:
	constexpr Direction(Dir direction);

	constexpr operator Dir() const;

	constexpr bool vertical() const;
	constexpr bool horizontal() const;
	constexpr bool positive() const;
	constexpr bool negative() const;
};

inline constexpr Direction::Direction(Dir direction) :
	dir(direction)
{}

inline constexpr Direction::operator Dir() const {
	return dir;
}

inline constexpr bool Direction::vertical() const {
	return dir <= down;
}

inline constexpr bool Direction::horizontal() const {
	return dir >= left;
}

inline constexpr bool Direction::positive() const {
	return dir & 1;
}

inline constexpr bool Direction::negative() const {
	return !positive();
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
		centerView,
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
		hide,
		boss,
		refresh,
		scrollUp,	// axis (hold down) bindings start here
		scrollDown,
		scrollLeft,
		scrollRight,
		cursorUp,
		cursorDown,
		cursorLeft,
		cursorRight,
		scrollFast,
		scrollSlow
	};
	static constexpr array<const char*, sizet(Type::scrollSlow)+1> names = {
		"enter",
		"escape",
		"up",
		"down",
		"left",
		"right",
		"center view",
		"next page",
		"prev page",
		"zoom in",
		"zoom out",
		"zoom reset",
		"to start",
		"to end",
		"next directory",
		"prev directory",
		"fullscreen",
		"show hidden",
		"boss",
		"refresh",
		"scroll up",
		"scroll down",
		"scroll left",
		"scroll right",
		"cursor up",
		"cursor down",
		"cursor left",
		"cursor right",
		"scroll fast",
		"scroll slow"
	};

	enum Assignment : uint8 {
		ASG_NONE	= 0x00,
		ASG_KEY     = 0x01,
		ASG_JBUTTON = 0x02,
		ASG_JHAT    = 0x04,
		ASG_JAXIS_P = 0x08,	// use only positive values
		ASG_JAXIS_N = 0x10,	// use only negative values
		ASG_GBUTTON = 0x20,
		ASG_GAXIS_P = 0x40,
		ASG_GAXIS_N = 0x80
	};

	union {
		SBCall bcall;
		SACall acall;
	};
private:
	SDL_Scancode key;	// keybord key
	uint8 jctID;		// joystick control id
	uint8 jHatVal;		// joystick hat value
	uint8 gctID;		// gamepad control id
	Assignment asg;		// stores data for checking whether key and/or button/axis are assigned
	Type type;

public:
	Binding();

	void reset(Type newType);
	bool isHolder() const;
	uint8 canRepeat() const;

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
};

inline Binding::Binding() :
	asg(ASG_NONE)
{}

inline bool Binding::isHolder() const {
	return type >= Type::scrollUp;
}

inline uint8 Binding::canRepeat() const {
	return type >= Type::up && type <= Type::right;
}

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

class PicLim {
public:
	enum class Type : uint8 {
		none,
		count,
		size
	};
	static constexpr array<const char*, sizet(Type::size)+1> names = {
		"none",
		"count",
		"size"
	};

	Type type;
private:
	uptrt count, size;	// size in bytes

	static constexpr uptrt defaultCount = 128;

public:
	PicLim(Type type = Type::none, uptrt count = defaultCount);

	uptrt getCount() const;
	void setCount(const string& str);
	uptrt getSize() const;
	void setSize(const string& str);
	void set(const string& str);

private:
	static uptrt toCount(const string& str);
	static uptrt toSize(const string& str);
	static uptrt defaultSize();
};

inline uptrt PicLim::getCount() const {
	return count;
}

inline void PicLim::setCount(const string& str) {
	count = toCount(str);
}

inline uptrt PicLim::getSize() const {
	return size;
}

inline void PicLim::setSize(const string& str) {
	size = toSize(str);
}

inline uptrt PicLim::defaultSize() {
	return uptrt(uint(SDL_GetSystemRAM()) / 2) * 1'000'000;
}

class Settings {
public:
	static constexpr float defaultZoom = 1.f;
	static constexpr int defaultSpacing = 10;
	static constexpr int axisLimit = SHRT_MAX;

	bool maximized, fullscreen;
	bool showHidden;
	Direction direction;
	PicLim picLim;
	float zoom;
	int spacing;
	vec2i resolution;
	string renderer;
	vec2f scrollSpeed;
private:
	int deadzone;
	string theme;
	string font;
	string dirLib;

	static constexpr char defaultFont[] = "BrisaSans";
	static constexpr char defaultDirLib[] = "library";

public:
	Settings();

	const string& getTheme() const;
	const string& setTheme(const string& name);
	const string& getFont() const;
	const string& setFont(const string& newFont);
	const string& getDirLib() const;
	const string& setDirLib(const string& drc);

	int getRendererIndex();
	string scrollSpeedString() const;
	int getDeadzone() const;
	void setDeadzone(int val);
};

inline const string& Settings::getTheme() const {
	return theme;
}

inline const string& Settings::getFont() const {
	return font;
}

inline const string& Settings::getDirLib() const {
	return dirLib;
}

inline string Settings::scrollSpeedString() const {
	return trimZero(to_string(scrollSpeed.x)) + ' ' + trimZero(to_string(scrollSpeed.y));
}

inline int Settings::getDeadzone() const {
	return deadzone;
}

inline void Settings::setDeadzone(int val) {
	deadzone = std::clamp(val, 0, axisLimit);
}
