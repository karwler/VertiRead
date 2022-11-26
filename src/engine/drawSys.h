#pragma once

#include "renderer.h"
#ifdef _WIN32
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

#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_Font* font = nullptr;
#else
	fs::path file;
	umap<int, TTF_Font*> fonts;
#endif
	float heightScale;	// for scaling down font size to fit requested height

public:
	~FontSet();

	void init(const fs::path& path);
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	void clear();
#endif
	TTF_Font* getFont(int height);
	int length(const char* text, int height);
	int length(const char* text, sizet length, int height);
};

inline FontSet::~FontSet() {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
#endif
}

// data needed to load pictures
struct PictureLoader {
	vector<pair<string, SDL_Surface*>> pics;
	string progVal, progLim;
	fs::path curDir;
	string firstPic;
	PicLim picLim;
	bool fwd, showHidden;

	PictureLoader(fs::path cdrc, string pfirst, const PicLim& plim, bool forward, bool hidden);
	~PictureLoader();

	string limitToStr(uptrt i, uptrt c, uptrt m, sizet mag) const;
};

// handles the drawing
class DrawSys {
public:
	static constexpr int cursorHeight = 20;
private:
	static constexpr vec4 colorPopupDim = vec4(0.f, 0.f, 0.f, 0.5f);

	Renderer* renderer = nullptr;
	ivec2 viewRes = ivec2(0);
	array<vec4, Settings::defaultColors.size()> colors;
	FontSet fonts;
	umap<string, Texture*> texes;
	const Texture* blank;

public:
	DrawSys(const umap<int, SDL_Window*>& windows, Settings* sets, const FileSys* fileSys, int iconSize);
	~DrawSys();

	ivec2 getViewRes() const;
	const Rect& getView(int id) const;
	void updateView();
	int findPointInView(ivec2 pos) const;
	void setVsync(Settings::VSync& vsync);
	void setTheme(string_view name, Settings* sets, const FileSys* fileSys);
	int textLength(const char* text, int height);
	int textLength(const string& text, int height);
	int textLength(const char* text, sizet length, int height);
	int textLength(const string& text, sizet length, int height);
	void setFont(string_view font, Settings* sets, const FileSys* fileSys);
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	void clearFonts();
#endif
	const Texture* texture(const string& name) const;
	vector<pair<string, Texture*>> transferPictures(vector<pair<string, SDL_Surface*>>& pics);

	void drawWidgets(Scene* scene, bool mouseLast);
	bool drawPicture(const Picture* wgt, const Rect& view);
	void drawCheckBox(const CheckBox* wgt, const Rect& view);
	void drawSlider(const Slider* wgt, const Rect& view);
	void drawProgressBar(const ProgressBar* wgt, const Rect& view);
	void drawLabel(const Label* wgt, const Rect& view);
	void drawCaret(const Rect& rect, const Rect& frame, const Rect& view);
	void drawWindowArranger(const WindowArranger* wgt, const Rect& view);
	void drawWaDisp(const Rect& rect, Color color, const Rect& text, const Texture* tex, const Rect& frame, const Rect& view);
	void drawScrollArea(const ScrollArea* box, const Rect& view);
	void drawReaderBox(const ReaderBox* box, const Rect& view);
	void drawPopup(const Popup* box, const Rect& view);
	void drawTooltip(Button* but, const Rect& view);

	Widget* getSelectedWidget(const Layout* box, ivec2 mPos);
	void drawPictureAddr(const Picture* wgt, const Rect& view);
	void drawLayoutAddr(const Layout* wgt, const Rect& view);

	Texture* renderText(const char* text, int height);
	Texture* renderText(const string& text, int height);
	Texture* renderText(const char* text, int height, uint length);
	Texture* renderText(const string& text, int height, uint length);
	static void loadTexturesDirectoryThreaded(bool* running, uptr<PictureLoader> pl);
	static void loadTexturesArchiveThreaded(bool* running, uptr<PictureLoader> pl);
private:
	static sizet initLoadLimits(const PictureLoader* pl, vector<fs::path>& files, uptrt& lim, uptrt& mem);
	static mapFiles initLoadLimits(const PictureLoader* pl, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem);
	umap<int, Renderer::View*>::const_iterator findViewForPoint(ivec2 pos) const;
};

inline ivec2 DrawSys::getViewRes() const {
	return viewRes;
}

inline const Rect& DrawSys::getView(int id) const {
	return renderer->getViews().at(id)->rect;
}

inline void DrawSys::updateView() {
	renderer->updateView(viewRes);
}

inline void DrawSys::setVsync(Settings::VSync& vsync) {
	renderer->setSwapInterval(vsync);
}

inline int DrawSys::textLength(const char* text, int height) {
	return fonts.length(text, height);
}

inline int DrawSys::textLength(const string& text, int height) {
	return fonts.length(text.c_str(), height);
}

inline int DrawSys::textLength(const char* text, sizet length, int height) {
	return fonts.length(text, length, height);
}

inline int DrawSys::textLength(const string& text, sizet length, int height) {
	return fonts.length(text.c_str(), length, height);
}

inline Texture* DrawSys::renderText(const char* text, int height) {
	return renderer->texFromText(TTF_RenderUTF8_Blended(fonts.getFont(height), text, { 255, 255, 255, 255 }));
}

inline Texture* DrawSys::renderText(const string& text, int height) {
	return renderText(text.c_str(), height);
}

inline Texture* DrawSys::renderText(const char* text, int height, uint length) {
	return renderer->texFromText(TTF_RenderUTF8_Blended_Wrapped(fonts.getFont(height), text, { 255, 255, 255, 255 }, length));
}

inline Texture* DrawSys::renderText(const string& text, int height, uint length) {
	return renderText(text.c_str(), height, length);
}

inline umap<int, Renderer::View*>::const_iterator DrawSys::findViewForPoint(ivec2 pos) const {
	return std::find_if(renderer->getViews().begin(), renderer->getViews().end(), [&pos](const pair<int, Renderer::View*>& it) -> bool { return it.second->rect.contain(pos); });
}

#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
inline void DrawSys::clearFonts() {
	fonts.clear();
}
#endif
