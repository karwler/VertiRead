#include "world.h"

WindowSys::WindowSys(const VideoSettings& SETS) :
	window(nullptr),
	renderer(nullptr),
	sets(SETS)
{}

WindowSys::~WindowSys() {
	DestroyWindow();
}

void WindowSys::SetWindow() {
	DestroyWindow();

	uint flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (sets.fullscreen)     flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (sets.maximized) flags |= SDL_WINDOW_MAXIMIZED;

	window = SDL_CreateWindow("VertRead", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets.resolution.x, sets.resolution.y, flags);
	if (!window)
		throw Exception("couldn't create window" + string(SDL_GetError()), 3);

	SDL_Surface* icon = IMG_Load(string(Filer::dirTexs()+"icon.ico").c_str());
	if (icon) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}
	CreateRenderer();
}

void WindowSys::CreateRenderer() {
	if (renderer)
		SDL_DestroyRenderer(renderer);

	uint flags = SDL_RENDERER_ACCELERATED;
	if (sets.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

	renderer = SDL_CreateRenderer(window, GetRenderDriverIndex(), flags);
	if (!renderer)
		throw Exception("couldn't create renderer" + string(SDL_GetError()), 4);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void WindowSys::DestroyWindow() {
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
}

void WindowSys::DrawObjects(vector<Object*> objects) {
	SDL_SetRenderDrawColor(renderer, sets.colors[EColor::background].x, sets.colors[EColor::background].y, sets.colors[EColor::background].z, sets.colors[EColor::background].a);
	SDL_RenderClear(renderer);
	for (uint i=0; i!=objects.size()-1; i++)
		PassDrawObject(objects[i]);									// draw normal widgets
	if (objects[objects.size()-1])
		DrawObject(static_cast<Popup*>(objects[objects.size()-1]));	// last element is a popup
	SDL_RenderPresent(renderer);
}

VideoSettings WindowSys::Settings() const {
	return sets;
}

vec2i WindowSys::Resolution() const {
	vec2i res;
	SDL_GetWindowSize(window, &res.x, &res.y);
	return res;
}

vec2i WindowSys::DesktopResolution() {
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);
	return vec2i(mode.w, mode.h);
}

void WindowSys::WindowEvent(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_RESIZED: {
		// update settings if needed
		uint flags = SDL_GetWindowFlags(window);
		if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			sets.maximized = (flags & SDL_WINDOW_MAXIMIZED) ? true : false;
			if (!sets.maximized)
				SDL_GetWindowSize(window, &sets.resolution.x, &sets.resolution.y);
		}
		break; }
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		World::scene()->ResizeMenu();
	}
}

void WindowSys::Fullscreen(bool on) {
	// determine what to do
	sets.fullscreen = on;
	if (sets.fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else if (sets.maximized)
		SetWindow();
	else
		SDL_SetWindowFullscreen(window, 0);
}

void WindowSys::VSync(bool on) {
	sets.vsync = on;
	CreateRenderer();
}

SDL_Renderer* WindowSys::Renderer() const {
	return renderer;
}

int WindowSys::GetRenderDriverIndex() {
	// get index of currently selected renderer (if name doesn't match, choose the first renderer)
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == sets.renderer)
			return i;
	sets.renderer = getRendererName(0);
	return 0;
}

void WindowSys::PassDrawObject(Object* obj) {
	// specific drawing for each object
	if (dynamic_cast<ButtonImage*>(obj))
		DrawImage(dynamic_cast<ButtonImage*>(obj)->CurTex());
	else if (dynamic_cast<Label*>(obj))
		DrawObject(dynamic_cast<Label*>(obj));
	else if (dynamic_cast<ListBox*>(obj))
		DrawObject(static_cast<ListBox*>(obj));
	else if (dynamic_cast<TileBox*>(obj))
		DrawObject(static_cast<TileBox*>(obj));
	else if (dynamic_cast<ReaderBox*>(obj))
		DrawObject(static_cast<ReaderBox*>(obj));
	else if (dynamic_cast<LineEdit*>(obj))
		DrawObject(static_cast<LineEdit*>(obj));
	else if (dynamic_cast<KeyGetter*>(obj))
		DrawObject(static_cast<KeyGetter*>(obj));
	else
		DrawRect(obj->getRect(), obj->color);
}

void WindowSys::DrawObject(Label* obj) {
	DrawRect(obj->getRect(), obj->color);

	Text txt = obj->getText();
	int len = txt.size().x;
	len = len+5 > obj->Size().x ? len - obj->Size().x +10 : 0;		// use len as crop variable
	DrawText(txt, {0, 0, len, 0});
}

void WindowSys::DrawObject(ListBox* obj) {
	vec2i interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop, &color);

		DrawRect(rect, color);
		DrawText(Text(items[i]->label, vec2i(rect.x+5, rect.y-crop.y), obj->ItemH(), 8), crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(TileBox* obj) {
	vec2i interval = obj->VisibleItems();
	const vector<ListItem*>& tiles = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop, &color);
		DrawRect(rect, color);

		int len = Text(tiles[i]->label, 0, obj->TileSize().y, 8).size().x;
		crop.w = len+5 > rect.w ? crop.w = len - rect.w +10 : 0;										// recalculate right side crop for text
		DrawText(Text(tiles[i]->label, vec2i(rect.x+5, rect.y-crop.y), obj->TileSize().y, 8), crop);	// left side crop can be ignored
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(ReaderBox* obj) {
	vec2i interval = obj->VisiblePictures();
	for (int i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		Image img = obj->getImage(i, &crop);
		DrawImage(img, crop);
	}
	if (obj->showSlider()) {
		DrawRect(obj->Bar(), EColor::darkened);
		DrawRect(obj->Slider(), EColor::highlighted);
	}
	if (obj->showList()) {
		DrawRect(obj->List(), EColor::darkened);
		for (ButtonImage& but : obj->ListButtons())
			DrawImage(but.CurTex());
	}
	if (obj->showPlayer()) {
		DrawRect(obj->Player(), EColor::darkened);
		for (ButtonImage& but : obj->PlayerButtons())
			DrawImage(but.CurTex());
	}
}

void WindowSys::DrawObject(LineEdit* obj) {
	DrawRect(obj->getRect(), obj->color);

	vec2i crop;
	Text txt = obj->getText(&crop);
	DrawText(txt, {crop.x, 0, crop.y, 0});

	SDL_Rect rect = obj->getRect();
	txt.text.resize(obj->Editor()->CursorPos());
	DrawRect({rect.x+txt.size().x, rect.y, 3, rect.h}, EColor::highlighted);	// draw caret
}

void WindowSys::DrawObject(KeyGetter* obj) {
	DrawRect(obj->getRect(), obj->color);
	DrawText(obj->getText());
}

void WindowSys::DrawObject(Popup* obj) {
	DrawRect(obj->getRect(), EColor::darkened);
	for (Object* it : obj->getObjects())
		PassDrawObject(it);
}

void WindowSys::DrawRect(const SDL_Rect& rect, EColor color) {
	SDL_SetRenderDrawColor(renderer, sets.colors[color].x, sets.colors[color].y, sets.colors[color].z, sets.colors[color].a);
	SDL_RenderFillRect(renderer, &rect);
}

void WindowSys::DrawImage(const Image& img, SDL_Rect crop) {
	if (!img.texture)	// no need to draw a nonexistent texture
		return;
	SDL_Rect rect = img.getRect();	// the rect the image is gonna be projected on
	if (needsCrop(crop)) {
		SDL_Surface* surf = IMG_Load(img.texture->File().c_str());
		SDL_Rect ori = {rect.x, rect.y, surf->w, surf->h};									// proportions of the original image
		vec2f fac(float(ori.w) / float(rect.w), float(ori.h) / float(rect.h));				// scaling factor
		rect = {rect.x+crop.x, rect.y+crop.y, rect.w-crop.x-crop.w, rect.h-crop.y-crop.h};	// adjust rect to crop

		// crop original image by factor
		SDL_Surface* sheet = cropSurface(surf, ori, {int(float(crop.x)*fac.x), int(float(crop.y)*fac.y), int(float(crop.w)*fac.x), int(float(crop.h)*fac.y)});
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, sheet);

		SDL_RenderCopy(renderer, tex, nullptr, &rect);
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(sheet);
		SDL_FreeSurface(surf);
	}
	else
		SDL_RenderCopy(renderer, img.texture->tex, nullptr, &rect);
}

void WindowSys::DrawText(const Text& txt, const SDL_Rect& crop) {
	vec2i siz = txt.size();
	SDL_Rect rect = {txt.pos.x, txt.pos.y, siz.x, siz.y};

	SDL_Surface* surface = TTF_RenderText_Blended(World::library()->Fonts()->Get(txt.height), txt.text.c_str(), { sets.colors[txt.color].x, sets.colors[txt.color].y, sets.colors[txt.color].z, sets.colors[txt.color].a });
	SDL_Texture* tex = nullptr;
	if (needsCrop(crop)) {
		SDL_Surface* sheet = cropSurface(surface, rect, crop);
		tex = SDL_CreateTextureFromSurface(renderer, sheet);
		SDL_FreeSurface(sheet);
	}
	else
		tex = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
	SDL_FreeSurface(surface);
}
