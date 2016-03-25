#include "engine.h"

Engine::Engine() :
	run(true),
	redraw(true)
{}

int Engine::Run() {
	// initialize all components
	if (SDL_Init(SDL_INIT_EVERYTHING)) {
		cerr << "SDL couldn't be initialized" << endl << SDL_GetError() << endl;
		throw 1;
	}
	if (TTF_Init()) {
		cerr << "fonts couldn't be initialized" << endl << TTF_GetError() << endl;
		throw 2;
	}

	PrintInfo();
	Filer::CheckDirectories();
	scene = new Scene;

	audioSys = new AudioSys(Filer::LoadAudioSettings());
	winSys = new WindowSys(scene, Filer::LoadVideoSettings());
	inputSys = new InputSys(Filer::LoadControlsSettings());
	kptr<SDL_Event> event = new SDL_Event;

	// initialize scene and timer
	scene->getProgram()->Event_OpenBookList();
	uint oldTime = SDL_GetTicks();

	while (run) {
		// get delta seconds
		uint newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// draw scene if requested
		if (redraw)
			winSys->DrawObjects(scene->Objects());

		// handle events
		audioSys->Tick(dSec);
		scene->Tick();
		inputSys->Tick();

		uint timeout = SDL_GetTicks() + 50;
		while (SDL_PollEvent(event) && timeout - SDL_GetTicks() > 0)
			HandleEvent(event);
	}
	Cleanup();
	return 0;
}

void Engine::Close() {
	// save all settings before quitting
	Filer::SaveSettings(GeneralSettings());
	Filer::SaveSettings(winSys->Settings());
	Filer::SaveSettings(audioSys->Settings());
	Filer::SaveSettings(inputSys->Settings());
	run = false;
}

void Engine::Cleanup() {
	if (audioSys)
		audioSys.reset();
	if (winSys)
		winSys.reset();
	TTF_Quit();
	SDL_Quit();
}

void Engine::HandleEvent(SDL_Event* event) {
	// pass event to a specific handler
	switch (event->type) {
	case SDL_KEYDOWN: case SDL_KEYUP:
		inputSys->KeypressEvent(event->key);
		break;
	case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
		inputSys->MouseButtonEvent(event->button);
		break;
	case SDL_MOUSEWHEEL:
		inputSys->MouseWheelEvent(event->wheel);
		break;
	case SDL_WINDOWEVENT:
		winSys->WindowEvent(event->window);
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

int Engine::frameRate() const {
	return 1/dSec;
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
