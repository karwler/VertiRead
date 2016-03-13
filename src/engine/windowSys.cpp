#include "windowSys.h"
#include "world.h"

WindowSys::WindowSys() :
	window(nullptr),
	renderer(nullptr)
{}

bool WindowSys::SetWindow(VideoSettings settings) {
	// destroy old window if one exists
	DestroyWindow();

	// set settings and set window flags
	sets = settings;
	uint flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (sets.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (sets.maximized) flags |= SDL_WINDOW_MAXIMIZED;
	
	// create window and renderer
	window = SDL_CreateWindow("VertRead", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets.resolution.x, sets.resolution.y, flags);
	if (!window) {
		cerr << "couldn't create window" << endl;
		return false;
	}
	if (!CreateRenderer())
		return false;
	return true;
}

void WindowSys::DestroyWindow() {
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
}

void WindowSys::DrawScene() {
	SDL_SetRenderDrawColor(renderer, sets.colors[EColor::background].x, sets.colors[EColor::background].y, sets.colors[EColor::background].z, sets.colors[EColor::background].a);
	SDL_RenderClear(renderer);
	for (Object* obj : World::scene()->Objects())
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
		DrawScene();
	}
}

void WindowSys::setFullscreen(bool on) {
	// determine what to do
	sets.fullscreen = on;
	if (sets.fullscreen)
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else if (sets.maximized)
		SetWindow(sets);
	else
		SDL_SetWindowFullscreen(window, 0);
}

void WindowSys::setVSync(bool on) {
	sets.vsync = on;
	if (!CreateRenderer())
		World::engine->Close();
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

vec2i WindowSys::GetTextureSize(string tex) {
	vec2i size;
	SDL_Texture* img = IMG_LoadTexture(renderer, tex.c_str());
	if (!img)
		return size;
	SDL_QueryTexture(img, nullptr, nullptr, &size.x, &size.y);
	SDL_DestroyTexture(img);
	return size;
}

SDL_Renderer* WindowSys::Renderer() const {
	return renderer;
}

bool WindowSys::CreateRenderer() {
	if (renderer)
		SDL_DestroyRenderer(renderer);

	// set neeeded flags
	uint flags = SDL_RENDERER_ACCELERATED;
	if (sets.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

	// create renderer based on the currently selected one
	renderer = SDL_CreateRenderer(window, GetRenderDriverIndex(), flags);
	if (!renderer) {
		cerr << "couldn't create renderer" << endl;
		return false;
	}
	return true;
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
	if (dynamic_cast<TextBox*>(obj)) {
		TextBox* object = static_cast<TextBox*>(obj);
		DrawRect(object->getRect(), object->color);
		vector<Text> lines = object->getLines();
		for (Text& line : lines)
			DrawText(line);
	}
	else if (dynamic_cast<ButtonImage*>(obj)) {
		ButtonImage* object = static_cast<ButtonImage*>(obj);
		DrawImage(object->getRect(), object->texname);
	}
	else if (dynamic_cast<ButtonText*>(obj)) {
		ButtonText* object = static_cast<ButtonText*>(obj);
		DrawRect(object->getRect(), object->color);
		DrawText(Text(object->text, vec2i(object->Pos().x+5, object->Pos().y), object->Size().y - object->Size().y/8, object->textColor));
	}
	else if (dynamic_cast<ListBox*>(obj))
		DrawObject(static_cast<ListBox*>(obj));
	else if (dynamic_cast<TileBox*>(obj))
		DrawObject(static_cast<TileBox*>(obj));
	else if (dynamic_cast<ReaderBox*>(obj))
		DrawObject(static_cast<ReaderBox*>(obj));
	else
		DrawRect(obj->getRect(), obj->color);
}

void WindowSys::DrawObject(ListBox* obj) {
	int posY = obj->Pos().y - obj->ListY();
	vector<ListItem*> items = obj->Items();
	for (uint i = 0; i != items.size(); i++) {
		int itemMax = posY + items[i]->height;
		int frameMin = obj->Pos().y;
		int frameMax = obj->Pos().y + obj->Size().y;

		if (itemMax >= frameMin && posY <= frameMax) {
			SDL_Rect crop = { 0, 0, 0, 0 };
			if (posY < frameMin && itemMax > frameMin)
				crop.y = itemMax - obj->Pos().x;
			if (posY < frameMax && itemMax > frameMax)
				crop.h = itemMax - frameMax;

			DrawRect({ obj->Pos().x, posY, obj->Size().x - obj->barW, items[i]->height }, EColor::rectangle);
			DrawText(Text(items[i]->label, vec2i(obj->Pos().x + 5, posY), items[i]->height - items[i]->height / 8), crop);
		}
		posY = itemMax + obj->Spacing();
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(TileBox* obj) {
	vector<TileItem>& tiles = obj->Items();
	for (uint i = 0; i != tiles.size(); i++) {
		int row = i / obj->TilesPerRow();
		int posX = (i - row * obj->TilesPerRow()) * (obj->TileSize().x + obj->Spacing()) + obj->Pos().x;
		int posY = row * (obj->TileSize().y + obj->Spacing()) + obj->Pos().y;

		int itemMax = posY + obj->TileSize().y;
		int frameMax = obj->Pos().y + obj->Size().y;

		if (itemMax < obj->Pos().y || posY > frameMax)
			continue;
		
		int textSize = obj->TileSize().y - obj->TileSize().y / 8;
		SDL_Rect crop = { 0, 0, 0, 0 };
		if (posY < obj->Pos().y && itemMax > obj->Pos().y)
			crop.y = itemMax - obj->Pos().x;
		if (posY < frameMax && itemMax > frameMax)
			crop.h = itemMax - frameMax;

		int textWidth = CalcTextLength(tiles[i].label, textSize);
		if (textWidth > obj->TileSize().x)
			crop.w = textWidth - obj->TileSize().x + 10;

		DrawRect({ posX, posY+crop.y, obj->TileSize().x, obj->TileSize().y-crop.y-crop.h}, EColor::rectangle);
		DrawText(Text(tiles[i].label, vec2i(posX+5, posY), textSize), crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawObject(ReaderBox* obj) {
	const vector<Image>& pics = obj->Pictures();
	for (const Image& it : pics) {
		vec2i pos(obj->Pos().x + obj->Size().x/2 - it.size.x/2 - obj->ListX(), it.pos.y - obj->ListY());
		vec2i itemMax = pos + it.size;
		vec2i frameMax = obj->Pos() + obj->Size();

		if (itemMax.y < obj->pos.y || pos.y > frameMax.y)
			continue;

		DrawImage({pos.x, pos.y, it.size.x, it.size.y}, it.texname);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawRect(SDL_Rect rect, EColor color) {
	SDL_SetRenderDrawColor(renderer, sets.colors[color].x, sets.colors[color].y, sets.colors[color].z, sets.colors[color].a);
	SDL_RenderFillRect(renderer, &rect);
}

void WindowSys::DrawImage(SDL_Rect rect, string texname, SDL_Rect crop) {
	SDL_Texture* tex = nullptr;
	if (needsCrop(crop)) {
		SDL_Surface* surface = IMG_Load(texname.c_str());
		SDL_Surface* sheet = CropSurface(surface, rect, crop);
		tex = SDL_CreateTextureFromSurface(renderer, sheet);

		SDL_FreeSurface(sheet);
		SDL_FreeSurface(surface);
	}
	else
		tex = IMG_LoadTexture(renderer, texname.c_str());

	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
}

void WindowSys::DrawText(const Text& txt, SDL_Rect crop) {
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
		SDL_Surface* sheet = CropSurface(surface, rect, crop);
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
