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

	float deltaSeconds() const;
	AudioSys* getAudioSys();
	InputSys* getInputSys();
	WindowSys* getWindowSys();
	Scene* getScene();

private:
	kk::sptr<AudioSys> audioSys;
	kk::sptr<InputSys> inputSys;
	kk::sptr<WindowSys> winSys;
	kk::sptr<Scene> scene;

	bool run;
	float dSec;

	void HandleEvent(const SDL_Event& event);
};
