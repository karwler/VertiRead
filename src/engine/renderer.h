#pragma once

#include "utils/utils.h"

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

public:
	virtual ~Renderer() = default;

	virtual void setClearColor(const vec4& color) = 0;
	virtual void setVsync(bool vsync) = 0;
	virtual void updateView(ivec2& viewRes) = 0;
	virtual void setCompression(bool);
	virtual void getAdditionalSettings(bool& compression, vector<pair<u32vec2, string>>& devices) = 0;
	virtual void startDraw(View* view) = 0;
	virtual void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) = 0;
	virtual void finishDraw(View* view) = 0;
	virtual void finishRender();
	virtual void startSelDraw(View* view, ivec2 pos) = 0;
	virtual void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) = 0;
	virtual Widget* finishSelDraw(View* view) = 0;
	virtual Texture* texFromImg(SDL_Surface* img) = 0;
	virtual Texture* texFromText(SDL_Surface* img) = 0;
	virtual void freeTexture(Texture* tex) = 0;

	const umap<int, View*>& getViews() const;
protected:
	static SDL_Surface* limitSize(SDL_Surface* img, uint32 limit);
};

inline const umap<int, Renderer::View*>& Renderer::getViews() const {
	return views;
}
