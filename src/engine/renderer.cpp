#include "renderer.h"

Renderer::View::View(SDL_Window* window, const Recti& area) :
	win(window),
	rect(area)
{}

void Renderer::setCompression(bool) {}

void Renderer::finishRender() {}
