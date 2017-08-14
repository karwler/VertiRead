#pragma once

#include "audioSys.h"
#include "filer.h"
#include "inputSys.h"
#include "windowSys.h"
#include "scene.h"

// the thing that runs and kills everything
class Engine {
public:
	Engine();

	void start();
	void close();
	void cleanup();

	float getDSec() const;
	AudioSys* getAudioSys();
	InputSys* getInputSys();
	WindowSys* getWindowSys();
	Scene* getScene();

private:
	kk::sptr<AudioSys> audioSys;
	kk::sptr<InputSys> inputSys;
	kk::sptr<WindowSys> winSys;
	kk::sptr<Scene> scene;

	bool run;	// whether the loop in which the program runs should continue
	float dSec;	// delta seconds, aka the time between each iteration of the above mentioned loop

	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
};
