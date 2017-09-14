#pragma once

#include "base.h"

// class that makes accessing stuff easier
class World {
public:
	static Base* base();
	static AudioSys* audioSys();
	static DrawSys* drawSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Library* library();
	static Program* program();

	static vector<string> args;	// arguments from main()
private:
	static Base engine;			// the thing on which everything runs
};
