#include "engine.h"

Engine::Engine() :
	run(true)
{}

void Engine::Run() {
	// initialize all components
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
		throw Exception("SDL couldn't be initialized" + string(SDL_GetError()), 1);
	if (TTF_Init())
		throw Exception("fonts couldn't be initialized" + string(SDL_GetError()), 2);
	PrintInfo();

	audioSys = new AudioSys(Filer::LoadAudioSettings());
	winSys = new WindowSys(Filer::LoadVideoSettings());
	inputSys = new InputSys(Filer::LoadControlsSettings());
	kptr<SDL_Event> event = new SDL_Event;

	winSys->SetWindow();
	scene = new Scene(Filer::LoadGeneralSettings());		// initializes program and library
	winSys->SetIcon(scene->getLibrary()->getTex("icon")->surface);

	// initialize values
	scene->getProgram()->Event_OpenBookList();
	redraws = 8;	// linux sometimes can't keep up with the window, which is why there need to be a few redraw calls at the start
	uint oldTime = SDL_GetTicks();

	while (run) {
		// get delta seconds
		uint newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// draw scene if requested
		if (redraws != 0) {
			redraws--;
			winSys->DrawObjects(scene->Objects());
		}

		// handle events
		audioSys->Tick(dSec);
		scene->Tick();
		inputSys->Tick();

		uint timeout = SDL_GetTicks() + 50;
		while (SDL_PollEvent(event) && timeout - SDL_GetTicks() > 0)
			HandleEvent(event);
	}
	Cleanup();
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
	scene.reset();
	inputSys.reset();
	winSys.reset();
	audioSys.reset();

	TTF_Quit();
	SDL_Quit();
}

void Engine::HandleEvent(const SDL_Event& event) {
	// pass event to a specific handler
	switch (event.type) {
	case SDL_KEYDOWN:
		inputSys->KeypressEvent(event.key);
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
	case SDL_WINDOWEVENT:
		winSys->WindowEvent(event.window);
		break;
	case SDL_TEXTINPUT:
		inputSys->TextEvent(event.text);
		break;
	case SDL_DROPFILE:
		scene->getProgram()->FileDropEvent(event.drop.file);
		break;
	case SDL_QUIT:
		Close();
	}
}

void Engine::SetRedrawNeeded(uint8 count) {
	redraws = count;
}

float Engine::deltaSeconds() const {
	return dSec;
}

AudioSys* Engine::getAudioSys() const {
	return audioSys;
}

InputSys* Engine::getInputSys() const {
	return inputSys;
}

WindowSys* Engine::getWindowSys() const {
	return winSys;
}

Scene* Engine::getScene() const {
	return scene;
}
