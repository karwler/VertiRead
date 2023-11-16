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

enum Actions : uint8 {
	ACT_NONE = 0x0,
	ACT_LEFT = 0x1,
	ACT_RIGHT = 0x2,
	ACT_DOUBLE = 0x4
};

class Direction {
public:
	enum Dir : uint8 {
		up,
		down,
		left,
		right
	};
	static constexpr array names = {
		"up",
		"down",
		"left",
		"right"
	};

private:
	Dir dir;

public:
	constexpr Direction(Dir direction) : dir(direction) {}

	constexpr operator Dir() const { return dir; }

	constexpr bool vertical() const { return dir <= down; }
	constexpr bool horizontal() const { return dir >= left; }
	constexpr bool positive() const { return dir & 1; }
	constexpr bool negative() const { return !positive(); }
};

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
		zoomFit,
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
	static constexpr array names = {
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
		"zoom_fit",
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
	static constexpr array gbuttonNames = {
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
	static constexpr array gaxisNames = {
		"LX",
		"LY",
		"RX",
		"RY",
		"LT",
		"RT"
	};

	union {
		void (ProgState::*bcall)();
		void (ProgState::*acall)(float);
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

	SDL_Scancode getKey() const { return key; }
	bool keyAssigned() const { return asg & ASG_KEY; }
	void clearAsgKey();
	void setKey(SDL_Scancode kkey);

	uint8 getJctID() const { return jctID; }
	bool jctAssigned() const { return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N); }
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

	SDL_GameControllerButton getGbutton() const { return SDL_GameControllerButton(gctID); }
	bool gbuttonAssigned() const { return asg & ASG_GBUTTON; }
	void setGbutton(SDL_GameControllerButton but);

	SDL_GameControllerAxis getGaxis() const { return SDL_GameControllerAxis(gctID); }
	bool gaxisAssigned() const { return asg & (ASG_GAXIS_P | ASG_GAXIS_N); }
	bool gposAxisAssigned() const { return asg & ASG_GAXIS_P; }
	bool gnegAxisAssigned() const { return asg & ASG_GAXIS_N; }
	void setGaxis(SDL_GameControllerAxis axis, bool positive);
};

inline void Binding::clearAsgKey() {
	asg &= ~ASG_KEY;
}

inline void Binding::clearAsgJct() {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

inline void Binding::clearAsgGct() {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

class PicLim {
public:
	enum class Type : uint8 {
		none,
		count,
		size
	};
	static constexpr array names = {
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

	uintptr_t getCount() const { return count; }
	void setCount(string_view str);
	uintptr_t getSize() const { return size; }
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

inline void PicLim::setCount(string_view str) {
	count = toCount(str);
}

inline uintptr_t PicLim::toCount(string_view str) {
	return coalesce(toNum<uintptr_t>(str), defaultCount);
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

	static constexpr array defaultColors = {
		vec4(0.04f, 0.04f, 0.04f, 1.f),	// background
		vec4(0.35f, 0.35f, 0.35f, 1.f),	// normal
		vec4(0.24f, 0.24f, 0.24f, 1.f),	// dark
		vec4(0.47f, 0.47f, 0.47f, 1.f),	// light
		vec4(0.41f, 0.41f, 0.41f, 1.f),	// select
		vec4(0.29f, 0.29f, 0.29f, 1.f),	// tooltip
		vec4(0.82f, 0.82f, 0.82f, 1.f),	// text
		vec4(0.82f, 0.82f, 0.82f, 1.f)	// texture
	};
	static constexpr array colorNames = {
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
	static constexpr array screenModeNames = {
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
		vulkan
#endif
	};
	static constexpr array rendererNames = {
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

	enum class Hinting : uint8 {
		normal,
		mono
	};
	static constexpr array hintingNames = {
		"normal",
		"mono"
	};

	enum class Compression : uint8 {
		none,
		b16,
		compress
	};
	static constexpr array compressionNames = {
		"none",
		"16 b",
		"compress"
	};

	static constexpr ushort defaultSpacing = 10;
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
	static constexpr int8 zoomLimit = 113;
	static constexpr double zoomBase = 1.2;

private:
	string theme;
public:
	string dirLib;
	string font = defaultFont;
	umap<int, Recti> displays;
	PicLim picLim;
	u32vec2 device = u32vec2(0);
	ivec2 resolution = ivec2(800, 600);
	vec2 scrollSpeed = vec2(1600.f, 1600.f);
	uint maxPicRes = UINT_MAX;
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
	int8 zoom = 0;
	Compression compression = defaultCompression;
	bool vsync = true;
	Renderer renderer = defaultRenderer;
	bool gpuSelecting = false;
	Hinting hinting = defaultHinting;

	Settings(const fs::path& dirSets, vector<string>&& themes);

	const string& getTheme() const { return theme; }
	const string& setTheme(string_view name, vector<string>&& themes);

	static umap<int, Recti> displayArrangement();
	static double zoomValue(int step);
	void unionDisplays();
	string scrollSpeedString() const { return toStr(scrollSpeed); }
	int getDeadzone() const { return deadzone; }
	void setDeadzone(int val);
};

inline double Settings::zoomValue(int step) {
	return std::pow(zoomBase, step);
}

inline void Settings::setDeadzone(int val) {
	deadzone = std::clamp(val, 0, axisLimit);
}
