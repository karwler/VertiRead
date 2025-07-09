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
#include <glm/gtc/color_space.hpp>
#include <stdexcept>

// RENDERER

Renderer::Info::Device::Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory) noexcept :
	id(vendev),
	name(std::move(devname)),
	dmem(memory)
{}

uint32* Renderer::PixmapColor::fromText(const Pixmap& pm, uvec2 res) {
	uint size = res.x * res.y;
	if (size > len) {
		pix = std::make_unique_for_overwrite<uint32[]>(size);
		len = size;
	}
	uint32* dp = pix.get();
	const uint8* sp = pm.pix.get();
	for (uint i = 0; i < size; ++i)
		dp[i] = (uint32(sp[i]) << 24) | 0x00FFFFFF;
	return pix.get();
}

Renderer::Renderer(uint8 viewcnt, uint maxTexRes) noexcept :
	viewRefs(std::make_unique_for_overwrite<View*[]>(viewcnt)),
	maxTextureSize(maxTexRes),
	numViews(viewcnt)
{}

Renderer::Action Renderer::beginRender() noexcept {
	return Action::yes;
}

Renderer::Action Renderer::finishRender() noexcept {
	return Action::yes;
}

Renderer::View* Renderer::findView(SDL_Window* win) noexcept {
	for (uint8 i = 0; i < numViews; ++i)
		if (viewRefs[i]->win == win)
			return viewRefs[i];
	return nullptr;
}

Renderer::View* Renderer::findView(ivec2 point) noexcept {
	for (uint8 i = 0; i < numViews; ++i)
		if (viewRefs[i]->rect.contains(point))
			return viewRefs[i];
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
			vecv[i] = glm::convertSRGBToLinear(dvec4(vecv[i]));
	} else if (gamma22)
		for (size_t i = 0; i < num; ++i)
			for (uint c = 0; c < 3; ++c)
				vecv[i][c] = std::pow(double(vecv[i][c]), 2.2);
}

// RENDERER SF

RendererSf::RendererSf(InitParams& initParams, Settings* sets) :
	Renderer(initParams.numWindows, std::sqrt(INT_MAX / 4)),
	views(std::make_unique<ViewSf[]>(initParams.numWindows))
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
		for (uint8 i = 0; i < numViews; ++i) {
			viewRefs[i] = &views[i];
			views[i].win = initParams.windows[i];
			views[i].rect.pos() = initParams.vofs[i] - initParams.vofs[numViews];
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(initParams.windows[i], &views[i].rect.w, &views[i].rect.h);
#else
			SDL_GetWindowSize(initParams.windows[i], &views[i].rect.w, &views[i].rect.h);
#endif
			initParams.viewRes = glm::max(initParams.viewRes, views[i].rect.end());
#ifdef WITH_SDL3
			createRenderer(views[i], rendererProps);
#else
			createRenderer(views[i], rendererFlags);
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
	for (uint8 i = 0; i < numViews; ++i)
		SDL_DestroyRenderer(views[i].renderer);
}

#ifdef WITH_SDL3
void RendererSf::createRenderer(ViewSf& view, SDL_PropertiesID props) {
	SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, view.win);
	if (view.renderer = SDL_CreateRendererWithProperties(props); !view.renderer)
		throw std::runtime_error(SDL_GetError());
	if (SDL_PropertiesID rprop = SDL_GetRendererProperties(view.renderer)) {
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
void RendererSf::createRenderer(ViewSf& view, SDL_RendererFlags flags) {
	if (view.renderer = SDL_CreateRenderer(view.win, -1, flags); !view.renderer)
		throw std::runtime_error(SDL_GetError());
	if (SDL_RendererInfo info; !SDL_GetRendererInfo(view.renderer, &info)) {
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
	for (uint8 i = 0; i < numViews; ++i) {
		const vec4& bgclr = colors[eint(Color::background)];
		SDL_SetRenderDrawColorFloat(views[i].renderer, bgclr.r, bgclr.g, bgclr.b, bgclr.a);
	}
	std::copy(colors.begin(), colors.end() - 1, rectColors.begin());
#else
	for (uint8 i = 0; i < numViews; ++i) {
		u8vec4 bgclr = colorToBytes(colors[eint(Color::background)]);
		SDL_SetRenderDrawColor(views[i].renderer, bgclr.r, bgclr.g, bgclr.b, bgclr.a);
	}
	std::transform(colors.begin(), colors.end() - 1, rectColors.begin(), colorToBytes);
#endif
}

bool RendererSf::setSettings(Settings* sets) {
	for (uint8 i = 0; i < numViews; ++i)
		SDL_RenderSetVSync(views[i].renderer, sets->vsync);
	setCompression(sets);
	return false;
}

void RendererSf::setCompression(Settings* sets) noexcept {
	if (sets->compression != Settings::Compression::none && !(sets->compression == Settings::Compression::b16 && canTexturesB16()))
		sets->compression = Settings::Compression::none;
	compression = sets->compression;
}

bool RendererSf::updateView(ivec2& viewRes) {
	if (numViews == 1) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
		SDL_GetWindowSizeInPixels(views[0].win, &views[0].rect.w, &views[0].rect.h);
#else
		SDL_GetWindowSize(views[0].win, &views[0].rect.w, &views[0].rect.h);
#endif
		if (views[0].rect.size() != viewRes) {
			viewRes = views[0].rect.size();
			return true;
		}
	}
	return false;
}

Renderer::Action RendererSf::startDraw(uint vid) noexcept {
	curView = vid;
	SDL_RenderClear(views[vid].renderer);
	return Action::yes;
}

void RendererSf::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
	if (Recti isct; SDL_IntersectRect(&rect.asRect(), &frame.asRect(), &isct.asRect())) {
		isct.pos() -= views[curView].rect.pos();
		auto stx = static_cast<const TextureSf*>(tex)->tex;
#ifdef WITH_SDL3
		const vec4& bclr = rectColors[eint(color)];
		SDL_SetTextureColorModFloat(stx, bclr.r, bclr.g, bclr.b);
		SDL_SetTextureAlphaModFloat(stx, bclr.a);
		SDL_RenderTexture(views[curView].renderer, stx, &cropTexRect(isct, rect, tex->getRes()).asFRect(), &Rectf(isct).asFRect());
#else
		const u8vec4& bclr = rectColors[eint(color)];
		SDL_SetTextureColorMod(stx, bclr.r, bclr.g, bclr.b);
		SDL_SetTextureAlphaMod(stx, bclr.a);
		SDL_RenderCopy(views[curView].renderer, stx, &Recti(cropTexRect(isct, rect, tex->getRes())).asRect(), &isct.asRect());
#endif
	}
}

Renderer::Action RendererSf::finishDraw(uint vid) noexcept {
	SDL_RenderPresent(views[vid].renderer);
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
		if (tex = SDL_CreateTextureFromSurface(views[0].renderer, img); tex) {
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

Renderer::Info RendererSf::getInfo() const {
	Info info = {
		.compressions = { Settings::Compression::none },
		.texSize = maxTextureSize,
		.curCompression = compression
	};
	if (canTexturesB16())
		info.compressions.push_back(Settings::Compression::b16);
	return info;
}
