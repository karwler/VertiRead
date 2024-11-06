#pragma once

#include "windowSys.h"

// makes accessing stuff easier
class World {
private:
	static inline WindowSys windowSys;	// the thing on which everything runs;

public:
	static DrawSys* drawSys() noexcept { return windowSys.getDrawSys(); }
	static FileSys* fileSys() noexcept { return windowSys.getFileSys(); }
	static InputSys* inputSys() noexcept { return windowSys.getInputSys(); }
	static Program* program() noexcept { return windowSys.getProgram(); }
	static Scene* scene() noexcept { return windowSys.getScene(); }
	static Settings* sets() noexcept { return windowSys.getSets(); }
	static WindowSys* winSys() noexcept { return &windowSys; }
};
