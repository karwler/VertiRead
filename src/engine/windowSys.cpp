#include "windowSys.h"
#include "scene.h"

WindowSys::WindowSys(Scene* SCNE, const VideoSettings& SETS) :
	window(nullptr),
	renderer(nullptr),
	sets(SETS),
	scene(SCNE)
{
	SetWindow();
}

WindowSys::~WindowSys() {
	DestroyWindow();
}

void WindowSys::SetWindow() {
	// destroy old window if one exists
	DestroyWindow();

	// set settings and set window flags
	uint flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (sets.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (sets.maximized) flags |= SDL_WINDOW_MAXIMIZED;

	// create window and renderer
	window = SDL_CreateWindow("VertRead", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets.resolution.x, sets.resolution.y, flags);
	if (!window)
		throw Exception("couldn't create window" + string(SDL_GetError()), 3);
	CreateRenderer();
}

void WindowSys::CreateRenderer() {
	if (renderer)
		SDL_DestroyRenderer(renderer);

	// set neeeded flags
	uint flags = SDL_RENDERER_ACCELERATED;
	if (sets.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

	// create renderer based on the currently selected one
	renderer = SDL_CreateRenderer(window, GetRenderDriverIndex(), flags);
	if (!renderer)
		throw Exception("couldn't create renderer" + string(SDL_GetError()), 4);
}

void WindowSys::DestroyWindow() {
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
}

void WindowSys::DrawObjects(const vector<Object*>& objects) {
	SDL_SetRenderDrawColor(renderer, sets.colors[EColor::background].x, sets.colors[EColor::background].y, sets.colors[EColor::background].z, sets.colors[EColor::background].a);
	SDL_RenderClear(renderer);
	for (Object* obj : objects)
		PassDrawObject(obj);
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
		scene->ResizeMenu();
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

int WindowSys::CalcTextLength(string text, int size) {
	int width = 0;
	TTF_Font* font = TTF_OpenFont(sets.font.c_str(), size);
	if (font) {
		TTF_SizeText(font, text.c_str(), &width, nullptr);
		TTF_CloseFont(font);
	}
	return width;
}

SDL_Renderer* WindowSys::Renderer() const {
	return renderer;
}

int WindowSys::GetRenderDriverIndex() {
	// get index of currently selected renderer (if name doesn't match, choose the first renderer)
	for (int i = 0; i != SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == sets.renderer)
			return i;
	sets.renderer = getRendererName(0);
	return 0;
}

void WindowSys::PassDrawObject(Object* obj) {
	// specific drawing for each object
	if (obj->isA<TextBox>()) {
		TextBox* object = static_cast<TextBox*>(obj);
		DrawRect(object->getRect(), object->color);
		vector<Text> lines = object->getLines();
		for (Text& line : lines)
			DrawText(line);
	}
	else if (obj->isA<ButtonImage>()) {
		ButtonImage* object = static_cast<ButtonImage*>(obj);
		DrawImage(Image(object->Pos(), object->Size(), object->CurTex()));
	}
	else if (obj->isA<ButtonText>()) {
		ButtonText* object = static_cast<ButtonText*>(obj);
		DrawRect(object->getRect(), object->color);

		vec2i textSize(0, object->Size().y - object->Size().y/8);
		textSize.x = CalcTextLength(object->text, textSize.y);
		textSize.x = textSize.x+5 > object->Size().x ? textSize.x - object->Size().x +10 : 0;		// use textSize.x as crop variable
		DrawText(Text(object->text, vec2i(object->Pos().x+5, object->Pos().y), textSize.y, object->textColor), {0, 0, textSize.x, 0});
	}
	else if (obj->isA<ListBox>())
		DrawObject(static_cast<ListBox*>(obj));
	else if (obj->isA<TileBox>())
		DrawObject(static_cast<TileBox*>(obj));
	else if (obj->isA<ReaderBox>())
		DrawObject(static_cast<ReaderBox*>(obj));
	else
		DrawRect(obj->getRect(), obj->color);
}

void WindowSys::DrawObject(ListBox* obj) {
	vec2i interval(obj->FirstVisibleItem(), obj->LastVisibleItem());
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop);

		DrawRect(rect, EColor::rectangle);
		DrawText(Text(items[i]->label, vec2i(rect.x +5, rect.y-crop.y), obj->ItemH() - obj->ItemH() /8), crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(TileBox* obj) {
	vec2i interval(obj->FirstVisibleItem(), obj->LastVisibleItem());
	const vector<TileItem>& tiles = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop);
		DrawRect(rect, EColor::rectangle);

		vec2i textSize(0, obj->TileSize().y - obj->TileSize().y /8);
		textSize.x = CalcTextLength(tiles[i].label, textSize.y);
		crop.w = textSize.x+5 > rect.w ? crop.w = textSize.x - rect.w +10 : 0;				// recalculate right side crop for text
		DrawText(Text(tiles[i].label, vec2i(rect.x+5, rect.y-crop.y), textSize.y), crop);	// left side crop can be ignored
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(ReaderBox* obj) {
	vec2i interval = obj->VisiblePicsInterval();
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
			DrawImage(Image(but.Pos(), but.Size(), but.CurTex()));
	}
	if (obj->showPlayer()) {
		DrawRect(obj->Player(), EColor::darkened);
		for (ButtonImage& but : obj->PlayerButtons())
			DrawImage(Image(but.Pos(), but.Size(), but.CurTex()));
	}
}

void WindowSys::DrawRect(const SDL_Rect& rect, EColor color) {
	SDL_SetRenderDrawColor(renderer, sets.colors[color].x, sets.colors[color].y, sets.colors[color].z, sets.colors[color].a);
	SDL_RenderFillRect(renderer, &rect);
}

void WindowSys::DrawImage(const Image& img, SDL_Rect crop) {
	SDL_Texture* tex = nullptr;
	SDL_Rect rect  = {img.pos.x, img.pos.y, img.size.x, img.size.y};	// the rect the image is gonna be projected on

	if (needsCrop(crop)) {
		SDL_Surface* surf = IMG_Load(img.texname.c_str());
		if (!surf) {
			cerr << "couldn't load texture " << img.texname << endl;
			return;
		}
		SDL_Rect ori = {rect.x, rect.y, surf->w, surf->h};									// proportions of the original image
		vec2f fac(float(ori.w) / float(rect.w), float(ori.h) / float(rect.h));				// scaling factor
		rect = {rect.x+crop.x, rect.y+crop.y, rect.w-crop.x-crop.w, rect.h-crop.y-crop.h};	// adjust rect to crop

		// crop original image by factor
		SDL_Surface* sheet = cropSurface(surf, ori, {int(float(crop.x)*fac.x), int(float(crop.y)*fac.y), int(float(crop.w)*fac.x), int(float(crop.h)*fac.y)});
		tex = SDL_CreateTextureFromSurface(renderer, sheet);
		SDL_FreeSurface(sheet);
		SDL_FreeSurface(surf);
	}
	else {
		tex = IMG_LoadTexture(renderer, img.texname.c_str());
		if (!tex) {
			cerr << "couldn't load texture " << img.texname << endl;
			return;
		}
	}
	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
}

void WindowSys::DrawText(const Text& txt, const SDL_Rect& crop) {
	TTF_Font* font = TTF_OpenFont(sets.font.c_str(), txt.size);
	if (!font) {
		cerr << "couldn't load font " << sets.font << endl;
		return;
	}
	SDL_Rect rect = { txt.pos.x, txt.pos.y };
	TTF_SizeText(font, txt.text.c_str(), &rect.w, &rect.h);

	SDL_Surface* surface = TTF_RenderText_Blended(font, txt.text.c_str(), { sets.colors[txt.color].x, sets.colors[txt.color].y, sets.colors[txt.color].z, sets.colors[txt.color].a });
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
	TTF_CloseFont(font);
}
