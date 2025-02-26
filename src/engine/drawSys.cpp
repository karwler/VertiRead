#include "drawSys.h"
#include "fileSys.h"
#include "rendererDx.h"
#include "rendererGl.h"
#include "rendererVk.h"
#include "scene.h"
#include "world.h"
#include "prog/program.h"
#include "prog/progs.h"
#include "utils/compare.h"
#include "utils/layouts.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#ifdef WITH_SDL3
#include <SDL3_image/SDL_image.h>
#else
#include <SDL_image.h>
#endif
#include <cwctype>

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

void FontSet::init(const nstring& path, bool mono) {
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

	float ydiff = ymax - ymin;
	heightScale = float(fontTestHeight) / ydiff;
	baseScale = float(ymax) / ydiff;
}

FontSet::Font FontSet::openFont(const nstring& path, uint size) const {
	Font face = FileSys::readBinaryFile(path.data());
	if (FT_Error err = FT_New_Memory_Face(lib, face.data.data(), face.data.size(), 0, &face.face))
		throw std::runtime_error(FT_Error_String(err));
	if (FT_Error err = FT_Set_Pixel_Sizes(face.face, 0, size)) {
		FT_Done_Face(face.face);
		throw std::runtime_error(FT_Error_String(err));
	}
	return face;
}

void FontSet::clearCache() noexcept {
	for (auto& [size, glyphs] : asciiCache)
		for (FT_BitmapGlyph it : glyphs)
			FT_Done_Glyph(&it->root);
	asciiCache.clear();
	height = 0;
	pm.pix.reset();
	bufSize = 0;
}

const Pixmap& FontSet::renderText(string_view text, uint size) noexcept {
	if (pm.res = uvec2(measureText(text, size), size); pm.res.x) {
		try {
			array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
			prepareBuffer();
			prepareAdvance(text.begin(), text.length(), xofs);
			ypos = float(size) * baseScale;

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
					auto ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
					copyGlyph(ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
					advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
				}
			}
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			pm.res = uvec2(0);
		}
	}
	return pm;
}

uint FontSet::measureText(string_view text, uint size) noexcept {
	try {
		if (setSize(text, size)) {
			array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
			prepareAdvance(text.begin(), text.length(), 0);
			xofs = 0;
			mfin = 0;

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
					auto ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
					checkXofs(ft->face->glyph->bitmap_left);
					advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width);
				}
			}
			return mfin;
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return 0;
}

const Pixmap& FontSet::renderText(string_view text, uint size, uint limit) noexcept {
	if (pm.res = measureText(text, size, limit); pm.res.x) {
		try {
			array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
			prepareBuffer();
			ypos = float(size) * baseScale;

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
						auto ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
						if (ft->face->glyph->bitmap_left + ft->face->glyph->bitmap.width <= limit) {	// for worst case scenario from checkSpace
							copyGlyph(ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
							advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
						}
					}
				}
				ypos += size;
			}
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			pm.res = uvec2(0);
		}
	}
	return pm;
}

uvec2 FontSet::measureText(string_view text, uint size, uint limit) noexcept {
	try {
		if (setSize(text, size)) {
			array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
			prepareAdvance(text.begin(), text.length(), 0);
			lineBreaks = { text.begin() };
			wordStart = text.begin();
			xofs = 0;
			mfin = 0;
			wordXpos = 0;
			maxWidth = 0;

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
					auto ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
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
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return uvec2(0);
}

void FontSet::prepareBuffer() {
	uint size = pm.res.x * pm.res.y;
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
void FontSet::advanceChar(FT_Face face, char32_t ch, char32_t prev, long advance) noexcept {
	FT_Vector kerning;
	if (FT_Error err = FT_Get_Kerning(face, prev, ch, FT_KERNING_DEFAULT, &kerning)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", FT_Error_String(err));
		kerning = {};
	}
	if constexpr (cached)
		xpos += (advance >> 16) + (kerning.x >> 6);
	else
		xpos += (advance + kerning.x) >> 6;
	++cpos;
}

template <bool cached>
void FontSet::advanceChar(FT_Face face, char32_t ch, char32_t prev, long advance, int left, uint width) noexcept {
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
	size = float(size) * heightScale;
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
	for (auto ft = fonts.begin(); ft != fonts.end(); ++ft) {
		if (FT_Error err = FT_Load_Char(ft->face, ch, flags))
			throw std::runtime_error(FT_Error_String(err));
		if (ft->face->glyph->glyph_index)
			return ft;
	}

	for (const nstring& path : World::fileSys()->listFontFiles(lib, ch, ch)) {
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
		dst += uint(offs) * pm.res.x;
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

DrawSys::DrawSys(const vector<SDL_Window*>& windows, const ivec2* vofs) {
	Renderer::InitParams initParams = {
		.windows = windows,
		.vofs = vofs,
		.viewRes = viewRes,
		.tooltipTexture = texes[eint(Tex::tooltip)],
		.colors = World::fileSys()->loadColors(World::sets()->setTheme(World::sets()->getTheme(), World::fileSys()->getAvailableThemes()))
	};
	switch (World::sets()->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		renderer = new RendererDx11(initParams, World::sets());
		break;
#endif
#ifdef WITH_OPENGL
#if !defined(__arm__) && !defined(__aarch64__)
	case opengl1:
		renderer = new RendererGl1(initParams, World::sets());
		break;
	case opengl3:
#endif
#ifndef _WIN32
	case opengles3:
#endif
		renderer = new RendererGl3(initParams, World::sets());
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderer = new RendererVk(initParams, World::sets());
		break;
#endif
	case software:
		renderer = new RendererSf(initParams, World::sets());
	}

	try {
		uint32 pixels[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
		SDL_Surface* white = SDL_CreateSurfaceFrom(2, 2, SDL_PIXELFORMAT_ABGR8888, pixels, 2 * sizeof(uint32));
		if (!white)
			throw std::runtime_error(SDL_GetError());
		if (texes[eint(Tex::blank)] = renderer->texFromSurface(white, false, false); !texes[eint(Tex::blank)])
			throw std::runtime_error("Failed to create blank texture");

		winDpi = maxDpi();
		cursorHeight = std::ceil(assumedCursorHeight * winDpi / fallbackDpi);

		int iconSize = std::ceil(assumedIconSize * winDpi / fallbackDpi);
		string dirIcons = World::fileSys()->dirIcons();
		for (size_t i = 0; i < iconStems.size() - 1; ++i)
			if (texes[i + fileTexBegin] = renderer->texFromSurface(loadIcon((dirIcons / iconStems[i] + iconExt).data(), iconSize), false, true); !texes[i + fileTexBegin])
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture '%s%s'", iconStems[i], iconExt);

		setFont(World::sets()->font);
	} catch (const std::exception&) {
		cleanup();
		throw;
	}
}

SDL_Surface* DrawSys::loadIcon(const char* path, int size) noexcept {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (uptr<SDL_RWops> ifh(SDL_RWFromFile(path, "rb")); ifh) {
		if (SDL_Surface* img = IMG_LoadSizedSVG_RW(ifh.get(), 0, size))
			return img;
		if (SDL_RWseek(ifh.get(), 0, RW_SEEK_SET) == 0)
			if (SDL_Surface* img = IMG_Load_RW(ifh.get(), SDL_FALSE))
				return img;
	}
	return nullptr;
#else
	return IMG_Load(path);
#endif
}

void DrawSys::cleanup() noexcept {
	renderer->waitIdle();
	for (Texture* tex : texes)
		renderer->freeTexture(tex);
	delete renderer;
}

bool DrawSys::updateView() {
	bool ret = renderer->updateView(viewRes);
	if (ret)
		fonts.clearCache();
	return ret;
}

bool DrawSys::updateDpi() {
	if (float vdpi = maxDpi(); vdpi != winDpi) {
		winDpi = vdpi;
		cursorHeight = std::ceil(assumedCursorHeight * winDpi / fallbackDpi);

		int iconSize = std::ceil(assumedIconSize * winDpi / fallbackDpi);
		string dirIcons = World::fileSys()->dirIcons();
		renderer->waitIdle();
		for (size_t i = 0; i < iconStems.size() - 1; ++i)
			if (!renderer->texFromSurface(texes[i + fileTexBegin], loadIcon((dirIcons / iconStems[i] + iconExt).data(), iconSize), false))
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reload texture '%s%s'", iconStems[i], iconExt);
		return true;
	}
	return false;
}

float DrawSys::maxDpi() const noexcept {
	float mdpi = 0.f;
#ifdef WITH_SDL3
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
	array<vec4, Settings::defaultColors.size()> colors = World::fileSys()->loadColors(World::sets()->setTheme(name, World::fileSys()->getAvailableThemes()));
	renderer->setColors(colors);
}

void DrawSys::setFont(const string& font) {
	nstring path = World::fileSys()->findFont(font);
	if (!path.empty())
		World::sets()->font = World::fileSys()->sanitizeFontPath(path);
	else if (path = World::fileSys()->findFont(Settings::defaultFont); !path.empty())
		World::sets()->font = World::fileSys()->sanitizeFontPath(path);
	else if (vector<nstring> fontPaths = World::fileSys()->listFontFiles(fonts.getLib(), ' ', '~'); !fontPaths.empty()) {
		World::sets()->font = World::fileSys()->sanitizeFontPath(fontPaths[0]);
		path = std::move(fontPaths[0]);
	} else
		throw std::runtime_error(fmt::format("Failed to find a font file for '{}'", font));
	fonts.init(path, World::sets()->monoFont);
}

void DrawSys::drawWidgets(bool mouseLast) noexcept {
	bool showTooltip = mouseLast && World::sets()->tooltips && prepareTooltip();
	const vector<Renderer::View*>& views = renderer->getViews();
	for (auto vw = views.begin(); vw != views.end() && drawState != Renderer::Action::no; ++vw) {
		Renderer::View* view = *vw;
		if (drawState = renderer->startDraw(view); drawState == Renderer::Action::yes) {
			// draw main widgets and visible overlays
			World::scene()->getLayout()->drawSelf(view->rect);
			if (World::scene()->getOverlay() && World::scene()->getOverlay()->on)
				World::scene()->getOverlay()->drawSelf(view->rect);

			// draw popup if exists and dim main widgets
			if (World::scene()->getPopup()) {
				renderer->drawRect(texes[eint(Tex::blank)], view->rect, view->rect, Color::dim);
				World::scene()->getPopup()->drawSelf(view->rect);
			}

			// draw context menu
			if (World::scene()->getContext())
				World::scene()->getContext()->drawSelf(view->rect);

			// draw extra stuff on top
			if (World::scene()->getCapture())
				World::scene()->getCapture()->drawTop(view->rect);
			if (showTooltip)
				drawTooltip(view->rect);

			drawState = renderer->finishDraw(view);
		}
	}
	if (drawState != Renderer::Action::no)
		drawState = renderer->finishRender();
}

void DrawSys::drawPicture(const Picture* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view))
		renderer->drawRect(wgt->getTex(), rect, wgt->frame(), Color::texture);
}

void DrawSys::drawCheckBox(const CheckBox* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		renderer->drawRect(texes[eint(Tex::blank)], wgt->boxRect(), frame, wgt->boxColor());
	}
}

void DrawSys::drawSlider(const Slider* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		renderer->drawRect(texes[eint(Tex::blank)], wgt->barRect(), frame, Color::dark);
		renderer->drawRect(texes[eint(Tex::blank)], wgt->sliderRect(), frame, Color::light);
	}
}

void DrawSys::drawLabel(const Label* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		if (wgt->showBg)
			renderer->drawRect(texes[eint(Tex::blank)], rect, frame, Color::normal);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), Color::text);
	}
}

void DrawSys::drawPushButton(const PushButton* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), Color::text);
	}
}

void DrawSys::drawIconButton(const IconButton* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		if (wgt->getTex())
			renderer->drawRect(wgt->getTex(), wgt->texRect(), frame, Color::texture);
	}
}

void DrawSys::drawIconPushButton(const IconPushButton* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		if (wgt->getIconTex())
			renderer->drawRect(wgt->getIconTex(), wgt->iconRect(), frame, Color::texture);
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), Color::text);
	}
}

void DrawSys::drawLabelEdit(const LabelEdit* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, wgt->getBgColor());
		if (wgt->getTextTex())
			renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), Color::text);
	}
}

void DrawSys::drawCaret(const Recti& rect, const Recti& frame, const Recti& view) noexcept {
	if (rect.overlaps(view))
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, Color::text);
}

void DrawSys::drawWindowArranger(const WindowArranger* wgt, const Recti& view) noexcept {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, Color::dark);
		for (const auto& [id, dsp] : wgt->getDisps())
			if (!wgt->draggingDisp(id)) {
				WindowArranger::DspDisp di = wgt->dispRect(id, dsp);
				drawWaDisp(di.rect, di.color, di.text, di.tex, frame, view);
			}
	}
}

void DrawSys::drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view) noexcept {
	if (rect.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rect, frame, color);
		if (tex)
			renderer->drawRect(tex, text, frame, Color::text);
	}
}

void DrawSys::drawScrollArea(const ScrollArea* box, const Recti& view) noexcept {
	uvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (uint i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(texes[eint(Tex::blank)], bar, frame, Color::dark);
		renderer->drawRect(texes[eint(Tex::blank)], box->sliderRect(), frame, Color::light);
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) noexcept {
	uvec2 vis = box->visibleWidgets();
	for (uint i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); box->showBar() && bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(texes[eint(Tex::blank)], bar, frame, Color::dark);
		renderer->drawRect(texes[eint(Tex::blank)], box->sliderRect(), frame, Color::light);
	}
}

void DrawSys::drawPopup(const Popup* box, const Recti& view) noexcept {
	if (Recti rect = box->rect(); rect.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rect, box->frame(), box->bgColor);
		for (Widget* it : box->getWidgets())
			it->drawSelf(view);
	}
}

void DrawSys::drawTooltip(const Recti& view) noexcept {
	Recti rct(World::winSys()->mousePos() + ivec2(0, cursorHeight), texes[eint(Tex::tooltip)] ? ivec2(texes[eint(Tex::tooltip)]->getRes()) + tooltipMargin * 2 : ivec2(0));
	if (rct.x + rct.w > viewRes.x)
		rct.x = viewRes.x - rct.w;
	if (rct.y + rct.h > viewRes.y)
		rct.y = rct.y - cursorHeight - rct.h;

	if (rct.overlaps(view)) {
		renderer->drawRect(texes[eint(Tex::blank)], rct, view, Color::tooltip);
		renderer->drawRect(texes[eint(Tex::tooltip)], Recti(rct.pos() + tooltipMargin, texes[eint(Tex::tooltip)]->getRes()), rct, Color::text);
	}
}

bool DrawSys::prepareTooltip() noexcept {
	auto but = dynamic_cast<Button*>(World::scene()->getSelect());
	if (!but)
		return false;
	const char* tip = but->getTooltip();
	if (!strfilled(tip))
		return false;
	if (tip == curTooltip)
		return true;
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
	renderer->waitIdle();
	return renderer->texFromText(texes[eint(Tex::tooltip)], fonts.renderText(curTooltip, tooltipHeight, width));
}
