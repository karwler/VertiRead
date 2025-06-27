#include "renderer.h"
#ifdef WITH_SDL3
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#else
#include <SDL_cpuinfo.h>
#include <SDL_log.h>
#include <SDL_timer.h>
#include <SDL_version.h>
#endif
#include <stdexcept>

// RENDERER

Renderer::Info::Device::Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory) noexcept :
	id(vendev),
	name(std::move(devname)),
	dmem(memory)
{}

uint32* Renderer::PixmapColor::fromText(const Pixmap& pm, uvec2 res) {
	if (size_t size = res.x * res.y; size > len) {
		pix = std::make_unique_for_overwrite<uint32[]>(size);
		len = size;
	}
	uint32* dp = pix.get();
	const uint8* sp = pm.pix.get();
	for (uint r = 0; r < res.y; ++r, sp += pm.res.x)
		for (uint c = 0; c < res.x; ++c)
			*dp++ = (uint32(sp[c]) << 24) | 0x00FFFFFF;
	return pix.get();
}

Renderer::Action Renderer::finishRender() noexcept {
	return Action::yes;
}

Renderer::View* Renderer::findView(SDL_Window* win) noexcept {
	for (View* it : views)
		if (it->win == win)
			return it;
	return nullptr;
}

Renderer::View* Renderer::findView(ivec2 point) noexcept {
	for (Renderer::View* it : views)
		if (it->rect.contains(point))
			return it;
	return nullptr;
}

void Renderer::setMaxPicRes(uint& size) noexcept {
	size = std::clamp(size, Settings::minPicRes, maxTextureSize);
	maxPictureSize = size;
}

SDL_Surface* Renderer::prepareImage(SDL_Surface* img, uint8* rpbpp) const noexcept {
	if (img = limitSize(img, rpbpp ? maxPictureSize : maxTextureSize); img) {
		auto [fmt, bpp] = prepareImageFormat(img);
		if (fmt != surfaceFormat(img))
			img = convertReplace(img, fmt);
		if (rpbpp)
			*rpbpp = bpp;
	}
	return img;
}

SDL_Surface* Renderer::convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format) noexcept {
	SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, format, 0);
	SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* Renderer::limitSize(SDL_Surface* img, uint limit) noexcept {
	if (img && (uint(img->w) > limit || uint(img->h) > limit)) {
		float scale = float(limit) / float(img->w > img->h ? img->w : img->h);
		SDL_Surface* dst = SDL_CreateSurface(float(img->w) * scale, float(img->h) * scale, surfaceFormat(img));
		if (dst) {
#ifdef WITH_SDL3
			copyPalette(dst, img);
#endif
			if (sdlFailed(surfaceScaleLinear(img, nullptr, dst, nullptr))) {
				SDL_FreeSurface(dst);
				dst = nullptr;
			}
		}
		SDL_FreeSurface(img);
		img = dst;
	}
	return img;
}

#ifdef WITH_SDL3
void Renderer::copyPalette(SDL_Surface* dst, SDL_Surface* src) noexcept {
	if (SDL_ISPIXELFORMAT_INDEXED(dst->format))
		if (SDL_Palette* splt = SDL_GetSurfacePalette(src))
			if (SDL_Palette* dplt = SDL_CreatePalette(splt->ncolors)) {
				SDL_SetPaletteColors(dplt, splt->colors, 0, splt->ncolors);
				SDL_SetSurfacePalette(dst, dplt);
				SDL_DestroyPalette(dplt);
			}
}
#endif

bool Renderer::isIndexedGrayscale(SDL_Surface* img) noexcept {
	SDL_Palette* palette = surfacePalette(img);
	if (!palette || palette->ncolors != 256)
		return false;
	for (uint i = 0; i < 256; ++i)
		if (const SDL_Color& clr = palette->colors[i]; clr.r != i || clr.g != i || clr.b != i || clr.a != 255)
			return false;
	return true;
}

Rectf Renderer::cropTexRect(const Recti& isct, const Recti& rect, uvec2 texRes) noexcept {
	vec2 fac = vec2(texRes) / vec2(rect.size());
	return Rectf(glm::floor(vec2(isct.pos() - rect.pos()) * fac), glm::ceil(vec2(isct.size()) * fac));
}

void Renderer::copyPalette(uint* dst, const SDL_Palette* palette) noexcept {
	if (palette)
		memcpy(dst, palette->colors, uint(palette->ncolors) * sizeof(SDL_Color));
	else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Surface has no palette");
}

void Renderer::recommendPicRamLimit(uintptr_t& mem) noexcept {
	if (!mem)
		mem = uintptr_t(SDL_GetSystemRAM() / 2) * 1024 * 1024;
}

void Renderer::convertColors(vec4* vecv, size_t num, bool srgb, bool gamma22) noexcept {
	if (srgb) {
		for (size_t i = 0; i < num; ++i)
			for (uint c = 0; c < 3; ++c)
				vecv[i][c] = srgb2linear(vecv[i][c]);
	} else if (gamma22)
		for (size_t i = 0; i < num; ++i)
			for (uint c = 0; c < 3; ++c)
				vecv[i][c] = std::pow(double(vecv[i][c]), 2.2);
}

double Renderer::srgb2linear(double x) noexcept {
	if (x <= 0.0)
		return 0.0;
	if (x >= 1.0)
		return 1.0;
	return x < 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
}

// RENDERER SF

RendererSf::RendererSf(InitParams& initParams, Settings* sets) :
	Renderer(initParams.windows.size(), std::sqrt(INT_MAX / 4))
{
#ifdef WITH_SDL3
	sthandle<SDL_PropertiesID> rendererProps = SDL_CreateProperties();
	if (!rendererProps)
		throw std::runtime_error(SDL_GetError());
	SDL_SetStringProperty(rendererProps, SDL_PROP_RENDERER_NAME_STRING, SDL_SOFTWARE_RENDERER);
	SDL_SetNumberProperty(rendererProps, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER, sets->vsync);
#else
	SDL_RendererFlags rendererFlags = SDL_RENDERER_SOFTWARE;
	if (sets->vsync)
		rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
#endif
	try {
		for (size_t i = 0; i < views.size(); ++i) {
			Recti wrect;
			wrect.pos() = initParams.vofs[i] - initParams.vofs[views.size()];
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(initParams.windows[i], &wrect.w, &wrect.h);
#else
			SDL_GetWindowSize(initParams.windows[i], &wrect.w, &wrect.h);
#endif
			initParams.viewRes = glm::max(initParams.viewRes, wrect.end());
#ifdef WITH_SDL3
			createRenderer(static_cast<ViewSf*>(views[i] = new ViewSf(initParams.windows[i], wrect)), rendererProps);
#else
			createRenderer(static_cast<ViewSf*>(views[i] = new ViewSf(initParams.windows[i], wrect)), rendererFlags);
#endif
		}
		if (!textureFormats.contains(defaultFormat) && textureFormats.contains(SDL_PIXELFORMAT_ARGB8888))
			defaultFormat = SDL_PIXELFORMAT_ARGB8888;

		initParams.tooltipTexture = new TextureSf(uvec2(0), nullptr);
		setColors(initParams.colors);
		setCompression(sets);
		setMaxPicRes(sets->maxPicRes);
		recommendPicRamLimit(sets->picLim.size);
	} catch (const std::exception&) {
		freeTexture(initParams.tooltipTexture);
		cleanup();
		throw;
	}
}

RendererSf::~RendererSf() {
	cleanup();
}

void RendererSf::cleanup() noexcept {
	for (View* it : views) {
		auto svw = static_cast<ViewSf*>(it);
		SDL_DestroyRenderer(svw->renderer);
		delete svw;
	}
}

#ifdef WITH_SDL3
void RendererSf::createRenderer(ViewSf* view, SDL_PropertiesID props) {
	SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, view->win);
	if (view->renderer = SDL_CreateRendererWithProperties(props); !view->renderer)
		throw std::runtime_error(SDL_GetError());
	if (SDL_PropertiesID rprop = SDL_GetRendererProperties(view->renderer)) {
		if (int64 maxSize = SDL_GetNumberProperty(rprop, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0); maxSize > 0 && maxSize < maxTextureSize)
			maxTextureSize = maxSize;
		if (textureFormats.empty())
			if (auto formats = static_cast<SDL_PixelFormat*>(SDL_GetPointerProperty(rprop, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, nullptr)))
				for (SDL_PixelFormat* it = formats; *it != SDL_PIXELFORMAT_UNKNOWN; ++it)
					textureFormats.insert(*it);
	} else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
}
#else
void RendererSf::createRenderer(ViewSf* view, SDL_RendererFlags flags) {
	if (view->renderer = SDL_CreateRenderer(view->win, -1, flags); !view->renderer)
		throw std::runtime_error(SDL_GetError());
	if (SDL_RendererInfo info; !SDL_GetRendererInfo(view->renderer, &info)) {
		if (int maxSize = std::min(info.max_texture_width, info.max_texture_height); maxSize > 0 && uint(maxSize) < maxTextureSize)
			maxTextureSize = maxSize;
		if (textureFormats.empty())
			for (uint32 i = 0; i < info.num_texture_formats; ++i)
				textureFormats.insert(SDL_PixelFormatEnum(info.texture_formats[i]));
	} else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
}
#endif

void RendererSf::setColors(array<vec4, Settings::defaultColors.size()>& colors) {
#ifdef WITH_SDL3
	for (View* it : views) {
		const vec4& bgclr = colors[eint(Color::background)];
		SDL_SetRenderDrawColorFloat(static_cast<ViewSf*>(it)->renderer, bgclr.r, bgclr.g, bgclr.b, bgclr.a);
	}
	std::copy(colors.begin(), colors.end() - 1, rectColors.begin());
#else
	for (View* it : views) {
		u8vec4 bgclr = colorToBytes(colors[eint(Color::background)]);
		SDL_SetRenderDrawColor(static_cast<ViewSf*>(it)->renderer, bgclr.r, bgclr.g, bgclr.b, bgclr.a);
	}
	std::transform(colors.begin(), colors.end() - 1, rectColors.begin(), colorToBytes);
#endif
}

bool RendererSf::setSettings(Settings* sets) {
	for (View* it : views)
		SDL_RenderSetVSync(static_cast<ViewSf*>(it)->renderer, sets->vsync);
	setCompression(sets);
	return false;
}

void RendererSf::setCompression(Settings* sets) noexcept {
	if (sets->compression != Settings::Compression::none && !(sets->compression == Settings::Compression::b16 && canTexturesB16()))
		sets->compression = Settings::Compression::none;
	compression = sets->compression;
}

bool RendererSf::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		ivec2 wres;
#if SDL_VERSION_ATLEAST(2, 26, 0)
		SDL_GetWindowSizeInPixels(views[0]->win, &wres.x, &wres.y);
#else
		SDL_GetWindowSize(views[0]->win, &wres.x, &wres.y);
#endif
		if (wres != viewRes) {
			viewRes = wres;
			return true;
		}
	}
	return false;
}

Renderer::Action RendererSf::startDraw(View* view) noexcept {
	curView = static_cast<ViewSf*>(view);
	SDL_RenderClear(curView->renderer);
	return Action::yes;
}

void RendererSf::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
	if (Recti isct; SDL_IntersectRect(&rect.asRect(), &frame.asRect(), &isct.asRect())) {
		isct.pos() -= curView->rect.pos();
		auto stx = static_cast<const TextureSf*>(tex)->tex;
#ifdef WITH_SDL3
		const vec4& bclr = rectColors[eint(color)];
		SDL_SetTextureColorModFloat(stx, bclr.r, bclr.g, bclr.b);
		SDL_SetTextureAlphaModFloat(stx, bclr.a);
		SDL_RenderTexture(curView->renderer, stx, &cropTexRect(isct, rect, tex->getRes()).asFRect(), &Rectf(isct).asFRect());
#else
		const u8vec4& bclr = rectColors[eint(color)];
		SDL_SetTextureColorMod(stx, bclr.r, bclr.g, bclr.b);
		SDL_SetTextureAlphaMod(stx, bclr.a);
		SDL_RenderCopy(curView->renderer, stx, &Recti(cropTexRect(isct, rect, tex->getRes())).asRect(), &isct.asRect());
#endif
	}
}

Renderer::Action RendererSf::finishDraw(View* view) noexcept {
	SDL_RenderPresent(static_cast<ViewSf*>(view)->renderer);
	return Action::yes;
}

Texture* RendererSf::texFromSurface(SDL_Surface* img, bool, bool linear) noexcept {
	if (auto [tex, res] = createTexture(limitSize(img, maxTextureSize), linear); tex) {
		if (auto stx = new (std::nothrow) TextureSf(res, tex))
			return stx;
		SDL_DestroyTexture(tex);
	}
	return nullptr;
}

bool RendererSf::texFromSurface(Texture* tex, SDL_Surface* img, bool) noexcept {
	SDL_ScaleMode scale;
	auto stx = static_cast<TextureSf*>(tex);
	if (auto [tmp, res] = createTexture(limitSize(img, maxTextureSize), sdlFailed(SDL_GetTextureScaleMode(stx->tex, &scale)) || scale != SDL_ScaleModeNearest); tmp) {
		replaceTexture(stx, tmp, uvec2(img->w, img->h));
		return true;
	}
	return false;
}

Texture* RendererSf::texFromText(const Pixmap& pm) noexcept {
	if (auto [tex, res] = createTextureText(pm); tex) {
		if (auto stx = new (std::nothrow) TextureSf(res, tex))
			return stx;
		SDL_DestroyTexture(tex);
	}
	return nullptr;
}

bool RendererSf::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (auto [tmp, res] = createTextureText(pm); tmp) {
		replaceTexture(static_cast<TextureSf*>(tex), tmp, res);
		return true;
	}
	return false;
}

void RendererSf::freeTexture(Texture* tex) noexcept {
	if (auto stx = static_cast<TextureSf*>(tex)) {
		SDL_DestroyTexture(stx->tex);
		delete stx;
	}
}

void RendererSf::replaceTexture(TextureSf* tex, SDL_Texture* ntex, uvec2 res) noexcept {
	tex->res = res;
	SDL_DestroyTexture(tex->tex);
	tex->tex = ntex;
}

pair<SDL_Texture*, uvec2> RendererSf::createTexture(SDL_Surface* img, bool linear) noexcept {
	uvec2 res;
	SDL_Texture* tex = nullptr;
	if (img) {
		if (tex = SDL_CreateTextureFromSurface(static_cast<ViewSf*>(views[0])->renderer, img); tex) {
			res = uvec2(img->w, img->h);
			SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(tex, SDL_ScaleMode(linear));
		} else
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
		SDL_FreeSurface(img);
	}
	return pair(tex, res);
}

pair<SDL_Texture*, uvec2> RendererSf::createTextureText(const Pixmap& pm) noexcept {
	if (!pm.res.x)
		return pair(nullptr, uvec2());
	uvec2 res = uvec2(std::min(pm.res.x, maxTextureSize), std::min(pm.res.y, maxTextureSize));
	return createTexture(SDL_CreateSurfaceFrom(res.x, res.y, defaultFormat, textBuffer.fromText(pm, res), res.x * 4), false);
}

pair<SDL_PixelFormatEnum, uint8> RendererSf::prepareImageFormat(SDL_Surface* img) const noexcept {
	SDL_PixelFormatEnum fmt = surfaceFormat(img);
	if (compression == Settings::Compression::b16 && SDL_BYTESPERPIXEL(fmt) > 2)
		fmt = SDL_ISPIXELFORMAT_ALPHA(fmt) ? SDL_PIXELFORMAT_ABGR1555 : SDL_PIXELFORMAT_BGR565;

	switch (fmt) {
#ifdef WITH_SDL3
	case SDL_PIXELFORMAT_ABGR2101010:
		return pickImageFormat({ SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_XBGR2101010, SDL_PIXELFORMAT_XRGB2101010 }, fmt);
	case SDL_PIXELFORMAT_ARGB2101010:
		return pickImageFormat({ SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_XRGB2101010, SDL_PIXELFORMAT_XBGR2101010 }, fmt);
	case SDL_PIXELFORMAT_XBGR2101010:
		return pickImageFormat({ SDL_PIXELFORMAT_XBGR2101010, SDL_PIXELFORMAT_XRGB2101010, SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_ARGB2101010 }, fmt);
	case SDL_PIXELFORMAT_XRGB2101010:
		return pickImageFormat({ SDL_PIXELFORMAT_XRGB2101010, SDL_PIXELFORMAT_XBGR2101010, SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_ABGR2101010 }, fmt);
#endif
	case SDL_PIXELFORMAT_BGR565:
		return pickImageFormat({ SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGBA5551 }, fmt);
	case SDL_PIXELFORMAT_RGB565: case SDL_PIXELFORMAT_RGB332:
		return pickImageFormat({ SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551 }, fmt);
	case SDL_PIXELFORMAT_ABGR1555: case SDL_PIXELFORMAT_ABGR4444:
		return pickImageFormat({ SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_RGB565 }, fmt);
	case SDL_PIXELFORMAT_ARGB1555: case SDL_PIXELFORMAT_ARGB4444:
		return pickImageFormat({ SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565 }, fmt);
	case SDL_PIXELFORMAT_BGRA5551: case SDL_PIXELFORMAT_BGRA4444:
		return pickImageFormat({ SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_RGB565 }, fmt);
	case SDL_PIXELFORMAT_RGBA5551: case SDL_PIXELFORMAT_RGBA4444:
		return pickImageFormat({ SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565 }, fmt);
	case SDL_PIXELFORMAT_XBGR1555: case SDL_PIXELFORMAT_XBGR4444:
		return pickImageFormat({ SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_RGB565 }, fmt);
	case SDL_PIXELFORMAT_XRGB1555: case SDL_PIXELFORMAT_XRGB4444:
		return pickImageFormat({ SDL_PIXELFORMAT_XRGB1555, SDL_PIXELFORMAT_XBGR1555, SDL_PIXELFORMAT_ARGB1555, SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565 }, fmt);
#ifdef WITH_SDL3
	default:
		if (SDL_BYTESPERPIXEL(fmt) > 4)
			return pickImageFormat({ SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_XBGR2101010, SDL_PIXELFORMAT_XRGB2101010 }, defaultFormat);
#endif
	}
	return pair(fmt, 4);
}

pair<SDL_PixelFormatEnum, uint8> RendererSf::pickImageFormat(std::initializer_list<SDL_PixelFormatEnum> fmtv, SDL_PixelFormatEnum orig) const noexcept {
	for (SDL_PixelFormatEnum it : fmtv)
		if (textureFormats.contains(it))
			return pair(it, SDL_BYTESPERPIXEL(it));
	return pair(orig, 4);
}

Renderer::Info RendererSf::getInfo() const noexcept {
	Info info = {
		.compressions = { Settings::Compression::none },
		.texSize = maxTextureSize,
		.curCompression = compression
	};
	if (canTexturesB16())
		info.compressions.push_back(Settings::Compression::b16);
	return info;
}
