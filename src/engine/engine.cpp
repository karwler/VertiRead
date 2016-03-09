#include "world.h"

Engine::Engine() :
	run(true)
{}

int Engine::Run() {
	// initialize all components
	if (SDL_Init(SDL_INIT_EVERYTHING))
		return 1;
	if (TTF_Init()) {
		SDL_Quit();
		return 2;
	}
	
	World::PrintInfo();
	Filer::CheckDirectories();
	scene = new Scene;

	winSys = new WindowSys;
	if (!winSys->SetWindow(Filer::LoadVideoSettings())) {
		Cleanup();
		return 3;
	}

	audioSys = new AudioSys;
	if (!audioSys->Initialize(Filer::LoadAudioSettings())) {
		Cleanup();
		return 4;
	}
	audioSys->setPlaylist(Filer::LoadPlaylist("test"));
	
	inputSys = new InputSys;
	inputSys->Settings(Filer::LoadControlsSettings());
	kptr<SDL_Event> event = new SDL_Event;
	
	// initialize scene and timer
	scene->SwitchMenu(EMenu::books);
	uint oldTime = SDL_GetTicks();

	while (run) {
		// get delta seconds
		uint newTime = SDL_GetTicks();
		dSec = (newTime - oldTime) / 1000.f;
		oldTime = newTime;

		// handle events
		audioSys->Tick(dSec);
		if (SDL_PollEvent(event))
			HandleEvent(event);
	}
	Cleanup();
	return 0;
}

void Engine::Close() {
	// save all settings before closing
	Filer::SaveSettings(GeneralSettings());
	Filer::SaveSettings(winSys->Settings());
	Filer::SaveSettings(audioSys->Settings());
	Filer::SaveSettings(inputSys->Settings());
	run = false;
}

void Engine::HandleEvent(SDL_Event* event) {
	// pass event to a specific handler
	switch (event->type) {
	case SDL_KEYDOWN: case SDL_KEYUP:
		inputSys->KeypressEvent(event->key);
		break;
	case SDL_MOUSEMOTION:
		inputSys->MouseMotionEvent(event->motion);
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

void Engine::Cleanup() {
	if (audioSys)
		audioSys->Cleanup();
	if (winSys)
		winSys->DestroyWindow();
	TTF_Quit();
	SDL_Quit();
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
