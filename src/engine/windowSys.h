#pragma once

#include "utils/settings.h"

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr float fallbackDpi = 96.f;
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
	float winDpi;
	float dSec = 0.f;		// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run = true;		// whether the loop in which the program runs should continue

public:
	int start();
	void close();

	float getDSec() const;
	float getWinDpi() const;
	ivec2 mousePos() const;
	ivec2 winViewOffset(uint32 wid) const;
	ivec2 displayResolution() const;
	void moveCursor(ivec2 mov);
	void toggleOpacity();
	void setScreenMode(Settings::Screen sm);
	void setWindowPos(ivec2 pos);
	void setResolution(ivec2 res);
	void resetSettings();
	void recreateWindows();

	FileSys* getFileSys();
	DrawSys* getDrawSys();
	InputSys* getInputSys();
	Program* getProgram();
	Scene* getScene();
	Settings* getSets();

private:
	void init();
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

inline float WindowSys::getDSec() const {
	return dSec;
}

inline FileSys* WindowSys::getFileSys() {
	return fileSys;
}

inline DrawSys* WindowSys::getDrawSys() {
	return drawSys;
}

inline InputSys* WindowSys::getInputSys() {
	return inputSys;
}

inline Program* WindowSys::getProgram() {
	return program;
}

inline Scene* WindowSys::getScene() {
	return scene;
}

inline Settings* WindowSys::getSets() {
	return sets;
}

inline float WindowSys::getWinDpi() const {
	return winDpi;
}
