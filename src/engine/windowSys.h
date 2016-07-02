#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"

class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());

	void CreateWindow();
	void DestroyWindow();
	void SetIcon(SDL_Surface* icon);
	void DrawObjects(const vector<Object*>& objects);
	void WindowEvent(const SDL_WindowEvent& window);
	
	SDL_Renderer* Renderer() const;
	VideoSettings Settings() const;
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

	void CreateRenderer();
	void DestroyRenderer();
	int GetRenderDriverIndex();

	void PassDrawObject(Object* obj);
	void DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void DrawObject(LineEditor* obj);
	void DrawObject(ListBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);
	void DrawObject(Popup* obj);
	
	void PassDrawItem(int id, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop={ 0, 0, 0, 0 });
	void DrawItem(Checkbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(Switchbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(LineEdit* item, ListBox* parent, const SDL_Rect& rect, SDL_Rect crop);
	void DrawItem(KeyGetter* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);

	void DrawRect(const SDL_Rect& rect, EColor color);
	void DrawImage(const Image& img, const SDL_Rect& crop = { 0, 0, 0, 0 });
	void DrawText(const Text& txt, const SDL_Rect& crop = { 0, 0, 0, 0 });
};
