#pragma once

#include "fileSys.h"
#include "utils/layouts.h"
#if defined(_WIN32) || defined(APPIMAGE)
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif

// loads different font sizes from one file
class FontSet {
public:
	static constexpr int fontTestHeight = 100;
private:
	static constexpr char fontTestString[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`~!@#$%^&*()_+-=[]{}'\\\"|;:,.<>/?";

	float heightScale;	// for scaling down font size to fit requested height
	fs::path file;
	umap<int, TTF_Font*> fonts;

public:
	~FontSet();

	void init(const fs::path& path);
	void clear();

	TTF_Font* getFont(int height);
	int length(const char* text, int height);
private:
	TTF_Font* addSize(int size);
};

// data needed to load pictures
struct PictureLoader {
	DrawSys* drawSys;
	vector<Texture> pics;
	string progVal, progLim;
	fs::path curDir;
	string firstPic;
	PicLim picLim;
	bool fwd, showHidden;

	PictureLoader(DrawSys* drawer, fs::path cdrc, string pfirst, const PicLim& plim, bool forward, bool hidden);

	string limitToStr(uptrt i, uptrt c, uptrt m, sizet mag) const;
};

// handles the drawing
class DrawSys {
public:
	static constexpr int cursorHeight = 20;
private:
	static constexpr SDL_Color colorPopupDim = { 0, 0, 0, 127 };

	SDL_Renderer* renderer;
	SDL_mutex* rendLock;									// lock before using renderer
	array<SDL_Color, FileSys::defaultColors.size()> colors;	// use Color as index
	FontSet fonts;
	umap<string, SDL_Texture*> texes;	// name, texture data

public:
	DrawSys(SDL_Window* window, int driverIndex, Settings* sets, const FileSys* fileSys);
	~DrawSys();

	Rect viewport() const;
	void setTheme(const string& name, Settings* sets, const FileSys* fileSys);
	int textLength(const char* text, int height);
	int textLength(const string& text, int height);
	void setFont(const string& font, Settings* sets, const FileSys* fileSys);
	void clearFonts();
	SDL_Texture* texture(const string& name) const;

	void drawWidgets(Scene* scene, bool mouseLast);
	void drawPicture(const Picture* wgt);
	void drawCheckBox(const CheckBox* wgt);
	void drawSlider(const Slider* wgt);
	void drawProgressBar(const ProgressBar* wgt);
	void drawLabel(const Label* wgt);
	void drawScrollArea(const ScrollArea* box);
	void drawReaderBox(const ReaderBox* box);
	void drawPopup(const Popup* box);
	void drawTooltip(Button* but);

	SDL_Texture* renderText(const char* text, int height);
	SDL_Texture* renderText(const char* text, int height, uint length);
	static int loadTexturesDirectoryThreaded(void* data);
	static int loadTexturesArchiveThreaded(void* data);
private:
	SDL_Texture* loadArchiveTexture(archive* arch, archive_entry* entry);
	static sizet initLoadLimits(PictureLoader* pl, vector<fs::path>& files, uptrt& lim, uptrt& mem);
	static mapFiles initLoadLimits(PictureLoader* pl, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem);
	static uptrt texMemory(SDL_Texture* tex);

	void drawRect(const Rect& rect, Color color);
	void drawText(SDL_Texture* tex, const Rect& rect, const Rect& frame);
	void drawImage(SDL_Texture* tex, const Rect& rect, const Rect& frame);
};

inline Rect DrawSys::viewport() const {
	Rect view;
	SDL_RenderGetViewport(renderer, &view);
	return view;
}

inline int DrawSys::textLength(const char* text, int height) {
	return fonts.length(text, height);
}

inline int DrawSys::textLength(const string& text, int height) {
	return fonts.length(text.c_str(), height);
}

inline void DrawSys::clearFonts() {
	fonts.clear();
}
