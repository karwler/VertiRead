#pragma once

#include "utils/scrollAreas.h"

// handles the drawing
class DrawSys {
public:
	DrawSys();
	~DrawSys();

	void createRenderer(SDL_Window* window, int driverIndex);
	void destroyRenderer();

	void drawObjects(const vector<Object*>& objects, const Popup* popup);

private:
	void passDrawObject(Object* obj);
	void drawObject(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void drawObject(LineEditor* obj);
	void drawObject(ScrollAreaItems* obj);
	void drawObject(ReaderBox* obj);

	void passDrawItem(ListItem* item, const SDL_Rect& rect, const SDL_Rect& crop={0, 0, 0, 0});
	void drawItem(Checkbox* item, const SDL_Rect& rect, const SDL_Rect& crop);
	void drawItem(Switchbox* item, const SDL_Rect& rect, const SDL_Rect& crop);
	void drawItem(LineEdit* item, const SDL_Rect& rect, SDL_Rect crop);
	void drawItem(KeyGetter* item, const SDL_Rect& rect, SDL_Rect crop);

	void drawRect(const SDL_Rect& rect, EColor color);
	void drawImage(const Image& img, const SDL_Rect& crop={0, 0, 0, 0});
	void drawText(const Text& txt, const SDL_Rect& crop={0, 0, 0, 0});

	SDL_Renderer* renderer;
	vec4c colorDim;		// currenly used for dimming background objects when popup is displayed (dimming is achieved through division)
};
