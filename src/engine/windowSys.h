#pragma once

#include "utils/settings.h"
#include "utils/scrollAreas.h"
#include "utils/popups.h"

class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());
	~WindowSys();

	void CreateWindow();
	void DestroyWindow();
	bool ShowMouse() const;
	void ShowMouse(bool on);
	void MoveMouse(const vec2i& mPos);
	void DrawObjects(const vector<Object*>& objects);
	void WindowEvent(const SDL_WindowEvent& window);
	
	SDL_Renderer* Renderer() const;
	const VideoSettings& Settings() const;
	void Renderer(const string& name);
	vec2i Resolution() const;			// returns the actual resolution (not the settings)
	static vec2i DesktopResolution();	// screen resolution
	void Fullscreen(bool on);
	void Font(const string& font);
	void Theme(const string& theme);

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	VideoSettings sets;
	bool showMouse;

	void CreateRenderer();
	void DestroyRenderer();
	int GetRenderDriverIndex();

	void PassDrawObject(Object* obj);
	void DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void DrawObject(LineEditor* obj);
	void DrawObject(ListBox* obj);
	void DrawObject(TableBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);
	void DrawObject(Popup* obj);
	
	void PassDrawItem(size_t id, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop={ 0, 0, 0, 0 });
	void DrawItem(Checkbox* item, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(Switchbox* item, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(LineEdit* item, ScrollAreaX1* parent, const SDL_Rect& rect, SDL_Rect crop);
	void DrawItem(KeyGetter* item, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop);

	void DrawRect(const SDL_Rect& rect, EColor color);
	void DrawImage(const Image& img, const SDL_Rect& crop = { 0, 0, 0, 0 });
	void DrawText(const Text& txt, const SDL_Rect& crop = { 0, 0, 0, 0 });
};
