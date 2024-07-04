#pragma once

#include "utils/settings.h"
#include <SDL_events.h>

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
	vector<SDL_Window*> windows;
	float dSec;		// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run;		// whether the loop in which the program runs should continue

public:
	int start(vector<string>&& cmdVals, uset<string>&& cmdFlags) noexcept;
	void close();

	float getDSec() const { return dSec; }
	ivec2 mousePos() const noexcept;
	ivec2 winViewOffset(uint32 wid) const noexcept;
	ivec2 displayResolution() const noexcept;
	void moveCursor(ivec2 mov) noexcept;
	void toggleOpacity() noexcept;
	void setScreenMode(Settings::Screen sm);
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
	uint32 initWindow(bool shared);
	void createSingleWindow(uint32 flags, SDL_Surface* icon);
	void createMultiWindow(uint32 flags, SDL_Surface* icon);
	void destroyWindows() noexcept;
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void eventDisplay(const SDL_DisplayEvent& dspEvent);
};

inline void WindowSys::close() {
	run = false;
}
