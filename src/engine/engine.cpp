#include "world.h"

Engine::Engine() :
	run(true)
{}

int Engine::Run() {
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();

	World::PrintInfo();
	Filer::CheckDirectories();
	scene = new Scene;

	winSys = new WindowSys;
	winSys->SetWindow(Filer::LoadVideoSettings());

	audioSys = new AudioSys;
	audioSys->Initialize(Filer::LoadAudioSettings());
	audioSys->setPlaylist(Filer::LoadPlaylist("test"));
	
	inputSys = new InputSys;
	inputSys->Settings(Filer::LoadControlsSettings());
	kptr<SDL_Event> event = new SDL_Event;
	
	scene->SwitchMenu(EMenu::books);
	uint oldTime = SDL_GetTicks();

	while (run) {
		uint newTime = SDL_GetTicks();
		dSec = (newTime - oldTime) / 1000.f;
		oldTime = newTime;

		audioSys->Tick(dSec);
		if (SDL_PollEvent(event))
			HandleEvent(event);
	}
	winSys->DestroyWindow();
	audioSys->Cleanup();
	TTF_Quit();
	SDL_Quit();
	return 0;
}

void Engine::Close() {
	Filer::SaveSettings(GeneralSettings());
	Filer::SaveSettings(winSys->Settings());
	Filer::SaveSettings(audioSys->Settings());
	Filer::SaveSettings(inputSys->Settings());
	run = false;
}

void Engine::HandleEvent(SDL_Event* event) {
	switch (event->type) {
	case SDL_KEYDOWN:
		inputSys->KeypressEvent(event->key);
		break;
	case SDL_MOUSEMOTION:
		inputSys->MouseMotionEvent(event->motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
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
