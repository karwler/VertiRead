#include "renderer.h"
#include <SDL_cpuinfo.h>
#include <SDL_log.h>
#include <SDL_timer.h>

// RENDERER

Renderer::Info::Device::Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory) noexcept :
	id(vendev),
	name(std::move(devname)),
	dmem(memory)
{}

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

SDL_Surface* Renderer::convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format) noexcept {
	SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, format, 0);
	SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* Renderer::limitSize(SDL_Surface* img, uint32 limit) noexcept {
	if (img && (uint32(img->w) > limit || uint32(img->h) > limit)) {
		float scale = float(limit) / float(img->w > img->h ? img->w : img->h);
		SDL_Surface* dst = SDL_CreateSurface(float(img->w) * scale, float(img->h) * scale, surfaceFormat(img));
		if (dst)
			surfaceScaleLinear(img, nullptr, dst, nullptr);
		SDL_FreeSurface(img);
		img = dst;
	}
	return img;
}

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
	return Rectf(vec2(isct.pos() - rect.pos()) * fac, glm::ceil(vec2(isct.size()) * fac));
}

void Renderer::copyTextPixels(void* dst, const Pixmap& pm, uvec2 res, uint dpitch) noexcept {
	auto dp = static_cast<uint8*>(dst);
	const uint8* sp = pm.pix.get();
	for (uint r = 0; r < res.y; ++r, dp += dpitch, sp += pm.res.x)
		for (uint c = 0, o = 0; c < res.x; ++c, o += 4) {
			dp[o] = dp[o + 1] = dp[o + 2] = 0xFF;
			dp[o + 3] = sp[c];
		}
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

// RENDERER SF

RendererSf::RendererSf(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor) :
	Renderer(windows.size(), uint(std::sqrt(INT_MAX / 4))),
	bgColor(colorToBytes(bgcolor))
{
	try {
		if (!vofs) {
			SDL_Surface* srf = SDL_GetWindowSurface(windows[0]);
			if (!srf)
				throw std::runtime_error(SDL_GetError());
			viewRes = ivec2(srf->w, srf->h);
			views[0] = new View(windows[0], Recti(ivec2(0), viewRes));
		} else
			for (size_t i = 0; i < views.size(); ++i) {
				SDL_Surface* srf = SDL_GetWindowSurface(windows[i]);
				if (!srf)
					throw std::runtime_error(SDL_GetError());
				Recti wrect(vofs[i] - vofs[views.size()], srf->w, srf->h);
				viewRes = glm::max(viewRes, wrect.end());
				views[i] = new View(windows[i], wrect);
			}
		setVsync(sets->vsync);
		setCompression(sets->compression);
		setMaxPicRes(sets->maxPicRes);
		recommendPicRamLimit(sets->picLim.size);
		lastDraw = SDL_GetPerformanceCounter();
	} catch (...) {
		cleanup();
		throw;
	}
}

RendererSf::~RendererSf() {
	cleanup();
}

void RendererSf::cleanup() noexcept {
	for (View* it : views)
		delete it;
}

void RendererSf::setClearColor(const vec4& color) {
	bgColor = colorToBytes(color);
}

void RendererSf::setVsync(bool vsync) {
	drawDelay = 0;
	if (vsync) {
		uint64 freq = SDL_GetPerformanceFrequency();
#if SDL_VERSION_ATLEAST(3, 0, 0)
		float maxHz = 0.f;
		if (int cnt; SDL_DisplayID* dids = SDL_GetDisplays(&cnt)) {
			for (int i = 0; i < cnt; ++i)
				if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(dids[i]); mode && mode->refresh_rate > maxHz)
					maxHz = mode->refresh_rate;
			SDL_free(dids);
		}
		if (maxHz > 0.f)
			drawDelay = float(freq) / maxHz;
#else
		int maxHz = 0;
		SDL_DisplayMode mode;
		for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
			if (!SDL_GetDesktopDisplayMode(i, &mode) && mode.refresh_rate > maxHz)
				maxHz = mode.refresh_rate;
		if (maxHz)
			drawDelay = freq / uint(maxHz);
#endif
	}
}

void RendererSf::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_Surface* srf = SDL_GetWindowSurface(views[0]->win);
		if (!srf)
			throw std::runtime_error(SDL_GetError());
		viewRes = ivec2(srf->w, srf->h);
	}
}

void RendererSf::startDraw(View* view) {
	if (curViewSrf = SDL_GetWindowSurface(view->win); !curViewSrf)
		throw std::runtime_error(SDL_GetError());
	curViewPos = view->rect.pos();
	SDL_FillRect(curViewSrf, nullptr, SDL_MapSurfaceRGBA(curViewSrf, bgColor.r, bgColor.g, bgColor.b, bgColor.a));
}

void RendererSf::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	if (Recti isct; SDL_IntersectRect(&rect.asRect(), &frame.asRect(), &isct.asRect())) {
		Recti crop = cropTexRect(isct, rect, tex->getRes());
		isct.pos() -= curViewPos;
		u8vec4 bclr = colorToBytes(color);
		SDL_Surface* img = static_cast<const TextureSf*>(tex)->srf;
		SDL_SetSurfaceColorMod(img, bclr.r, bclr.g, bclr.b);
		SDL_SetSurfaceAlphaMod(img, bclr.a);
		surfaceScaleLinear(img, &crop.asRect(), curViewSrf, &isct.asRect());
	}
}

void RendererSf::finishDraw(View* view) {
	uint64 now = SDL_GetPerformanceCounter();
	if (uint64 timeout = lastDraw + drawDelay; !SDL_TICKS_PASSED(now, timeout))
		SDL_Delay((timeout - now) * 1000 / SDL_GetPerformanceFrequency());
	SDL_UpdateWindowSurface(view->win);
	lastDraw = now;
}

Texture* RendererSf::texFromEmpty() {
	return new TextureSf(uvec2(0), nullptr);
}

Texture* RendererSf::texFromIcon(SDL_Surface* img) noexcept {
	if (img = limitSize(img, maxTextureSize); img) {
		SDL_SetSurfaceRLE(img, SDL_TRUE);
		return new TextureSf(uvec2(img->w, img->h), img);
	}
	return nullptr;
}

bool RendererSf::texFromIcon(Texture* tex, SDL_Surface* img) noexcept {
	if (img = limitSize(img, maxTextureSize); img) {
		SDL_SetSurfaceRLE(img, SDL_TRUE);

		auto stx = static_cast<TextureSf*>(tex);
		SDL_FreeSurface(stx->srf);
		stx->res = uvec2(img->w, img->h);
		stx->srf = img;
		return true;
	}
	return false;
}

Texture* RendererSf::texFromRpic(SDL_Surface* img) noexcept {
	if (img) {
		SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_NONE);
		return new TextureSf(uvec2(img->w, img->h), img);
	}
	return nullptr;
}

Texture* RendererSf::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x)
		if (SDL_Surface* img = SDL_CreateSurface(std::min(pm.res.x, maxTextureSize), std::min(pm.res.y, maxTextureSize), SDL_PIXELFORMAT_ABGR4444)) {
			copyTextPixels(img, pm);
			SDL_SetSurfaceRLE(img, SDL_TRUE);
			return new TextureSf(uvec2(img->w, img->h), img);
		}
	return nullptr;
}

bool RendererSf::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (pm.res.x)
		if (SDL_Surface* img = SDL_CreateSurface(std::min(pm.res.x, maxTextureSize), std::min(pm.res.y, maxTextureSize), SDL_PIXELFORMAT_ABGR4444)) {
			copyTextPixels(img, pm);
			SDL_SetSurfaceRLE(img, SDL_TRUE);

			auto stx = static_cast<TextureSf*>(tex);
			SDL_FreeSurface(stx->srf);
			stx->res = uvec2(img->w, img->h);
			stx->srf = img;
			return true;
		}
	return false;
}

void RendererSf::copyTextPixels(SDL_Surface* img, const Pixmap& pm) noexcept {
	auto dp = static_cast<uint8*>(img->pixels);
	const uint8* sp = pm.pix.get();
	for (int r = 0; r < img->h; ++r, dp += img->pitch, sp += pm.res.x)
		for (int c = 0; c < img->w; ++c)
			static_cast<uint16*>(static_cast<void*>(dp))[c] = (uint16(sp[c]) << 12) | 0x0FFF;
}

void RendererSf::freeTexture(Texture* tex) noexcept {
	if (auto stx = static_cast<TextureSf*>(tex)) {
		SDL_FreeSurface(stx->srf);
		delete stx;
	}
}

void RendererSf::setCompression(Settings::Compression cmpr) noexcept {
	compression = cmpr;
}

SDL_Surface* RendererSf::prepareImage(SDL_Surface* img, bool rpic) const noexcept {
	if (img = limitSize(img, rpic ? maxPictureSize : maxTextureSize); img)
		switch (compression) {
		using enum Settings::Compression;
		case b8:
			return convertReplace(img, SDL_PIXELFORMAT_RGB332);
		case b16:
			return convertReplace(img, SDL_ISPIXELFORMAT_ALPHA(surfaceFormat(img)) ? SDL_PIXELFORMAT_ABGR1555 : SDL_PIXELFORMAT_BGR565);
		}
	return img;
}

Renderer::Info RendererSf::getInfo() const noexcept {
	return Info{
		.compressions = { Settings::Compression::none, Settings::Compression::b8, Settings::Compression::b16 },
		.texSize = maxTextureSize,
		.curCompression = compression
	};
}
