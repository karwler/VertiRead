#pragma once

// stuff that'll be used pretty much everywhere
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "kklib/aliases.h"
#include "kklib/grid2.h"
#include "kklib/vec2.h"
#include "kklib/vec3.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

// to make life easier
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::wstring;
using std::to_string;

using kk::vec2i;
using kk::vec2u;
using kk::vec2f;
using vec2t = kk::vec2<size_t>;
using kk::grid2;

// get rid of SDL's main
#ifdef main
#undef main
#endif

// forward declaraions
enum class ETextCase : uint8;

class Program;

class Widget;
class ScrollAreaItems;

// directory separator
#ifdef _WIN32
const char dsep = '\\';
#else
const char dsep = '/';
#endif

namespace Default {

// general settings
const char language[] = "english";

// video settings
const bool maximized = false;
const bool fullscreen = false;
const vec2i resolution(800, 600);
const char font[] = "arial";

// window
const vec2i windowPos(SDL_WINDOWPOS_UNDEFINED);
const uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
const uint32 rendererFlags = SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;

// colors
const SDL_Color colorBackground = {10, 10, 10, 255};
const SDL_Color colorRectangle = {90, 90, 90, 255};
const SDL_Color colorHighlighted = {120, 120, 120, 255};
const SDL_Color colorDarkened = {60, 60, 60, 255};
const SDL_Color colorText = {210, 210, 210, 255};
const SDL_Color colorPopupDim = {2, 2, 2, 1};
const SDL_Color colorNoDim = {1, 1, 1, 1};

// audio settings
const int volumeMusic = 128;
const int volumeSound = 0;
const int songDelay = 0.5f;

// controls settings
const vec2f scrollSpeed(4.f, 8.f);
const int16 controllerDeadzone = 256;

// shortcut names
const char shortcutOk[] = "ok";
const char shortcutBack[] = "back";
const char shortcutZoomIn[] = "zoom_in";
const char shortcutZoomOut[] = "zoom_out";
const char shortcutZoomReset[] = "zoom_reset";
const char shortcutCenterView[] = "center_view";
const char shortcutFast[] = "fast";
const char shortcutSlow[] = "slow";
const char shortcutPlayPause[] = "play_pause";
const char shortcutFullscreen[] = "fullscreen";
const char shortcutNextDir[] = "next_dir";
const char shortcutPrevDir[] = "prev_dir";
const char shortcutNextSong[] = "next_song";
const char shortcutPrevSong[] = "prev_song";
const char shortcutVolumeUp[] = "volume_up";
const char shortcutVolumeDown[] = "volume_down";
const char shortcutPageUp[] = "page_up";
const char shortcutPageDown[] = "page_down";
const char shortcutUp[] = "up";
const char shortcutDown[] = "down";
const char shortcutRight[] = "right";
const char shortcutLeft[] = "left";

// key bindings
const SDL_Scancode keyOk = SDL_SCANCODE_RETURN;
const SDL_Scancode keyBack = SDL_SCANCODE_ESCAPE;
const SDL_Scancode keyZoomIn = SDL_SCANCODE_E;
const SDL_Scancode keyZoomOut = SDL_SCANCODE_Q;
const SDL_Scancode keyZoomReset = SDL_SCANCODE_R;
const SDL_Scancode keyCenterView = SDL_SCANCODE_C;
const SDL_Scancode keyFast = SDL_SCANCODE_LSHIFT;
const SDL_Scancode keySlow = SDL_SCANCODE_LALT;
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
const uint8 jbuttonOk = 2;
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
const SDL_GameControllerButton gbuttonOk = SDL_CONTROLLER_BUTTON_A;
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
const int itemSpacing = 5;
const int itemHeight = 30;
const int scrollBarWidth = 10;
const float rbMenuHideTimeout = 1.f;
const int rbBlistW = 48;
const int rbPlayerW = 400;
const int rbPlayerH = 72;
const int checkboxSpacing = 5;
const int caretWidth = 4;

// scroll area related constants
const int scrollFactorWheel = 10;
const float normalScrollFactor = 100.f;
const float scrollFactorFast = normalScrollFactor * 4.f;
const float scrollFactorSlow = normalScrollFactor / 2.f;
const float scrollThrottle = 10.f;
const float zoomFactor = 1.2f;

// files and directories
const char cueNameBack[] = "back";
const char cueNameClick[] = "click";
const char cueNameError[] = "error";
const char cueNameOpen[] = "open";
const char fileIcon[] = "icon.ico";
const char fileThemes[] = "themes.ini";
const char fileGeneralSettings[] = "general.ini";
const char fileVideoSettings[] = "video.ini";
const char fileAudioSettings[] = "audio.ini";
const char fileControlsSettings[] = "controls.ini";
const char fileLastPages[] = "last.ini";
const char dirLibrary[] = "library";
const char dirPlaylists[] = "playlists";
const char dirLanguages[] = "languages";
const char dirSounds[] = "sounds";
const char dirTextures[] = "textures";
const int dirExecMaxBufferLength = 2048;

// INI keywords
const char iniKeywordBook[] = "book";
const char iniKeywordSong[] = "song";

const char iniKeywordLanguage[] = "language";
const char iniKeywordLibrary[] = "library";
const char iniKeywordPlaylists[] = "playlists";

const char iniKeywordFont[] = "font";
const char iniKeywordRenderer[] = "renderer";
const char iniKeywordMaximized[] = "maximized";
const char iniKeywordFullscreen[] = "fullscreen";
const char iniKeywordResolution[] = "resolution";
const char iniKeywordTheme[] = "theme";

const char iniKeywordVolMusic[] = "vol_music";
const char iniKeywordVolSound[] = "vol_sound";
const char iniKeywordSongDelay[] = "song_delay";

const char iniKeywordScrollSpeed[] = "scroll_speed";
const char iniKeywordDeadzone[] = "deadzone";
const char iniKeywordShortcut[] = "shortcut";

// other random crap
const char titleDefault[] = "VertiRead";
const char titleExtra[] = "vertiread";
const float clickThreshold = 8.f;
const float textHeightScale = 0.85f;
const int textOffset = 5;
const int lineEditOffset = 20;
const uint32 eventCheckTimeout = 50;

}
