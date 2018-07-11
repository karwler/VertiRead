#pragma once

// stuff that's used pretty much everywhere
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "utils/vec2.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>

// get rid of SDL's main
#ifdef main
#undef main
#endif

// to make life easier
using std::vector;
using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::wstring;
using std::ostringstream;
using sizt = size_t;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using char16 = char16_t;
using char32 = char32_t;
using wchar = wchar_t;

using vec2i = vec2<int>;
using vec2u = vec2<uint>;
using vec2f = vec2<float>;
using vec2t = vec2<sizt>;

template <typename... T>
using umap = std::unordered_map<T...>;
template <typename... T>
using uset = std::unordered_set<T...>;
template <typename... T>
using uptr = std::unique_ptr<T...>;

// forward declaraions
class Button;
class Layout;
class Program;
class ProgState;

// directory separator
#ifdef _WIN32
const char dsep = '\\';
#else
const char dsep = '/';
#endif

namespace Default {

// general settings
const char language[] = "English";
const bool maximized = false;
const bool fullscreen = false;
const vec2i resolution(800, 600);
const char font[] = "Arial";
const vec2f scrollSpeed(800.f, 1000.f);
const int16 controllerDeadzone = 256;

// window
const vec2i windowPos(SDL_WINDOWPOS_UNDEFINED);
const vec2i windowMinSize(500, 300);
const uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
const uint32 rendererFlags = SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;

// colors
const vector<SDL_Color> colors = {
	{10, 10, 10, 255},		// background
	{90, 90, 90, 255},		// normal
	{60, 60, 60, 255},		// dark
	{120, 120, 120, 255},	// light
	{105, 105, 105, 255},	// select
	{210, 210, 210, 255},	// text
	{210, 210, 210, 255}	// texture
};
const SDL_Color colorPopupDim = {0, 0, 0, 127};

// key bindings
const SDL_Scancode keyEnter = SDL_SCANCODE_RETURN;
const SDL_Scancode keyEscape = SDL_SCANCODE_ESCAPE;
const SDL_Scancode keyUp = SDL_SCANCODE_UP;
const SDL_Scancode keyDown = SDL_SCANCODE_DOWN;
const SDL_Scancode keyLeft = SDL_SCANCODE_LEFT;
const SDL_Scancode keyRight = SDL_SCANCODE_RIGHT;
const SDL_Scancode keyCenterView = SDL_SCANCODE_C;
const SDL_Scancode keyScrollFast = SDL_SCANCODE_X;
const SDL_Scancode keyScrollSlow = SDL_SCANCODE_Z;
const SDL_Scancode keyPageUp = SDL_SCANCODE_W;
const SDL_Scancode keyPageDown = SDL_SCANCODE_S;
const SDL_Scancode keyZoomIn = SDL_SCANCODE_D;
const SDL_Scancode keyZoomOut = SDL_SCANCODE_A;
const SDL_Scancode keyZoomReset = SDL_SCANCODE_R;
const SDL_Scancode keyNextDir = SDL_SCANCODE_E;
const SDL_Scancode keyPrevDir = SDL_SCANCODE_Q;
const SDL_Scancode keyFullscreen = SDL_SCANCODE_F;

// joystick bindings
const uint8 jbuttonEnter = 2;
const uint8 jbuttonEscape = 1;
const uint8 jhatUp = SDL_HAT_UP;
const uint8 jhatDown = SDL_HAT_DOWN;
const uint8 jhatLeft = SDL_HAT_LEFT;
const uint8 jhatRight = SDL_HAT_RIGHT;
const uint8 jaxisScrollVertical = 1;
const uint8 jaxisScrollHorizontal = 0;
const uint8 jaxisCursorVertical = 2;
const uint8 jaxisCursorHorizontal = 3;
const uint8 jbuttonCenterView = 10;
const uint8 jbuttonScrollFast = 0;
const uint8 jbuttonScrollSlow = 3;
const uint8 jbuttonZoomIn = 5;
const uint8 jbuttonZoomOut = 4;
const uint8 jbuttonZoomReset = 11;
const uint8 jbuttonNextDir = 7;
const uint8 jbuttonPrevDir = 6;
const uint8 jbuttonFullscreen = 8;

// gamepad bindings
const SDL_GameControllerButton gbuttonEnter = SDL_CONTROLLER_BUTTON_A;
const SDL_GameControllerButton gbuttonEscape = SDL_CONTROLLER_BUTTON_B;
const SDL_GameControllerButton gbuttonUp = SDL_CONTROLLER_BUTTON_DPAD_UP;
const SDL_GameControllerButton gbuttonDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
const SDL_GameControllerButton gbuttonLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
const SDL_GameControllerButton gbuttonRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
const SDL_GameControllerAxis gaxisScrollVertical = SDL_CONTROLLER_AXIS_LEFTY;
const SDL_GameControllerAxis gaxisScrollHorizontal = SDL_CONTROLLER_AXIS_LEFTX;
const SDL_GameControllerAxis gaxisCursorVertical = SDL_CONTROLLER_AXIS_RIGHTY;
const SDL_GameControllerAxis gaxisCursorHorizontal = SDL_CONTROLLER_AXIS_RIGHTX;
const SDL_GameControllerButton gbuttonCenterView = SDL_CONTROLLER_BUTTON_LEFTSTICK;
const SDL_GameControllerButton gbuttonScrollFast = SDL_CONTROLLER_BUTTON_Y;
const SDL_GameControllerButton gbuttonScrollSlow = SDL_CONTROLLER_BUTTON_X;
const SDL_GameControllerButton gbuttonZoomIn = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
const SDL_GameControllerButton gbuttonZoomOut = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
const SDL_GameControllerButton gbuttonZoomReset = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
const SDL_GameControllerAxis gaxisNextDir = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
const SDL_GameControllerAxis gaxisPrevDir = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
const SDL_GameControllerButton gbuttonFullscreen = SDL_CONTROLLER_BUTTON_BACK;

// other controller bindings related stuff
const uint8 jhatID = 0;
const bool axisDirUp = false;
const bool axisDirDown = true;
const bool axisDirRight = true;
const bool axisDirLeft = false;

// widgets' properties
const int spacing = 5;
const int itemHeight = 30;
const int sliderWidth = 10;
const int checkboxSpacing = 5;
const int caretWidth = 4;

// movement
const float cursorMoveFactor = 10.f;
const int scrollFactorWheel = 140;
const float scrollFactor = 2.f;
const float scrollThrottle = 10.f;
const float zoomFactor = 1.2f;

// files and directories
const char fileIcon[] = "icon.ico";
const char fileThemes[] = "themes.ini";
const char fileSettings[] = "settings.ini";
const char fileBindings[] = "bindings.ini";
const char fileBooks[] = "books.ini";
const char dirLibrary[] = "library";
const char dirLanguages[] = "languages";
const char dirTextures[] = "textures";

// INI keywords
const char iniKeywordMaximized[] = "maximized";
const char iniKeywordFullscreen[] = "fullscreen";
const char iniKeywordResolution[] = "resolution";
const char iniKeywordFont[] = "font";
const char iniKeywordLanguage[] = "language";
const char iniKeywordTheme[] = "theme";
const char iniKeywordLibrary[] = "library";
const char iniKeywordRenderer[] = "renderer";
const char iniKeywordScrollSpeed[] = "scroll_speed";
const char iniKeywordDeadzone[] = "deadzone";

const vector<string> bindingNames = {
	"enter",
	"escape",
	"up",
	"down",
	"left",
	"right",
	"scroll up",
	"scroll down",
	"scroll left",
	"scroll right",
	"cursor up",
	"cursor down",
	"cursor left",
	"cursor right",
	"center view",
	"scroll fast",
	"scroll slow",
	"page up",
	"page down",
	"zoom in",
	"zoom out",
	"zoom reset",
	"next dir",
	"prev dir",
	"fullscreen"
};
const map<uint8, string> hatNames = {
	pair<uint8, string>(SDL_HAT_CENTERED, "Center"),
	pair<uint8, string>(SDL_HAT_UP, "Up"),
	pair<uint8, string>(SDL_HAT_RIGHT, "Right"),
	pair<uint8, string>(SDL_HAT_DOWN, "Down"),
	pair<uint8, string>(SDL_HAT_LEFT, "Left"),
	pair<uint8, string>(SDL_HAT_RIGHTUP, "Right-Up"),
	pair<uint8, string>(SDL_HAT_RIGHTDOWN, "Right-Down"),
	pair<uint8, string>(SDL_HAT_LEFTDOWN, "Left-Down"),
	pair<uint8, string>(SDL_HAT_LEFTUP, "Left-Up")
};
const vector<string> gbuttonNames = {
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
const vector<string> gaxisNames = {
	"LX",
	"LY",
	"RX",
	"RY",
	"LT",
	"RT"
};
const vector<string> colorNames = {
	"background",
	"normal",
	"dark",
	"light",
	"select",
	"text",
	"texture"
};

// other random crap
const char titleDefault[] = "VertiRead";
const char titleExtra[] = "vertiread";
const float clickThreshold = 8;
const int fontTestHeight = 100;
const char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
const int textMargin = 5;
const int iconMargin = 2;
const uint32 eventCheckTimeout = 50;
const float menuHideTimeout = 3.f;
const int axisLimit = 32768;

}
