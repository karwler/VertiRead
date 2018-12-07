#pragma once

#include "drawSys.h"
#include "inputSys.h"
#include "prog/program.h"
#include "scene.h"

// handles window events and contains video settings
class WindowSys {
public:
	WindowSys();

	int start();
	void close();

	float getDSec() const;
	vec2i displayResolution() const;
	void setWindowPos(const vec2i& pos);
	void moveCursor(const vec2i& mov);
	void setFullscreen(bool on);
	void setResolution(const vec2i& res);
	void setRenderer(const string& name);
	void resetSettings();

	FileSys* getFileSys();
	DrawSys* getDrawSys();
	InputSys* getInputSys();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();

private:
	uptr<FileSys> fileSys;
	uptr<DrawSys> drawSys;
	uptr<InputSys> inputSys;
	uptr<Program> program;
	uptr<Scene> scene;
	uptr<Settings> sets;

	SDL_Window* window;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue

	void init();
	void exec();
	void cleanup();

	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& window);
	void setDSec(uint32& oldTicks);
};

inline void WindowSys::close() {
	run = false;
}

inline float WindowSys::getDSec() const {
	return dSec;
}

inline FileSys* WindowSys::getFileSys() {
	return fileSys.get();
}

inline DrawSys* WindowSys::getDrawSys() {
	return drawSys.get();
}

inline InputSys* WindowSys::getInputSys() {
	return inputSys.get();
}

inline Program* WindowSys::getProgram() {
	return program.get();
}

inline Scene* WindowSys::getScene() {
	return scene.get();
}

inline Settings* WindowSys::getSets() {
	return sets.get();
}

inline vec2i WindowSys::displayResolution() const {
	SDL_DisplayMode mode;
	return SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(window), &mode) ? 0 : vec2i(mode.w, mode.h);
}

inline void WindowSys::setWindowPos(const vec2i& pos) {
	SDL_SetWindowPosition(window, pos.x, pos.y);
}
