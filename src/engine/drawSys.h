#pragma once

#include "renderer.h"

struct FT_Bitmap_;
struct FT_BitmapGlyphRec_;
struct FT_FaceRec_;
struct FT_LibraryRec_;

// loads different font sizes from one file
class FontSet {
private:
	static constexpr char fontTestString[] = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	static constexpr uint fontTestHeight = 100;
	static constexpr uint fontTestMinPad = 4, fontTestMaxPad = 10;
	static constexpr uint tabsize = 4;
	static constexpr uint cacheSize = 127 - ' ';

	struct Font {
		FT_FaceRec_* face;
		vector<byte_t> data;
	};

	FT_LibraryRec_* lib = nullptr;
	vector<Font> fonts;
	umap<uint, array<FT_BitmapGlyphRec_*, cacheSize>> asciiCache;
	float heightScale;
	float baseScale;
	uint height;
	int mode;

	uptr<uint32[]> buffer;
	size_t bufSize = 0;
	string_view::iterator ptr;
	string_view::iterator wordStart;
	size_t len;
	size_t cpos;
	uint xpos, ypos;
	uint mfin;
	uint gXpos;

public:
	FontSet();
	~FontSet();

	void init(const fs::path& path, Settings::Hinting hinting);
	void clearCache();
	void setMode(Settings::Hinting hinting);
	uint measureText(string_view text, uint size);
	pair<uint, vector<string_view::iterator>> measureText(string_view text, uint size, uint limit);
	PixmapRgba renderText(string_view text, uint size);
	PixmapRgba renderText(string_view text, uint size, uint limit);
	FT_LibraryRec_* getLib() const { return lib; }

private:
	Font openFont(const fs::path& path, uint size) const;
	uvec2 prepareBuffer(uint resx, uint resy);
	void prepareAdvance(string_view::iterator begin, size_t length);
	template <bool ml> void advanceTab(array<FT_BitmapGlyphRec_*, cacheSize>& glyphs);
	template <bool cached> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance);
	template <bool cached, bool ml> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance, int left, uint width);
	bool breakLine(vector<string_view::iterator>& ln, uint size, uint limit, int left, uint width);
	void advanceLine(vector<string_view::iterator>& ln, string_view::iterator pos, uint size);
	void resetLine();
	bool setSize(string_view text, uint size);
	void cacheGlyph(array<FT_BitmapGlyphRec_*, cacheSize>& glyphs, char32_t ch, uint id);
	vector<Font>::iterator loadGlyph(char32_t ch, int32 flags);
	void copyGlyph(uvec2 res, const FT_Bitmap_& bmp, int top, int left);
};

// handles the drawing
class DrawSys {
public:
	static constexpr float fallbackDpi = 96.f;
private:
	static constexpr float assumedCursorHeight = 17.f;	// 16 p probably + some spacing
	static constexpr float assumedIconSize = 128.f;
	static constexpr ivec2 tooltipMargin = ivec2(4, 1);
	static constexpr vec4 colorPopupDim = vec4(0.f, 0.f, 0.f, 0.5f);

	Renderer* renderer = nullptr;
	ivec2 viewRes = ivec2(0);
	array<vec4, Settings::defaultColors.size()> colors;
	FontSet fonts;
	umap<string, Texture*> texes;
	Texture* blank;		// reference to texes[""]
	Texture* tooltip = nullptr;
	const char* curTooltip = nullptr;	// reference to text of the currently rendered tooltip texture
	float winDpi = 0.f;
	int cursorHeight;

public:
	DrawSys(const umap<int, SDL_Window*>& windows);
	~DrawSys();

	Renderer* getRenderer() { return renderer; }
	ivec2 getViewRes() const { return viewRes; }
	void updateView();
	int findPointInView(ivec2 pos) const;
	float getWinDpi() const { return winDpi; }
	bool updateDpi(int dsp);	// should only be called when there's only one window
	void setTheme(string_view name);
	void setFont(const fs::path& font);
	void setFontHinting(Settings::Hinting hinting);
	SDL_Surface* loadIcon(const string& path, int size);
	const Texture* texture(const string& name) const;
	void resetTooltip();

	void drawWidgets(bool mouseLast);
	void drawPicture(const Picture* wgt, const Recti& view);
	void drawCheckBox(const CheckBox* wgt, const Recti& view);
	void drawSlider(const Slider* wgt, const Recti& view);
	void drawLabel(const Label* wgt, const Recti& view);
	void drawPushButton(const PushButton* wgt, const Recti& view);
	void drawIconButton(const IconButton* wgt, const Recti& view);
	void drawIconPushButton(const IconPushButton* wgt, const Recti& view);
	void drawLabelEdit(const LabelEdit* wgt, const Recti& view);
	void drawCaret(const Recti& rect, const Recti& frame, const Recti& view);
	void drawWindowArranger(const WindowArranger* wgt, const Recti& view);
	void drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view);
	void drawScrollArea(const ScrollArea* box, const Recti& view);
	void drawReaderBox(const ReaderBox* box, const Recti& view);
	void drawPopup(const Popup* box, const Recti& view);
	void drawTooltip(optional<bool>& syncTooltip, const Recti& view);

	Widget* getSelectedWidget(Layout* box, ivec2 mPos);
	void drawButtonAddr(const Button* wgt, const Recti& view);
	void drawLayoutAddr(const Layout* wgt, const Recti& view);

	uint textLength(string_view text, uint height);
	Texture* renderText(string_view text, uint height);
	Texture* renderText(string_view text, uint height, uint length);
	FT_LibraryRec_* ftLib() const { return fonts.getLib(); }

private:
	umap<int, Renderer::View*>::const_iterator findViewForPoint(ivec2 pos) const;
	optional<bool> prepareTooltip();	// returns if a new texture has been created or nullopt to not display a tooltip
};

inline uint DrawSys::textLength(string_view text, uint height) {
	return fonts.measureText(text, height);
}

inline Texture* DrawSys::renderText(string_view text, uint height) {
	return renderer->texFromText(fonts.renderText(text, height));
}

inline Texture* DrawSys::renderText(string_view text, uint height, uint length) {
	return renderer->texFromText(fonts.renderText(text, height, length));
}

inline void DrawSys::setFontHinting(Settings::Hinting hinting) {
	fonts.setMode(hinting);
}

inline void DrawSys::resetTooltip() {
	curTooltip = nullptr;
}

inline umap<int, Renderer::View*>::const_iterator DrawSys::findViewForPoint(ivec2 pos) const {
	return rng::find_if(renderer->getViews(), [&pos](const pair<int, Renderer::View*>& it) -> bool { return it.second->rect.contains(pos); });
}
