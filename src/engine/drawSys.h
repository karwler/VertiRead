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
		FT_FaceRec_* face = nullptr;
		Data data;

		Font(Data&& font) noexcept : data(std::move(font)) {}
	};

	FT_LibraryRec_* lib = nullptr;
	vector<Font> fonts;
	umap<uint, array<FT_BitmapGlyphRec_*, cacheSize>> asciiCache;
	float heightScale;
	float baseScale;
	uint height;
	int mode;

	Pixmap pm;
	vector<string_view::iterator> lineBreaks;
	string_view::iterator wordStart;
	string_view::iterator ptr;
	size_t len;
	size_t cpos;		// current UTF-8 character index
	uint bufSize = 0;
	uint xpos, ypos;	// current x position and y baseline position
	uint xofs;			// additional width padding before the first character
	uint mfin;			// total line width
	uint wordXpos;
	uint maxWidth;

public:
	FontSet();
	~FontSet();

	void init(const nstring& path, bool mono);
	void clearCache() noexcept;
	void setMode(bool mono) noexcept;
	uint measureText(string_view text, uint size) noexcept;
	uvec2 measureText(string_view text, uint size, uint limit) noexcept;
	const Pixmap& renderText(string_view text, uint size) noexcept;
	const Pixmap& renderText(string_view text, uint size, uint limit) noexcept;
	FT_LibraryRec_* getLib() const noexcept { return lib; }
	uint getXpos() const noexcept { return xpos; }

private:
	Font openFont(const nstring& path, uint size) const;
	void prepareBuffer();
	void prepareAdvance(string_view::iterator begin, size_t length, uint xstart) noexcept;
	void advanceTab(array<FT_BitmapGlyphRec_*, cacheSize>& glyphs);
	template <bool cached> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance) noexcept;
	template <bool cached> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance, int left, uint width) noexcept;
	void checkXofs(int left) noexcept;
	bool checkSpace(uint limit, int left, uint width);
	void advanceLine(string_view::iterator pos);
	bool setSize(string_view text, uint size);
	void cacheGlyph(array<FT_BitmapGlyphRec_*, cacheSize>& glyphs, char32_t ch, uint id);
	vector<Font>::iterator loadGlyph(char32_t ch, int32 flags);
	void copyGlyph(const FT_Bitmap_& bmp, int top, int left) noexcept;
};

// handles the drawing
class DrawSys {
public:
	enum class Tex : uint8 {
		blank,
		tooltip,
		center,	// textures loaded from files start here
		cross,
		file,
		fit,
		folder,
		left,
		minus,
		plus,
		reset,
		right,
		search,	// stored textures end here
		vertiread
	};

	static constexpr float fallbackDpi = 96.f;
private:
	static constexpr float assumedCursorHeight = 20.f;	// 16 p probably + some spacing
	static constexpr float assumedIconSize = 128.f;
	static constexpr ivec2 tooltipMargin = ivec2(4, 1);
	static constexpr uint fileTexBegin = eint(Tex::center);
	static constexpr char iconExt[] = ".svg";

	static constexpr array<const char*, eint(Tex::vertiread) + 1 - fileTexBegin> iconStems = {
		"center",
		"cross",
		"file",
		"fit",
		"folder",
		"left",
		"minus",
		"plus",
		"reset",
		"right",
		"search",
		"vertiread"
	};

	Renderer* renderer;
	ivec2 viewRes = ivec2(0);
	FontSet fonts;
	array<Texture*, eint(Tex::vertiread)> texes{};
	const char* curTooltip = nullptr;	// reference to text of the currently rendered tooltip texture
	float winDpi;
	int cursorHeight;
	Renderer::Action drawState = Renderer::Action::yes;

public:
	DrawSys(const vector<SDL_Window*>& windows, const array<vec4, Settings::defaultColors.size()>& colors, const ivec2* vofs = nullptr);
	~DrawSys() { cleanup(); }

	Renderer* getRenderer() noexcept { return renderer; }
	ivec2 getViewRes() const noexcept { return viewRes; }
	bool updateView();	// returns whether a resize happened
	float getWinDpi() const noexcept { return winDpi; }
	bool updateDpi();
	void setFont(const string& font);
	void setMonoFont(bool on) noexcept;
	static SDL_Surface* loadIcon(const char* path, int size) noexcept;
	static string iconName(Tex name);
	const Texture* texture(Tex name) const noexcept;
	void resetTooltip() noexcept;

	void drawWidgets(bool mouseLast) noexcept;
	void drawPicture(const Picture* wgt, const Recti& view) noexcept;
	void drawCheckBox(const CheckBox* wgt, const Recti& view) noexcept;
	void drawSlider(const Slider* wgt, const Recti& view) noexcept;
	void drawLabel(const Label* wgt, const Recti& view) noexcept;
	void drawPushButton(const PushButton* wgt, const Recti& view) noexcept;
	void drawIconButton(const IconButton* wgt, const Recti& view) noexcept;
	void drawIconPushButton(const IconPushButton* wgt, const Recti& view) noexcept;
	void drawLabelEdit(const LabelEdit* wgt, const Recti& view) noexcept;
	void drawCaret(const Recti& rect, const Recti& frame, const Recti& view) noexcept;
	void drawWindowArranger(const WindowArranger* wgt, const Recti& view) noexcept;
	void drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view) noexcept;
	void drawScrollArea(const ScrollArea* box, const Recti& view) noexcept;
	void drawReaderBox(const ReaderBox* box, const Recti& view) noexcept;
	void drawPopup(const Popup* box, const Recti& view) noexcept;
	void drawTooltip(const Recti& view) noexcept;

	uint textLength(string_view text, uint height) noexcept;
	uint textOffset(string_view text, uint height) noexcept;
	Texture* renderText(string_view text, uint height) noexcept;
	Texture* renderText(string_view text, uint height, uint length) noexcept;
	bool renderText(Texture* tex, string_view text, uint height) noexcept;
	bool renderText(Texture* tex, string_view text, uint height, uint length) noexcept;
	FT_LibraryRec_* ftLib() const noexcept { return fonts.getLib(); }

private:
	void cleanup() noexcept;
	bool prepareTooltip() noexcept;	// returns if to draw a tooltip and if a new texture has been created
	float maxDpi() const noexcept;
};

inline string DrawSys::iconName(Tex name) {
	return string(iconStems[eint(name) - fileTexBegin]) + iconExt;
}

inline const Texture* DrawSys::texture(Tex name) const noexcept {
	return coalesce(texes[eint(name)], texes[eint(Tex::blank)]);
}

inline uint DrawSys::textLength(string_view text, uint height) noexcept {
	return fonts.measureText(text, height);
}

inline uint DrawSys::textOffset(string_view text, uint height) noexcept {
	return fonts.measureText(text, height) ? fonts.getXpos() : 0;
}

inline Texture* DrawSys::renderText(string_view text, uint height) noexcept {
	return renderer->texFromText(fonts.renderText(text, height));
}

inline Texture* DrawSys::renderText(string_view text, uint height, uint length) noexcept {
	return renderer->texFromText(fonts.renderText(text, height, length));
}

inline bool DrawSys::renderText(Texture* tex, string_view text, uint height) noexcept {
	return renderer->texFromText(tex, fonts.renderText(text, height));
}

inline bool DrawSys::renderText(Texture* tex, string_view text, uint height, uint length) noexcept {
	return renderer->texFromText(tex, fonts.renderText(text, height, length));
}

inline void DrawSys::setMonoFont(bool on) noexcept {
	fonts.setMode(on);
}

inline void DrawSys::resetTooltip() noexcept {
	curTooltip = nullptr;
}
