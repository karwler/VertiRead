#pragma once

#include "utils.h"
#include <unordered_set>
#include <SDL_gamecontroller.h>
#include <SDL_scancode.h>

template <class... T> using uset = std::unordered_set<T...>;

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

template <IntEnum T, size_t N>
T strToEnum(const array<const char*, N>& names, string_view str, T defaultValue = T(N)) {
	typename array<const char*, N>::const_iterator p = rng::find_if(names, [str](const char* it) -> bool { return strciequal(it, str); });
	return p != names.end() ? T(p - names.begin()) : defaultValue;
}

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

	static constexpr array hatNames = {
		"Up",
		"Right",
		"Down",
		"Left",
		"Right-Up",
		"Right-Down",
		"Left-Down",
		"Left-Up"
	};
	static constexpr array<uint8, hatNames.size()> hatValues = {
		SDL_HAT_UP,
		SDL_HAT_RIGHT,
		SDL_HAT_DOWN,
		SDL_HAT_LEFT,
		SDL_HAT_RIGHTUP,
		SDL_HAT_RIGHTDOWN,
		SDL_HAT_LEFTDOWN,
		SDL_HAT_LEFTUP
	};

public:
	void reset(Type newType);

	SDL_Scancode getKey() const { return key; }
	bool keyAssigned() const { return asg & ASG_KEY; }
	void clearAsgKey() noexcept;
	void setKey(SDL_Scancode kkey) noexcept;

	uint8 getJctID() const { return jctID; }
	bool jctAssigned() const { return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N); }
	void clearAsgJct() noexcept;

	bool jbuttonAssigned() const { return asg & ASG_JBUTTON; }
	void setJbutton(uint8 but) noexcept;

	bool jaxisAssigned() const { return asg & (ASG_JAXIS_P | ASG_JAXIS_N); }
	bool jposAxisAssigned() const { return asg & ASG_JAXIS_P; }
	bool jnegAxisAssigned() const { return asg & ASG_JAXIS_N; }
	void setJaxis(uint8 axis, bool positive) noexcept;

	uint8 getJhatVal() const { return jHatVal; }
	bool jhatAssigned() const { return asg & ASG_JHAT; }
	void setJhat(uint8 hat, uint8 val) noexcept;

	uint8 getGctID() const { return gctID; }
	bool gctAssigned() const { return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N); }
	void clearAsgGct() noexcept;

	SDL_GameControllerButton getGbutton() const { return SDL_GameControllerButton(gctID); }
	bool gbuttonAssigned() const { return asg & ASG_GBUTTON; }
	void setGbutton(SDL_GameControllerButton but) noexcept;

	SDL_GameControllerAxis getGaxis() const { return SDL_GameControllerAxis(gctID); }
	bool gaxisAssigned() const { return asg & (ASG_GAXIS_P | ASG_GAXIS_N); }
	bool gposAxisAssigned() const { return asg & ASG_GAXIS_P; }
	bool gnegAxisAssigned() const { return asg & ASG_GAXIS_N; }
	void setGaxis(SDL_GameControllerAxis axis, bool positive) noexcept;

	static uint8 hatNameToValue(string_view name) noexcept;
	static const char* hatValueToName(uint8 val) noexcept;
};

inline void Binding::clearAsgKey() noexcept {
	asg &= ~ASG_KEY;
}

inline void Binding::clearAsgJct() noexcept {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

inline void Binding::clearAsgGct() noexcept {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

struct PicLim {
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

	static constexpr uintptr_t defaultCount = 128;

	uintptr_t count = defaultCount;
	uintptr_t size = 0; // if it statys 0 then it should be set to a recommended value by a renderer
	Type type = Type::none;

	void set(string_view str);

	static pair<uint8, uint8> memSizeMag(uintptr_t num);
	static string memoryString(uintptr_t num, uint8 dmag, uint8 smag);
	static string memoryString(uintptr_t num);
	static uintptr_t toCount(string_view str);
	static uintptr_t toSize(string_view str);
};

inline uintptr_t PicLim::toCount(string_view str) {
	return coalesce(toNum<uintptr_t>(str), defaultCount);
}

class Settings {
public:
	static constexpr char flagLog[] = "l";
#ifndef _WIN32
	static constexpr char flagCompositor[] = "c";
#endif
#ifdef WITH_DIRECT3D
	static constexpr char flagDirect3d11[] = "d11";
#endif
#ifdef WITH_OPENGL
	static constexpr char flagOpenGl1[] = "g1";
	static constexpr char flagOpenGl3[] = "g3";
	static constexpr char flagOpenEs3[] = "e3";
#endif
#ifdef WITH_VULKAN
	static constexpr char flagVulkan[] = "vk";
#endif
	static constexpr char flagSoftware[] = "sf";

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

	enum class Zoom : uint8 {
		value,
		first,
		largest
	};
	static constexpr array zoomNames = {
		"value",
		"first",
		"largest"
	};

	enum class Renderer : uint8 {
#ifdef WITH_DIRECT3D
		direct3d11,
#endif
#ifdef WITH_OPENGL
		opengl1,
		opengl3,
		opengles3,
#endif
#ifdef WITH_VULKAN
		vulkan,
#endif
		software
	};
	static constexpr array rendererNames = {
#ifdef WITH_DIRECT3D
		"Direct3D 11",
#endif
#ifdef WITH_OPENGL
		"OpenGL 1.1",
		"OpenGL 3.0",
		"OpenGL ES 3.0",
#endif
#ifdef WITH_VULKAN
		"Vulkan 1.0",
#endif
		"Software"
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
		b8,
		b16,
		compress
	};
	static constexpr array compressionNames = {
		"none",
		"8 b",
		"16 b",
		"compress"
	};

	static constexpr ushort defaultSpacing = 10;
	static constexpr uint minPicRes = 1;
	static constexpr int axisLimit = SHRT_MAX + 1;
	static constexpr Screen defaultScreenMode = Screen::windowed;
	static constexpr Direction::Dir defaultDirection = Direction::down;
	static constexpr Zoom defaultZoomType = Zoom::value;
#ifdef WITH_OPENGL
#if !defined(_WIN32) && (defined(__arm__) || defined(__aarch64__))
	static constexpr Renderer defaultRenderer = Renderer::opengles3;
#else
	static constexpr Renderer defaultRenderer = Renderer::opengl1;
#endif
#elif defined(WITH_DIRECT3D)
	static constexpr Renderer defaultRenderer = Renderer::direct3d11;
#elif defined(WITH_VULKAN)
	static constexpr Renderer defaultRenderer = Renderer::vulkan;
#else
	static constexpr Renderer defaultRenderer = Renderer::software;
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
	Zoom zoomType = defaultZoomType;
	int8 zoom = 0;
	Compression compression = defaultCompression;
	bool vsync = true;
	Renderer renderer = defaultRenderer;
	bool gpuSelecting = false;
	Hinting hinting = defaultHinting;

	Settings(const fs::path& dirSets, vector<string>&& themes);

	void setZoom(string_view str);
	const string& getTheme() const { return theme; }
	const string& setTheme(string_view name, vector<string>&& themes);

	static umap<int, Recti> displayArrangement();
	static double zoomValue(int step);
	void unionDisplays();
	static Renderer getRenderer(string_view name);
	void setRenderer(const uset<string>& cmdFlags);
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
