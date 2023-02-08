#pragma once

#include "renderer.h"
#include "utils/settings.h"
#ifdef _WIN32
#include <SDL_ttf.h>
#else
#include <SDL2/SDL_ttf.h>
#endif
#include <atomic>

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
	int length(char* text, size_t length, int height);
};

inline FontSet::~FontSet() {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
#endif
}

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
	Texture* blank;		// reference to texes[""]

public:
	DrawSys(const umap<int, SDL_Window*>& windows, Settings* sets, const FileSys* fileSys, int iconSize);
	~DrawSys();

	Renderer* getRenderer();
	ivec2 getViewRes() const;
	void updateView();
	int findPointInView(ivec2 pos) const;
	void setTheme(string_view name, Settings* sets, const FileSys* fileSys);
	int textLength(const char* text, int height);
	int textLength(const string& text, int height);
	int textLength(char* text, size_t length, int height);
	int textLength(string& text, size_t length, int height);
	void setFont(const string& font, Settings* sets, const FileSys* fileSys);
#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	void clearFonts();
#endif
	SDL_Surface* loadIcon(const string& path, int size);
	pair<Texture*, bool> texture(const string& name) const;

	void drawWidgets(Scene* scene, bool mouseLast);
	bool drawPicture(const Picture* wgt, const Recti& view);
	void drawCheckBox(const CheckBox* wgt, const Recti& view);
	void drawSlider(const Slider* wgt, const Recti& view);
	void drawProgressBar(const ProgressBar* wgt, const Recti& view);
	void drawLabel(const Label* wgt, const Recti& view);
	void drawCaret(const Recti& rect, const Recti& frame, const Recti& view);
	void drawWindowArranger(const WindowArranger* wgt, const Recti& view);
	void drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view);
	void drawScrollArea(const ScrollArea* box, const Recti& view);
	void drawReaderBox(const ReaderBox* box, const Recti& view);
	void drawPopup(const Popup* box, const Recti& view);
	void drawTooltip(Button* but, const Recti& view);

	Widget* getSelectedWidget(Layout* box, ivec2 mPos);
	void drawPictureAddr(const Picture* wgt, const Recti& view);
	void drawLayoutAddr(const Layout* wgt, const Recti& view);

	Texture* renderText(const char* text, int height);
	Texture* renderText(const string& text, int height);
	Texture* renderText(const char* text, int height, uint length);
	Texture* renderText(const string& text, int height, uint length);

private:
	umap<int, Renderer::View*>::const_iterator findViewForPoint(ivec2 pos) const;
};

inline Renderer* DrawSys::getRenderer() {
	return renderer;
}

inline ivec2 DrawSys::getViewRes() const {
	return viewRes;
}

inline void DrawSys::updateView() {
	renderer->updateView(viewRes);
}

inline int DrawSys::textLength(const char* text, int height) {
	return fonts.length(text, height);
}

inline int DrawSys::textLength(const string& text, int height) {
	return fonts.length(text.c_str(), height);
}

inline int DrawSys::textLength(char* text, size_t length, int height) {
	return fonts.length(text, length, height);
}

inline int DrawSys::textLength(string& text, size_t length, int height) {
	return fonts.length(text.data(), length, height);
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
	return std::find_if(renderer->getViews().begin(), renderer->getViews().end(), [&pos](const pair<int, Renderer::View*>& it) -> bool { return it.second->rect.contains(pos); });
}

#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
inline void DrawSys::clearFonts() {
	fonts.clear();
}
#endif
