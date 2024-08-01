#pragma once

#include "windowSys.h"

// makes accessing stuff easier
class World {
private:
	static inline WindowSys windowSys;	// the thing on which everything runs;

public:
	static DrawSys* drawSys() { return windowSys.getDrawSys(); }
	static FileSys* fileSys() { return windowSys.getFileSys(); }
	static InputSys* inputSys() { return windowSys.getInputSys(); }
	static Program* program() { return windowSys.getProgram(); }
	static Scene* scene() { return windowSys.getScene(); }
	static Settings* sets() { return windowSys.getSets(); }
	static WindowSys* winSys() { return &windowSys; }
};
