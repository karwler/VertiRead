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

		Font(Data&& font) : data(std::move(font)) {}
	};

	FT_LibraryRec_* lib;
	vector<Font> fonts;
	umap<uint, array<FT_BitmapGlyphRec_*, cacheSize>> asciiCache;
	float heightScale;
	float baseScale;
	uint height;
	int mode;

	Pixmap pm;
	size_t bufSize = 0;
	vector<string_view::iterator> lineBreaks;
	string_view::iterator wordStart;
	string_view::iterator ptr;
	size_t len;
	size_t cpos;		// current UTF-8 character index
	uint xpos, ypos;	// current x position and y baseline position
	uint xofs;			// additional width padding before the first character
	uint mfin;			// total line width
	uint wordXpos;
	uint maxWidth;

public:
	FontSet();
	~FontSet();

	void init(const fs::path& path, bool mono);
	void clearCache();
	void setMode(bool mono) noexcept;
	uint measureText(string_view text, uint size);
	uvec2 measureText(string_view text, uint size, uint limit);
	const Pixmap& renderText(string_view text, uint size);
	const Pixmap& renderText(string_view text, uint size, uint limit);
	FT_LibraryRec_* getLib() const { return lib; }

private:
	Font openFont(const fs::path& path, uint size) const;
	void prepareBuffer();
	void prepareAdvance(string_view::iterator begin, size_t length, uint xstart) noexcept;
	void advanceTab(array<FT_BitmapGlyphRec_*, cacheSize>& glyphs);
	template <bool cached> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance);
	template <bool cached> void advanceChar(FT_FaceRec_* face, char32_t ch, char32_t prev, long advance, int left, uint width);
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
	static constexpr vec4 colorPopupDim = vec4(0.f, 0.f, 0.f, 0.5f);
	static constexpr uint fileTexBegin = uint(Tex::center);
	static constexpr char iconExt[] = ".svg";

	static constexpr array<const char*, size_t(Tex::vertiread) + 1 - fileTexBegin> iconStems = {
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
	array<vec4, Settings::defaultColors.size()> colors;
	FontSet fonts;
	array<Texture*, size_t(Tex::vertiread)> texes{};
	const char* curTooltip = nullptr;	// reference to text of the currently rendered tooltip texture
	float winDpi;
	int cursorHeight;

public:
	DrawSys(const vector<SDL_Window*>& windows, const ivec2* vofs = nullptr);
	~DrawSys() { cleanup(); }

	Renderer* getRenderer() { return renderer; }
	ivec2 getViewRes() const { return viewRes; }
	void updateView();
	float getWinDpi() const { return winDpi; }
	bool updateDpi();
	void setTheme(string_view name);
	void setFont(const fs::path& font);
	void setMonoFont(bool on);
	SDL_Surface* loadIcon(const string& path, int size);
	static string iconName(Tex name);
	const Texture* texture(Tex name) const;
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

	uint textLength(string_view text, uint height);
	Texture* renderText(string_view text, uint height);
	Texture* renderText(string_view text, uint height, uint length);
	bool renderText(Texture* tex, string_view text, uint height);
	bool renderText(Texture* tex, string_view text, uint height, uint length);
	FT_LibraryRec_* ftLib() const { return fonts.getLib(); }

private:
	void cleanup();
	optional<bool> prepareTooltip();	// returns if a new texture has been created or nullopt to not display a tooltip
	float maxDpi() const;
};

inline string DrawSys::iconName(Tex name) {
	return string(iconStems[eint(name) - fileTexBegin]) + iconExt;
}

inline const Texture* DrawSys::texture(Tex name) const {
	return coalesce(texes[eint(name)], texes[eint(Tex::blank)]);
}

inline uint DrawSys::textLength(string_view text, uint height) {
	return fonts.measureText(text, height);
}

inline Texture* DrawSys::renderText(string_view text, uint height) {
	return renderer->texFromText(fonts.renderText(text, height));
}

inline Texture* DrawSys::renderText(string_view text, uint height, uint length) {
	return renderer->texFromText(fonts.renderText(text, height, length));
}

inline bool DrawSys::renderText(Texture* tex, string_view text, uint height) {
	return renderer->texFromText(tex, fonts.renderText(text, height));
}

inline bool DrawSys::renderText(Texture* tex, string_view text, uint height, uint length) {
	return renderer->texFromText(tex, fonts.renderText(text, height, length));
}

inline void DrawSys::setMonoFont(bool on) {
	fonts.setMode(on);
}

inline void DrawSys::resetTooltip() {
	curTooltip = nullptr;
}
