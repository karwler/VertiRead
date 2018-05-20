#include "world.h"

WindowSys::WindowSys() :
	run(true),
	dSec(0.f),
	window(nullptr)
{}

int WindowSys::start() {
	try {
		init();
		exec();
	} catch (string str) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", str.c_str(), window);
	} catch (...) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Unknown error.", window);
	}
	cleanup();
	return 0;
}

void WindowSys::init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw "Couldn't initialize SDL:\n" + string(SDL_GetError());
	if (TTF_Init())
		throw "Couldn't initialize fonts:\n" + string(SDL_GetError());

	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
	if (!(flags & IMG_INIT_JPG))
		cerr << "Couldn't initialize JPG:" << endl << IMG_GetError() << endl;
	if (!(flags & IMG_INIT_PNG))
		cerr << "Couldn't initialize PNG:" << endl << IMG_GetError() << endl;
	if (!(flags & IMG_INIT_TIF))
		cerr << "Couldn't initialize TIF:" << endl << IMG_GetError() << endl;
	if (!(flags & IMG_INIT_WEBP))
		cerr << "Couldn't initialize WEBP:" << endl << IMG_GetError() << endl;

	SDL_StopTextInput();	// for some reason TextInput is on
	sets = Filer::getSettings();

	// check if all (more or less) necessary files and directories exist
	if (Filer::fileType(Filer::dirSets) != FTYPE_DIR)
		if (!Filer::mkDir(Filer::dirSets))
			cerr << "Couldn't create settings directory." << endl;
	if (Filer::fileType(Filer::dirExec + Default::fileThemes) != FTYPE_FILE)
		cerr << "Couldn't find themes file." << endl;
	if (Filer::fileType(Filer::dirLangs) != FTYPE_DIR)
		cerr << "Couldn't find language directory." << endl;
	if (Filer::fileType(Filer::dirTexs) != FTYPE_DIR)
		cerr << "Couldn't find texture directory." << endl;

	createWindow();
	inputSys.reset(new InputSys);
	scene.reset(new Scene);
	program.reset(new Program);
}

void WindowSys::exec() {
	scene->resetLayouts();
	uint32 oldTime = SDL_GetTicks();

	// the loop :o
	while (run) {
		// get delta seconds
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		drawSys->drawWidgets();

		// handle events
		inputSys->tick(dSec);
		scene->tick(dSec);

		SDL_Event event;
		uint32 timeout = SDL_GetTicks() + Default::eventCheckTimeout;
		while (SDL_PollEvent(&event) && SDL_GetTicks() < timeout)
			handleEvent(event);
	}
	program->getState()->eventClosing();
	Filer::saveSettings(sets);
	Filer::saveBindings(inputSys->getBindings());
}

void WindowSys::cleanup() {
	program.reset();
	scene.reset();
	inputSys.reset();
	destroyWindow();

	IMG_Quit();
	TTF_Quit();
	SDL_Quit();
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	uint32 flags = Default::windowFlags;
	if (sets.maximized)
		flags |= SDL_WINDOW_MAXIMIZED;
	if (sets.fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow(Default::titleDefault, Default::windowPos.x, Default::windowPos.y, sets.resolution.x, sets.resolution.y, flags);
	if (!window)
		throw "Couldn't create window:\n" + string(SDL_GetError());

	// minor stuff
	SDL_Surface* icon = IMG_Load(string(Filer::dirExec + Default::fileIcon).c_str());
	if (icon) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}

	drawSys.reset(new DrawSys(window, sets.getRendererIndex()));
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
		// update settings if needed
		uint32 flags = SDL_GetWindowFlags(window);
		if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			sets.maximized = flags & SDL_WINDOW_MAXIMIZED;
			if (!sets.maximized)
				sets.resolution = resolution();
		}
		scene->onResize();
	} else if (winEvent.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		scene->onResize();
	else if (winEvent.event == SDL_WINDOWEVENT_LEAVE)
		scene->onMouseLeave();
	else if (winEvent.event == SDL_WINDOWEVENT_CLOSE)
		close();
}

vec2i WindowSys::resolution() const {
	vec2i res;
	SDL_GetWindowSize(window, &res.x, &res.y);
	return res;
}

void WindowSys::moveCursor(const vec2i& mov) {
	vec2i pos;
	SDL_GetMouseState(&pos.x, &pos.y);
	SDL_WarpMouseInWindow(window, pos.x + mov.x, pos.y + mov.y);
}

void WindowSys::setFullscreen(bool on) {
	sets.fullscreen = on;
	SDL_SetWindowFullscreen(window, on ? SDL_GetWindowFlags(window) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(window) & ~SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void WindowSys::setRenderer(const string& name) {
	sets.renderer = name;
	createWindow();
}
