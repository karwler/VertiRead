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
		"up",
		"down",
		"left",
		"right"
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
		multiFullscreen,
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
	static constexpr array<const char*, size_t(Type::scrollSlow) + 1> names = {
		"up",
		"down",
		"left",
		"right",
		"enter",
		"escape",
		"center_view",
		"next_page",
		"prev_page",
		"zoom_in",
		"zoom_out",
		"zoom_reset",
		"to_start",
		"to_end",
		"next_directory",
		"prev_directory",
		"fullscreen",
		"multi_fullscreen",
		"show_hidden",
		"boss",
		"refresh",
		"scroll_up",
		"scroll_down",
		"scroll_left",
		"scroll_right",
		"cursor_up",
		"cursor_down",
		"cursor_left",
		"cursor_right",
		"scroll_fast",
		"scroll_slow"
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
	SDL_Scancode key;			// keyboard key
	uint8 jctID;				// joystick control id
	uint8 jHatVal;				// joystick hat value
	uint8 gctID;				// gamepad control id
	Assignment asg = ASG_NONE;	// stores data for checking whether key and/or button/axis are assigned
	Type type;

public:
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
	static constexpr array<const char*, size_t(Type::size) + 1> names = {
		"none",
		"count",
		"size"
	};

	static constexpr array<uintptr_t, 4> sizeFactors = {
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

	static constexpr uintptr_t defaultCount = 128;

private:
	uintptr_t count, size;	// size in bytes
public:
	Type type;

	PicLim(Type ltype = Type::none, uintptr_t cnt = defaultCount);

	uintptr_t getCount() const;
	void setCount(string_view str);
	uintptr_t getSize() const;
	void setSize(string_view str);
	void set(string_view str);

	static uint8 memSizeMag(uintptr_t num);
	static string memoryString(uintptr_t num, uint8 mag);
	static string memoryString(uintptr_t num);

private:
	static uintptr_t toCount(string_view str);
	static uintptr_t toSize(string_view str);
	static uintptr_t defaultSize();
};

inline uintptr_t PicLim::getCount() const {
	return count;
}

inline void PicLim::setCount(string_view str) {
	count = toCount(str);
}

inline uintptr_t PicLim::toCount(string_view str) {
	return coalesce(toNum<uintptr_t>(str), defaultCount);
}

inline uintptr_t PicLim::getSize() const {
	return size;
}

inline void PicLim::setSize(string_view str) {
	size = toSize(str);
}

inline uintptr_t PicLim::defaultSize() {
	return uintptr_t(SDL_GetSystemRAM() / 2) * 1'000'000;
}

inline string PicLim::memoryString(uintptr_t num) {
	return memoryString(num, memSizeMag(num));
}

class Settings {
public:
	static constexpr char flagLog[] = "l";
#ifndef _WIN32
	static constexpr char flagCompositor[] = "c";
#endif

	static constexpr array<vec4, size_t(Color::texture) + 1> defaultColors = {
		vec4(0.04f, 0.04f, 0.04f, 1.f),	// background
		vec4(0.35f, 0.35f, 0.35f, 1.f),	// normal
		vec4(0.24f, 0.24f, 0.24f, 1.f),	// dark
		vec4(0.47f, 0.47f, 0.47f, 1.f),	// light
		vec4(0.41f, 0.41f, 0.41f, 1.f),	// select
		vec4(0.29f, 0.29f, 0.29f, 1.f),	// tooltip
		vec4(0.82f, 0.82f, 0.82f, 1.f),	// text
		vec4(0.82f, 0.82f, 0.82f, 1.f)	// texture
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

	enum class Screen : uint8 {
		windowed,
		fullscreen,
		multiFullscreen
	};
	static constexpr array<const char*, size_t(Screen::multiFullscreen) + 1> screenModeNames = {
		"windowed",
		"fullscreen",
		"multi fullscreen"
	};

	enum class Renderer : uint8 {
#ifdef WITH_DIRECTX
		directx,
#endif
#ifdef WITH_OPENGL
		opengl,
#endif
#ifdef WITH_VULKAN
		vulkan,
#endif
		max
	};
	static constexpr array<const char*, size_t(Renderer::max)> rendererNames = {
#ifdef WITH_DIRECTX
		"DirectX 11",
#endif
#ifdef WITH_OPENGL
#ifdef OPENGLES
		"OpenGL ES 3.0",
#else
		"OpenGL 3.0",
#endif
#endif
#ifdef WITH_VULKAN
		"Vulkan 1.0"
#endif
	};

	static constexpr array<const char*, 5> styleNames = {
		"bold",
		"italic",
		"underline",
		"strikethrough",
		"normal"
	};

	enum class Hinting : uint8 {
		normal,
		mono
	};
	static constexpr array<const char*, size_t(Hinting::mono) + 1> hintingNames = {
		"normal",
		"mono"
	};

	enum class Compression : uint8 {
		none,
		b16,
		compress
	};
	static constexpr array<const char*, size_t(Compression::compress) + 1> compressionNames = {
		"none",
		"16 b",
		"compress"
	};

	static constexpr float defaultZoom = 1.f;
	static constexpr int defaultSpacing = 10;
	static constexpr uint minPicRes = 1;
	static constexpr int axisLimit = SHRT_MAX + 1;
	static constexpr Screen defaultScreenMode = Screen::windowed;
	static constexpr Direction::Dir defaultDirection = Direction::down;
#ifdef WITH_OPENGL
	static constexpr Renderer defaultRenderer = Renderer::opengl;
#elif defined(WITH_DIRECTX)
	static constexpr Renderer defaultRenderer = Renderer::directx;
#elif defined(WITH_VULKAN)
	static constexpr Renderer defaultRenderer = Renderer::vulkan;
#else
#error "No renderer supported"
#endif
	static constexpr char defaultFont[] = "BrisaSans";
	static constexpr Hinting defaultHinting = Hinting::normal;
	static constexpr Compression defaultCompression = Compression::none;
	static constexpr char defaultDirLib[] = "library";

	string font = defaultFont;
private:
	string theme;
	string dirLib;
public:
	umap<int, Recti> displays;
	PicLim picLim;
	u32vec2 device = u32vec2(0);
	ivec2 resolution = ivec2(800, 600);
	vec2 scrollSpeed = vec2(1600.f, 1600.f);
	uint maxPicRes = UINT_MAX;
	float zoom = defaultZoom;
private:
	int deadzone = 256;
public:
	ushort spacing = defaultSpacing;
	bool maximized = false;
	Screen screen = defaultScreenMode;
	bool preview = true;
	bool showHidden = false;
	bool tooltips = true;
	Direction direction = defaultDirection;
	Compression compression = defaultCompression;
	bool vsync = true;
	Renderer renderer = defaultRenderer;
	bool gpuSelecting = false;
	Hinting hinting = defaultHinting;

	Settings(const fs::path& dirSets, vector<string>&& themes);

	const string& getTheme() const;
	const string& setTheme(string_view name, vector<string>&& themes);
	const string& getDirLib() const;
	const string& setDirLib(string_view drc, const fs::path& dirSets);

	static umap<int, Recti> displayArrangement();
	void unionDisplays();
	string scrollSpeedString() const;
	int getDeadzone() const;
	void setDeadzone(int val);
};

inline const string& Settings::getTheme() const {
	return theme;
}

inline const string& Settings::getDirLib() const {
	return dirLib;
}

inline string Settings::scrollSpeedString() const {
	return toStr(scrollSpeed);
}

inline int Settings::getDeadzone() const {
	return deadzone;
}

inline void Settings::setDeadzone(int val) {
	deadzone = std::clamp(val, 0, axisLimit);
}
