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
	void close() { run = false; }

	float getDSec() const { return dSec; }
	vec2i displayResolution() const;
	void setWindowPos(const vec2i& pos);
	void moveCursor(const vec2i& mov);
	void setFullscreen(bool on);
	void setResolution(const vec2i& res);
	void setRenderer(const string& name);
	void resetSettings();

	FileSys* getFileSys() { return fileSys.get(); }
	DrawSys* getDrawSys() { return drawSys.get(); }
	InputSys* getInputSys() { return inputSys.get(); }
	Program* getProgram() { return program.get(); }
	Scene* getScene() { return scene.get(); }
	Settings* getSets() { return sets.get(); }

private:
	uptr<FileSys> fileSys;
	uptr<DrawSys> drawSys;
	uptr<InputSys> inputSys;
	uptr<Program> program;
	uptr<Scene> scene;
	uptr<Settings> sets;

	bool run;			// whether the loop in which the program runs should continue
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	SDL_Window* window;

	void init();
	void exec();
	void cleanup();

	void createWindow();
	void destroyWindow();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& window);
};
