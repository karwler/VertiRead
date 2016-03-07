#include "windowSys.h"
#include "world.h"

WindowSys::WindowSys() :
	window(nullptr),
	renderer(nullptr)
{}

int WindowSys::SetWindow(VideoSettings settings) {
	DestroyWindow();
	sets = settings;
	
	uint flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if (sets.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (sets.maximized) flags |= SDL_WINDOW_MAXIMIZED;
	
	window = SDL_CreateWindow("VertRead", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets.resolution.x, sets.resolution.y, flags);
	CreateRenderer();
	return 0;
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
		uint flags = SDL_GetWindowFlags(window);
		if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			sets.maximized = (flags & SDL_WINDOW_MAXIMIZED) ? true : false;
			if (!sets.maximized)
				SDL_GetWindowSize(window, &sets.resolution.x, &sets.resolution.y);
		}
		break; }
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		World::scene()->SwitchMenu(World::program()->CurrentMenu());
	}
}

void WindowSys::setFullscreen(bool on) {
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
	CreateRenderer();
}

bool WindowSys::SetFont(string name) {
	TTF_Font* tmp = TTF_OpenFont(Filer::getFontPath(name).string().c_str(), 12);
	if (!tmp) {
		if (name[0] >= 'A' && name[0] <= 'Z')
			name[0] = tolower(name[0]);
		else if (name[0] >= 'a' && name[0] <= 'z')
			name[0] = toupper(name[0]);
		else
			return false;
		tmp = TTF_OpenFont(Filer::getFontPath(name).string().c_str(), 12);
		if (!tmp)
			return false;
	}
	TTF_CloseFont(tmp);
	sets.font = name;
	return true;
}

SDL_Renderer* WindowSys::Renderer() const {
	return renderer;
}

bool WindowSys::CreateRenderer() {
	if (renderer)
		SDL_DestroyRenderer(renderer);

	uint flags = SDL_RENDERER_ACCELERATED;
	if (sets.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;

	renderer = SDL_CreateRenderer(window, GetRenderDriverIndex(), flags);
	return 0;
}

int WindowSys::GetRenderDriverIndex() {
	for (int i = 0; i != SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == sets.renderer)
			return i;
	sets.renderer = getRendererName(0);
	return 0;
}

void WindowSys::PassDrawObject(Object* obj) {
	if (dynamic_cast<Image*>(obj)) {
		Image* object = static_cast<Image*>(obj);
		DrawImage(object->getRect(), object->texname);
	}
	else if (dynamic_cast<TextBox*>(obj)) {
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
		DrawText(Text(object->text, vec2i(object->pos.x+5, object->pos.y), object->Size().y - object->Size().y/8, object->textColor));
	}
	else if (dynamic_cast<ListBox*>(obj))
		DrawObject(static_cast<ListBox*>(obj));
	else if (dynamic_cast<TileBox*>(obj))
		DrawObject(static_cast<TileBox*>(obj));
	else
		DrawRect(obj->getRect(), obj->color);
}

void WindowSys::DrawObject(ListBox* obj) {
	int posY = obj->listY() + obj->pos.y;
	vector<ListItem*> items = obj->Items();
	for (uint i = 0; i != items.size(); i++) {
		int itemMax = posY + items[i]->height;
		int frameMin = obj->pos.y;
		int frameMax = obj->pos.y + obj->Size().y;

		if (itemMax >= frameMin && posY <= frameMax) {
			SDL_Rect crop = { 0, 0, 0, 0 };
			if (posY < frameMin && itemMax > frameMin)
				crop.y = itemMax - obj->pos.x;
			if (posY < frameMax && itemMax > frameMax)
				crop.h = itemMax - frameMax;

			DrawRect({ obj->pos.x, posY, obj->Size().x - obj->barW, items[i]->height }, EColor::rectangle);
			DrawText(Text(items[i]->label, vec2i(obj->pos.x + 5, posY), items[i]->height - items[i]->height / 8), crop);
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
		int posX = (i - row * obj->TilesPerRow()) * (obj->TileSize().x + obj->Spacing()) + obj->pos.x;
		int posY = row * (obj->TileSize().y + obj->Spacing()) + obj->pos.y;

		int itemMax = posY + obj->TileSize().y;
		int frameMin = obj->pos.y;
		int frameMax = obj->pos.y + obj->Size().y;

		if (itemMax < frameMin || posY > frameMax)
			continue;
		
		SDL_Rect crop = { 0, 0, 0, 0 };
		if (posY < frameMin && itemMax > frameMin)
			crop.y = itemMax - obj->pos.x;
		if (posY < frameMax && itemMax > frameMax)
			crop.h = itemMax - frameMax;

		DrawRect({ posX, posY, obj->TileSize().x, obj->TileSize().y}, EColor::rectangle);
		DrawText(Text(tiles[i].label, vec2i(posX+5, posY), obj->TileSize().y - obj->TileSize().y/8), crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void WindowSys::DrawRect(SDL_Rect rect, EColor color) {
	SDL_SetRenderDrawColor(renderer, sets.colors[color].x, sets.colors[color].y, sets.colors[color].z, sets.colors[color].a);
	SDL_RenderFillRect(renderer, &rect);
}

void WindowSys::DrawImage(SDL_Rect rect, string texname, bool keepSize, SDL_Rect crop) {
	if (!keepSize) {
		SDL_Texture* tex = IMG_LoadTexture(renderer, texname.c_str());
		SDL_QueryTexture(tex, nullptr, nullptr, &rect.w, &rect.h);
		SDL_DestroyTexture(tex);
	}
	rect = { rect.x + crop.x, rect.y + crop.y, rect.w - crop.x - crop.w, rect.h - crop.y - crop.h };
	crop = { crop.x, crop.y, rect.w - crop.x - crop.w, rect.h - crop.y - crop.h };

	SDL_Surface* surface = IMG_Load(texname.c_str());
	SDL_Surface* sheet = SDL_CreateRGBSurface(surface->flags, crop.w, crop.h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_BlitSurface(surface, &crop, sheet, 0);

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, sheet);
	SDL_RenderCopy(renderer, tex, nullptr, &rect);

	SDL_DestroyTexture(tex);
	SDL_FreeSurface(sheet);
	SDL_FreeSurface(surface);
}

void WindowSys::DrawText(const Text& txt, SDL_Rect crop) {
	TTF_Font* font = TTF_OpenFont(Filer::getFontPath(sets.font).string().c_str(), txt.size);

	SDL_Rect rect = { txt.pos.x, txt.pos.y };
	TTF_SizeText(font, txt.text.c_str(), &rect.w, &rect.h);
	rect = { rect.x + crop.x, rect.y + crop.y, rect.w - crop.x - crop.w, rect.h - crop.y - crop.h };
	crop = { crop.x, crop.y, rect.w - crop.x - crop.w, rect.h - crop.y - crop.h };

	SDL_Surface* surface = TTF_RenderText_Blended(font, txt.text.c_str(), { sets.colors[txt.color].x, sets.colors[txt.color].y, sets.colors[txt.color].z, sets.colors[txt.color].a });
	SDL_Surface* sheet = SDL_CreateRGBSurface(surface->flags, crop.w, crop.h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_BlitSurface(surface, &crop, sheet, 0);

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, sheet);
	SDL_RenderCopy(renderer, tex, nullptr, &rect);

	SDL_DestroyTexture(tex);
	SDL_FreeSurface(sheet);
	SDL_FreeSurface(surface);
	TTF_CloseFont(font);
}
