#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
private:
	static WindowSys windowSys;			// the thing on which everything runs
	static vector<string> argVals;
	static uset<string> argFlags;
	static umap<string, string> argOpts;

public:
	static FileSys* fileSys();
	static DrawSys* drawSys();
	static InputSys* inputSys();
	static WindowSys* winSys();
	static Scene* scene();
	static Program* program();
	static ProgState* state();
	static Downloader* downloader();
	static Browser* browser();
	static Settings* sets();

	static const vector<string>& getArgVals();
	static const uset<string>& getArgFlags();
	static const umap<string, string>& getArgOpts();

#ifdef _WIN32
#ifdef _DEBUG
	static void init(int argc, wchar** argv);
#else
	static void init(wchar* argstr);
#endif
#else
	static void init(int argc, char** argv);
#endif

	template <class F, class... A> static void prun(F func, A... args);
	template <class F, class... A> static void srun(F func, A... args);
private:
	template <class C, class F, class... A> static void run(C* obj, F func, A... args);

#ifdef _WIN32
	static void setArgs(int i, int argc, wchar** argv);
#else
	static void setArgs(int i, int argc, char** argv);
#endif
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

inline Downloader* World::downloader() {
	return windowSys.getProgram()->getDownloader();
}

inline Browser* World::browser() {
	return windowSys.getProgram()->getBrowser();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

inline const vector<string>&World::getArgVals() {
	return argVals;
}

inline const uset<string>&World::getArgFlags() {
	return argFlags;
}

inline const umap<string, string>&World::getArgOpts() {
	return argOpts;
}

template <class F, class... A>
void World::prun(F func, A... args) {
	run(program(), func, args...);
}

template <class F, class... A>
void World::srun(F func, A... args) {
	run(state(), func, args...);
}

template <class C, class F, class... A>
void World::run(C* obj, F func, A... args) {
	if (func)
		(obj->*func)(args...);
}
#if defined(_WIN32) && defined(_DEBUG)
inline void World::init(int argc, wchar** argv) {
	setArgs(1, argc, argv);
}
#elif !defined(_WIN32)
inline void World::init(int argc, char** argv) {
	setArgs(1, argc, argv);
}
#endif
