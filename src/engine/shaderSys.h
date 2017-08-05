#pragma once

#include "utils/scrollAreas.h"

class ShaderSys {
public:
	ShaderSys();
	~ShaderSys();

	void CreateRenderer(SDL_Window* window, int driverIndex);
	void DestroyRenderer();

	void DrawObjects(const vector<Object*>& objects, const Popup* popup);

private:
	void PassDrawObject(Object* obj);
	void DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void DrawObject(LineEditor* obj);
	void DrawObject(ListBox* obj);
	void DrawObject(TableBox* obj);
	void DrawObject(TileBox* obj);
	void DrawObject(ReaderBox* obj);

	void PassDrawItem(size_t id, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop={0, 0, 0, 0});
	void DrawItem(Checkbox* item, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(Switchbox* item, const SDL_Rect& rect, const SDL_Rect& crop);
	void DrawItem(LineEdit* item, ScrollAreaX1* parent, const SDL_Rect& rect, SDL_Rect crop);
	void DrawItem(KeyGetter* item, const SDL_Rect& rect, const SDL_Rect& crop);

	void DrawRect(const SDL_Rect& rect, EColor color);
	void DrawImage(const Image& img, const SDL_Rect& crop={0, 0, 0, 0});
	void DrawText(const Text& txt, const SDL_Rect& crop={0, 0, 0, 0});

	SDL_Renderer* renderer;
	vec4c colorDim;		// currenly used for dimming background objects when popup is displayed (dimming is achieved through division)
};
