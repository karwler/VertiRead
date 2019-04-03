#include "windowSys.h"

WindowSys::WindowSys() :
	window(nullptr),
	dSec(0.f),
	run(true)
{}

int WindowSys::start() {
	try {
		init();
		exec();
	} catch (const std::runtime_error& e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), window);
	} catch (...) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Unknown error.", window);
	}
	cleanup();
	return 0;
}

void WindowSys::init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(string("Failed to initialize SDL:\n") + SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error(string("Failed to initialize fonts:\n") + TTF_GetError());
	SDL_StopTextInput();	// for some reason TextInput is on

	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
	if (!(flags & IMG_INIT_JPG))
		std::cerr << "failed to initialize JPG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_PNG))
		std::cerr << "failed to initialize PNG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_TIF))
		std::cerr << "failed to initialize TIF:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_WEBP))
		std::cerr << "failed to initialize WEBP:\n" << IMG_GetError() << std::endl;

	fileSys.reset(new FileSys);
	sets.reset(fileSys->loadSettings());
	createWindow();
	inputSys.reset(new InputSys);
	scene.reset(new Scene);
	program.reset(new Program);
	program->start();
}

void WindowSys::exec() {
	// the loop :o
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		setDSec(oldTime);
		drawSys->drawWidgets();

		// handle events
		inputSys->tick(dSec);
		scene->tick(dSec);

		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		for (SDL_Event event; SDL_PollEvent(&event) && SDL_GetTicks() < timeout;)
			handleEvent(event);
	}
	fileSys->saveSettings(sets.get());
	fileSys->saveBindings(inputSys->getBindings());
}

void WindowSys::cleanup() {
	program.reset();
	scene.reset();
	inputSys.reset();
	destroyWindow();
	fileSys.reset();

	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	sets->resolution = sets->resolution.clamp(windowMinSize, displayResolution());
	if (!(window = SDL_CreateWindow(title, defaultWindowPos.x, defaultWindowPos.y, sets->resolution.x, sets->resolution.y, windowFlags | (sets->maximized ? SDL_WINDOW_MAXIMIZED : 0) | (sets->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))))
		throw std::runtime_error(string("Failed to create window:\n") + SDL_GetError());

	// minor stuff
	if (SDL_Surface* icon = IMG_Load(fileIcon)) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	drawSys.reset(new DrawSys(window, sets->getRendererIndex()));
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
	case SDL_MOUSEMOTION:
		inputSys->eventMouseMotion(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
		scene->onMouseDown(vec2i(event.button.x, event.button.y), event.button.button, event.button.clicks);
		break;
	case SDL_MOUSEBUTTONUP:
		scene->onMouseUp(vec2i(event.button.x, event.button.y), event.button.button, event.button.clicks);
		break;
	case SDL_MOUSEWHEEL:
		scene->onMouseWheel(vec2i(event.wheel.x, -event.wheel.y));
		break;
	case SDL_KEYDOWN:
		inputSys->eventKeypress(event.key);
		break;
	case SDL_JOYBUTTONDOWN:
		inputSys->eventJoystickButton(event.jbutton);
		break;
	case SDL_JOYHATMOTION:
		inputSys->eventJoystickHat(event.jhat);
		break;
	case SDL_JOYAXISMOTION:
		inputSys->eventJoystickAxis(event.jaxis);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		inputSys->eventGamepadButton(event.cbutton);
		break;
	case SDL_CONTROLLERAXISMOTION:
		inputSys->eventGamepadAxis(event.caxis);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
	case SDL_DROPFILE:
		program->getState()->eventFileDrop(event.drop.file);
		SDL_free(event.drop.file);
		break;
	case SDL_JOYDEVICEADDED:
		inputSys->addController(event.jdevice.which);
		break;
	case SDL_JOYDEVICEREMOVED:
		inputSys->removeController(event.jdevice.which);
		break;
	case SDL_USEREVENT:
		program->eventUser(event.user);
		break;
	case SDL_QUIT:
		program->eventTryExit();
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_RESIZED:
		if (uint32 flags = SDL_GetWindowFlags(window); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP) && !(sets->maximized = flags & SDL_WINDOW_MAXIMIZED))	// update settings if needed
			SDL_GetWindowSize(window, &sets->resolution.x, &sets->resolution.y);
		scene->onResize();
		break;
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

void WindowSys::pushEvent(UserCode code, void* data1, void* data2) const {
	SDL_Event event;
	event.user = { SDL_USEREVENT, SDL_GetTicks(), SDL_GetWindowID(window), int32(code), data1, data2 };
	SDL_PushEvent(&event);
}

void WindowSys::setDSec(uint32& oldTicks) {
	uint32 newTime = SDL_GetTicks();
	dSec = float(newTime - oldTicks) / ticksPerSec;
	oldTicks = newTime;
}

void WindowSys::moveCursor(const vec2i& mov) {
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

void WindowSys::setResolution(const vec2i& res) {
	sets->resolution = res.clamp(windowMinSize, displayResolution());
	SDL_SetWindowSize(window, res.x, res.y);
}

void WindowSys::setRenderer(const string& name) {
	sets->renderer = name;
	createWindow();
	scene->resetLayouts();
}

void WindowSys::resetSettings() {
	sets.reset(new Settings);
	createWindow();
	scene->resetLayouts();
}
