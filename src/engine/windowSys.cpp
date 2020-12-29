#include "windowSys.h"

WindowSys::WindowSys() :
	window(nullptr),
	dSec(0.f),
	run(true)
{}

int WindowSys::start() {
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
	program.reset();
	scene.reset();
	inputSys.reset();
	destroyWindow();
	fileSys.reset();

	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
	return rc;
}

void WindowSys::init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(string("Failed to initialize SDL:\n") + SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("Failed to initialize fonts:\n") + TTF_GetError());
	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
	if (!(flags & IMG_INIT_JPG))
		std::cerr << "failed to initialize JPG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_PNG))
		std::cerr << "failed to initialize PNG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_TIF))
		std::cerr << "failed to initialize TIF:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_WEBP))
		std::cerr << "failed to initialize WEBP:\n" << IMG_GetError() << std::endl;

#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#if SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_EventState(SDL_DISPLAYEVENT, SDL_DISABLE);
#endif
	SDL_EventState(SDL_TEXTEDITING, SDL_DISABLE);
	SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARRECORD, SDL_DISABLE);
	SDL_EventState(SDL_MULTIGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
#if SDL_VERSION_ATLEAST(2, 0, 9)
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
#endif
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
	SDL_StopTextInput();

	fileSys = std::make_unique<FileSys>();
	sets.reset(fileSys->loadSettings());
	createWindow();
	inputSys = std::make_unique<InputSys>();
	scene = std::make_unique<Scene>();
	program = std::make_unique<Program>();
	program->start();
}

void WindowSys::exec() {
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / ticksPerSec;
		oldTime = newTime;

		drawSys->drawWidgets(scene.get(), inputSys->mouseLast);
		inputSys->tick();
		scene->tick(dSec);

		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		do {
			SDL_Event event;
			if (!SDL_PollEvent(&event))
				break;
			handleEvent(event);
		} while (SDL_GetTicks() < timeout);
	}
	fileSys->saveSettings(sets.get());
	fileSys->saveBindings(inputSys->getBindings());
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	sets->resolution = clamp(sets->resolution, windowMinSize, displayResolution());
	if (!(window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, SDL_WINDOW_RESIZABLE | (sets->maximized ? SDL_WINDOW_MAXIMIZED : 0) | (sets->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))))
		throw std::runtime_error(string("Failed to create window:\n") + SDL_GetError());

	// visual stuff
	if (SDL_Surface* icon = IMG_Load(fileSys->windowIconPath().c_str())) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	drawSys = std::make_unique<DrawSys>(window, sets->getRendererIndex(), sets.get(), fileSys.get());
	SDL_SetWindowMinimumSize(window, windowMinSize.x, windowMinSize.y);	// for some reason this function has to be called after the renderer is created
}

void WindowSys::destroyWindow() {
	if (window) {
		drawSys.reset();
		SDL_DestroyWindow(window);
		window = nullptr;
	}
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
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
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
		if (uint32 flags = SDL_GetWindowFlags(window); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && !(sets->maximized = flags & SDL_WINDOW_MAXIMIZED))	// update settings if needed
			SDL_GetWindowSize(window, &sets->resolution.x, &sets->resolution.y);
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
	case SDL_WINDOWEVENT_CLOSE:
		program->eventTryExit();
	}
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
	SDL_SetWindowFullscreen(window, on ? SDL_GetWindowFlags(window) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(window) & uint32(~SDL_WINDOW_FULLSCREEN_DESKTOP));
}

void WindowSys::setResolution(ivec2 res) {
	sets->resolution = clamp(res, windowMinSize, displayResolution());
	SDL_SetWindowSize(window, res.x, res.y);
}

void WindowSys::setRenderer(const string& name) {
	sets->renderer = name;
	createWindow();
	scene->resetLayouts();
}

void WindowSys::resetSettings() {
	sets = std::make_unique<Settings>(fileSys->getDirSets(), fileSys->getAvailableThemes());
	createWindow();
	scene->resetLayouts();
}
