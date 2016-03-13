#pragma once

#include "utils/objects.h"

class WindowSys {
public:
	WindowSys();

	bool SetWindow(VideoSettings settings);
	void DestroyWindow();
	void DrawScene();
	
	int CalcTextLength(string text, int size);
	vec2i GetTextureSize(string tex);
	SDL_Renderer* Renderer() const;

	VideoSettings Settings() const;
	vec2i Resolution() const;
	void WindowEvent(const SDL_WindowEvent& window);
	void setFullscreen(bool on);
	void setVSync(bool on);

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	VideoSettings sets;

	bool CreateRenderer();
	int GetRenderDriverIndex();

	void PassDrawObject(Object* obj);
	void DrawObject(ListBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);

	void DrawRect(SDL_Rect rect, EColor color);
	void DrawImage(SDL_Rect rect, string texname, SDL_Rect crop = { 0, 0, 0, 0 });
	void DrawText(const Text& txt, SDL_Rect crop = { 0, 0, 0, 0 });
};
