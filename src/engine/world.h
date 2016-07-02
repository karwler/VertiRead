#pragma once

#include "engine.h"

class World {
public:
	static Engine* engine();
	static AudioSys* audioSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Library* library();
	static Program* program();

	static void PlaySound(const string& sound);

	static vector<string> args;
private:
	static Engine base;
};
