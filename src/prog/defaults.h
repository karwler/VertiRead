#pragma once

// stuff that's used pretty much everywhere
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "utils/vec2.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

// to make life easier
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
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

// get rid of SDL's main
#ifdef main
#undef main
#endif

// forward declaraions
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
const int volume = MIX_MAX_VOLUME;

// video settings
const bool maximized = false;
const bool fullscreen = false;
const vec2i resolution(800, 600);
const char font[] = "Arial";

// window
const vec2i windowPos(SDL_WINDOWPOS_UNDEFINED);
const vec2i windowMinSize(500, 300);
const uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
const uint32 rendererFlags = SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;

// colors
const SDL_Color colorBackground = {10, 10, 10, 255};
const SDL_Color colorNormal = {90, 90, 90, 255};
const SDL_Color colorDark = {60, 60, 60, 255};
const SDL_Color colorLight = {120, 120, 120, 255};
const SDL_Color colorText = {210, 210, 210, 255};
const SDL_Color colorPopupDim = {0, 0, 0, 127};

// controls settings
const vec2f scrollSpeed(800.f, 1000.f);
const int16 controllerDeadzone = 256;

// key bindings
const SDL_Scancode keyBack = SDL_SCANCODE_ESCAPE;
const SDL_Scancode keyZoomIn = SDL_SCANCODE_E;
const SDL_Scancode keyZoomOut = SDL_SCANCODE_Q;
const SDL_Scancode keyZoomReset = SDL_SCANCODE_R;
const SDL_Scancode keyCenterView = SDL_SCANCODE_C;
const SDL_Scancode keyFast = SDL_SCANCODE_X;
const SDL_Scancode keySlow = SDL_SCANCODE_Z;
const SDL_Scancode keyPlayPause = SDL_SCANCODE_SPACE;
const SDL_Scancode keyFullscreen = SDL_SCANCODE_L;

const SDL_Scancode keyNextDir = SDL_SCANCODE_P;
const SDL_Scancode keyPrevDir = SDL_SCANCODE_O;

const SDL_Scancode keyNextSong = SDL_SCANCODE_D;
const SDL_Scancode keyPrevSong = SDL_SCANCODE_A;
const SDL_Scancode keyVolumeUp = SDL_SCANCODE_W;
const SDL_Scancode keyVolumeDown = SDL_SCANCODE_S;

const SDL_Scancode keyPageUp = SDL_SCANCODE_PAGEUP;
const SDL_Scancode keyPageDown = SDL_SCANCODE_PAGEDOWN;
const SDL_Scancode keyUp = SDL_SCANCODE_UP;
const SDL_Scancode keyDown = SDL_SCANCODE_DOWN;
const SDL_Scancode keyRight = SDL_SCANCODE_RIGHT;
const SDL_Scancode keyLeft = SDL_SCANCODE_LEFT;

// joystick bindings
const uint8 jbuttonBack = 1;
const uint8 jbuttonZoomIn = 5;
const uint8 jbuttonZoomOut = 4;
const uint8 jbuttonZoomReset = 11;
const uint8 jbuttonCenterView = 10;
const uint8 jbuttonFast = 0;
const uint8 jbuttonSlow = 3;
const uint8 jbuttonPlayPause = 9;
const uint8 jbuttonFullscreen = 8;

const uint8 jbuttonNextDir = 7;
const uint8 jbuttonPrevDir = 6;

const uint8 jhatDpadRight = SDL_HAT_RIGHT;
const uint8 jhatDpadLeft = SDL_HAT_LEFT;
const uint8 jhatDpadUp = SDL_HAT_UP;
const uint8 jhatDpadDown = SDL_HAT_DOWN;

const uint8 jaxisVertical = 1;
const uint8 jaxisHorizontal = 0;

// gamepad bindings
const SDL_GameControllerButton gbuttonBack = SDL_CONTROLLER_BUTTON_B;
const SDL_GameControllerButton gbuttonZoomIn = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
const SDL_GameControllerButton gbuttonZoomOut = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
const SDL_GameControllerButton gbuttonZoomReset = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
const SDL_GameControllerButton gbuttonCenterView = SDL_CONTROLLER_BUTTON_LEFTSTICK;
const SDL_GameControllerButton gbuttonFast = SDL_CONTROLLER_BUTTON_Y;
const SDL_GameControllerButton gbuttonSlow = SDL_CONTROLLER_BUTTON_X;
const SDL_GameControllerButton gbuttonPlayPause = SDL_CONTROLLER_BUTTON_START;
const SDL_GameControllerButton gbuttonFullscreen = SDL_CONTROLLER_BUTTON_BACK;

const SDL_GameControllerAxis gaxisNextDir = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
const SDL_GameControllerAxis gaxisPrevDir = SDL_CONTROLLER_AXIS_TRIGGERLEFT;

const SDL_GameControllerButton gbuttonDpadRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
const SDL_GameControllerButton gbuttonDpadLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
const SDL_GameControllerButton gbuttonDpadUp = SDL_CONTROLLER_BUTTON_DPAD_UP;
const SDL_GameControllerButton gbuttonDpadDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN;

const SDL_GameControllerAxis gaxisVertical = SDL_CONTROLLER_AXIS_LEFTY;
const SDL_GameControllerAxis gaxisHorizontal = SDL_CONTROLLER_AXIS_LEFTX;

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

// scroll area related constants
const int scrollFactorWheel = 20;
const float scrollFactor = 2.f;
const float scrollThrottle = 10.f;
const float zoomFactor = 1.2f;

// files and directories
const char fileIcon[] = "icon.ico";
const char fileThemes[] = "themes.ini";
const char fileSettings[] = "settings.ini";
const char fileBindings[] = "bindings.ini";
const char fileLastPages[] = "last.ini";
const char dirLibrary[] = "library";
const char dirPlaylists[] = "playlists";
const char dirLanguages[] = "languages";
const char dirTextures[] = "textures";

// INI keywords
const char iniKeywordBook[] = "book";
const char iniKeywordSong[] = "song";

const char iniKeywordMaximized[] = "maximized";
const char iniKeywordFullscreen[] = "fullscreen";
const char iniKeywordResolution[] = "resolution";
const char iniKeywordFont[] = "font";
const char iniKeywordLanguage[] = "language";
const char iniKeywordTheme[] = "theme";
const char iniKeywordLibrary[] = "library";
const char iniKeywordPlaylists[] = "playlists";
const char iniKeywordRenderer[] = "renderer";
const char iniKeywordVolume[] = "volume";
const char iniKeywordScrollSpeed[] = "scroll_speed";
const char iniKeywordDeadzone[] = "deadzone";

// other random crap
const char titleDefault[] = "VertiRead";
const char titleExtra[] = "vertiread";
const float clickThreshold = 8;
const int fontTestHeight = 100;
const char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";
const int textOffset = 5;
const int lineEditOffset = 20;
const uint32 eventCheckTimeout = 50;
const int volumeStep = 8;
const float menuHideTimeout = 3.f;
const int axisLimit = 32768;

}
