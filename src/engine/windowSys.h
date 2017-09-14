#pragma once

#include "drawSys.h"
#include "utils/settings.h"

// handles window events and contains video settings
class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());
	~WindowSys();

	void createWindow();
	void destroyWindow();

	bool getShowMouse() const;
	void setShowMouse(bool on);
	void eventWindow(const SDL_WindowEvent& window);

	void setRedrawNeeded();
	void drawWidgets(const vector<Widget*>& widgets, const Popup* popup);
	static vec2i displayResolution();
	vec2i resolution() const;
	vec2i position() const;
	DrawSys* getDrawSys();

	const VideoSettings& getSettings() const;
	void setRenderer(const string& name);
	void setFullscreen(bool on);
	void setFont(const string& font);
	void setTheme(const string& theme);

private:
	DrawSys drawSys;
	SDL_Window* window;

	VideoSettings sets;
	bool redraw;		// whether window contents need to be redrawn
	bool showMouse;
};
