#include "world.h"
#include "network.h"
#include "optional/glib.h"
#include "prog/types.h"
#ifdef WITH_ICU
#include "utils/compare.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <SDL_image.h>

static constexpr SDL_EventType unusedEvents[] = {
	SDL_LOCALECHANGED,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_SYSTEM_THEME_CHANGED,
	SDL_EVENT_WINDOW_SHOWN,
	SDL_EVENT_WINDOW_HIDDEN,
	SDL_EVENT_WINDOW_EXPOSED,
	SDL_EVENT_WINDOW_MOVED,
	SDL_EVENT_WINDOW_METAL_VIEW_RESIZED,
	SDL_EVENT_WINDOW_MINIMIZED,
	SDL_EVENT_WINDOW_MAXIMIZED,
	SDL_EVENT_WINDOW_RESTORED,
	SDL_EVENT_WINDOW_MOUSE_ENTER,
	SDL_EVENT_WINDOW_CLOSE_REQUESTED,
	SDL_EVENT_WINDOW_HIT_TEST,
	SDL_EVENT_WINDOW_ICCPROF_CHANGED,
	SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED,
	SDL_EVENT_WINDOW_SAFE_AREA_CHANGED,
	SDL_EVENT_WINDOW_OCCLUDED,
	SDL_EVENT_WINDOW_ENTER_FULLSCREEN,
	SDL_EVENT_WINDOW_LEAVE_FULLSCREEN,
	SDL_EVENT_WINDOW_DESTROYED,
	SDL_EVENT_WINDOW_HDR_STATE_CHANGED,
#else
	SDL_SYSWMEVENT,
#endif
	SDL_KEYUP,
	SDL_KEYMAPCHANGED,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_KEYBOARD_ADDED,
	SDL_EVENT_KEYBOARD_REMOVED,
	SDL_EVENT_TEXT_EDITING_CANDIDATES,
	SDL_EVENT_MOUSE_ADDED,
	SDL_EVENT_MOUSE_REMOVED,
#endif
	SDL_JOYBALLMOTION,
	SDL_JOYBUTTONUP,
	SDL_JOYBATTERYUPDATED,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_JOYSTICK_UPDATE_COMPLETE,
#endif
	SDL_CONTROLLERBUTTONUP,
	SDL_CONTROLLERDEVICEREMAPPED,
	SDL_CONTROLLERTOUCHPADDOWN,
	SDL_CONTROLLERTOUCHPADMOTION,
	SDL_CONTROLLERTOUCHPADUP,
	SDL_CONTROLLERSENSORUPDATE,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_GAMEPAD_UPDATE_COMPLETE,
#endif
	SDL_CONTROLLERSTEAMHANDLEUPDATED,
#if !SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_DOLLARGESTURE,
	SDL_DOLLARRECORD,
	SDL_MULTIGESTURE,
#endif
	SDL_CLIPBOARDUPDATE,
	SDL_DROPBEGIN,
	SDL_DROPCOMPLETE,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_DROP_POSITION,
#endif
	SDL_AUDIODEVICEADDED,
	SDL_AUDIODEVICEREMOVED,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED,
#endif
	SDL_SENSORUPDATE,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_PEN_DOWN,
	SDL_EVENT_PEN_UP,
	SDL_EVENT_PEN_MOTION,
	SDL_EVENT_PEN_BUTTON_DOWN,
	SDL_EVENT_PEN_BUTTON_UP,
	SDL_EVENT_CAMERA_DEVICE_ADDED,
	SDL_EVENT_CAMERA_DEVICE_REMOVED,
	SDL_EVENT_CAMERA_DEVICE_APPROVED,
	SDL_EVENT_CAMERA_DEVICE_DENIED,
#endif
	SDL_RENDER_TARGETS_RESET,
	SDL_RENDER_DEVICE_RESET,
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_EVENT_POLL_SENTINEL
#endif
};

static int printError(const char* message) {
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, nullptr);
	return EXIT_FAILURE;
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	Settings::argv = strfilled(lpCmdLine) ? CommandLineToArgvW(sstow(lpCmdLine).data(), &Settings::argc) : nullptr;
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	Settings::argv = strfilled(pCmdLine) ? CommandLineToArgvW(pCmdLine, &Settings::argc) : nullptr;
#endif
#else
int main(int argc, char** argv) {
	Settings::argv = argv;
	Settings::argc = argc;
#endif
	int rc = EXIT_SUCCESS;
	setlocale(LC_CTYPE, "");
	try {
#ifdef WITH_ICU
		Strcomp::init();
#endif
#if defined(_WIN32) && defined(WITH_FTP)
		NetConnection::initWsa();
#endif
#if SDL_VERSION_ATLEAST(3, 0, 0)
		SDL_SetAppMetadata(WindowSys::title, "1.0.0", "org.kk.vertiread");
		SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition");
#else
#if SDL_VERSION_ATLEAST(2, 0, 18)
		SDL_SetHint(SDL_HINT_APP_NAME, WindowSys::title);
#endif
#if SDL_VERSION_ATLEAST(2, 0, 22)
		SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
#endif
		SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
		SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 10)
		SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
		SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
		SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
		SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
		SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#ifndef _WIN32
		if (Settings::hasFlag(Settings::flagCompositor))
			SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
		if (sdlFailed(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)))
			throw std::runtime_error(SDL_GetError());
		if (IMG_InitFlags imgFlags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP; IMG_Init(imgFlags) != imgFlags) {
			const char* err = SDL_GetError();
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", strfilled(err) ? err : "Failed to initialize all image formats");
		}
		for (SDL_EventType it : unusedEvents)
#if SDL_VERSION_ATLEAST(3, 0, 0)
			SDL_SetEventEnabled(it, SDL_FALSE);
#else
			SDL_EventState(it, SDL_DISABLE);
#endif
		SDL_RegisterEvents(SDL_EventType(SDL_USEREVENT_MAX) - SDL_USEREVENT);
#if !SDL_VERSION_ATLEAST(3, 0, 0)
		SDL_StopTextInput();
#endif
		World::winSys()->init();
		SDL_PumpEvents();
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_USEREVENT - 1);
		World::winSys()->exec();
	} catch (const std::runtime_error& e) {
		rc = printError(e.what());
#ifdef NDEBUG
	} catch (...) {
		rc = printError("Unknown fatal error");
#endif
	}
	World::winSys()->cleanup();
	IMG_Quit();
	SDL_Quit();
#if defined(_WIN32) && defined(WITH_FTP)
	NetConnection::cleanupWsa();
#endif
#ifdef WITH_ICU
	Strcomp::free();
#endif
#if defined(CAN_SECRET) || defined(CAN_POPPLER)
	closeGlib();
#endif
#ifdef _WIN32
	LocalFree(Settings::argv);
#endif
	return rc;
}
