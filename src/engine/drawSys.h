#pragma once

#include "utils/scrollAreas.h"

// handles the drawing
class DrawSys {
public:
	DrawSys();
	~DrawSys();

	SDL_Renderer* getRenderer();
	void createRenderer(SDL_Window* window, int driverIndex);
	void destroyRenderer();

	void drawWidgets(const vector<Widget*>& widgets, const Popup* popup);

private:
	void passDrawWidget(Widget* wgt);
	void drawWidget(const SDL_Rect& bg, EColor bgColor, const Text& text);
	void drawWidget(LineEditor* wgt);
	void drawWidget(ScrollAreaItems* wgt);
	void drawWidget(ReaderBox* wgt);

	void passDrawItem(ListItem* item, const SDL_Rect& rect, const vec2i& textPos);
	void drawItem(Checkbox* item, const SDL_Rect& rect, const vec2i& textPos);
	void drawItem(Switchbox* item, const SDL_Rect& rect, const vec2i& textPos);
	void drawItem(LineEdit* item, const SDL_Rect& rect, const vec2i& textPos);
	void drawItem(KeyGetter* item, const SDL_Rect& rect, const vec2i& textPos);

	void drawRect(const SDL_Rect& rect, EColor color);
	void drawImage(const Image& img, const SDL_Rect& frame);
	void drawText(const Text& txt, const SDL_Rect& frame);

	SDL_Renderer* renderer;
	SDL_Color colorDim;		// currenly used for dimming background widgets when popup is displayed (dimming is achieved through division)
};
