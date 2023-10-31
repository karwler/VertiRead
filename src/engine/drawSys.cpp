#include "drawSys.h"
#include "fileSys.h"
#include "rendererDx.h"
#include "rendererGl.h"
#include "rendererVk.h"
#include "scene.h"
#include "world.h"
#include "prog/fileOps.h"
#include "utils/compare.h"
#include "utils/layouts.h"
#include <cwctype>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif

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

void FontSet::init(const fs::path& path, Settings::Hinting hinting) {
	clearCache();
	for (Font& it : fonts)
		FT_Done_Face(it.face);
	fonts.clear();
	setMode(hinting);

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
	Font face = { .data = FileOpsLocal::readFile(path) };
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
	buffer.reset();
	bufSize = 0;
}

PixmapRgba FontSet::renderText(string_view text, uint size) {
	uint width = measureText(text, size);
	if (!width)
		return PixmapRgba();
	ptr = text.begin();
	len = text.length();
	cpos = 0;
	xpos = 0;
	ypos = uint(float(size) * baseScale);

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
	uvec2 res = prepareBuffer(width, size);
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (std::iswcntrl(ch)) {
			if (ch == '\t')
				advanceTab<false>(glyphs);
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			copyGlyph(res, glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
			advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x);
		} else {
			vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
			copyGlyph(res, ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
			advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
		}
	}
	return PixmapRgba(buffer.get(), res);
}

uint FontSet::measureText(string_view text, uint size) {
	if (!setSize(text, size))
		return 0;
	ptr = text.begin();
	len = text.length();
	cpos = 0;
	xpos = 0;
	mfin = 0;

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
	for (char32_t ch, prev = '\0'; len; prev = ch) {
		ch = mbstowc(ptr, len);
		if (std::iswcntrl(ch)) {
			if (ch == '\t')
				advanceTab<false>(glyphs);
			continue;
		}

		if (uint id = ch - ' '; id < glyphs.size()) {
			cacheGlyph(glyphs, ch, id);
			advanceChar<true, false>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
			advanceChar<false, false>(ft->face, ch, prev, ft->face->glyph->advance.x, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width);
		}
	}
	return mfin;
}

PixmapRgba FontSet::renderText(string_view text, uint size, uint limit) {
	auto [width, ln] = measureText(text, size, limit);
	if (!width)
		return PixmapRgba();
	ypos = uint(float(size) * baseScale);

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
	uvec2 res = prepareBuffer(width, (ln.size() - 1) * size);
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
					copyGlyph(res, glyphs[id]->bitmap, glyphs[id]->top, glyphs[id]->left);
					advanceChar<true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x);
				}
			} else {
				vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_RENDER | FT_LOAD_TARGET_(mode));
				if (xpos + ft->face->glyph->bitmap_left + ft->face->glyph->bitmap.width <= width) {
					copyGlyph(res, ft->face->glyph->bitmap, ft->face->glyph->bitmap_top, ft->face->glyph->bitmap_left);
					advanceChar<false>(ft->face, ch, prev, ft->face->glyph->advance.x);
				}
			}
		}
		ypos += size;
	}
	return PixmapRgba(buffer.get(), res);
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

	array<FT_BitmapGlyph, cacheSize>& glyphs = asciiCache.at(height);
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
				advanceChar<true, true>(fonts[0].face, ch, prev, glyphs[id]->root.advance.x, glyphs[id]->left, glyphs[id]->bitmap.width);
		} else {
			vector<Font>::iterator ft = loadGlyph(ch, FT_LOAD_IGNORE_TRANSFORM | FT_LOAD_BITMAP_METRICS_ONLY);
			if (breakLine(ln, size, limit, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width))
				advanceChar<false, true>(ft->face, ch, prev, ft->face->glyph->advance.x, ft->face->glyph->bitmap_left, ft->face->glyph->bitmap.width);
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

uvec2 FontSet::prepareBuffer(uint resx, uint resy) {
	size_t size = size_t(resx) * size_t(resy);
	if (size > bufSize) {
		buffer = std::make_unique_for_overwrite<uint32[]>(size);
		bufSize = size;
	}
	std::fill_n(buffer.get(), size, 0);
	return uvec2(resx, resy);
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

template <bool cached, bool ml>
void FontSet::advanceChar(FT_Face face, char32_t ch, char32_t prev, long advance, int left, uint width) {
	if constexpr (ml)
		mfin = gXpos + xpos;
	else
		mfin = xpos;
	mfin += left + width;
	advanceChar<cached>(face, ch, prev, advance);
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
				logError(FT_Error_String(err));
			else if (font.face->glyph->glyph_index) {
				fonts.push_back(std::move(font));
				return fonts.end() - 1;
			}
			FT_Done_Face(font.face);
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
	}
	if (FT_Error err = FT_Load_Char(fonts[0].face, ch, flags))
		throw std::runtime_error(FT_Error_String(err));
	return fonts.begin();
}

void FontSet::copyGlyph(uvec2 res, const FT_Bitmap_& bmp, int top, int left) {
	uint32* dst;
	byte_t* src;
	int offs = ypos - top;
	if (offs >= 0) {
		dst = buffer.get() + uint(offs) * res.x + xpos + left;
		src = reinterpret_cast<byte_t*>(bmp.buffer);
	} else {
		dst = buffer.get() + xpos + left;
		src = reinterpret_cast<byte_t*>(bmp.buffer) + uint(-offs) * bmp.pitch;
	}
	uint bot = offs + bmp.rows;
	uint rows = bot <= res.y ? bmp.rows : bmp.rows - bot + res.y;

	for (uint r = 0; r < rows; ++r, dst += res.x, src += bmp.pitch)
		for (uint c = 0; c < bmp.width; ++c)
			dst[c] = 0x00FFFFFF | (uint32(std::max(reinterpret_cast<byte_t*>(dst + c)[3], src[c])) << 24);
}

void FontSet::setMode(Settings::Hinting hinting) {
	height = 0;
	mode = array<int, size_t(Settings::Hinting::mono) + 1>{ FT_RENDER_MODE_NORMAL, FT_RENDER_MODE_MONO }[eint(hinting)];
}

// DRAW SYS

DrawSys::DrawSys(const umap<int, SDL_Window*>& windows, int iconSize) :
	colors(World::fileSys()->loadColors(World::sets()->setTheme(World::sets()->getTheme(), World::fileSys()->getAvailableThemes())))
{
	ivec2 origin(INT_MAX);
	for (auto [id, rct] : World::sets()->displays)
		origin = glm::min(origin, rct.pos());

	switch (World::sets()->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECTX
	case directx:
		renderer = new RendererDx(windows, World::sets(), viewRes, origin, colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_OPENGL
	case opengl:
		renderer = new RendererGl(windows, World::sets(), viewRes, origin, colors[eint(Color::background)]);
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderer = new RendererVk(windows, World::sets(), viewRes, origin, colors[eint(Color::background)]);
#endif
	}

	SDL_Surface* white = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
	if (!white)
		throw std::runtime_error(std::format("Failed to create blank texture: {}", SDL_GetError()));
	SDL_FillRect(white, nullptr, 0xFFFFFFFF);
	if (blank = renderer->texFromIcon(white); !blank)
		throw std::runtime_error("Failed to create blank texture");
	texes.emplace(string(), blank);

	for (const fs::directory_entry& it : fs::directory_iterator(World::fileSys()->dirIcons(), fs::directory_options::skip_permission_denied))
		if (Texture* tex = renderer->texFromIcon(loadIcon(fromPath(it.path()), iconSize)))
			texes.emplace(fromPath(it.path().stem()), tex);
	setFont(toPath(World::sets()->font));
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

void DrawSys::setTheme(string_view name) {
	colors = World::fileSys()->loadColors(World::sets()->setTheme(name, World::fileSys()->getAvailableThemes()));
	renderer->setClearColor(colors[eint(Color::background)]);
}

void DrawSys::setFont(const fs::path& font) {
	fs::path path = World::fileSys()->findFont(font);
	if (FileSys::isFont(path))
		World::sets()->font = fromPath(font);
	else {
		World::sets()->font = Settings::defaultFont;
		path = World::fileSys()->findFont(Settings::defaultFont);
	}
	fonts.init(path, World::sets()->hinting);
}

pair<Texture*, bool> DrawSys::texture(const string& name) const {
	try {
		return pair(texes.at(name), false);
	} catch (const std::out_of_range&) {
		logError("Texture ", name, " doesn't exist");
	}
	return pair(blank, false);
}

void DrawSys::drawWidgets(bool mouseLast) {
	for (auto [id, view] : renderer->getViews()) {
		try {
			renderer->startDraw(view);

			// draw main widgets and visible overlays
			World::scene()->getLayout()->drawSelf(view->rect);
			if (World::scene()->getOverlay() && World::scene()->getOverlay()->on)
				World::scene()->getOverlay()->drawSelf(view->rect);

			// draw popup if exists and dim main widgets
			if (World::scene()->getPopup()) {
				renderer->drawRect(blank, view->rect, view->rect, colorPopupDim);
				World::scene()->getPopup()->drawSelf(view->rect);
			}

			// draw context menu
			if (World::scene()->getContext())
				World::scene()->getContext()->drawSelf(view->rect);

			// draw extra stuff on top
			if (World::scene()->getCapture())
				World::scene()->getCapture()->drawTop(view->rect);
			else if (Button* but = dynamic_cast<Button*>(World::scene()->select); mouseLast && but)
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
	uvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (uint i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[eint(Color::dark)]);					// draw scroll bar
		renderer->drawRect(blank, box->sliderRect(), frame, colors[eint(Color::light)]);	// draw scroll slider
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) {
	uvec2 vis = box->visibleWidgets();
	for (uint i = vis.x; i < vis.y; ++i)
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
