#include "windowSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "prog/program.h"
#include "prog/progs.h"

int WindowSys::start() {
	fileSys = nullptr;
	inputSys = nullptr;
	program = nullptr;
	scene = nullptr;
	sets = nullptr;
	window = nullptr;
	int rc = EXIT_SUCCESS;
	try {
		init();
		exec();
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), window);
		rc = EXIT_FAILURE;
#ifdef NDEBUG
	} catch (...) {
		std::cerr << "unknown error" << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "unknown error", window);
		rc = EXIT_FAILURE;
#endif
	}
	delete program;
	delete scene;
	delete inputSys;
	destroyWindow();
	delete fileSys;
	delete sets;

	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
	return rc;
}

void WindowSys::init() {
#if SDL_VERSION_ATLEAST(2, 0, 22)
	SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_SetHint(SDL_HINT_APP_NAME, "VertiRead");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error("Failed to initialize SDL:\n"s + SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error("Failed to initialize fonts:\n"s + TTF_GetError());
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP | IMG_INIT_JXL | IMG_INIT_AVIF);
#else
	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
#endif
	if (!(flags & IMG_INIT_JPG))
		std::cerr << "failed to initialize JPG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_PNG))
		std::cerr << "failed to initialize PNG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_TIF))
		std::cerr << "failed to initialize TIF:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_WEBP))
		std::cerr << "failed to initialize WEBP:\n" << IMG_GetError() << std::endl;
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (!(flags & IMG_INIT_JXL))
		std::cerr << "failed to initialize JXL:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_AVIF))
		std::cerr << "failed to initialize AVIF:\n" << IMG_GetError() << std::endl;
#endif
	SDL_EventState(SDL_LOCALECHANGED, SDL_DISABLE);
	SDL_EventState(SDL_DISPLAYEVENT, SDL_DISABLE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
	SDL_EventState(SDL_KEYUP, SDL_DISABLE);
	SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_DISABLE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADDOWN, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERSENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARRECORD, SDL_DISABLE);
	SDL_EventState(SDL_MULTIGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
	if (SDL_RegisterEvents(1) == UINT32_MAX)
		throw std::runtime_error("Failed to register application events:\n"s + SDL_GetError());
	SDL_StopTextInput();

	fileSys = new FileSys;
	sets = fileSys->loadSettings();
	createWindow();
	inputSys = new InputSys;
	scene = new Scene;
	program = new Program;
	program->start();

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_USEREVENT - 1);
}

void WindowSys::exec() {
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / ticksPerSec;
		oldTime = newTime;

		drawSys->drawWidgets(scene, inputSys->mouseLast);
		inputSys->tick();
		scene->tick(dSec);

		SDL_Event event;
		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		do {
			if (!SDL_PollEvent(&event))
				break;
			handleEvent(event);
		} while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout));
	}
	fileSys->saveSettings(sets);
	fileSys->saveBindings(inputSys->getBindings());
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	sets->resolution = clamp(sets->resolution, windowMinSize, displayResolution());
	if (window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (sets->maximized ? SDL_WINDOW_MAXIMIZED : 0) | (sets->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)); !window)
		throw std::runtime_error("Failed to create window:\n"s + SDL_GetError());

	// visual stuff
	if (SDL_Surface* icon = IMG_Load((fileSys->dirIcons() / "vertiread.svg").u8string().c_str())) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	drawSys = new DrawSys(window, sets->getRendererInfo(), sets, fileSys);
	SDL_SetWindowMinimumSize(window, windowMinSize.x, windowMinSize.y);	// for some reason this function has to be called after the renderer is created
	if (SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), nullptr, nullptr, &winDpi))
		winDpi = fallbackDpi;
}

void WindowSys::destroyWindow() {
	delete drawSys;
	drawSys = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;
}

void WindowSys::handleEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_QUIT:
		program->eventTryExit();
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
	case SDL_KEYDOWN:
		inputSys->eventKeypress(event.key);
		break;
	case SDL_TEXTEDITING:
		scene->onCompose(event.edit.text);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
#if SDL_VERSION_ATLEAST(2, 0, 22)
	case SDL_TEXTEDITING_EXT:
		scene->onCompose(event.editExt.text);
		SDL_free(event.editExt.text);
		break;
#endif
	case SDL_MOUSEMOTION:
		inputSys->eventMouseMotion(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
		inputSys->eventMouseButtonDown(event.button);
		break;
	case SDL_MOUSEBUTTONUP:
		inputSys->eventMouseButtonUp(event.button);
		break;
	case SDL_MOUSEWHEEL:
		inputSys->eventMouseWheel(event.wheel);
		break;
	case SDL_JOYAXISMOTION:
		inputSys->eventJoystickAxis(event.jaxis);
		break;
	case SDL_JOYHATMOTION:
		inputSys->eventJoystickHat(event.jhat);
		break;
	case SDL_JOYBUTTONDOWN:
		inputSys->eventJoystickButton(event.jbutton);
		break;
	case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
		inputSys->reloadControllers();
		break;
	case SDL_CONTROLLERAXISMOTION:
		inputSys->eventGamepadAxis(event.caxis);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		inputSys->eventGamepadButton(event.cbutton);
		break;
	case SDL_FINGERDOWN:
		inputSys->eventFingerDown(event.tfinger);
		break;
	case SDL_FINGERUP:
		inputSys->eventFingerUp(event.tfinger);
		break;
	case SDL_FINGERMOTION:
		inputSys->eventFingerMove(event.tfinger);
		break;
	case SDL_DROPFILE:
		program->getState()->eventFileDrop(fs::u8path(event.drop.file));
		SDL_free(event.drop.file);
		break;
	case SDL_DROPTEXT:
		scene->onText(event.drop.file);
		SDL_free(event.drop.file);
		break;
	case SDL_USEREVENT:
		program->eventUser(event.user);
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_RESIZED:
		if (uint32 flags = SDL_GetWindowFlags(window); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
			if (sets->maximized = flags & SDL_WINDOW_MAXIMIZED; !sets->maximized)
				sets->resolution = ivec2(winEvent.data1, winEvent.data2);
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
	case SDL_WINDOWEVENT_CLOSE:
		program->eventTryExit();
		break;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED: {
		float newDpi = fallbackDpi;
		if (int rc = SDL_GetDisplayDPI(winEvent.data1, nullptr, nullptr, &newDpi); !rc && newDpi != winDpi) {
#else
	case SDL_WINDOWEVENT_MOVED: {
		float newDpi = fallbackDpi;
		if (int rc = SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window), nullptr, nullptr, &newDpi); !rc && newDpi != winDpi) {
#endif
			winDpi = newDpi;
			scene->onResize();
		}
	} }
}

void WindowSys::moveCursor(ivec2 mov) {
	int px, py;
	SDL_GetMouseState(&px, &py);
	SDL_WarpMouseInWindow(window, px + mov.x, py + mov.y);
}

void WindowSys::toggleOpacity() {
	if (float val; !SDL_GetWindowOpacity(window, &val))
		SDL_SetWindowOpacity(window, val < 1.f ? 1.f : 0.f);
	else
		SDL_MinimizeWindow(window);
}

void WindowSys::setFullscreen(bool on) {
	sets->fullscreen = on;
	SDL_SetWindowFullscreen(window, on ? SDL_GetWindowFlags(window) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(window) & ~SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void WindowSys::setResolution(ivec2 res) {
	sets->resolution = clamp(res, windowMinSize, displayResolution());
	SDL_SetWindowSize(window, res.x, res.y);
}

void WindowSys::setRenderer(string_view name) {
	sets->renderer = name;
	createWindow();
	scene->resetLayouts();
}

void WindowSys::resetSettings() {
	delete sets;
	sets = new Settings(fileSys->getDirSets(), fileSys->getAvailableThemes());
	createWindow();
	scene->resetLayouts();
}
