#pragma once

#include "drawSys.h"
#include "inputSys.h"
#include "prog/program.h"
#include "scene.h"

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "VertiRead";
private:
	static constexpr char fileIcon[] = "icon.ico";
	static constexpr vec2i windowMinSize = { 500, 300 };
	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;

	uptr<FileSys> fileSys;
	uptr<DrawSys> drawSys;
	uptr<InputSys> inputSys;
	uptr<Program> program;
	uptr<Scene> scene;
	uptr<Settings> sets;

	SDL_Window* window;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;			// whether the loop in which the program runs should continue

public:
	WindowSys();

	int start();
	void close();

	void pushEvent(UserCode code, void* data1 = nullptr, void* data2 = nullptr) const;
	float getDSec() const;
	vec2i displayResolution() const;
	void setWindowPos(vec2i pos);
	void moveCursor(vec2i mov);
	void toggleOpacity();
	void setFullscreen(bool on);
	void setResolution(vec2i res);
	void setRenderer(const string& name);
	void resetSettings();

	FileSys* getFileSys();
	DrawSys* getDrawSys();
	InputSys* getInputSys();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();

private:
	void init();
	void exec();
	void cleanup();

	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
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
	return !SDL_GetDesktopDisplayMode(window ? SDL_GetWindowDisplayIndex(window) : 0, &mode) ? vec2i(mode.w, mode.h) : INT_MAX;
}

inline void WindowSys::setWindowPos(vec2i pos) {
	SDL_SetWindowPosition(window, pos.x, pos.y);
}
