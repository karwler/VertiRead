#include "renderer.h"
#include <SDL2/SDL_cpuinfo.h>
#include <SDL2/SDL_timer.h>

// RENDERER

Renderer::Info::Device::Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory) :
	id(vendev),
	name(std::move(devname)),
	dmem(memory)
{}

Widget* Renderer::finishSelDraw(View*) {
	return nullptr;
}

void Renderer::setMaxPicRes(uint& size) {
	size = std::clamp(size, Settings::minPicRes, maxTextureSize);
	maxPictureSize = size;
}

SDL_Surface* Renderer::makeCompatible(SDL_Surface* img, bool rpic) const {
	if (img = limitSize(img, rpic ? maxPictureSize : maxTextureSize); !img)
		return nullptr;
	umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>::const_iterator sit = preconvertFormats.find(SDL_PixelFormatEnum(img->format->format));
	return sit == preconvertFormats.end() ? img : convertReplace(img, sit->second);
}

SDL_Surface* Renderer::convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format) {
	SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, format, 0);
	SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* Renderer::limitSize(SDL_Surface* img, uint32 limit) {
	if (img && (uint32(img->w) > limit || uint32(img->h) > limit)) {
		float scale = float(limit) / float(img->w > img->h ? img->w : img->h);
		SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, int(float(img->w) * scale), int(float(img->h) * scale), img->format->BitsPerPixel, img->format->format);
		if (dst)
			SDL_BlitScaled(img, nullptr, dst, nullptr);
		SDL_FreeSurface(img);
		img = dst;
	}
	return img;
}

void Renderer::copyPixels(byte_t* dst, const byte_t* src, uint dpitch, uint spitch, uint bwidth, uint height) {
	if (dpitch == spitch)
		memcpy(dst, src, size_t(dpitch) * size_t(height));
	else
		for (uint r = 0; r < height; ++r, dst += dpitch, src += spitch)
			memcpy(dst, src, bwidth);
}

Rectf Renderer::cropTexRect(const Recti& isct, const Recti& rect, uvec2 texRes) {
	vec2 fac = vec2(texRes) / vec2(rect.size());
	return Rectf(vec2(isct.pos() - rect.pos()) * fac, glm::ceil(vec2(isct.size()) * fac));
}

void Renderer::recommendPicRamLimit(uintptr_t& mem) {
	if (!mem)
		mem = uintptr_t(SDL_GetSystemRAM() / 2) * 1024 * 1024;
}

// RENDERER SF

RendererSf::RendererSf(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	Renderer(uint(std::sqrt(INT_MAX / 4))),
	bgColor(colorToBytes(bgcolor))
{
	if (isSingleWindow(windows)) {
		SDL_Surface* srf = SDL_GetWindowSurface(windows.begin()->second);
		if (!srf)
			throw std::runtime_error(SDL_GetError());
		viewRes = ivec2(srf->w, srf->h);
		views.emplace(singleDspId, new View(windows.begin()->second, Recti(ivec2(0), viewRes)));
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			SDL_Surface* srf = SDL_GetWindowSurface(win);
			if (!srf)
				throw std::runtime_error(SDL_GetError());
			Recti wrect(sets->displays.at(id).pos() - origin, ivec2(srf->w, srf->h));
			viewRes = glm::max(viewRes, wrect.end());
			views.emplace(id, new View(win, wrect));
		}
	}
	setVsync(sets->vsync);
	setCompression(sets->compression);
	setMaxPicRes(sets->maxPicRes);
	sets->gpuSelecting = false;
	recommendPicRamLimit(sets->picLim.size);
}

RendererSf::~RendererSf() {
	for (auto [id, view] : views)
		delete view;
}

void RendererSf::setClearColor(const vec4& color) {
	bgColor = colorToBytes(color);
}

void RendererSf::setVsync(bool vsync) {
	drawDelay = 0;
	if (vsync) {
		int maxHz = 0;
		SDL_DisplayMode mode;
		for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
			if (!SDL_GetDesktopDisplayMode(i, &mode) && mode.refresh_rate > maxHz)
				maxHz = mode.refresh_rate;
		if (maxHz)
			drawDelay = 1000 / uint(maxHz);
	}
}

void RendererSf::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_Surface* srf = SDL_GetWindowSurface(views.begin()->second->win);
		if (!srf)
			throw std::runtime_error(SDL_GetError());
		viewRes = ivec2(srf->w, srf->h);
	}
}

void RendererSf::startDraw(View* view) {
	if (curViewSrf = SDL_GetWindowSurface(view->win); !curViewSrf)
		throw std::runtime_error(SDL_GetError());
	curViewPos = view->rect.pos();
	SDL_FillRect(curViewSrf, nullptr, SDL_MapRGBA(curViewSrf->format, bgColor.r, bgColor.g, bgColor.b, bgColor.a));
}

void RendererSf::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	if (Recti isct; SDL_IntersectRect(reinterpret_cast<const SDL_Rect*>(&rect), reinterpret_cast<const SDL_Rect*>(&frame), reinterpret_cast<SDL_Rect*>(&isct))) {
		Recti crop = cropTexRect(isct, rect, tex->getRes());
		isct.pos() -= curViewPos;
		u8vec4 bclr = colorToBytes(color);
		SDL_Surface* img = static_cast<const TextureSf*>(tex)->srf;
		SDL_SetSurfaceColorMod(img, bclr.r, bclr.g, bclr.b);
		SDL_SetSurfaceAlphaMod(img, bclr.a);
		SDL_BlitScaled(img, reinterpret_cast<const SDL_Rect*>(&crop), curViewSrf, reinterpret_cast<SDL_Rect*>(&isct));
	}
}

void RendererSf::finishDraw(View* view) {
	uint32 now = SDL_GetTicks();
	if (uint32 timeout = lastDraw + drawDelay; !SDL_TICKS_PASSED(now, timeout))
		SDL_Delay(timeout - now);
	SDL_UpdateWindowSurface(view->win);
	lastDraw = now;
}

Texture* RendererSf::texFromEmpty() {
	return new TextureSf(uvec2(0), nullptr);
}

Texture* RendererSf::texFromIcon(SDL_Surface* img) {
	if (img = limitSize(img, maxTextureSize); img) {
		SDL_SetSurfaceRLE(img, SDL_TRUE);
		return new TextureSf(uvec2(img->w, img->h), img);
	}
	return nullptr;
}

bool RendererSf::texFromIcon(Texture* tex, SDL_Surface* img) {
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

Texture* RendererSf::texFromRpic(SDL_Surface* img) {
	if (img) {
		SDL_SetSurfaceBlendMode(img, SDL_BLENDMODE_NONE);
		return new TextureSf(uvec2(img->w, img->h), img);
	}
	return nullptr;
}

Texture* RendererSf::texFromText(const PixmapRgba& pm) {
	if (pm.pix)
		if (SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, std::min(pm.res.x, maxTextureSize), std::min(pm.res.y, maxTextureSize), 32, SDL_PIXELFORMAT_RGBA32)) {
			copyPixels(static_cast<byte_t*>(img->pixels), reinterpret_cast<const byte_t*>(pm.pix), img->pitch, pm.res.x * 4, pm.res.x * 4, pm.res.y);
			SDL_SetSurfaceRLE(img, SDL_TRUE);
			return new TextureSf(uvec2(img->w, img->h), img);
		}
	return nullptr;
}

bool RendererSf::texFromText(Texture* tex, const PixmapRgba& pm) {
	if (pm.pix)
		if (SDL_Surface* img = SDL_CreateRGBSurfaceWithFormat(0, std::min(pm.res.x, maxTextureSize), std::min(pm.res.y, maxTextureSize), 32, SDL_PIXELFORMAT_RGBA32)) {
			copyPixels(static_cast<byte_t*>(img->pixels), reinterpret_cast<const byte_t*>(pm.pix), img->pitch, pm.res.x * 4, pm.res.x * 4, pm.res.y);
			SDL_SetSurfaceRLE(img, SDL_TRUE);

			auto stx = static_cast<TextureSf*>(tex);
			SDL_FreeSurface(stx->srf);
			stx->res = uvec2(img->w, img->h);
			stx->srf = img;
			return true;
		}
	return false;
}

void RendererSf::freeTexture(Texture* tex) {
	auto stx = static_cast<TextureSf*>(tex);
	SDL_FreeSurface(stx->srf);
	delete stx;
}

void RendererSf::setCompression(Settings::Compression compression) {
	if (compression == Settings::Compression::b16) {
		preconvertFormats = {
			{ SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_ABGR1555 },
			{ SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_BGRA8888, SDL_PIXELFORMAT_BGRA5551 },
			{ SDL_PIXELFORMAT_RGBA8888, SDL_PIXELFORMAT_RGBA5551 },
			{ SDL_PIXELFORMAT_XBGR8888, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_BGRX8888, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_RGBX8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_BGR24, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_ARGB1555 }
		};
	} else
		preconvertFormats.clear();
}

Renderer::Info RendererSf::getInfo() const {
	return Info{
		.compressions = { Settings::Compression::none, Settings::Compression::b8, Settings::Compression::b16 },
		.texSize = maxTextureSize
	};
}
