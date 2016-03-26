#pragma once

#include "audioSys.h"
#include "filer.h"
#include "inputSys.h"
#include "windowSys.h"
#include "scene.h"

class Engine {
public:
	Engine();

	int Run();
	void Close();
	void Cleanup();

	void SetRedrawNeeded();
	float deltaSeconds() const;
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
	bool redraw;
	float dSec;

	void HandleEvent(SDL_Event* event);
};
