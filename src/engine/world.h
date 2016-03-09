#pragma once

#include "engine.h"

class World {
public:
	static kptr<Engine> engine;

	static void PrintInfo();

	static AudioSys* audioSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Program* program();

};
