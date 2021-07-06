#pragma once

#include "windowSys.h"
#include "prog/program.h"

// class that makes accessing stuff easier and holds command line arguments
class World {
private:
	static inline WindowSys windowSys;			// the thing on which everything runs
	static inline vector<string> vals;
	static inline uset<string> flags;
	static inline umap<string, string> opts;

	static inline const uset<string> checkFlags;
	static inline const umap<string, string> checkOpts;

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

	static const vector<string>& getVals();
	static bool hasFlag(const char* key);
	static const char* getOpt(const char* key);
	template <class C, class F> static void setArgs(int argc, C** argv, F conv);

	template <class F, class... A> static void prun(F func, A... args);
	template <class F, class... A> static void srun(F func, A... args);
private:
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

inline Downloader* World::downloader() {
	return windowSys.getProgram()->getDownloader();
}

inline Browser* World::browser() {
	return windowSys.getProgram()->getBrowser();
}

inline Settings* World::sets() {
	return windowSys.getSets();
}

inline const vector<string>& World::getVals() {
	return vals;
}

inline bool World::hasFlag(const char* key) {
	return flags.count(key);
}

inline const char* World::getOpt(const char* key) {
	umap<string, string>::const_iterator it = opts.find(key);
	return it != opts.end() ? it->second.c_str() : nullptr;
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
