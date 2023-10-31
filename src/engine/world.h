#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier and holds command line arguments
class World {
private:
	static inline WindowSys windowSys;	// the thing on which everything runs;

public:
	static DrawSys* drawSys();
	static FileSys* fileSys();
	static InputSys* inputSys();
	static Program* program();
	static Scene* scene();
	static Settings* sets();
	static WindowSys* winSys();
};

inline DrawSys* World::drawSys() {
	return windowSys.getDrawSys();
}

inline FileSys* World::fileSys() {
	return windowSys.getFileSys();
}

inline InputSys* World::inputSys() {
	return windowSys.getInputSys();
}

inline Program* World::program() {
	return windowSys.getProgram();
}

inline Scene* World::scene() {
	return windowSys.getScene();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

inline WindowSys* World::winSys() {
	return &windowSys;
}
