#include "world.h"

DrawSys::DrawSys() :
	renderer(nullptr)
{}

DrawSys::~DrawSys() {
	destroyRenderer();
}

void DrawSys::createRenderer(SDL_Window* window, int driverIndex) {
	renderer = SDL_CreateRenderer(window, driverIndex, Default::rendererFlags);
	if (!renderer)
		throw Exception("couldn't create renderer\n" + string(SDL_GetError()), 4);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void DrawSys::destroyRenderer() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
}

void DrawSys::drawObjects(const vector<Object*>& objects, const Popup* popup) {
	// clear screen
	colorDim = popup ? Default::colorPopupDim : 1;
	vec4c bgcolor = World::winSys()->getSettings().colors.at(EColor::background);
	SDL_SetRenderDrawColor(renderer, bgcolor.r/colorDim.r, bgcolor.g/colorDim.g, bgcolor.b/colorDim.b, bgcolor.a/colorDim.a);
	SDL_RenderClear(renderer);

	// draw main objects
	for (Object* it : objects)
		passDrawObject(it);

	// draw popup if exists
	colorDim = 1;
	if (popup) {
		drawRect(popup->rect(), popup->color);
		for (Object* it : popup->objects)
			passDrawObject(it);
	}

	SDL_RenderPresent(renderer);
}

void DrawSys::passDrawObject(Object* obj) {
	// specific drawing for each object
	if (Label* lbl = dynamic_cast<Label*>(obj))
		drawObject(lbl->rect(), lbl->color, lbl->getText());
	else if (ButtonText* btxt = dynamic_cast<ButtonText*>(obj))
		drawObject(btxt->rect(), btxt->color, btxt->text());
	else if (ButtonImage* bimg = dynamic_cast<ButtonImage*>(obj))
		drawImage(bimg->getCurTex());
	else if (LineEditor* edt = dynamic_cast<LineEditor*>(obj))
		drawObject(edt);
	else if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(obj))
		drawObject(box);
	else if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
		drawObject(box);
	else
		drawRect(obj->rect(), obj->color);
}

void DrawSys::drawObject(const SDL_Rect& bg, EColor bgColor, const Text& text) {
	drawRect(bg, bgColor);

	int len = text.size().x;
	int left = (text.pos.x < bg.x) ? bg.x-text.pos.x : 0;
	int right = (text.pos.x+len > bg.x+bg.w) ? text.pos.x+len-bg.x-bg.w : 0;
	drawText(text, {left, 0, right, 0});
}

void DrawSys::drawObject(LineEditor* obj) {
	SDL_Rect bg = obj->rect();
	drawRect(bg, obj->color);

	// get text and calculate left and right crop
	Text text = obj->text();
	int len = text.size().x;
	int left = (text.pos.x < bg.x) ? bg.x - text.pos.x : 0;
	int right = (len-left > bg.w) ? len - left - bg.w : 0;
	drawText(text, {left, 0, right, 0});

	if (World::inputSys()->getCaptured() == obj)
		drawRect(obj->caretRect(), EColor::highlighted);
}

void DrawSys::drawObject(ScrollAreaItems* obj) {
	// get index interval of items on screen
	vec2t interval = obj->visibleItems();
	if (interval.x > interval.y)
		return;

	// draw items
	const vector<ListItem*>& items = obj->getItems();
	for (size_t i=interval.x; i<=interval.y; i++) {
		// draw background rect
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->itemRect(i, crop, color);
		drawRect(rect, color);

		// draw label
		Text txt(items[i]->label, vec2i(rect.x+Default::textOffset, rect.y-crop.y), Default::itemHeight);
		textCropRight(crop, txt.size().x, rect.w);
		drawText(txt, crop);

		// draw the rest specific to the derived class
		passDrawItem(items[i], rect, crop);
	}
	// draw scroll bar
	drawRect(obj->barRect(), EColor::darkened);
	drawRect(obj->sliderRect(), EColor::highlighted);
}

void DrawSys::drawObject(ReaderBox* obj) {
	vec2t interval = obj->visibleItems();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		Image img = obj->image(i, crop);
		drawImage(img, crop);
	}
	if (obj->showSlider()) {
		drawRect(obj->barRect(), EColor::darkened);
		drawRect(obj->sliderRect(), EColor::highlighted);
	}
	if (obj->showList()) {
		drawRect(obj->listRect(), EColor::darkened);
		for (Object* it : obj->getListObjects())
			passDrawObject(it);
	}
	if (obj->showPlayer()) {
		drawRect(obj->playerRect(), EColor::darkened);
		for (Object* it : obj->getPlayerObjects())
			passDrawObject(it);
	}
}

void DrawSys::passDrawItem(ListItem* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	if (Checkbox* obj = dynamic_cast<Checkbox*>(item))
		drawItem(obj, rect, crop);
	else if (Switchbox* obj = dynamic_cast<Switchbox*>(item))
		drawItem(obj, rect, crop);
	else if (LineEdit* obj = dynamic_cast<LineEdit*>(item))
		drawItem(obj, rect, crop);
	else if (KeyGetter* obj = dynamic_cast<KeyGetter*>(item))
		drawItem(obj, rect, crop);
}

void DrawSys::drawItem(Checkbox* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	int top = (crop.y < Default::checkboxSpacing) ? Default::checkboxSpacing - crop.y : crop.y;
	int bot = (crop.h > Default::checkboxSpacing) ? crop.h - Default::checkboxSpacing : Default::checkboxSpacing;

	EColor clr = item->getOn() ? EColor::highlighted : EColor::darkened;
	drawRect({rect.x+offset+Default::checkboxSpacing, rect.y-crop.y+top, Default::itemHeight-Default::checkboxSpacing*2, rect.h+crop.h-top-bot}, clr);
}

void DrawSys::drawItem(Switchbox* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;

	drawText(Text(item->getCurOption(), vec2i(rect.x+offset, rect.y-crop.y), Default::itemHeight), crop);
}

void DrawSys::drawItem(LineEdit* item, const SDL_Rect& rect, SDL_Rect crop) {
	// set up text
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	Text text(item->getEditor().getText(), vec2i(rect.x+offset-item->getTextPos(), rect.y-crop.y), Default::itemHeight);

	// set text's left and right crop
	int end = rect.x + rect.w;
	int len = text.size().x;
	crop.x = (text.pos.x < rect.x+offset) ? rect.x + offset - text.pos.x : 0;
	crop.w = (text.pos.x+len > end) ? text.pos.x + len - end : 0;
	drawText(text, crop);

	// draw caret if selected
	if (World::inputSys()->getCaptured() == item) {
		offset += Text(item->getEditor().getText().substr(0, item->getEditor().getCaretPos()), 0, Default::itemHeight).size().x - item->getTextPos();
		drawRect({rect.x+offset, rect.y, Default::textOffset, rect.h}, EColor::highlighted);
	}
}

void DrawSys::drawItem(KeyGetter* item, const SDL_Rect& rect, SDL_Rect crop) {
	// set up text
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	string str = (World::inputSys()->getCaptured() == item) ? "..." : item->text();
	EColor clr = (World::inputSys()->getCaptured() == item) ? EColor::highlighted : EColor::text;
	Text text(str, vec2i(rect.x+offset, rect.y-crop.y), Default::itemHeight, clr);

	// set text's left and right crop
	int end = rect.x + rect.w;
	int len = text.size().x;
	crop.w = (text.pos.x+len > end) ? text.pos.x + len - end : 0;
	drawText(text, crop);
}

void DrawSys::drawRect(const SDL_Rect& rect, EColor color) {
	vec4c clr = World::winSys()->getSettings().colors.at(color) / colorDim;
	SDL_SetRenderDrawColor(renderer, clr.r, clr.g, clr.b, clr.a);
	SDL_RenderFillRect(renderer, &rect);
}

void DrawSys::drawImage(const Image& img, const SDL_Rect& crop) {
	SDL_Rect rect = img.rect();	// the rect the image is gonna be projected on
	SDL_Texture* tex;
	if (needsCrop(crop)) {
		SDL_Rect ori = {rect.x, rect.y, img.texture->surface->w, img.texture->surface->h};	// proportions of the original image
		vec2f fac(float(ori.w) / float(rect.w), float(ori.h) / float(rect.h));				// scaling factor
		rect = cropRect(rect, crop);														// adjust rect to crop

		// crop original image by factor
		SDL_Surface* sheet = cropSurface(img.texture->surface, ori, {int(float(crop.x)*fac.x), int(float(crop.y)*fac.y), int(float(crop.w)*fac.x), int(float(crop.h)*fac.y)});
		tex = SDL_CreateTextureFromSurface(renderer, sheet);
		SDL_FreeSurface(sheet);
	} else
		tex = SDL_CreateTextureFromSurface(renderer, img.texture->surface);

	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
}

void DrawSys::drawText(const Text& txt, const SDL_Rect& crop) {
	if (txt.text.empty())
		return;
	
	vec4c tclr = World::winSys()->getSettings().colors.at(txt.color) / colorDim;
	vec2i siz = txt.size();
	SDL_Rect rect = {txt.pos.x, txt.pos.y, siz.x, siz.y};
	SDL_Surface* surface = TTF_RenderUTF8_Blended(World::library()->getFonts().get(txt.height), txt.text.c_str(), {tclr.r, tclr.g, tclr.b, tclr.a});
	SDL_Texture* tex;
	if (needsCrop(crop)) {
		SDL_Surface* sheet = cropSurface(surface, rect, crop);
		tex = SDL_CreateTextureFromSurface(renderer, sheet);
		SDL_FreeSurface(sheet);
	} else
		tex = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_RenderCopy(renderer, tex, nullptr, &rect);
	SDL_DestroyTexture(tex);
	SDL_FreeSurface(surface);
}
