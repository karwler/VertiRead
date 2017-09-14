#include "base.h"
#include "filer.h"

Base::Base() :
	run(false),
	dSec(0.f)
{}

void Base::start() {
	// initialize all components
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER))
		throw Exception("couldn't initialize SDL\n" + string(SDL_GetError()), 1);
	if (TTF_Init())
		throw Exception("couldn't initialize fonts\n" + string(SDL_GetError()), 2);
	
	winSys = new WindowSys(Filer::getVideoSettings());
	inputSys = new InputSys(Filer::getControlsSettings());
	audioSys = new AudioSys(Filer::getAudioSettings());

	winSys->createWindow();
	scene = new Scene();	// initializes program and library

	// initialize values
	scene->getProgram().eventOpenBookList();
	run = true;
	uint32 oldTime = SDL_GetTicks();

	// the loop :o
	while (run) {
		// get delta seconds
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// draw scene if requested
		winSys->drawWidgets(scene->getWidgets(), scene->getPopup());

		// handle events
		audioSys->tick(dSec);
		inputSys->tick(dSec);
		scene->tick(dSec);

		SDL_Event event;
		uint32 timeout = SDL_GetTicks() + Default::eventCheckTimeout;
		while (SDL_PollEvent(&event) && SDL_GetTicks() < timeout)
			handleEvent(event);
	}
}

void Base::close() {
	// save all settings before quitting
	Filer::saveSettings(scene->getLibrary().getSettings());
	Filer::saveSettings(winSys->getSettings());
	Filer::saveSettings(audioSys->getSettings());
	Filer::saveSettings(inputSys->getSettings());
	
	run = false;
}

void Base::cleanup() {
	scene.clear();
	audioSys.clear();
	inputSys.clear();
	winSys.clear();

	TTF_Quit();
	SDL_Quit();
}

void Base::handleEvent(const SDL_Event& event) {
	if (event.type == SDL_KEYDOWN)
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
	else if (event.type == SDL_MOUSEMOTION)
		inputSys->eventMouseMotion(event.motion);
	else if (event.type == SDL_MOUSEBUTTONDOWN)
		inputSys->eventMouseButtonDown(event.button);
	else if (event.type == SDL_MOUSEBUTTONUP)
		inputSys->eventMouseButtonUp(event.button);
	else if (event.type == SDL_MOUSEWHEEL)
		inputSys->eventMouseWheel(event.wheel);
	else if (event.type == SDL_TEXTINPUT)
		inputSys->eventText(event.text);
	else if (event.type == SDL_WINDOWEVENT)
		winSys->eventWindow(event.window);
	else if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED)
		inputSys->updateControllers();
	else if (event.type == SDL_DROPFILE)
		scene->getProgram().eventFileDrop(event.drop.file);
	else if (event.type == SDL_QUIT)
		close();
}

float Base::getDSec() const {
	return dSec;
}

AudioSys* Base::getAudioSys() {
	return audioSys;
}

InputSys* Base::getInputSys() {
	return inputSys;
}

WindowSys* Base::getWindowSys() {
	return winSys;
}

Scene* Base::getScene() {
	return scene;
}
