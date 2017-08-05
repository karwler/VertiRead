#include "world.h"

ShaderSys::ShaderSys() :
	renderer(nullptr)
{}

ShaderSys::~ShaderSys() {
	DestroyRenderer();
}

void ShaderSys::CreateRenderer(SDL_Window* window, int driverIndex) {
	renderer = SDL_CreateRenderer(window, driverIndex, Default::rendererFlags);
	if (!renderer)
		throw Exception("couldn't create renderer\n" + string(SDL_GetError()), 4);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void ShaderSys::DestroyRenderer() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
}

void ShaderSys::DrawObjects(const vector<Object*>& objects, const Popup* popup) {
	// clear screen
	colorDim = popup ? Default::colorPopupDim : 1;
	vec4c bgcolor = World::winSys()->Settings().colors.at(EColor::background);
	SDL_SetRenderDrawColor(renderer, bgcolor.r/colorDim.r, bgcolor.g/colorDim.g, bgcolor.b/colorDim.b, bgcolor.a/colorDim.a);
	SDL_RenderClear(renderer);

	// draw main objects
	for (Object* it : objects)
		PassDrawObject(it);

	// draw popup if exists
	colorDim = 1;
	if (popup) {
		DrawRect(popup->getRect(), popup->color);
		for (Object* it : popup->objects)
			PassDrawObject(it);
	}

	SDL_RenderPresent(renderer);
}

void ShaderSys::PassDrawObject(Object* obj) {
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
	else if (TableBox* box = dynamic_cast<TableBox*>(obj))
		DrawObject(box);
	else if (TileBox* box = dynamic_cast<TileBox*>(obj))
		DrawObject(box);
	else if (ReaderBox* box = dynamic_cast<ReaderBox*>(obj))
		DrawObject(box);
	else
		DrawRect(obj->getRect(), obj->color);
}

void ShaderSys::DrawObject(const SDL_Rect& bg, EColor bgColor, const Text& text) {
	DrawRect(bg, bgColor);

	int len = text.size().x;
	int left = (text.pos.x < bg.x) ? bg.x-text.pos.x : 0;
	int right = (text.pos.x+len > bg.x+bg.w) ? text.pos.x+len-bg.x-bg.w : 0;
	DrawText(text, {left, 0, right, 0});
}

void ShaderSys::DrawObject(LineEditor* obj) {
	SDL_Rect bg = obj->getRect();
	DrawRect(bg, obj->color);

	// get text and calculate left and right crop
	Text text = obj->getText();
	int len = text.size().x;
	int left = (text.pos.x < bg.x) ? bg.x - text.pos.x : 0;
	int right = (len-left > bg.w) ? len - left - bg.w : 0;
	DrawText(text, {left, 0, right, 0});

	if (World::inputSys()->Captured() == obj)
		DrawRect(obj->getCaret(), EColor::highlighted);
}

void ShaderSys::DrawObject(ListBox* obj) {
	vec2t interval = obj->VisibleItems();
	if (interval.x > interval.y)
		return;

	const vector<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++) {
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, crop, color);
		DrawRect(rect, color);

		Text txt(items[i]->label, vec2i(rect.x+Default::textOffset, rect.y-crop.y), Default::itemHeight);
		textCropRight(crop, txt.size().x, rect.w);
		DrawText(txt, crop);

		PassDrawItem(i, obj, rect, crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void ShaderSys::DrawObject(TableBox* obj) {
	vec2t interval = obj->VisibleItems();
	if (interval.x > interval.y)
		return;

	const grid2<ListItem*>& items = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, crop);
		DrawRect(rect, EColor::rectangle);

		Text txt(items[i]->label, vec2i(rect.x+Default::textOffset, rect.y-crop.y), Default::itemHeight);
		textCropRight(crop, txt.size().x, rect.w);
		DrawText(txt, crop);

		PassDrawItem(i, obj, rect, crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void ShaderSys::DrawObject(TileBox* obj) {
	vec2t interval = obj->VisibleItems();
	if (interval.x > interval.y)
		return;

	const vector<ListItem*>& tiles = obj->Items();
	for (size_t i=interval.x; i<=interval.y; i++) {
		EColor color;
		SDL_Rect crop;
		SDL_Rect rect = obj->ItemRect(i, crop, color);
		DrawRect(rect, color);

		Text txt(tiles[i]->label, vec2i(rect.x+5, rect.y-crop.y), obj->TileSize().y);
		textCropRight(crop, txt.size().x, rect.w);
		DrawText(txt, crop);
	}
	DrawRect(obj->Bar(), EColor::darkened);
	DrawRect(obj->Slider(), EColor::highlighted);
}

void ShaderSys::DrawObject(ReaderBox* obj) {
	vec2t interval = obj->VisibleItems();
	for (size_t i=interval.x; i<=interval.y; i++) {
		SDL_Rect crop;
		Image img = obj->getImage(i, crop);
		DrawImage(img, crop);
	}
	if (obj->showSlider()) {
		DrawRect(obj->Bar(), EColor::darkened);
		DrawRect(obj->Slider(), EColor::highlighted);
	}
	if (obj->showList()) {
		DrawRect(obj->List(), EColor::darkened);
		for (Object* it : obj->ListObjects())
			PassDrawObject(it);
	}
	if (obj->showPlayer()) {
		DrawRect(obj->Player(), EColor::darkened);
		for (Object* it : obj->PlayerObjects())
			PassDrawObject(it);
	}
}

void ShaderSys::PassDrawItem(size_t id, ScrollAreaX1* parent, const SDL_Rect& rect, const SDL_Rect& crop) {
	if (Checkbox* obj = dynamic_cast<Checkbox*>(parent->Item(id)))
		DrawItem(obj, rect, crop);
	else if (Switchbox* obj = dynamic_cast<Switchbox*>(parent->Item(id)))
		DrawItem(obj, rect, crop);
	else if (LineEdit* obj = dynamic_cast<LineEdit*>(parent->Item(id)))
		DrawItem(obj, parent, rect, crop);
	else if (KeyGetter* obj = dynamic_cast<KeyGetter*>(parent->Item(id)))
		DrawItem(obj, rect, crop);
}

void ShaderSys::DrawItem(Checkbox* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	int top = (crop.y < Default::checkboxSpacing) ? Default::checkboxSpacing - crop.y : crop.y;
	int bot = (crop.h > Default::checkboxSpacing) ? crop.h - Default::checkboxSpacing : Default::checkboxSpacing;

	DrawRect({rect.x+offset+Default::checkboxSpacing, rect.y-crop.y+top, Default::itemHeight-Default::checkboxSpacing*2, rect.h+crop.h-top-bot}, item->On() ? EColor::highlighted : EColor::darkened);
}

void ShaderSys::DrawItem(Switchbox* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;

	DrawText(Text(item->CurOption(), vec2i(rect.x+offset, rect.y-crop.y), Default::itemHeight), crop);
}

void ShaderSys::DrawItem(LineEdit* item, ScrollAreaX1* parent, const SDL_Rect& rect, SDL_Rect crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	Text text(item->Editor().Text(), vec2i(rect.x+offset-item->TextPos(), rect.y-crop.y), Default::itemHeight);

	// set text's left and right crop
	int end = parent->End().x;
	int len = text.size().x;
	crop.x = (text.pos.x < rect.x+offset) ? rect.x + offset - text.pos.x : 0;
	crop.w = (text.pos.x+len > end) ? text.pos.x + len - end : 0;
	DrawText(text, crop);

	// draw caret if selected
	if (World::inputSys()->Captured() == item) {
		offset += Text(item->Editor().Text().substr(0, item->Editor().CursorPos()), 0, Default::itemHeight).size().x - item->TextPos();
		DrawRect({rect.x+offset, rect.y, Default::textOffset, rect.h}, EColor::highlighted);
	}
}

void ShaderSys::DrawItem(KeyGetter* item, const SDL_Rect& rect, const SDL_Rect& crop) {
	int offset = item->label.empty() ? Default::textOffset : Text(item->label, 0, Default::itemHeight).size().x + Default::lineEditOffset;
	string text = (World::inputSys()->Captured() == item) ? "..." : item->Text();
	EColor color = (World::inputSys()->Captured() == item) ? EColor::highlighted : EColor::text;

	DrawText(Text(text, vec2i(rect.x+offset, rect.y-crop.y), Default::itemHeight, color), crop);
}

void ShaderSys::DrawRect(const SDL_Rect& rect, EColor color) {
	vec4c cclr = World::winSys()->Settings().colors.at(color) / colorDim;
	SDL_SetRenderDrawColor(renderer, cclr.r, cclr.g, cclr.b, cclr.a);
	SDL_RenderFillRect(renderer, &rect);
}

void ShaderSys::DrawImage(const Image& img, const SDL_Rect& crop) {
	SDL_Rect rect = img.getRect();	// the rect the image is gonna be projected on
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

void ShaderSys::DrawText(const Text& txt, const SDL_Rect& crop) {
	if (txt.text.empty())
		return;

	vec4c tclr = World::winSys()->Settings().colors.at(txt.color) / colorDim;
	vec2i siz = txt.size();
	SDL_Rect rect = {txt.pos.x, txt.pos.y, siz.x, siz.y};
	SDL_Surface* surface = TTF_RenderUTF8_Blended(World::library()->Fonts()->Get(txt.height), txt.text.c_str(), {tclr.r, tclr.g, tclr.b, tclr.a});
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
