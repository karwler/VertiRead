#pragma once

#include "utils/settings.h"
#include <set>

class Texture {
private:
	ivec2 res;

protected:
	Texture(ivec2 size);

public:
	ivec2 getRes() const;
};

inline Texture::Texture(ivec2 size) :
	res(size)
{}

inline ivec2 Texture::getRes() const {
	return res;
}

class Renderer {
public:
	static constexpr int singleDspId = -1;

	struct View {
		SDL_Window* win;
		Recti rect;

		View(SDL_Window* window, const Recti& area = Recti());
	};

	struct ErrorSkip {};

protected:
	umap<int, View*> views;
	std::set<SDL_PixelFormatEnum> supportedFormats;
	uint maxPicRes;	// should only get accessed from one thread at a time

	Renderer(std::set<SDL_PixelFormatEnum>&& formats);
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
	virtual Texture* texFromIcon(SDL_Surface* img) = 0;	// scales down image to largest possible size
	virtual Texture* texFromRpic(SDL_Surface* img) = 0;	// image must have been scaled down in advance
	virtual Texture* texFromText(const Pixmap& pm) = 0;	// cuts off image if it's too large and uses nearest filter if possible
	virtual void freeTexture(Texture* tex) = 0;
	virtual void synchTransfer();

	const umap<int, View*>& getViews() const;
	void setMaxPicRes(uint& size);
	SDL_Surface* makeCompatible(SDL_Surface* img, bool rpic) const;	// must be thread safe
protected:
	virtual uint maxTexSize() const = 0;
	virtual const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* getSquashableFormats() const = 0;
	static SDL_Surface* convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format = SDL_PIXELFORMAT_RGBA32);
	static SDL_Surface* limitSize(SDL_Surface* img, uint32 limit);
};

inline Renderer::Renderer(std::set<SDL_PixelFormatEnum>&& formats) :
	supportedFormats(std::move(formats))
{}

inline const umap<int, Renderer::View*>& Renderer::getViews() const {
	return views;
}
