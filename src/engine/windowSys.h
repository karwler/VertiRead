#pragma once

#include "utils/settings.h"
#include <SDL_events.h>

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "VertiRead";
private:
	static constexpr ivec2 windowMinSize = ivec2(500, 300);
	static constexpr uint32 eventCheckTimeout = 50;

	FileSys* fileSys = nullptr;
	DrawSys* drawSys = nullptr;
	InputSys* inputSys = nullptr;
	Program* program = nullptr;
	Scene* scene = nullptr;
	uptr<Settings> sets;
	vector<SDL_Window*> windows;
	float dSec;			// delta seconds, aka the time between each iteration of the above mentioned loop
	bool run = true;	// whether the loop in which the program runs should continue

public:
	void init();
	void cleanup() noexcept;
	void exec();
	void close() noexcept;

	float getDSec() const noexcept { return dSec; }
	ivec2 mousePos() const noexcept;
	ivec2 winViewOffset(uint32 wid) const noexcept;
	ivec2 displayResolution() const noexcept;
	void moveCursor(ivec2 mov) noexcept;
	void toggleOpacity() noexcept;
	void setScreenMode(Settings::Screen sm);
	void resetSettings();
	void recreateWindows();

	FileSys* getFileSys() noexcept { return fileSys; }
	DrawSys* getDrawSys() noexcept { return drawSys; }
	InputSys* getInputSys() noexcept { return inputSys; }
	Program* getProgram() noexcept { return program; }
	Scene* getScene() noexcept { return scene; }
	Settings* getSets() noexcept { return sets.get(); }

private:
	void createWindow();
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_PropertiesID initWindow(size_t numWindows);
#else
	uint32 initWindow(size_t numWindows);
#endif
	void createSingleWindow(SDL_Surface* icon);
	void createMultiWindow(SDL_Surface* icon);
	void destroyWindows() noexcept;
	void handleEvent(const SDL_Event& event);	// pass events to their specific handlers
	void eventWindow(const SDL_WindowEvent& winEvent);
	void eventDisplay();
};

inline void WindowSys::close() noexcept {
	run = false;
}
