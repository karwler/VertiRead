#pragma once

#include "utils.h"

enum class Color : uint8 {
	background,
	normal,
	dark,
	light,
	select,
	tooltip,
	text,
	texture
};

enum class Alignment : uint8 {
	left,
	center,
	right
};

class Direction {
public:
	enum Dir : uint8 {
		up,
		down,
		left,
		right
	};
	static constexpr array<const char*, right + 1> names = {
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

constexpr Direction::Direction(Dir direction) :
	dir(direction)
{}

constexpr Direction::operator Dir() const {
	return dir;
}

constexpr bool Direction::vertical() const {
	return dir <= down;
}

constexpr bool Direction::horizontal() const {
	return dir >= left;
}

constexpr bool Direction::positive() const {
	return dir & 1;
}

constexpr bool Direction::negative() const {
	return !positive();
}

class Binding {
public:
	enum class Type : uint8 {
		up,
		down,
		left,
		right,
		enter,
		escape,
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
	static constexpr array<const char*, sizet(Type::scrollSlow) + 1> names = {
		"up",
		"down",
		"left",
		"right",
		"enter",
		"escape",
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
	static constexpr Type holders = Type::scrollUp;

	static inline const umap<uint8, const char*> hatNames = {
		pair(SDL_HAT_CENTERED, "Center"),
		pair(SDL_HAT_UP, "Up"),
		pair(SDL_HAT_RIGHT, "Right"),
		pair(SDL_HAT_DOWN, "Down"),
		pair(SDL_HAT_LEFT, "Left"),
		pair(SDL_HAT_RIGHTUP, "Right-Up"),
		pair(SDL_HAT_RIGHTDOWN, "Right-Down"),
		pair(SDL_HAT_LEFTDOWN, "Left-Down"),
		pair(SDL_HAT_LEFTUP, "Left-Up")
	};
	static constexpr array<const char*, SDL_CONTROLLER_BUTTON_MAX> gbuttonNames = {
		"A",
		"B",
		"X",
		"Y",
		"Back",
		"Guide",
		"Start",
		"LS",
		"RS",
		"LB",
		"RB",
		"Up",
		"Down",
		"Left",
		"Right"
	};
	static constexpr array<const char*, SDL_CONTROLLER_AXIS_MAX> gaxisNames = {
		"LX",
		"LY",
		"RX",
		"RY",
		"LT",
		"RT"
	};

	union {
		SBCall bcall;
		SACall acall;
	};
private:
	SDL_Scancode key;	// keyboard key
	uint8 jctID;		// joystick control id
	uint8 jHatVal;		// joystick hat value
	uint8 gctID;		// gamepad control id
	Assignment asg;		// stores data for checking whether key and/or button/axis are assigned
	Type type;

public:
	Binding();

	void reset(Type newType);

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
	static constexpr array<const char*, sizet(Type::size) + 1> names = {
		"none",
		"count",
		"size"
	};

	static constexpr array<uptrt, 4> sizeFactors = {
		1,
		1'000,
		1'000'000,
		1'000'000'000
	};
	static constexpr array<char, 4> sizeLetters = {
		'B',
		'K',
		'M',
		'G'
	};

	Type type;
private:
	uptrt count, size;	// size in bytes

	static constexpr uptrt defaultCount = 128;

public:
	PicLim(Type ltype = Type::none, uptrt cnt = defaultCount);

	uptrt getCount() const;
	void setCount(string_view str);
	uptrt getSize() const;
	void setSize(string_view str);
	void set(string_view str);

	static sizet memSizeMag(uptrt num);
	static string memoryString(uptrt num, sizet mag);
	static string memoryString(uptrt num);

private:
	static uptrt toCount(string_view str);
	static uptrt toSize(string_view str);
	static uptrt defaultSize();
};

inline uptrt PicLim::getCount() const {
	return count;
}

inline void PicLim::setCount(string_view str) {
	count = toCount(str);
}

inline uptrt PicLim::getSize() const {
	return size;
}

inline void PicLim::setSize(string_view str) {
	size = toSize(str);
}

inline uptrt PicLim::defaultSize() {
	return SDL_GetSystemRAM() / 2 * 1'000'000;
}

inline string PicLim::memoryString(uptrt num) {
	return memoryString(num, memSizeMag(num));
}

class Settings {
public:
	static constexpr float defaultZoom = 1.f;
	static constexpr int defaultSpacing = 10;
	static constexpr int axisLimit = SHRT_MAX + 1;
	static constexpr char defaultFont[] = "BrisaSans";
	static constexpr char defaultDirLib[] = "library";

	static constexpr array<SDL_Color, sizet(Color::texture) + 1> defaultColors = {
		SDL_Color{ 10, 10, 10, 255 },		// background
		SDL_Color{ 90, 90, 90, 255 },		// normal
		SDL_Color{ 60, 60, 60, 255 },		// dark
		SDL_Color{ 120, 120, 120, 255 },	// light
		SDL_Color{ 105, 105, 105, 255 },	// select
		SDL_Color{ 75, 75, 75, 255 },		// tooltip
		SDL_Color{ 210, 210, 210, 255 },	// text
		SDL_Color{ 210, 210, 210, 255 }		// texture
	};
	static constexpr array<const char*, defaultColors.size()> colorNames = {
		"background",
		"normal",
		"dark",
		"light",
		"select",
		"tooltip",
		"text",
		"texture"
	};

	bool maximized = false;
	bool fullscreen = false;
	bool showHidden = false;
	Direction direction = Direction::down;
	PicLim picLim;
	float zoom = defaultZoom;
	int spacing = defaultSpacing;
	ivec2 resolution = { 800, 600 };
	string renderer;
	string font = defaultFont;
	vec2 scrollSpeed = { 1600.f, 1600.f };
private:
	int deadzone = 256;
	string theme;
	fs::path dirLib;

public:
	Settings(const fs::path& dirSets, vector<string>&& themes);

	const string& getTheme() const;
	const string& setTheme(string_view name, vector<string>&& themes);
	const fs::path& getDirLib() const;
	const fs::path& setDirLib(const fs::path& drc, const fs::path& dirSets);

	pair<int, uint32> getRendererInfo();
	string scrollSpeedString() const;
	int getDeadzone() const;
	void setDeadzone(int val);
};

inline const string& Settings::getTheme() const {
	return theme;
}

inline const fs::path& Settings::getDirLib() const {
	return dirLib;
}

inline string Settings::scrollSpeedString() const {
	return toStr(scrollSpeed.x) + ' ' + toStr(scrollSpeed.y);
}

inline int Settings::getDeadzone() const {
	return deadzone;
}

inline void Settings::setDeadzone(int val) {
	deadzone = std::clamp(val, 0, axisLimit);
}
