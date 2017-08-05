#pragma once

#include "shaderSys.h"
#include "utils/settings.h"

class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());
	~WindowSys();

	void CreateWindow();
	void DestroyWindow();

	bool ShowMouse() const;
	void ShowMouse(bool on);
	void WindowEvent(const SDL_WindowEvent& window);

	void SetRedrawNeeded();
	void DrawObjects(const vector<Object*>& objects, const Popup* popup);
	static vec2i DesktopResolution();
	vec2i Resolution() const;
	vec2i Position() const;

	const VideoSettings& Settings() const;
	void Renderer(const string& name);
	void Fullscreen(bool on);
	void Font(const string& font);
	void Theme(const string& theme);

private:
	ShaderSys shaderSys;
	SDL_Window* window;
	bool redraw;
	bool showMouse;
	VideoSettings sets;
};
