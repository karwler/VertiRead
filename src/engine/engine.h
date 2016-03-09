#pragma once

#include "audioSys.h"
#include "inputSys.h"
#include "windowSys.h"
#include "scene.h"

class Engine {
public:
	Engine();

	int Run();
	void Close();

	float deltaSeconds() const;
	int frameRate() const;
	AudioSys* getAudioSys() const;
	InputSys* getInputSys() const;
	WindowSys* getWindowSys() const;
	Scene* getScene() const;

private:
	kptr<AudioSys> audioSys;
	kptr<InputSys> inputSys;
	kptr<WindowSys> winSys;
	kptr<Scene> scene;
	bool run;
	float dSec;

	void HandleEvent(SDL_Event* event);
	void Cleanup();
};
