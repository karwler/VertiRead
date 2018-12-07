#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
public:
	static FileSys* fileSys();
	static DrawSys* drawSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Program* program();
	static ProgState* state();
	static Browser* browser();
	static Settings* sets();
	static const vector<string>& getArgs();
	static const string& getArg(sizt id);

	template <class F, class... A> static void prun(F func, A... args);
	template <class F, class... A> static void srun(F func, A... args);

#ifdef _WIN32
#ifdef _DEBUG
	static void setArgs(int argc, wchar** argv);
#else
	static void setArgs(wchar* argstr);
#endif
#else
	static void setArgs(int argc, char** argv);
#endif
private:
	static WindowSys windowSys;			// the thing on which everything runs
	static vector<string> args;

	template <class C, class F, class... A> static void run(C* obj, F func, A... args);
};

inline FileSys* World::fileSys() {
	return windowSys.getFileSys();
}

inline DrawSys* World::drawSys() {
	return windowSys.getDrawSys();
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

inline const vector<string>& World::getArgs() {
	return args;
}

inline const string& World::getArg(sizt id) {
	return args[id];
}

template <class F, class... A>
void World::prun(F func, A... args) {
	return run(program(), func, args...);
}

template <class F, class... A>
void World::srun(F func, A... args) {
	return run(state(), func, args...);
}

template <class C, class F, class... A>
void World::run(C* obj, F func, A... args) {
	if (func)
		(obj->*func)(args...);
}
