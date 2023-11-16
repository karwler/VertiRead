#pragma once

#include "utils/settings.h"

// handles window events and contains video settings
class WindowSys {
private:
	static constexpr char title[] = "VertiRead";
	static constexpr ivec2 windowMinSize = { 500, 300 };
	static constexpr uint32 eventCheckTimeout = 50;
	static constexpr float ticksPerSec = 1000.f;

	FileSys* fileSys;
	DrawSys* drawSys;
	InputSys* inputSys;
	Program* program;
	Scene* scene;
	Settings* sets;
	umap<int, SDL_Window*> windows;
	float dSec = 0.f;		// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run = true;		// whether the loop in which the program runs should continue

public:
	int start(vector<string>&& cmdVals, uset<string>&& cmdFlags);
	void close();

	float getDSec() const { return dSec; }
	ivec2 mousePos();
	ivec2 winViewOffset(uint32 wid);
	ivec2 displayResolution() const;
	void moveCursor(ivec2 mov);
	void toggleOpacity();
	void setScreenMode(Settings::Screen sm);
	void reposizeWindow(ivec2 dres, ivec2 wsiz);
	void resetSettings();
	void recreateWindows();

	FileSys* getFileSys() { return fileSys; }
	DrawSys* getDrawSys() { return drawSys; }
	InputSys* getInputSys() { return inputSys; }
	Program* getProgram() { return program; }
	Scene* getScene() { return scene; }
	Settings* getSets() { return sets; }

private:
	void init(vector<string>&& cmdVals, uset<string>&& cmdFlags);
	void exec();

	void createWindow();
	void createSingleWindow(uint32 flags, SDL_Surface* icon);
	void createMultiWindow(uint32 flags, SDL_Surface* icon);
	void destroyWindows();
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void eventDisplay(const SDL_DisplayEvent& dspEvent);
};

inline void WindowSys::close() {
	run = false;
}
