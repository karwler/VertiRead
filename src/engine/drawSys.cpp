#include "rendererDx.h"
#include "rendererGl.h"
#include "rendererVk.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "utils/compare.h"
#include "utils/layouts.h"
#include <cwctype>
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

// FONT SET

FontSet::FontSet() {
	if (FT_Error err = FT_Init_FreeType(&lib))
		throw std::runtime_error(FT_Error_String(err));
}

FontSet::~FontSet() {
	clearCache();
	FT_Done_Face(face);
	FT_Done_FreeType(lib);
}

void FontSet::init(const fs::path& path, Settings::Hinting hinting) {
	clearCache();
	FT_Done_Face(face);
	face = nullptr;
	setMode(hinting);

	fontData = FileSys::readBinFile(path);
	if (FT_Error err = FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(fontData.data()), fontData.size(), 0, &face))
		throw std::runtime_error(FT_Error_String(err));

	if (FT_Error err = FT_Set_Pixel_Sizes(face, 0, fontTestHeight))
		throw std::runtime_error(FT_Error_String(err));
	int ymin = 0, ymax = 0;
	for (char it : fontTestString) {
		if (FT_Error err = FT_Load_Char(face, it, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY))
			throw std::runtime_error(FT_Error_String(err));
		ymin = std::min(ymin, face->glyph->bitmap_top - int(face->glyph->bitmap.rows));
		ymax = std::max(ymax, face->glyph->bitmap_top);
	}
	ymin -= fontTestMinPad;
	ymax += fontTestMaxPad;

	float ydiff = float(ymax - ymin);
	heightScale = float(fontTestHeight) / ydiff;
	baseScale = float(ymax) / ydiff;
}

void FontSet::clearCache() {
	for (auto& [size, glyphs] : glyphCache)
		for (FT_BitmapGlyph it : glyphs)
			FT_Done_Glyph(&it->root);
	glyphCache.clear();
	height = 0;
}

Pixmap FontSet::renderText(string_view text, uint size) {
	uint width = measureText(text, size);
	if (!width)
		return Pixmap();
	ptr = text.begin();
	len = text.length();
	cpos = 0;
	xpos = 0;
	ypos = uint(float(size) * baseScale);

	array<FT_BitmapGlyph, cacheSize>& glyphs = glyphCache.at(height);
	Pixmap pm(uvec2(width, size));
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (std::iswcntrl(ch)) {
			if (ch == '\t')
				advanceTab<false>(glyphs);
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			copyGlyph(pm, width, glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
			advanceChar<true>(ch, prev, glyphs[id]->root.advance.x);
		} else {
			if (FT_Error err = FT_Load_Char(face, ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode)))
				throw std::runtime_error(FT_Error_String(err));
			copyGlyph(pm, width, face->glyph->bitmap, face->glyph->bitmap_top, face->glyph->bitmap_left);
			advanceChar<false>(ch, prev, face->glyph->advance.x);
		}
	}
	return pm;
}

uint FontSet::measureText(string_view text, uint size) {
	if (!setSize(text, size))
		return 0;
	ptr = text.begin();
	len = text.length();
	cpos = 0;
	xpos = 0;
	mfin = 0;

	array<FT_BitmapGlyph, cacheSize>& glyphs = glyphCache.at(height);
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (std::iswcntrl(ch)) {
			if (ch == '\t')
				advanceTab<false>(glyphs);
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			cacheGlyph(glyphs, ch, id);
			advanceChar<true, false>(ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			if (FT_Error err = FT_Load_Char(face, ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY))
				throw std::runtime_error(FT_Error_String(err));
			advanceChar<false, false>(ch, prev, face->glyph->advance.x, face->glyph->bitmap_left, face->glyph->bitmap.width);
		}
	}
	return mfin;
}

Pixmap FontSet::renderText(string_view text, uint size, uint limit) {
	auto [width, ln] = measureText(text, size, limit);
	if (!width)
		return Pixmap();
	ypos = uint(float(size) * baseScale);

	array<FT_BitmapGlyph, cacheSize>& glyphs = glyphCache.at(height);
	Pixmap pm(uvec2(width, (ln.size() - 1) * size));
	for (size_t i = 0; i < ln.size() - 1; ++i) {
		ptr = ln[i];
		len = ln[i + 1] - ln[i];
		cpos = 0;
		xpos = 0;

		for (char32_t ch, prev = '\0'; len; prev = ch) {
			ch = mbstowc(ptr, len);
			if (std::iswcntrl(ch)) {
				if (ch == '\t')
					advanceTab<true>(glyphs);
				continue;
			}

			if (uint id = ch - ' '; id < glyphs.size()) {
				if (xpos + glyphs[id]->left + glyphs[id]->bitmap.width <= width) {
					copyGlyph(pm, width, glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
					advanceChar<true>(ch, prev, glyphs[id]->root.advance.x);
				}
			} else {
				if (FT_Error err = FT_Load_Char(face, ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode)))
					throw std::runtime_error(FT_Error_String(err));
				if (xpos + face->glyph->bitmap_left + face->glyph->bitmap.width <= width) {
					copyGlyph(pm, width, face->glyph->bitmap, face->glyph->bitmap_top, face->glyph->bitmap_left);
					advanceChar<false>(ch, prev, face->glyph->advance.x);
				}
			}
		}
		ypos += size;
	}
	return pm;
}

pair<uint, vector<string_view::iterator>> FontSet::measureText(string_view text, uint size, uint limit) {
	if (!setSize(text, size))
		return pair(0, vector<string_view::iterator>());
	ptr = text.begin();
	len = text.length();
	wordStart = ptr;
	cpos = 0;
	xpos = 0;
	mfin = 0;
	gXpos = 0;

	array<FT_BitmapGlyph, cacheSize>& glyphs = glyphCache.at(height);
	vector<string_view::iterator> ln = { text.begin() };
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (std::iswcntrl(ch)) {
			switch (ch) {
			case '\t':
				if (advanceTab<true>(glyphs); xpos + mfin >= limit) {
					advanceLine(ln, ptr, size);
					resetLine();
				}
				break;
			case '\n':
				advanceLine(ln, ptr, size);
				resetLine();
			}
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			cacheGlyph(glyphs, ch, id);
			if (breakLine(ln, size, limit, glyphs[id]->left, glyphs[id]->bitmap.width))
				advanceChar<true, true>(ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			if (FT_Error err = FT_Load_Char(face, ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY))
				throw std::runtime_error(FT_Error_String(err));
			if (breakLine(ln, size, limit, face->glyph->bitmap_left, face->glyph->bitmap.width))
				advanceChar<false, true>(ch, prev, face->glyph->advance.x, face->glyph->bitmap_left, face->glyph->bitmap.width);
		}

		if (std::iswblank(ch) || std::iswpunct(ch)) {
			wordStart = ptr;
			gXpos += xpos;
			xpos = 0;
		}
	}
	ln.push_back(text.end());
	return pair(mfin, std::move(ln));
}

template <bool ml>
void FontSet::advanceTab(array<FT_BitmapGlyph, cacheSize>& glyphs) {
	cacheGlyph(glyphs, ' ', 0);
	uint n = coalesce(uint(cpos % tabsize), tabsize);
	FT_Pos adv = glyphs[0]->root.advance.x >> 16;
	if constexpr (ml)
		mfin = gXpos + xpos;
	else
		mfin = xpos;
	mfin += adv * (n - 1) + glyphs[0]->left + glyphs[0]->bitmap.width;
	xpos += adv * n;
	cpos += n;
}

template <bool cached>
void FontSet::advanceChar(char32_t ch, char32_t prev, long advance) {
	FT_Vector kerning;
	if (FT_Error err = FT_Get_Kerning(face, prev, ch, FT_KERNING_DEFAULT, &kerning))
		throw std::runtime_error(FT_Error_String(err));
	if constexpr (cached)
		xpos += (advance >> 16) + (kerning.x >> 6);
	else
		xpos += (advance + kerning.x) >> 6;
	++cpos;
}

template <bool cached, bool ml>
void FontSet::advanceChar(char32_t ch, char32_t prev, long advance, int left, uint width) {
	if constexpr (ml)
		mfin = gXpos + xpos;
	else
		mfin = xpos;
	mfin += left + width;
	advanceChar<cached>(ch, prev, advance);
}

bool FontSet::breakLine(vector<string_view::iterator>& ln, uint size, uint limit, int left, uint width) {
	if (gXpos + xpos + left + width <= limit)
		return true;
	if (mfin <= limit) {
		advanceLine(ln, wordStart, size);
		resetLine();
		return false;
	}
	if (left + width <= limit) {
		advanceLine(ln, ptr, size);
		return true;
	}
	return false;
}

void FontSet::advanceLine(vector<string_view::iterator>& ln, string_view::iterator pos, uint size) {
	ln.push_back(pos);
	wordStart = pos;
	ypos += size;
}

void FontSet::resetLine() {
	mfin = 0;
	cpos = 0;
	xpos = 0;
	gXpos = 0;
}

bool FontSet::setSize(string_view text, uint size) {
	size = uint(float(size) * heightScale);
	if (!(text.length() && size))
		return false;

	if (size != height) {
		if (FT_Error err = FT_Set_Pixel_Sizes(face, 0, size))
			throw std::runtime_error(FT_Error_String(err));
		if (auto [it, isnew] = glyphCache.try_emplace(size); isnew)
			it->second.fill(nullptr);
		height = size;
	}
	return true;
}

void FontSet::cacheGlyph(array<FT_BitmapGlyph, cacheSize>& glyphs, char32_t ch, uint id) {
	if (!glyphs[id]) {
		FT_Glyph glyph;
		if (FT_Error err = FT_Load_Char(face, ch, FT_LOAD_IGNORE_TRANSFORM))
			throw std::runtime_error(FT_Error_String(err));
		if (FT_Error err = FT_Get_Glyph(face->glyph, &glyph))
			throw std::runtime_error(FT_Error_String(err));
		if (glyph->format != FT_GLYPH_FORMAT_BITMAP)
			if (FT_Error err = FT_Glyph_To_Bitmap(&glyph, FT_Render_Mode(mode), nullptr, true)) {
				FT_Done_Glyph(glyph);
				throw std::runtime_error(FT_Error_String(err));
			}
		glyphs[id] = reinterpret_cast<FT_BitmapGlyph>(glyph);
	}
}

void FontSet::copyGlyph(Pixmap& pm, uint dpitch, const FT_Bitmap_& bmp, int top, int left) {
	uint32* dst;
	cbyte* src;
	int offs = ypos - top;
	if (offs >= 0) {
		dst = pm.pix.get() + uint(offs) * dpitch + xpos + left;
		src = reinterpret_cast<cbyte*>(bmp.buffer);
	} else {
		dst = pm.pix.get() + xpos + left;
		src = reinterpret_cast<cbyte*>(bmp.buffer) + uint(-offs) * bmp.pitch;
	}
	uint bot = offs + bmp.rows;
	uint rows = bot <= pm.res.y ? bmp.rows : bmp.rows - bot + pm.res.y;

	for (uint r = 0; r < rows; ++r, dst += dpitch, src += bmp.pitch)
		for (uint c = 0; c < bmp.width; ++c)
			dst[c] = 0x00FFFFFF | (uint32(std::max(reinterpret_cast<cbyte*>(dst + c)[3], src[c])) << 24);
}

void FontSet::setMode(Settings::Hinting hinting) {
	height = 0;
	mode = array<int, size_t(Settings::Hinting::mono) + 1>{ FT_RENDER_MODE_NORMAL, FT_RENDER_MODE_MONO }[eint(hinting)];
}

// DRAW SYS

DrawSys::DrawSys(const umap<int, SDL_Window*>& windows, Settings* sets, const FileSys* fileSys, int iconSize) :
	colors(fileSys->loadColors(sets->setTheme(sets->getTheme(), fileSys->getAvailableThemes())))
{
	ivec2 origin(INT_MAX);
	for (auto [id, rct] : sets->displays)
		origin = glm::min(origin, rct.pos());

	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECTX
	case directx:
		renderer = new RendererDx(windows, sets, viewRes, origin, colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_OPENGL
	case opengl:
		renderer = new RendererGl(windows, sets, viewRes, origin, colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderer = new RendererVk(windows, sets, viewRes, origin, colors[eint(Color::background)]);
#endif
	}

	SDL_Surface* white = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
	if (!white)
		throw std::runtime_error(std::format("Failed to create blank texture: {}", SDL_GetError()));
	SDL_FillRect(white, nullptr, 0xFFFFFFFF);
	if (blank = renderer->texFromIcon(white); !blank)
		throw std::runtime_error("Failed to create blank texture");
	texes.emplace(string(), blank);

	for (const fs::directory_entry& it : fs::directory_iterator(fileSys->dirIcons(), fs::directory_options::skip_permission_denied))
		if (Texture* tex = renderer->texFromIcon(loadIcon(fromPath(it.path()), iconSize)))
			texes.emplace(fromPath(it.path().stem()), tex);
	setFont(toPath(sets->font), sets, fileSys);
	renderer->synchTransfer();
}

DrawSys::~DrawSys() {
	if (renderer) {
		for (auto& [name, tex] : texes)
			renderer->freeTexture(tex);
		delete renderer;
	}
}

SDL_Surface* DrawSys::loadIcon(const string& path, int size) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (SDL_RWops* ifh = SDL_RWFromFile(path.c_str(), "rb")) {
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
	return IMG_Load(path.c_str());
#endif
}

void DrawSys::updateView() {
	renderer->updateView(viewRes);
	fonts.clearCache();
}

int DrawSys::findPointInView(ivec2 pos) const {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(pos);
	return vit != renderer->getViews().end() ? vit->first : Renderer::singleDspId;
}

void DrawSys::setTheme(string_view name, Settings* sets, const FileSys* fileSys) {
	colors = fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
	renderer->setClearColor(colors[eint(Color::background)]);
}

void DrawSys::setFont(const fs::path& font, Settings* sets, const FileSys* fileSys) {
	fs::path path = fileSys->findFont(font);
	if (FileSys::isFont(path))
		sets->font = fromPath(font);
	else {
		sets->font = Settings::defaultFont;
		path = fileSys->findFont(Settings::defaultFont);
	}
	fonts.init(path, sets->hinting);
}

pair<Texture*, bool> DrawSys::texture(const string& name) const {
	try {
		return pair(texes.at(name), false);
	} catch (const std::out_of_range&) {
		logError("Texture ", name, " doesn't exist");
	}
	return pair(blank, false);
}

void DrawSys::drawWidgets(Scene* scene, bool mouseLast) {
	for (auto [id, view] : renderer->getViews()) {
		try {
			renderer->startDraw(view);

			// draw main widgets and visible overlays
			scene->getLayout()->drawSelf(view->rect);
			if (scene->getOverlay() && scene->getOverlay()->on)
				scene->getOverlay()->drawSelf(view->rect);

			// draw popup if exists and dim main widgets
			if (scene->getPopup()) {
				renderer->drawRect(blank, view->rect, view->rect, colorPopupDim);
				scene->getPopup()->drawSelf(view->rect);
			}

			// draw context menu
			if (scene->getContext())
				scene->getContext()->drawSelf(view->rect);

			// draw extra stuff on top
			if (scene->getCapture())
				scene->getCapture()->drawTop(view->rect);
			else if (Button* but = dynamic_cast<Button*>(scene->select); mouseLast && but)
				drawTooltip(but, view->rect);

			renderer->finishDraw(view);
		} catch (const Renderer::ErrorSkip&) {}
	}
	renderer->finishRender();
}

bool DrawSys::drawPicture(const Picture* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		if (wgt->showBG)
			renderer->drawRect(blank, rect, frame, colors[eint(wgt->color())]);
		if (wgt->getTex())
			renderer->drawRect(wgt->getTex(), wgt->texRect(), frame, colors[eint(Color::texture)]);
		return true;
	}
	return false;
}

void DrawSys::drawCheckBox(const CheckBox* wgt, const Recti& view) {
	if (drawPicture(wgt, view))																	// draw background
		renderer->drawRect(blank, wgt->boxRect(), wgt->frame(), colors[eint(wgt->boxColor())]);	// draw checkbox
}

void DrawSys::drawSlider(const Slider* wgt, const Recti& view) {
	if (drawPicture(wgt, view)) {															// draw background
		Recti frame = wgt->frame();
		renderer->drawRect(blank, wgt->barRect(), frame, colors[eint(Color::dark)]);		// draw bar
		renderer->drawRect(blank, wgt->sliderRect(), frame, colors[eint(Color::light)]);	// draw slider
	}
}

void DrawSys::drawProgressBar(const ProgressBar* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[eint(Color::normal)]);			// draw background
		renderer->drawRect(blank, wgt->barRect(), frame, colors[eint(Color::light)]);	// draw bar
	}
}

void DrawSys::drawLabel(const Label* wgt, const Recti& view) {
	if (drawPicture(wgt, view) && wgt->getTextTex())
		renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[eint(Color::text)]);
}

void DrawSys::drawCaret(const Recti& rect, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view))
		renderer->drawRect(blank, rect, frame, colors[eint(Color::light)]);
}

void DrawSys::drawWindowArranger(const WindowArranger* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[eint(wgt->color())]);
		for (const auto& [id, dsp] : wgt->getDisps())
			if (!wgt->draggingDisp(id)) {
				auto [rct, color, text, tex] = wgt->dispRect(id, dsp);
				drawWaDisp(rct, color, text, tex, frame, view);
			}
	}
}

void DrawSys::drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view)) {
		renderer->drawRect(blank, rect, frame, colors[eint(color)]);
		if (tex)
			renderer->drawRect(tex, text, frame, colors[eint(Color::text)]);
	}
}

void DrawSys::drawScrollArea(const ScrollArea* box, const Recti& view) {
	mvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (size_t i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[eint(Color::dark)]);					// draw scroll bar
		renderer->drawRect(blank, box->sliderRect(), frame, colors[eint(Color::light)]);	// draw scroll slider
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) {
	mvec2 vis = box->visibleWidgets();
	for (size_t i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); box->showBar() && bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[eint(Color::dark)]);
		renderer->drawRect(blank, box->sliderRect(), frame, colors[eint(Color::light)]);
	}
}

void DrawSys::drawPopup(const Popup* box, const Recti& view) {
	if (Recti rect = box->rect(); rect.overlaps(view)) {
		renderer->drawRect(blank, rect, box->frame(), colors[eint(box->bgColor)]);	// draw background
		for (Widget* it : box->getWidgets())										// draw children
			it->drawSelf(view);
	}
}

void DrawSys::drawTooltip(Button* but, const Recti& view) {
	const Texture* tip = but->getTooltip();
	if (Recti rct = but->tooltipRect(); tip && rct.overlaps(view)) {
		renderer->drawRect(blank, rct, view, colors[eint(Color::tooltip)]);
		renderer->drawRect(tip, Recti(rct.pos() + Button::tooltipMargin, tip->getRes()), rct, colors[eint(Color::text)]);
	}
}

Widget* DrawSys::getSelectedWidget(Layout* box, ivec2 mPos) {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(mPos);
	if (vit == renderer->getViews().end())
		return nullptr;

	renderer->startSelDraw(vit->second, mPos - vit->second->rect.pos());
	box->drawAddr(vit->second->rect);
	return renderer->finishSelDraw(vit->second);
}

void DrawSys::drawPictureAddr(const Picture* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view))
		renderer->drawSelRect(wgt, rect, wgt->frame());
}

void DrawSys::drawLayoutAddr(const Layout* wgt, const Recti& view) {
	renderer->drawSelRect(wgt, wgt->rect(), wgt->frame());	// invisible background to set selection area
	for (Widget* it : wgt->getWidgets())
		it->drawAddr(view);
}
