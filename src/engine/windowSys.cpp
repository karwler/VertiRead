#include "world.h"

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
		throw std::runtime_error("Couldn't initialize SDL:\n" + string(SDL_GetError()));
	if (TTF_Init())
		throw std::runtime_error("Couldn't initialize fonts:\n" + string(SDL_GetError()));
	SDL_StopTextInput();	// for some reason TextInput is on

	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
	if (!(flags & IMG_INIT_JPG))
		std::cerr << "Couldn't initialize JPG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_PNG))
		std::cerr << "Couldn't initialize PNG:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_TIF))
		std::cerr << "Couldn't initialize TIF:\n" << IMG_GetError() << std::endl;
	if (!(flags & IMG_INIT_WEBP))
		std::cerr << "Couldn't initialize WEBP:\n" << IMG_GetError() << std::endl;

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

		uint32 timeout = SDL_GetTicks() + Default::eventCheckTimeout;
		for (SDL_Event event; SDL_PollEvent(&event) && SDL_GetTicks() < timeout;)
			handleEvent(event);
	}
	program->getState()->eventClosing();
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
	uint32 flags = Default::windowFlags;
	if (sets->maximized)
		flags |= SDL_WINDOW_MAXIMIZED;
	if (sets->fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (!(window = SDL_CreateWindow(Default::titleDefault, Default::windowPos.x, Default::windowPos.y, sets->resolution.x, sets->resolution.y, flags)))
		throw std::runtime_error("Couldn't create window:\n" + string(SDL_GetError()));

	// minor stuff
	if (SDL_Surface* icon = IMG_Load(string(World::fileSys()->getDirExec() + Default::fileIcon).c_str())) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	drawSys.reset(new DrawSys(window, sets->getRendererIndex()));
	SDL_SetWindowMinimumSize(window, Default::windowMinSize.x, Default::windowMinSize.y);	// for some reason this function has to be called after the renderer is created
}

void WindowSys::destroyWindow() {
	if (window) {
		drawSys.reset();
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

void WindowSys::handleEvent(const SDL_Event& event) {
	if (event.type == SDL_MOUSEMOTION)
		inputSys->eventMouseMotion(event.motion);
	else if (event.type == SDL_MOUSEBUTTONDOWN)
		scene->onMouseDown(vec2i(event.button.x, event.button.y), event.button.button, event.button.clicks);
	else if (event.type == SDL_MOUSEBUTTONUP)
		scene->onMouseUp(vec2i(event.button.x, event.button.y), event.button.button);
	else if (event.type == SDL_MOUSEWHEEL)
		scene->onMouseWheel(vec2i(event.wheel.x, -event.wheel.y));
	else if (event.type == SDL_KEYDOWN)
		inputSys->eventKeypress(event.key);
	else if (event.type == SDL_JOYBUTTONDOWN)
		inputSys->eventJoystickButton(event.jbutton);
	else if (event.type == SDL_JOYHATMOTION)
		inputSys->eventJoystickHat(event.jhat);
	else if (event.type == SDL_JOYAXISMOTION)
		inputSys->eventJoystickAxis(event.jaxis);
	else if (event.type == SDL_CONTROLLERBUTTONDOWN)
		inputSys->eventGamepadButton(event.cbutton);
	else if (event.type == SDL_CONTROLLERAXISMOTION)
		inputSys->eventGamepadAxis(event.caxis);
	else if (event.type == SDL_TEXTINPUT)
		scene->onText(event.text.text);
	else if (event.type == SDL_WINDOWEVENT)
		eventWindow(event.window);
	else if (event.type == SDL_DROPFILE) {
		program->getState()->eventFileDrop(event.drop.file);
		SDL_free(event.drop.file);
	} else if (event.type == SDL_JOYDEVICEADDED)
		inputSys->addController(event.jdevice.which);
	else if (event.type == SDL_JOYDEVICEREMOVED)
		inputSys->removeController(event.jdevice.which);
	else if (event.type == SDL_QUIT)
		close();
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	if (winEvent.event == SDL_WINDOWEVENT_RESIZED) {
		if (uint32 flags = SDL_GetWindowFlags(window); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP))	// update settings if needed
			if (!(sets->maximized = flags & SDL_WINDOW_MAXIMIZED))
				SDL_GetWindowSize(window, &sets->resolution.x, &sets->resolution.y);
		scene->onResize();
	} else if (winEvent.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		scene->onResize();
	else if (winEvent.event == SDL_WINDOWEVENT_LEAVE)
		scene->onMouseLeave();
	else if (winEvent.event == SDL_WINDOWEVENT_CLOSE)
		close();
}

void WindowSys::setDSec(uint32& oldTicks) {
	uint32 newTime = SDL_GetTicks();
	dSec = float(newTime - oldTicks) / 1000.f;
	oldTicks = newTime;
}

vec2i WindowSys::displayResolution() const {
	SDL_DisplayMode mode;
	return SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(window), &mode) ? 0 : vec2i(mode.w, mode.h);
}

void WindowSys::setWindowPos(const vec2i& pos) {
	SDL_SetWindowPosition(window, pos.x, pos.y);
}

void WindowSys::moveCursor(const vec2i& mov) {
	int px, py;
	SDL_GetMouseState(&px, &py);
	SDL_WarpMouseInWindow(window, px + mov.x, py + mov.y);
}

void WindowSys::setFullscreen(bool on) {
	sets->fullscreen = on;
	SDL_SetWindowFullscreen(window, on ? SDL_GetWindowFlags(window) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(window) & ~SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void WindowSys::setResolution(const vec2i& res) {
	sets->resolution = res;
	SDL_SetWindowSize(window, res.x, res.y);
}

void WindowSys::setRenderer(const string& name) {
	sets->renderer = name;
	createWindow();
}

void WindowSys::resetSettings() {
	sets.reset(new Settings);
	createWindow();
}
