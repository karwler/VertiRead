#pragma once

#include "utils/layouts.h"

// loads different font sizes from one file
class FontSet {
public:
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

	Rect viewport() const;
	void setTheme(const string& name);
	int textLength(const string& text, int height) { return fonts.length(text, height); }
	void setFont(const string& font);
	void clearFonts() { fonts.clear(); }
	SDL_Texture* texture(const string& name) const;
	string translation(const string& line, bool firstCapital = true) const;
	void setLanguage(const string& lang);

	void drawWidgets();
	void drawButton(Button* wgt);
	void drawCheckBox(CheckBox* wgt);
	void drawSlider(Slider* wgt);
	void drawLabel(Label* wgt);
	void drawScrollArea(ScrollArea* box);
	void drawReaderBox(ReaderBox* box);
	void drawPopup(Popup* box);

	SDL_Texture* renderText(const string& text, int height);
	vector<pair<string, SDL_Texture*>> loadTexturesDirectory(string drc);
	vector<pair<string, SDL_Texture*>> loadTexturesArchive(const string& arc);

	SDL_Renderer* renderer;
private:
	vector<SDL_Color> colors;	// use Color as index
	FontSet fonts;
	umap<string, SDL_Texture*> texes;	// name, texture data
	umap<string, string> trans;	// english keyword, translated word

	void drawRect(const Rect& rect, Color color);
	void drawText(SDL_Texture* tex, const Rect& rect, const Rect& frame);
	void drawImage(SDL_Texture* tex, const Rect& rect, const Rect& frame);
};
