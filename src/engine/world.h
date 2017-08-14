#pragma once

#include "engine.h"

// class that makes accessing stuff easier
class World {
public:
	static Engine* engine();
	static AudioSys* audioSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Library* library();
	static Program* program();

	static vector<string> args;	// arguments from main()
private:
	static Engine base;			// the thing on which everything runs
};
