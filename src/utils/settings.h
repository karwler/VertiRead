#pragma once

#include "utils.h"
#ifdef WITH_SDL3
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#else
#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>
#include <SDL_video.h>
#endif
#include <unordered_set>

template <class... T> using uset = std::unordered_set<T...>;

enum class Color : uint8 {
	normal,
	dark,
	light,
	select,
	tooltip,
	text,
	texture,
	dim,
	background
};

enum class Alignment : uint8 {
	left,
	center,
	right
};

enum Actions : uint8 {
	ACT_NONE	= 0x00,
	ACT_LEFT	= 0x01,
	ACT_RIGHT	= 0x02,
	ACT_DOUBLE	= 0x04
};

template <IntEnum T, size_t N>
T strToEnum(const array<const char*, N>& names, string_view str, T defaultValue = T(N)) noexcept {
	auto p = rng::find_if(names, [str](const char* it) -> bool { return strciequal(it, str); });
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
	constexpr Direction(Dir direction) noexcept : dir(direction) {}

	constexpr operator Dir() const noexcept { return dir; }

	constexpr bool vertical() const noexcept { return dir <= down; }
	constexpr bool horizontal() const noexcept { return dir >= left; }
	constexpr bool positive() const noexcept { return dir & 1; }
	constexpr bool negative() const noexcept { return !positive(); }
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

	enum class Device : uint8 {
		keyboard,
		joystick,
		gamepad
	};

	enum Assignment : uint8 {
		ASG_NONE	= 0x00,
		ASG_KEY		= 0x01,
		ASG_JBUTTON	= 0x02,
		ASG_JHAT	= 0x04,
		ASG_JAXIS_P	= 0x08,	// use only positive values
		ASG_JAXIS_N	= 0x10,	// use only negative values
		ASG_GBUTTON	= 0x20,
		ASG_GAXIS_P	= 0x40,
		ASG_GAXIS_N	= 0x80
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
		"Right",
#ifdef WITH_SDL3
		"Misc1",
		"RP1",
		"LP",
		"RP2",
		"LP2",
		"Touchpad",
		"Misc2",
		"Misc3",
		"Misc4",
		"Misc5",
		"Misc6"
#endif
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
	SDL_Keycode key;			// keyboard key
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
	void reset(Type newType) noexcept;

	SDL_Keycode getKey() const noexcept { return key; }
	bool keyAssigned() const noexcept { return asg & ASG_KEY; }
	void clearAsgKey() noexcept;
	void setKey(SDL_Keycode kkey) noexcept;

	uint8 getJctID() const noexcept { return jctID; }
	bool jctAssigned() const noexcept { return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N); }
	void clearAsgJct() noexcept;

	bool jbuttonAssigned() const noexcept { return asg & ASG_JBUTTON; }
	void setJbutton(uint8 but) noexcept;

	bool jaxisAssigned() const noexcept { return asg & (ASG_JAXIS_P | ASG_JAXIS_N); }
	bool jposAxisAssigned() const noexcept { return asg & ASG_JAXIS_P; }
	bool jnegAxisAssigned() const noexcept { return asg & ASG_JAXIS_N; }
	void setJaxis(uint8 axis, bool positive) noexcept;

	uint8 getJhatVal() const noexcept { return jHatVal; }
	bool jhatAssigned() const noexcept { return asg & ASG_JHAT; }
	void setJhat(uint8 hat, uint8 val) noexcept;

	uint8 getGctID() const noexcept { return gctID; }
	bool gctAssigned() const noexcept { return asg & (ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N); }
	void clearAsgGct() noexcept;

	SDL_GameControllerButton getGbutton() const noexcept { return SDL_GameControllerButton(gctID); }
	bool gbuttonAssigned() const noexcept { return asg & ASG_GBUTTON; }
	void setGbutton(SDL_GameControllerButton but) noexcept;

	SDL_GameControllerAxis getGaxis() const noexcept { return SDL_GameControllerAxis(gctID); }
	bool gaxisAssigned() const noexcept { return asg & (ASG_GAXIS_P | ASG_GAXIS_N); }
	bool gposAxisAssigned() const noexcept { return asg & ASG_GAXIS_P; }
	bool gnegAxisAssigned() const noexcept { return asg & ASG_GAXIS_N; }
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

	void set(string_view str) noexcept;

	static pair<uint8, uint8> memSizeMag(uintptr_t num) noexcept;
	static string memoryString(uintptr_t num, uint8 dmag, uint8 smag);
	static string memoryString(uintptr_t num);
	static uintptr_t toCount(string_view str) noexcept;
	static uintptr_t toSize(string_view str) noexcept;
};

inline uintptr_t PicLim::toCount(string_view str) noexcept {
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
#if !defined(__arm__) && !defined(__aarch64__)
	static constexpr char flagOpenGl1[] = "g1";
	static constexpr char flagOpenGl3[] = "g3";
#endif
#ifndef _WIN32
	static constexpr char flagOpenEs3[] = "e3";
#endif
#endif
#ifdef WITH_VULKAN
	static constexpr char flagVulkan[] = "vk";
#endif
	static constexpr char flagSoftware[] = "sf";

	static constexpr array defaultColors = {
		vec4(0.35f, 0.35f, 0.35f, 1.f),	// normal
		vec4(0.24f, 0.24f, 0.24f, 1.f),	// dark
		vec4(0.47f, 0.47f, 0.47f, 1.f),	// light
		vec4(0.41f, 0.41f, 0.41f, 1.f),	// select
		vec4(0.29f, 0.29f, 0.29f, 1.f),	// tooltip
		vec4(0.82f, 0.82f, 0.82f, 1.f),	// text
		vec4(0.82f, 0.82f, 0.82f, 1.f),	// texture
		vec4(0.f, 0.f, 0.f, 0.5f),		// dim
		vec4(0.04f, 0.04f, 0.04f, 1.f)	// background (must be last so the rest can be addressed properly)
	};
	static constexpr array colorNames = {
		"normal",
		"dark",
		"light",
		"select",
		"tooltip",
		"text",
		"texture",
		"dim",
		"background"
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
#if !defined(__arm__) && !defined(__aarch64__)
		opengl1,
		opengl3,
#endif
#ifndef _WIN32
		opengles3,
#endif
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
#if !defined(__arm__) && !defined(__aarch64__)
		"OpenGL 1.2",
		"OpenGL 3.0",
#endif
#ifndef _WIN32
		"OpenGL ES 3.0",
#endif
#endif
#ifdef WITH_VULKAN
		"Vulkan 1.0",
#endif
		"Software"
	};

	enum class Gamma : uint8 {
		none,
		srgb,
		value
	};
	static constexpr array gammaNames = {
		"none",
		"sRGB",
		"value"
	};

	enum class Preview : uint8 {
		off,
		local,
		all
	};
	static constexpr array previewNames = {
		"off",
		"local",
		"all"
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

	struct Display {
		Recti rect;
		SDL_DisplayID did;

		Display(const Recti& bounds, SDL_DisplayID dispId) : rect(bounds), did(dispId) {}

		bool operator==(const Display& d) const { return rect == d.rect && did == d.rect; }
		bool operator<(const Display& d) const { return did < d.did; }
	};

	static constexpr ushort defaultSpacing = 10;
	static constexpr uint minPicRes = 1;
	static constexpr uint16 axisLimit = SDL_JOYSTICK_AXIS_MAX + 1;
	static constexpr Screen defaultScreenMode = Screen::windowed;
	static constexpr Direction::Dir defaultDirection = Direction::down;
	static constexpr Zoom defaultZoomType = Zoom::value;
#ifdef WITH_OPENGL
#if defined(__arm__) || defined(__aarch64__)
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
	static constexpr Preview defaultPreview = Preview::local;
	static constexpr Gamma defaultGammaType = Gamma::srgb;
	static constexpr uint8 minGamma = 1, maxGamma = 40;
	static constexpr Compression defaultCompression = Compression::none;
	static constexpr int8 zoomLimit = 113;
	static constexpr double zoomBase = 1.2;
	static constexpr uint maxPageElements = 4;

#ifdef _WIN32
	static inline wchar_t** argv;
#else
	static inline char** argv;
#endif
	static inline int argc = 0;

private:
	string theme;
public:
	string dirLib;
	string font = defaultFont;
	vector<Display> displays;
	PicLim picLim;
	u32vec2 device = u32vec2(0);
	ivec2 resolution = ivec2(800, 600);
	vec2 scrollSpeed = vec2(14.f, 16.f);
	uint maxPicRes = UINT_MAX;
private:
	uint16 deadzone = 256;
public:
	ushort spacing = defaultSpacing;
	bool maximized = false;
	Screen screen = defaultScreenMode;
	Preview preview = defaultPreview;
	bool showHidden = false;
	bool tooltips = true;
	Direction direction = defaultDirection;
	Zoom zoomType = defaultZoomType;
	int8 zoom = 0;
	Compression compression = defaultCompression;
	bool vsync = true;
	Renderer renderer = defaultRenderer;
	Gamma gammaType = defaultGammaType;
	uint8 gammaValue = 22;
	bool monoFont = false;

	Settings(vector<string>&& themes);

	void setZoom(string_view str) noexcept;
	const string& getTheme() const noexcept { return theme; }
	const string& setTheme(string_view name, vector<string>&& themes);

	static vector<Display> displayArrangement();
	static double zoomValue(int step) noexcept;
	void unionDisplays();
	static Renderer getRenderer(string_view name);
	void setRenderer() noexcept;
	void setGamma(string_view str) noexcept;
	bool needsTransparentWindow(const array<vec4, defaultColors.size()>& colors) const noexcept;
	string scrollSpeedString() const noexcept { return toStr(scrollSpeed); }
	uint16 getDeadzone() const noexcept { return deadzone; }
	void setDeadzone(uint16 val) noexcept;

	static string firstArg() noexcept;
	static bool hasFlag(const char* name) noexcept;
	static bool cmpFlag(const char* name, int id) noexcept;
	static string homeDir();
};

inline const string& Settings::setTheme(string_view name, vector<string>&& themes) {
	return theme = rng::find(themes, name) != themes.end() ? name : !themes.empty() ? std::move(themes[0]) : string();
}

inline double Settings::zoomValue(int step) noexcept {
	return std::pow(zoomBase, step);
}

inline bool Settings::needsTransparentWindow(const array<vec4, defaultColors.size()>& colors) const noexcept {
#ifndef WITH_SDL3
	return false;
#elif defined(WITH_DIRECT3D)
	return colors[eint(Color::background)].a < 1.f && renderer != Renderer::direct3d11;
#else
	return colors[eint(Color::background)].a < 1.f;
#endif
}

inline void Settings::setDeadzone(uint16 val) noexcept {
	deadzone = std::clamp(val, 0_u16, axisLimit);
}

inline bool Settings::cmpFlag(const char* name, int id) noexcept {
#ifdef _WIN32
	return argv[id][0] == '-' && strasymequal(name, argv[id] + 1 + (argv[id][1] == '-'));
#else
	return argv[id][0] == '-' && !strcmp(name, argv[id] + 1 + (argv[id][1] == '-'));
#endif
}

inline string Settings::homeDir() {
#ifdef _WIN32
	return swtos(_wgetenv(L"UserProfile"));
#else
	return getenv("HOME");
#endif
}
