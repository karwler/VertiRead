#include "engine.h"

Engine::Engine() :
	run(false),
	dSec(0.f)
{}

void Engine::Run() {
	// initialize all components
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER))
		throw Exception("couldn't initialize SDL\n" + string(SDL_GetError()), 1);
	if (TTF_Init())
		throw Exception("couldn't initialize fonts\n" + string(SDL_GetError()), 2);

	winSys = new WindowSys(Filer::LoadVideoSettings());
	inputSys = new InputSys(Filer::LoadControlsSettings());
	audioSys = new AudioSys(Filer::LoadAudioSettings());

	winSys->CreateWindow();
	scene = new Scene(Filer::LoadGeneralSettings());		// initializes program and library

	// initialize values
	scene->getProgram()->Event_OpenBookList();
	run = true;
	SDL_Event event;
	uint32 oldTime = SDL_GetTicks();

	while (run) {
		// get delta seconds
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// draw scene if requested
		winSys->DrawObjects(scene->Objects(), scene->getPopup());

		// handle events
		audioSys->Tick(dSec);
		scene->Tick(dSec);

		uint32 timeout = SDL_GetTicks() + 50;
		while (SDL_PollEvent(&event) && timeout - SDL_GetTicks() > 0)
			HandleEvent(event);
	}
}

void Engine::Close() {
	// save all settings before quitting
	Filer::SaveSettings(scene->Settings());
	Filer::SaveSettings(winSys->Settings());
	Filer::SaveSettings(audioSys->Settings());
	Filer::SaveSettings(inputSys->Settings());
	
	run = false;
}

void Engine::Cleanup() {
	scene.clear();
	audioSys.clear();
	inputSys.clear();
	winSys.clear();

	TTF_Quit();
	SDL_Quit();
}

void Engine::HandleEvent(const SDL_Event& event) {
	// pass event to a specific handler
	if (event.type == SDL_KEYDOWN)
		inputSys->KeypressEvent(event.key);
	else if (event.type == SDL_JOYBUTTONDOWN)
		inputSys->JoystickButtonEvent(event.jbutton);
	else if (event.type == SDL_JOYHATMOTION)
		inputSys->JoystickHatEvent(event.jhat);
	else if (event.type == SDL_JOYAXISMOTION)
		inputSys->JoystickAxisEvent(event.jaxis);
	else if (event.type == SDL_CONTROLLERBUTTONDOWN)
		inputSys->GamepadButtonEvent(event.cbutton);
	else if (event.type == SDL_CONTROLLERAXISMOTION)
		inputSys->GamepadAxisEvent(event.caxis);
	else if (event.type == SDL_MOUSEMOTION)
		inputSys->MouseMotionEvent(event.motion);
	else if (event.type == SDL_MOUSEBUTTONDOWN)
		inputSys->MouseButtonDownEvent(event.button);
	else if (event.type == SDL_MOUSEBUTTONUP)
		inputSys->MouseButtonUpEvent(event.button);
	else if (event.type == SDL_MOUSEWHEEL)
		inputSys->MouseWheelEvent(event.wheel);
	else if (event.type == SDL_TEXTINPUT)
		inputSys->TextEvent(event.text);
	else if (event.type == SDL_WINDOWEVENT)
		winSys->WindowEvent(event.window);
	else if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED)
		inputSys->UpdateControllers();
	else if (event.type == SDL_DROPFILE)
		scene->getProgram()->FileDropEvent(event.drop.file);
	else if (event.type == SDL_QUIT)
		Close();
}

float Engine::deltaSeconds() const {
	return dSec;
}

AudioSys* Engine::getAudioSys() {
	return audioSys;
}

InputSys* Engine::getInputSys() {
	return inputSys;
}

WindowSys* Engine::getWindowSys() {
	return winSys;
}

Scene* Engine::getScene() {
	return scene;
}
