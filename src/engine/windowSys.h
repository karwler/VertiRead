#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"

class WindowSys {
public:
	WindowSys(const VideoSettings& SETS=VideoSettings());
	~WindowSys();

	void SetWindow();
	void DestroyWindow();
	void DrawObjects(vector<Object*> objects);
	
	SDL_Renderer* Renderer() const;
	VideoSettings Settings() const;
	vec2i Resolution() const;
	static vec2i DesktopResolution();
	void WindowEvent(const SDL_WindowEvent& window);
	void Fullscreen(bool on);
	void VSync(bool on);

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	VideoSettings sets;

	void CreateRenderer();
	int GetRenderDriverIndex();

	void PassDrawObject(Object* obj);
	void DrawObject(Label* obj);
	void DrawObject(ListBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);
	void DrawObject(LineEdit* obj);
	void DrawObject(KeyGetter* obj);
	void DrawObject(Popup* obj);

	void DrawRect(const SDL_Rect& rect, EColor color);
	void DrawImage(const Image& img, SDL_Rect crop = { 0, 0, 0, 0 });
	void DrawText(const Text& txt, const SDL_Rect& crop = { 0, 0, 0, 0 });
};
