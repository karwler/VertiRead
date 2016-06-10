#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"

class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());
	~WindowSys();

	void SetWindow();
	void DestroyWindow();
	void SetIcon(SDL_Surface* icon);
	void DrawObjects(const vector<Object*>& objects);
	
	SDL_Renderer* Renderer() const;
	VideoSettings Settings() const;
	vec2i Resolution() const;
	static vec2i DesktopResolution();
	void WindowEvent(const SDL_WindowEvent& window);
	void Fullscreen(bool on);

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	VideoSettings sets;

	void CreateRenderer();
	int GetRenderDriverIndex();

	void PassDrawObject(Object* obj);
	void DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void DrawObject(ListBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);
	void DrawObject(Popup* obj);
	
	void PassDrawItem(int id, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop={ 0, 0, 0, 0 });
	void DrawItem(Checkbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(Switchbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(LineEdit* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(KeyGetter* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop);

	void DrawRect(const SDL_Rect& rect, EColor color);
	void DrawImage(const Image& img, const SDL_Rect& crop = { 0, 0, 0, 0 });
	void DrawText(const Text& txt, const SDL_Rect& crop = { 0, 0, 0, 0 });
};
