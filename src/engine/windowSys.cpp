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

	uint flags = SDL_WINDOW_RESIZABLE;
	if (sets.fullscreen)     flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else if (sets.maximized) flags |= SDL_WINDOW_MAXIMIZED;
	
	window = SDL_CreateWindow("VertRead", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets.resolution.x, sets.resolution.y, flags);
	if (!window)
		throw Exception("couldn't create window" + string(SDL_GetError()), 3);
	
	CreateRenderer();
}

void WindowSys::CreateRenderer() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}

	renderer = SDL_CreateRenderer(window, GetRenderDriverIndex(), SDL_RENDERER_ACCELERATED);
	if (!renderer)
		throw Exception("couldn't create renderer" + string(SDL_GetError()), 4);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void WindowSys::DestroyWindow() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

void WindowSys::SetIcon(SDL_Surface* icon) {
	SDL_SetWindowIcon(window, icon);
}

void WindowSys::DrawObjects(const vector<Object*>& objects) {
	vec4b bgcolor = sets.colors[EColor::background];
	SDL_SetRenderDrawColor(renderer, bgcolor.x, bgcolor.y, bgcolor.z, bgcolor.a);
	SDL_RenderClear(renderer);

	for (uint i=0; i!=objects.size()-1; i++)
		PassDrawObject(objects[i]);									// draw normal widgets
	if (objects[objects.size()-1])
		DrawObject(static_cast<Popup*>(objects[objects.size()-1]));	// last element is a popup
	SDL_RenderPresent(renderer);
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

SDL_Renderer* WindowSys::Renderer() const {
	return renderer;
}

VideoSettings WindowSys::Settings() const {
	return sets;
}

void WindowSys::Renderer(const string& name) {
	sets.renderer = name;
	CreateRenderer();
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

void WindowSys::Font(const string& font) {
	sets.SetFont(font);
	World::library()->LoadFont(sets.Fontpath());
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
	if (Label* lbl = dynamic_cast<Label*>(obj))
		DrawObject(lbl->getRect(), lbl->color, lbl->getText());
	else if (ButtonText* btxt = dynamic_cast<ButtonText*>(obj))
		DrawObject(btxt->getRect(), btxt->color, btxt->getText());
	else if (ButtonImage* bimg = dynamic_cast<ButtonImage*>(obj))
		DrawImage(bimg->CurTex());
	else if (LineEditor* edt = dynamic_cast<LineEditor*>(obj))
		DrawObject(edt);
	else if (ListBox* box = dynamic_cast<ListBox*>(obj))
		DrawObject(box);
	else if (TileBox* box = dynamic_cast<TileBox*>(obj))
		DrawObject(box);
	else if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
		DrawObject(box);
	else
		DrawRect(obj->getRect(), obj->color);
}

void WindowSys::DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text) {
	DrawRect(bg, bgColor);
	
	int len = text.size().x;
	len = len+5 > bg.w ? len - bg.w +10 : 0;	// use len as crop variable
	DrawText(text, { 0, 0, len, 0 });
}

void WindowSys::DrawObject(LineEditor* obj) {
	SDL_Rect bg = obj->getRect();
	DrawRect(bg, obj->color);

	// get text and calculate left and right crop
	Text text = obj->getText();
	int len = text.size().x;
	int left = (text.pos.x < bg.x) ? bg.x - text.pos.x : 0;
	int right = (len-left > bg.w) ? len - left - bg.w : 0;
	DrawText(text, { left, 0, right, 0 });

	DrawRect(obj->getCaret(), EColor::highlighted);
}

void WindowSys::DrawObject(ListBox* obj) {
	vec2i interval = obj->VisibleItems();
	vector<ListItem*> items = obj->Items();
	for (int i=interval.x; i<=interval.y; i++) {
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, &crop, &color);

		DrawRect(rect, color);
		DrawText(Text(items[i]->label, vec2i(rect.x+5, rect.y-crop.y), obj->ItemH()), crop);

		PassDrawItem(i, obj, rect, crop);
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

		int len = Text(tiles[i]->label, 0, obj->TileSize().y).size().x;
		crop.w = len+5 > rect.w ? crop.w = len - rect.w +10 : 0;									// recalculate right side crop for text
		DrawText(Text(tiles[i]->label, vec2i(rect.x+5, rect.y-crop.y), obj->TileSize().y), crop);	// left side crop can be ignored
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

void WindowSys::DrawObject(Popup* obj) {
	DrawRect(obj->getRect(), EColor::darkened);
	for (Object* it : obj->getObjects())
		PassDrawObject(it);
}

void WindowSys::PassDrawItem(int id, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop) {
	if (Checkbox* obj = dynamic_cast<Checkbox*>(parent->Item(id)))
		DrawItem(obj, parent, rect, crop);
	else if (Switchbox* obj = dynamic_cast<Switchbox*>(parent->Item(id)))
		DrawItem(obj, parent, rect, crop);
	else if (LineEdit* obj = dynamic_cast<LineEdit*>(parent->Item(id)))
		DrawItem(obj, parent, rect, crop);
	else if (KeyGetter* obj = dynamic_cast<KeyGetter*>(parent->Item(id)))
		DrawItem(obj, parent, rect, crop);
}

void WindowSys::DrawItem(Checkbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = Text(item->label, 0, parent->ItemH()).size().x + 20;
	int top = (crop.y < item->spacing) ? item->spacing - crop.y : crop.y;
	int bot = (crop.h > item->spacing) ? crop.h - item->spacing : item->spacing;

	DrawRect({ rect.x+offset+item->spacing, rect.y-crop.y+top, parent->ItemH()-item->spacing*2, rect.h+crop.h-top-bot }, item->On() ? EColor::highlighted : EColor::darkened);
}

void WindowSys::DrawItem(Switchbox* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = Text(item->label, 0, parent->ItemH()).size().x + 20;

	DrawText(Text(item->CurOption(), vec2i(rect.x+offset, rect.y-crop.y), parent->ItemH()), crop);
}

void WindowSys::DrawItem(LineEdit* item, ListBox* parent, const SDL_Rect& rect, SDL_Rect crop) {
	int offset = Text(item->label, 0, parent->ItemH()).size().x + 20;
	Text text(item->Editor()->Text(), vec2i(rect.x+offset-item->TextPos(), rect.y-crop.y), parent->ItemH());

	// set text's left and right crop
	int end = parent->End().x;
	int len = text.size().x;
	crop.x = (text.pos.x < rect.x+offset) ? rect.x + offset - text.pos.x : 0;
	crop.w = (text.pos.x+len > end) ? text.pos.x + len - end : 0;
	DrawText(text, crop);

	// draw caret if selected
	if (World::inputSys()->CapturedObject() == item) {
		offset += Text(item->Editor()->Text().substr(0, item->Editor()->CursorPos()), 0, parent->ItemH()).size().x - item->TextPos();
		DrawRect({ rect.x+offset, rect.y, 5, rect.h }, EColor::highlighted);
	}
}

void WindowSys::DrawItem(KeyGetter* item, ListBox* parent, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = Text(item->label, 0, parent->ItemH()).size().x + 20;
	string text = (World::inputSys()->CapturedObject() == item) ? "Gimme a key..." : item->KeyName();
	EColor color = (World::inputSys()->CapturedObject() == item) ? EColor::highlighted : EColor::text;

	DrawText(Text(text, vec2i(rect.x+offset, rect.y-crop.y), parent->ItemH(), 8, color), crop);
}

void WindowSys::DrawRect(const SDL_Rect& rect, EColor color) {
	vec4b cclr = sets.colors[color];
	SDL_SetRenderDrawColor(renderer, cclr.x, cclr.y, cclr.z, cclr.a);
	SDL_RenderFillRect(renderer, &rect);
}

void WindowSys::DrawImage(const Image& img, const SDL_Rect& crop) {
	SDL_Rect rect = img.getRect();	// the rect the image is gonna be projected on

	SDL_Texture* tex;
	if (needsCrop(crop)) {
		SDL_Rect ori = { rect.x, rect.y, img.texture->surface->w, img.texture->surface->h };	// proportions of the original image
		vec2f fac(float(ori.w) / float(rect.w), float(ori.h) / float(rect.h));					// scaling factor
		rect = cropRect(rect, crop);															// adjust rect to crop

		// crop original image by factor
		SDL_Surface* sheet = cropSurface(img.texture->surface, ori, { int(float(crop.x)*fac.x), int(float(crop.y)*fac.y), int(float(crop.w)*fac.x), int(float(crop.h)*fac.y) });
		tex = SDL_CreateTextureFromSurface(renderer, sheet);
		SDL_FreeSurface(sheet);
	}
	else
		tex = SDL_CreateTextureFromSurface(renderer, img.texture->surface);

	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
}

void WindowSys::DrawText(const Text& txt, const SDL_Rect& crop) {
	vec2i siz = txt.size();
	SDL_Rect rect = {txt.pos.x, txt.pos.y, siz.x, siz.y};

	SDL_Surface* surface = TTF_RenderUTF8_Blended(World::library()->Fonts()->Get(txt.height), txt.text.c_str(), { sets.colors[txt.color].x, sets.colors[txt.color].y, sets.colors[txt.color].z, sets.colors[txt.color].a });
	SDL_Texture* tex;
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
