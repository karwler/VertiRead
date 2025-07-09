#pragma once

#include "utils/settings.h"
#ifdef WITH_SDL3
#include <SDL3/SDL_events.h>
#else
#include <SDL_events.h>
#endif

// handles window events and contains video settings
class WindowSys {
public:
	static constexpr char title[] = "VertiRead";
private:
	static constexpr ivec2 windowMinSize = ivec2(100);
	static constexpr uint32 eventCheckTimeout = 50;

	FileSys* fileSys = nullptr;
	DrawSys* drawSys = nullptr;
	InputSys* inputSys = nullptr;
	Program* program = nullptr;
	Scene* scene = nullptr;
	uptr<Settings> sets;
	uptr<SDL_Window*[]> windows;
	float dSec;
	uint16 refreshMs;
	bool run = true;
	bool active = true;	// TODO: restrict also when focus lost
	uint8 numWindows = 0;
#if defined(WITH_SDL3) && defined( __linux__)
	bool isWayland;
#endif

public:
	void init();
	void cleanup() noexcept;
	void exec();
	void close() noexcept;

	float getDSec() const noexcept { return dSec; }
	ivec2 mousePos() const noexcept;
	ivec2 winViewOffset(SDL_WindowID wid) const noexcept;
	bool hasTransparentWindow() const noexcept;
	array<vec4, Settings::defaultColors.size()> loadColors(string_view name);
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
#ifdef WITH_SDL3
	SDL_PropertiesID initWindow(uint8 wincnt, const array<vec4, Settings::defaultColors.size()>& colors);
#else
	uint32 initWindow(uint8 wincnt);
#endif
	void createSingleWindow(SDL_Surface* icon, const array<vec4, Settings::defaultColors.size()>& colors);
	void createMultiWindow(SDL_Surface* icon, const array<vec4, Settings::defaultColors.size()>& colors);
	void destroyWindows() noexcept;
	void handleEvent(SDL_Event& event);
	void eventWindow(const SDL_WindowEvent& winEvent);
	void eventDisplay(const SDL_DisplayEvent& dspEvent);
	void setRefreshTime() noexcept;
};

inline void WindowSys::close() noexcept {
	run = false;
}

inline bool WindowSys::hasTransparentWindow() const noexcept {
#ifdef WITH_SDL3
	return SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_TRANSPARENT;
#else
	return false;
#endif
}
