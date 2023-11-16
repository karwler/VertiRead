#pragma once

#include "utils/settings.h"
#include <set>

struct PixmapRgba {
	const uint32* pix = nullptr;
	uvec2 res;

	PixmapRgba() = default;
	PixmapRgba(const PixmapRgba&) = default;
	PixmapRgba(PixmapRgba&&) = default;
	PixmapRgba(const uint32* data, uvec2 size) : pix(data), res(size) {}
};

class Texture {
protected:
	uvec2 res;

	Texture(uvec2 size) : res(size) {}

public:
	uvec2 getRes() const { return res; }
};

class Renderer {
public:
	static constexpr int singleDspId = -1;

	struct View {
		SDL_Window* win;
		Recti rect;

		View(SDL_Window* window, const Recti& area = Recti()) : win(window), rect(area) {}
	};

	struct ErrorSkip {};

protected:
	static inline const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum> squashableFormats = {
		{ SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_RGBA5551 },
		{ SDL_PIXELFORMAT_BGRA32, SDL_PIXELFORMAT_BGRA5551 },
		{ SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB565 },
		{ SDL_PIXELFORMAT_BGR24, SDL_PIXELFORMAT_BGR565 }
	};

	umap<int, View*> views;
	std::set<SDL_PixelFormatEnum> supportedFormats;
	uint maxPicRes;	// should only get accessed from one thread at a time

	Renderer(std::set<SDL_PixelFormatEnum>&& formats) : supportedFormats(std::move(formats)) {}
public:
	virtual ~Renderer() = default;

	virtual void setClearColor(const vec4& color) = 0;
	virtual void setVsync(bool vsync) = 0;
	virtual void updateView(ivec2& viewRes) = 0;
	virtual void setCompression(Settings::Compression compression) = 0;
	virtual pair<uint, Settings::Compression> getSettings(vector<pair<u32vec2, string>>& devices) const = 0;
	virtual void startDraw(View* view) = 0;
	virtual void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) = 0;
	virtual void finishDraw(View* view) = 0;
	virtual void finishRender();
	virtual void startSelDraw(View* view, ivec2 pos) = 0;
	virtual void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) = 0;
	virtual Widget* finishSelDraw(View* view) = 0;
	virtual Texture* texFromEmpty() = 0;					// creates empty texture handle (currently only used for text)
	virtual Texture* texFromIcon(SDL_Surface* img) = 0;		// scales down image to largest possible size
	virtual bool texFromIcon(Texture* tex, SDL_Surface* img) = 0;	// ^ but refills tex and returns true if successful
	virtual Texture* texFromRpic(SDL_Surface* img) = 0;		// image must have been scaled down in advance
	virtual Texture* texFromText(const PixmapRgba& pm) = 0;	// cuts off image if it's too large and uses nearest filter if possible
	virtual bool texFromText(Texture* tex, const PixmapRgba& pm) = 0;	// ^ but refills tex and returns true if successful
	virtual void freeTexture(Texture* tex) = 0;
	virtual void synchTransfer();

	const umap<int, View*>& getViews() const { return views; }
	void setMaxPicRes(uint& size);
	SDL_Surface* makeCompatible(SDL_Surface* img, bool rpic) const;	// must be thread safe
	static bool isSingleWindow(const umap<int, SDL_Window*>& windows);
protected:
	virtual uint maxTexSize() const = 0;
	virtual const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* getSquashableFormats() const = 0;
	static SDL_Surface* convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format = SDL_PIXELFORMAT_RGBA32);
	static SDL_Surface* limitSize(SDL_Surface* img, uint32 limit);
};

inline bool Renderer::isSingleWindow(const umap<int, SDL_Window*>& windows) {
	return windows.size() == 1 && windows.begin()->first == singleDspId;
}
