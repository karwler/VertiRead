#include "renderer.h"

Renderer::View::View(SDL_Window* window, const Recti& area) :
	win(window),
	rect(area)
{}

void Renderer::setCompression(bool) {}

void Renderer::finishRender() {}

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
