#pragma once

#include "utils/layouts.h"

// loads different font sizes from one file
class FontSet {
public:
	FontSet(const string& FILE);
	~FontSet();

	void init(const string& path);
	void clear();

	TTF_Font* getFont(int height);
	int length(const string& text, int height);

private:
	float heightScale;	// for scaling down font size to fit requested height
	string file;
	umap<int, TTF_Font*> fonts;

	TTF_Font* addSize(int size);
};

// handles the drawing
class DrawSys {
public:
	DrawSys(SDL_Window* window, int driverIndex);
	~DrawSys();

	SDL_Rect viewport() const;	// not to be comfused with GraphView's viewport
	void setTheme(const string& name);
	int textLength(const string& text, int height) { return fonts.length(text, height); }
	void setFont(const string& font);
	void clearFonts() { fonts.clear(); }
	string translation(const string& line, bool firstCapital=true) const;
	void setLanguage(const string& lang);

	void drawWidgets();
	void drawButton(Button* wgt);
	void drawCheckBox(CheckBox* wgt);
	void drawSlider(Slider* wgt);
	void drawPicture(Picture* wgt);
	void drawLabel(Label* wgt);
	void drawScrollArea(ScrollArea* box);
	void drawReaderBox(ReaderBox* box);
	void drawPopup(Popup* box);

	SDL_Texture* renderText(const string& text, int height, vec2i& size);
	SDL_Texture* loadTexture(const string& file, vec2i& res);

	SDL_Renderer* renderer;
private:

	vector<SDL_Color> colors;	// use Color as index
	FontSet fonts;
	umap<string, string> trans;	// english keyword, translated word

	void drawRect(const SDL_Rect& rect, Color color);
	void drawText(SDL_Texture* tex, const SDL_Rect& rect, const SDL_Rect& frame);
	void drawImage(SDL_Texture* tex, const vec2i& res, const SDL_Rect& rect, const SDL_Rect& frame);
};
