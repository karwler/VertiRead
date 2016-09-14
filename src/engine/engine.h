#pragma once

#include "audioSys.h"
#include "filer.h"
#include "inputSys.h"
#include "windowSys.h"
#include "scene.h"

class Engine {
public:
	void Run();
	void Close();
	void Cleanup();

	void SetRedrawNeeded(uint8 count=1);
	float deltaSeconds() const;
	AudioSys* getAudioSys() const;
	InputSys* getInputSys() const;
	WindowSys* getWindowSys() const;
	Scene* getScene() const;

private:
	kk::sptr<AudioSys> audioSys;
	kk::sptr<InputSys> inputSys;
	kk::sptr<WindowSys> winSys;
	kk::sptr<Scene> scene;

	bool run;
	uint8 redraws;
	float dSec;

	void HandleEvent(const SDL_Event& event);
};
