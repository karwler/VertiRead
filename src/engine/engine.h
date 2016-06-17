#pragma once

#include "audioSys.h"
#include "filer.h"
#include "inputSys.h"
#include "windowSys.h"
#include "scene.h"

class Engine {
public:
	Engine();

	void Run();
	void Close();
	void Cleanup();

	void SetRedrawNeeded(byte count=1);
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
	byte redraws;
	float dSec;

	void HandleEvent(SDL_Event* event);
};
