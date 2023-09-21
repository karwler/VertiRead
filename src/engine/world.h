#pragma once

#include "drawSys.h"
#include "windowSys.h"
#include "prog/program.h"

// class that makes accessing stuff easier and holds command line arguments
class World {
private:
	static inline WindowSys windowSys;	// the thing on which everything runs;

public:
	static FileSys* fileSys();
	static DrawSys* drawSys();
	static Renderer* renderer();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Program* program();
	static ProgState* state();
	static Browser* browser();
	static Settings* sets();

	template <MemberFunction F, class... A> static void prun(F func, A... args);
	template <MemberFunction F, class... A> static void srun(F func, A... args);
private:
	template <Class C, MemberFunction F, class... A> static void run(C* obj, F func, A... args);
};

inline FileSys* World::fileSys() {
	return windowSys.getFileSys();
}

inline DrawSys* World::drawSys() {
	return windowSys.getDrawSys();
}

inline Renderer* World::renderer() {
	return windowSys.getDrawSys()->getRenderer();
}

inline InputSys* World::inputSys() {
	return windowSys.getInputSys();
}

inline WindowSys* World::winSys() {
	return &windowSys;
}

inline Scene* World::scene() {
	return windowSys.getScene();
}

inline Program* World::program() {
	return windowSys.getProgram();
}

inline ProgState* World::state() {
	return windowSys.getProgram()->getState();
}

inline Browser* World::browser() {
	return windowSys.getProgram()->getBrowser();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

template <MemberFunction F, class... A>
void World::prun(F func, A... args) {
	run(program(), func, args...);
}

template <MemberFunction F, class... A>
void World::srun(F func, A... args) {
	run(state(), func, args...);
}

template <Class C, MemberFunction F, class... A>
void World::run(C* obj, F func, A... args) {
	if (func)
		(obj->*func)(args...);
}
