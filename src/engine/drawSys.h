#pragma once

#include "fileSys.h"
#include "utils/layouts.h"
#include <SDL2/SDL_ttf.h>

// loads different font sizes from one file
class FontSet {
public:
	static constexpr int fontTestHeight = 100;
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";

	float heightScale;	// for scaling down font size to fit requested height
	string file;
	umap<int, TTF_Font*> fonts;

public:
	~FontSet();

	void init(const string& path);
	void clear();

	TTF_Font* getFont(int height);
	int length(const string& text, int height);
private:
	TTF_Font* addSize(int size);
};

// handles the drawing
class DrawSys {
private:
	static constexpr uint32 rendererFlags = SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED;
	static constexpr SDL_Color colorPopupDim = { 0, 0, 0, 127 };
	static constexpr sizet numBuffSize = 23;

	SDL_Renderer* renderer;
	SDL_mutex* rendLock;									// lock before using renderer
	array<SDL_Color, FileSys::defaultColors.size()> colors;	// use Color as index
	FontSet fonts;
	umap<string, SDL_Texture*> texes;	// name, texture data

public:
	DrawSys(SDL_Window* window, int driverIndex);
	~DrawSys();

	Rect viewport() const;
	void setTheme(const string& name);
	int textLength(const string& text, int height);
	void setFont(const string& font);
	void clearFonts();
	SDL_Texture* texture(const string& name) const;

	void drawWidgets();
	void drawPicture(const Picture* wgt);
	void drawCheckBox(const CheckBox* wgt);
	void drawSlider(const Slider* wgt);
	void drawProgressBar(const ProgressBar* wgt);
	void drawLabel(const Label* wgt);
	void drawScrollArea(const ScrollArea* box);
	void drawReaderBox(const ReaderBox* box);
	void drawPopup(const Popup* box);

	SDL_Texture* renderText(const string& text, int height);
	static int loadTexturesDirectoryThreaded(void* data);
	static int loadTexturesArchiveThreaded(void* data);
private:
	SDL_Texture* loadArchiveTexture(archive* arch, archive_entry* entry);
	static bool initLoadLimits(vector<string>& files, Thread* thread, uptrt& lim, uptrt& mem);
	static mapFiles initLoadLimits(Thread* thread, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem);
	static string limitToStr(uptrt i, uptrt c, uptrt m, sizet mag);

	void drawRect(const Rect& rect, Color color);
	void drawText(SDL_Texture* tex, const Rect& rect, const Rect& frame);
	void drawImage(SDL_Texture* tex, const Rect& rect, const Rect& frame);
};

inline Rect DrawSys::viewport() const {
	Rect view;
	SDL_RenderGetViewport(renderer, &view);
	return view;
}

inline int DrawSys::textLength(const string& text, int height) {
	return fonts.length(text, height);
}

inline void DrawSys::clearFonts() {
	fonts.clear();
}
