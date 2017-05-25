#include "engine.h"

Engine::Engine() :
	run(false),
	redraw(false),
	dSec(0.f)
{}

void Engine::Run() {
	// initialize all components
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER))
		throw Exception("couldn't initialize SDL\n" + string(SDL_GetError()), 1);
	if (TTF_Init())
		throw Exception("couldn't initialize fonts\n" + string(SDL_GetError()), 2);
	PrintInfo();

	winSys = new WindowSys(Filer::LoadVideoSettings());
	inputSys = new InputSys(Filer::LoadControlsSettings());
	audioSys = new AudioSys(Filer::LoadAudioSettings());
	audioSys->Initialize();

	winSys->CreateWindow();
	scene = new Scene(Filer::LoadGeneralSettings());		// initializes program and library
	winSys->SetIcon(scene->getLibrary()->getTex("icon")->surface);

	// initialize values
	scene->getProgram()->Event_OpenBookList();
	run = true;
	redraw = true;
	SDL_Event event;
	uint32 oldTime = SDL_GetTicks();

	while (run) {
		// get delta seconds
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// draw scene if requested
		if (redraw) {
			redraw = false;
			winSys->DrawObjects(scene->Objects());
		}

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
    if (audioSys)
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
	switch (event.type) {
	case SDL_KEYDOWN:
		inputSys->KeypressEvent(event.key);
		break;
	case SDL_JOYBUTTONDOWN:
		inputSys->JoystickButtonEvent(event.jbutton);
		break;
	case SDL_JOYHATMOTION:
		inputSys->JoystickHatEvent(event.jhat);
		break;
	case SDL_JOYAXISMOTION:
		inputSys->JoystickAxisEvent(event.jaxis);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		inputSys->GamepadButtonEvent(event.cbutton);
		break;
	case SDL_CONTROLLERAXISMOTION:
		inputSys->GamepadAxisEvent(event.caxis);
		break;
	case SDL_MOUSEMOTION:
		inputSys->MouseMotionEvent(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
		inputSys->MouseButtonDownEvent(event.button);
		break;
	case SDL_MOUSEBUTTONUP:
		inputSys->MouseButtonUpEvent(event.button);
		break;
	case SDL_MOUSEWHEEL:
		inputSys->MouseWheelEvent(event.wheel);
		break;
	case SDL_TEXTINPUT:
		inputSys->TextEvent(event.text);
		break;
	case SDL_WINDOWEVENT:
		winSys->WindowEvent(event.window);
		break;
	case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
		inputSys->UpdateControllers();
		break;
	case SDL_DROPFILE:
		scene->getProgram()->FileDropEvent(event.drop.file);
		break;
	case SDL_QUIT:
		Close();
	}
}

void Engine::SetRedrawNeeded() {
	redraw = true;
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
