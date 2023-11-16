#include "renderer.h"

void Renderer::finishRender() {}

void Renderer::synchTransfer() {}

void Renderer::setMaxPicRes(uint& size) {
	size = std::clamp(size, Settings::minPicRes, maxTexSize());
	maxPicRes = size;
}

SDL_Surface* Renderer::makeCompatible(SDL_Surface* img, bool rpic) const {
	if (img = limitSize(img, rpic ? maxPicRes : maxTexSize()); !img)
		return nullptr;
	if (const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* sconv = getSquashableFormats())
		if (umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>::const_iterator sit = sconv->find(SDL_PixelFormatEnum(img->format->format)); sit != sconv->end())
			return convertReplace(img, sit->second);
	return supportedFormats.contains(SDL_PixelFormatEnum(img->format->format)) ? img : convertReplace(img);
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
