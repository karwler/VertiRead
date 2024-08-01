#include "drawSys.h"
#include "fileSys.h"
#include "rendererDx.h"
#include "rendererGl.h"
#include "rendererVk.h"
#include "scene.h"
#include "world.h"
#include "prog/fileOps.h"
#include "prog/program.h"
#include "prog/progs.h"
#include "utils/compare.h"
#include "utils/layouts.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <cwctype>
#include <format>

// FONT SET

FontSet::FontSet() {
	if (FT_Error err = FT_Init_FreeType(&lib))
		throw std::runtime_error(FT_Error_String(err));
}

FontSet::~FontSet() {
	clearCache();
	for (Font& it : fonts)
		FT_Done_Face(it.face);
	FT_Done_FreeType(lib);
}

void FontSet::init(const fs::path& path, bool mono) {
	clearCache();
	for (Font& it : fonts)
		FT_Done_Face(it.face);
	fonts.clear();
	setMode(mono);

	fonts.push_back(openFont(path, fontTestHeight));
	int ymin = 0, ymax = 0;
	for (char ch : fontTestString) {
		if (FT_Error err = FT_Load_Char(fonts[0].face, ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY))
			throw std::runtime_error(FT_Error_String(err));
		ymin = std::min(ymin, fonts[0].face->glyph->bitmap_top - int(fonts[0].face->glyph->bitmap.rows));
		ymax = std::max(ymax, fonts[0].face->glyph->bitmap_top);
	}
	ymin -= fontTestMinPad;
	ymax += fontTestMaxPad;

	float ydiff = float(ymax - ymin);
	heightScale = float(fontTestHeight) / ydiff;
	baseScale = float(ymax) / ydiff;
}

FontSet::Font FontSet::openFont(const fs::path& path, uint size) const {
	Font face = FileOpsLocal::readFile(path.c_str());
	if (FT_Error err = FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(face.data.data()), face.data.size(), 0, &face.face))
		throw std::runtime_error(FT_Error_String(err));
	if (FT_Error err = FT_Set_Pixel_Sizes(face.face, 0, size)) {
		FT_Done_Face(face.face);
		throw std::runtime_error(FT_Error_String(err));
	}
	return face;
}

void FontSet::clearCache() {
	for (auto& [size, glyphs] : asciiCache)
		for (FT_BitmapGlyph it : glyphs)
			FT_Done_Glyph(&it->root);
	asciiCache.clear();
	height = 0;
	pm.pix.reset();
	bufSize = 0;
}

const Pixmap& FontSet::renderText(string_view text, uint size) {
	if (pm.res = uvec2(measureText(text, size), size); pm.res.x) {
		prepareBuffer();
		prepareAdvance(text.begin(), text.length(), xofs);
		ypos = uint(float(size) * baseScale);

		array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
		for (char32_t ch, prev = '\0'; len; prev = ch) {
			ch = mbstowc(ptr, len);
			if (iswcntrl(ch)) {
				if (ch == '\t')
					advanceTab(glyphs);
				continue;
			}

			if (uint id = ch - ' '; id < glyphs.size()) {
				copyGlyph(glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
				advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x);
			} else {
				vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
				copyGlyph(ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
				advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
			}
		}
	}
	return pm;
}

uint FontSet::measureText(string_view text, uint size) {
	if (!setSize(text, size))
		return 0;
	prepareAdvance(text.begin(), text.length(), 0);
	xofs = 0;
	mfin = 0;

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (iswcntrl(ch)) {
			if (ch == '\t')
				advanceTab(glyphs);
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			cacheGlyph(glyphs, ch, id);
			checkXofs(glyphs[id]->left);
			advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
			checkXofs(ft->face->glyph->bitmap_left);
			advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width);
		}
	}
	return mfin;
}

const Pixmap& FontSet::renderText(string_view text, uint size, uint limit) {
	if (pm.res = measureText(text, size, limit); pm.res.x) {
		ypos = uint(float(size) * baseScale);

		array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
		prepareBuffer();
		for (size_t i = 0; i < lineBreaks.size() - 1; ++i) {
			prepareAdvance(lineBreaks[i], lineBreaks[i + 1] - lineBreaks[i], xofs);
			for (char32_t ch, prev = '\0'; len; prev = ch) {
				ch = mbstowc(ptr, len);
				if (iswcntrl(ch)) {
					if (ch == '\t')
						advanceTab(glyphs);
					continue;
				}

				if (uint id = ch - ' '; id < glyphs.size()) {
					if (glyphs[id]->left + glyphs[id]->bitmap.width <= limit) {	// for worst case scenario from checkSpace
						copyGlyph(glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
						advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x);
					}
				} else {
					vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
					if (ft->face->glyph->bitmap_left + ft->face->glyph->bitmap.width <= limit) {	// for worst case scenario from checkSpace
						copyGlyph(ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
						advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
					}
				}
			}
			ypos += size;
		}
	}
	return pm;
}

uvec2 FontSet::measureText(string_view text, uint size, uint limit) {
	if (!setSize(text, size))
		return uvec2(0);
	prepareAdvance(text.begin(), text.length(), 0);
	lineBreaks = { text.begin() };
	wordStart = text.begin();
	xofs = 0;
	mfin = 0;
	wordXpos = 0;
	maxWidth = 0;

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (iswcntrl(ch)) {
			switch (ch) {
			case '\t':
				if (advanceTab(glyphs); mfin >= limit)
					advanceLine(ptr);
				break;
			case '\n':
				advanceLine(ptr);
			}
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			cacheGlyph(glyphs, ch, id);
			checkXofs(glyphs[id]->left);
			if (checkSpace(limit, glyphs[id]->left, glyphs[id]->bitmap.width))
				advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
			checkXofs(ft->face->glyph->bitmap_left);
			if (checkSpace(limit, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width))
				advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width);
		}

		if (iswblank(ch) || iswpunct(ch)) {
			wordStart = ptr;
			wordXpos = xpos;
		}
	}
	lineBreaks.push_back(text.end());
	return uvec2(std::max(maxWidth, mfin), (lineBreaks.size() - 1) * size);
}

void FontSet::prepareBuffer() {
	size_t size = size_t(pm.res.x) * size_t(pm.res.y);
	if (size > bufSize) {
		pm.pix = std::make_unique_for_overwrite<uint8[]>(size);
		bufSize = size;
	}
	std::fill_n(pm.pix.get(), size, 0);
}

void FontSet::prepareAdvance(string_view::iterator begin, size_t length, uint xstart) noexcept {
	ptr = begin;
	len = length;
	cpos = 0;
	xpos = xstart;
}

void FontSet::advanceTab(array<FT_BitmapGlyph, cacheSize>& glyphs) {
	cacheGlyph(glyphs, ' ', 0);
	uint n = coalesce(uint(cpos % tabsize), tabsize);
	FT_Pos adv = glyphs[0]->root.advance.x >> 16;
	mfin = xpos + adv * (n - 1) + glyphs[0]->left + glyphs[0]->bitmap.width;
	xpos += adv * n;
	cpos += n;
}

template <bool cached>
void FontSet::advanceChar(FT_Face face, char32_t ch, char32_t prev, long advance) {
	FT_Vector kerning;
	if (FT_Error err = FT_Get_Kerning(face, prev, ch, FT_KERNING_DEFAULT, &kerning))
		throw std::runtime_error(FT_Error_String(err));
	if constexpr (cached)
		xpos += (advance >> 16) + (kerning.x >> 6);
	else
		xpos += (advance + kerning.x) >> 6;
	++cpos;
}

template <bool cached>
void FontSet::advanceChar(FT_Face face, char32_t ch, char32_t prev, long advance, int left, uint width) {
	mfin = xpos + left + width;
	advanceChar<cached>(face, ch, prev, advance);
}

void FontSet::checkXofs(int left) noexcept {
	if (int dif = int64(xpos) + left; dif < 0) {
		xpos -= dif;
		xofs -= dif;
		mfin -= dif;
	}
}

bool FontSet::checkSpace(uint limit, int left, uint width) {
	uint end = xpos + left + width;
	if (end <= limit)	// next character fits within limit
		return true;
	if (end - wordXpos <= limit) {	// try to fit the word onto a new line
		advanceLine(wordStart);
		return true;
	}
	bool fits = left + width <= limit;
	if (fits)	// split word if the character can fit onto a line
		advanceLine(ptr - 1);
	return fits;
}

void FontSet::advanceLine(string_view::iterator pos) {
	lineBreaks.push_back(pos);
	wordStart = pos;
	wordXpos = 0;
	maxWidth = std::max(maxWidth, mfin);
	cpos = 0;
	xpos = 0;
	mfin = 0;
}

bool FontSet::setSize(string_view text, uint size) {
	size = uint(float(size) * heightScale);
	if (!(text.length() && size))
		return false;

	if (size != height) {
		for (Font& it : fonts)
			if (FT_Error err = FT_Set_Pixel_Sizes(it.face, 0, size))
				throw std::runtime_error(FT_Error_String(err));
		if (auto [it, isnew] = asciiCache.try_emplace(size); isnew)
			it->second.fill(nullptr);
		height = size;
	}
	return true;
}

void FontSet::cacheGlyph(array<FT_BitmapGlyph, cacheSize>& glyphs, char32_t ch, uint id) {
	if (!glyphs[id]) {
		FT_Glyph glyph;
		if (FT_Error err = FT_Load_Char(fonts[0].face, ch, FT_LOAD_IGNORE_TRANSFORM))
			throw std::runtime_error(FT_Error_String(err));
		if (FT_Error err = FT_Get_Glyph(fonts[0].face->glyph, &glyph))
			throw std::runtime_error(FT_Error_String(err));
		if (glyph->format != FT_GLYPH_FORMAT_BITMAP)
			if (FT_Error err = FT_Glyph_To_Bitmap(&glyph, FT_Render_Mode(mode), nullptr, true)) {
				FT_Done_Glyph(glyph);
				throw std::runtime_error(FT_Error_String(err));
			}
		glyphs[id] = reinterpret_cast<FT_BitmapGlyph>(glyph);
	}
}

vector<FontSet::Font>::iterator FontSet::loadGlyph(char32_t ch, int32 flags) {
	for (vector<Font>::iterator ft = fonts.begin(); ft != fonts.end(); ++ft) {
		if (FT_Error err = FT_Load_Char(ft->face, ch, flags))
			throw std::runtime_error(FT_Error_String(err));
		if (ft->face->glyph->glyph_index)
			return ft;
	}

	for (const fs::path& path : World::fileSys()->listFontFiles(lib, ch, ch)) {
		try {
			Font font = openFont(path, height);
			if (FT_Error err = FT_Load_Char(font.face, ch, flags))
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", FT_Error_String(err));
			else if (font.face->glyph->glyph_index) {
				fonts.push_back(std::move(font));
				return fonts.end() - 1;
			}
			FT_Done_Face(font.face);
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	if (FT_Error err = FT_Load_Char(fonts[0].face, ch, flags))
		throw std::runtime_error(FT_Error_String(err));
	return fonts.begin();
}

void FontSet::copyGlyph(const FT_Bitmap& bmp, int top, int left) noexcept {
	uint8* dst = pm.pix.get() + xpos + left;
	uchar* src = bmp.buffer;
	int offs = ypos - top;
	if (offs >= 0)
		dst += size_t(offs) * size_t(pm.res.x);
	else
		src += uint(-offs) * bmp.pitch;
	uint bot = offs + bmp.rows;
	uint rows = bot <= pm.res.y ? bmp.rows : bmp.rows - bot + pm.res.y;

	for (uint r = 0; r < rows; ++r, dst += pm.res.x, src += bmp.pitch)
		for (uint c = 0; c < bmp.width; ++c)
			if (dst[c] < src[c])
				dst[c] = src[c];
}

void FontSet::setMode(bool mono) noexcept {
	height = 0;
	mode = !mono ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO;
}

// DRAW SYS

DrawSys::DrawSys(const vector<SDL_Window*>& windows, const ivec2* vofs) :
	colors(World::fileSys()->loadColors(World::sets()->setTheme(World::sets()->getTheme(), World::fileSys()->getAvailableThemes())))
{
	switch (World::sets()->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		renderer = new RendererDx11(windows, vofs, viewRes, World::sets(), colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_OPENGL
	case opengl1:
		renderer = new RendererGl1(windows, vofs, viewRes, World::sets(), colors[eint(Color::background)]);
		break;
	case opengl3: case opengles3:
		renderer = new RendererGl3(windows, vofs, viewRes, World::sets(), colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderer = new RendererVk(windows, vofs, viewRes, World::sets(), colors[eint(Color::background)]);
		break;
#endif
	case software:
		renderer = new RendererSf(windows, vofs, viewRes, World::sets(), colors[eint(Color::background)]);
	}

	try {
		SDL_Surface* white = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
		if (!white)
			throw std::runtime_error(std::format("Failed to create blank texture: {}", SDL_GetError()));
		SDL_FillRect(white, nullptr, 0xFFFFFFFF);
		if (texes[eint(Tex::blank)] = renderer->texFromIcon(white); !texes[eint(Tex::blank)])
			throw std::runtime_error("Failed to create blank texture");

		winDpi = maxDpi();
		cursorHeight = std::ceil(assumedCursorHeight * winDpi / fallbackDpi);

		int iconSize = std::ceil(assumedIconSize * winDpi / fallbackDpi);
		string dirIcons = fromPath(World::fileSys()->dirIcons());
		for (size_t i = 0; i < iconStems.size() - 1; ++i)
			if (texes[i + fileTexBegin] = renderer->texFromIcon(loadIcon(dirIcons / iconStems[i] + iconExt, iconSize)); !texes[i + fileTexBegin])
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture '%s%s'", iconStems[i], iconExt);

		setFont(toPath(World::sets()->font));
		texes[eint(Tex::tooltip)] = renderer->texFromEmpty();
		renderer->synchTransfer();
	} catch (...) {
		cleanup();
		throw;
	}
}

SDL_Surface* DrawSys::loadIcon(const string& path, int size) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (SDL_RWops* ifh = SDL_RWFromFile(path.data(), "rb")) {
		if (SDL_Surface* img = IMG_LoadSizedSVG_RW(ifh, 0, size)) {
			SDL_RWclose(ifh);
			return img;
		}
		if (SDL_RWseek(ifh, 0, RW_SEEK_SET) == 0) {
			if (SDL_Surface* img = IMG_Load_RW(ifh, SDL_TRUE))
				return img;
		} else
			SDL_RWclose(ifh);
	}
	return nullptr;
#else
	return IMG_Load(path.data());
#endif
}

void DrawSys::cleanup() {
	renderer->synchTransfer();
	for (Texture* tex : texes)
		renderer->freeTexture(tex);
	delete renderer;
}

void DrawSys::updateView() {
	renderer->updateView(viewRes);
	fonts.clearCache();
}

bool DrawSys::updateDpi() {
	if (float vdpi = maxDpi(); vdpi != winDpi) {
		winDpi = vdpi;
		cursorHeight = std::ceil(assumedCursorHeight * winDpi / fallbackDpi);

		int iconSize = std::ceil(assumedIconSize * winDpi / fallbackDpi);
		string dirIcons = fromPath(World::fileSys()->dirIcons());
		for (size_t i = 0; i < iconStems.size() - 1; ++i)
			if (!renderer->texFromIcon(texes[i + fileTexBegin], loadIcon(dirIcons / iconStems[i] + iconExt, iconSize)))
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload texture '%s%s'", iconStems[i], iconExt);
		renderer->synchTransfer();
		return true;
	}
	return false;
}

float DrawSys::maxDpi() const {
	float mdpi = 0.f;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	for (Renderer::View* it : renderer->getViews())
		if (float scl = SDL_GetWindowDisplayScale(it->win); scl > mdpi)
			mdpi = scl;
	return mdpi > 0.f ? mdpi * fallbackDpi : fallbackDpi;
#else
	for (Renderer::View* it : renderer->getViews())
		if (float vdpi; !SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(it->win), nullptr, nullptr, &vdpi) && vdpi > mdpi)
			mdpi = vdpi;
	return mdpi > 0.f ? mdpi : fallbackDpi;
#endif
}

void DrawSys::setTheme(string_view name) {
	colors = World::fileSys()->loadColors(World::sets()->setTheme(name, World::fileSys()->getAvailableThemes()));
	renderer->setClearColor(colors[eint(Color::background)]);
}

void DrawSys::setFont(const fs::path& font) {
	fs::path path = World::fileSys()->findFont(font);
	if (!path.empty())
		World::sets()->font = World::fileSys()->sanitizeFontPath(path);
	else if (path = World::fileSys()->findFont(Settings::defaultFont); !path.empty())
		World::sets()->font = World::fileSys()->sanitizeFontPath(path);
	else if (vector<fs::path> fontPaths = World::fileSys()->listFontFiles(fonts.getLib(), ' ', '~'); !fontPaths.empty()) {
		World::sets()->font = World::fileSys()->sanitizeFontPath(fontPaths[0]);
		path = std::move(fontPaths[0]);
	} else
		throw std::runtime_error(std::format("Failed to find a font file for '{}'", fromPath(font)));
	fonts.init(path, World::sets()->monoFont);
}

void DrawSys::drawWidgets(bool mouseLast) {
	optional<bool> syncTooltip = mouseLast && World::sets()->tooltips ? prepareTooltip() : std::nullopt;
	for (Renderer::View* view : renderer->getViews()) {
		try {
			renderer->startDraw(view);

			// draw main widgets and visible overlays
			World::scene()->getLayout()->drawSelf(view->rect);
			if (World::scene()->getOverlay() && World::scene()->getOverlay()->on)
				World::scene()->getOverlay()->drawSelf(view->rect);

			// draw popup if exists and dim main widgets
			if (World::scene()->getPopup()) {
				renderer->drawRect(texes[eint(Tex::blank)], view->rect, view->rect, colorPopupDim);
				World::scene()->getPopup()->drawSelf(view->rect);
			}

			// draw context menu
			if (World::scene()->getContext())
				World::scene()->getContext()->drawSelf(view->rect);

			// draw extra stuff on top
			if (World::scene()->getCapture())
				World::scene()->getCapture()->drawTop(view->rect);
			if (syncTooltip)
				drawTooltip(syncTooltip, view->rect);

			renderer->finishDraw(view);
		} catch (const Renderer::ErrorSkip&) {}
	}
	renderer->finishRender();
}

void DrawSys::drawPicture(const Picture* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view))
		renderer->drawRect(wgt->getTex(), rect, wgt->frame(), colors[eint(Color::texture)]);
}

void DrawSys::drawCheckBox(const CheckBox* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		renderer->drawRect(texes[eint(Tex::blank)], wgt->boxRect(), frame, colors[eint(wgt->boxColor())]);
	}
}

void DrawSys::drawSlider(const Slider* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		renderer->drawRect(texes[eint(Tex::blank)], wgt->barRect(), frame, colors[eint(Color::dark)]);
		renderer->drawRect(texes[eint(Tex::blank)], wgt->sliderRect(), frame, colors[eint(Color::light)]);
	}
}

void DrawSys::drawLabel(const Label* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		if (wgt->showBg)
			renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(Color::normal)]);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[eint(Color::text)]);
	}
}

void DrawSys::drawPushButton(const PushButton* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[eint(Color::text)]);
	}
}

void DrawSys::drawIconButton(const IconButton* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		if (wgt->getTex())
			renderer->drawRect(wgt->getTex(), wgt->texRect(), frame, colors[eint(Color::texture)]);
	}
}

void DrawSys::drawIconPushButton(const IconPushButton* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		if (wgt->getIconTex())
			renderer->drawRect(wgt->getIconTex(), wgt->iconRect(), frame, colors[eint(Color::texture)]);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[eint(Color::text)]);
	}
}

void DrawSys::drawLabelEdit(const LabelEdit* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(wgt->getBgColor())]);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[eint(Color::text)]);
	}
}

void DrawSys::drawCaret(const Recti& rect, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view))
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(Color::text)]);
}

void DrawSys::drawWindowArranger(const WindowArranger* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(Color::dark)]);
		for (const auto& [id, dsp] : wgt->getDisps())
			if (!wgt->draggingDisp(id)) {
				WindowArranger::DspDisp di = wgt->dispRect(id, dsp);
				drawWaDisp(di.rect, di.color, di.text, di.tex, frame, view);
			}
	}
}

void DrawSys::drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, colors[eint(color)]);
		if (tex)
			renderer->drawRect(tex, text, frame, colors[eint(Color::text)]);
	}
}

void DrawSys::drawScrollArea(const ScrollArea* box, const Recti& view) {
	uvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (uint i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(texes[eint(Tex::blank)], bar, frame, colors[eint(Color::dark)]);
		renderer->drawRect(texes[eint(Tex::blank)], box->sliderRect(), frame, colors[eint(Color::light)]);
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) {
	uvec2 vis = box->visibleWidgets();
	for (uint i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); box->showBar() && bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(texes[eint(Tex::blank)], bar, frame, colors[eint(Color::dark)]);
		renderer->drawRect(texes[eint(Tex::blank)], box->sliderRect(), frame, colors[eint(Color::light)]);
	}
}

void DrawSys::drawPopup(const Popup* box, const Recti& view) {
	if (Recti rect = box->rect(); rect.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rect, box->frame(), colors[eint(box->bgColor)]);
		for (Widget* it : box->getWidgets())
			it->drawSelf(view);
	}
}

void DrawSys::drawTooltip(optional<bool>& syncTooltip, const Recti& view) {
	if (*syncTooltip) {
		renderer->synchTransfer();
		*syncTooltip = false;
	}

	Recti rct(World::winSys()->mousePos() + ivec2(0, cursorHeight), texes[eint(Tex::tooltip)] ? ivec2(texes[eint(Tex::tooltip)]->getRes()) + tooltipMargin * 2 : ivec2(0));
	if (rct.x + rct.w > viewRes.x)
		rct.x = viewRes.x - rct.w;
	if (rct.y + rct.h > viewRes.y)
		rct.y = rct.y - cursorHeight - rct.h;

	if (rct.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rct, view, colors[eint(Color::tooltip)]);
		renderer->drawRect(texes[eint(Tex::tooltip)], Recti(rct.pos() + tooltipMargin, texes[eint(Tex::tooltip)]->getRes()), rct, colors[eint(Color::text)]);
	}
}

optional<bool> DrawSys::prepareTooltip() {
	auto but = dynamic_cast<Button*>(World::scene()->getSelect());
	if (!but)
		return std::nullopt;
	const char* tip = but->getTooltip();
	if (!strfilled(tip))
		return std::nullopt;
	if (tip == curTooltip)
		return false;
	curTooltip = tip;

	auto [tooltipHeight, maxTooltipWidth] = World::program()->getState()->getTooltipParams();
	uint width = 0;
	for (const char* pos = curTooltip;;) {
		const char* brk = strchr(pos, '\n');
		if (uint siz = fonts.measureText(string_view(pos, brk ? brk : pos + strlen(pos)), tooltipHeight) + tooltipMargin.x * 2; siz > width)
			if (width = std::min(siz, maxTooltipWidth); width == maxTooltipWidth)
				break;
		if (!brk)
			break;
		pos = brk + 1;
	}
	return renderer->texFromText(texes[eint(Tex::tooltip)], fonts.renderText(curTooltip, tooltipHeight, width)) ? optional(true) : std::nullopt;
}
