#include "world.h"

DrawSys::DrawSys() :
	renderer(nullptr)
{}

DrawSys::~DrawSys() {
	destroyRenderer();
}

SDL_Renderer* DrawSys::getRenderer() {
	return renderer;
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

void DrawSys::drawWidgets(const vector<Widget*>& widgets, const Popup* popup) {
	// clear screen
	colorDim = popup ? Default::colorPopupDim : Default::colorNoDim;
	SDL_Color bgcolor = World::winSys()->getSettings().colors.at(EColor::background);
	SDL_SetRenderDrawColor(renderer, bgcolor.r/colorDim.r, bgcolor.g/colorDim.g, bgcolor.b/colorDim.b, bgcolor.a/colorDim.a);
	SDL_RenderClear(renderer);

	// draw main widgets
	for (Widget* it : widgets)
		passDrawWidget(it);

	// draw popup if exists
	colorDim = Default::colorNoDim;
	if (popup) {
		drawRect(popup->rect(), popup->color);
		for (Widget* it : popup->widgets)
			passDrawWidget(it);
	}

	SDL_RenderPresent(renderer);
}

void DrawSys::passDrawWidget(Widget* wgt) {
	// specific drawing for each widget
	if (Label* lbl = dynamic_cast<Label*>(wgt))
		drawWidget(lbl->rect(), lbl->color, lbl->text());
	else if (ButtonText* btxt = dynamic_cast<ButtonText*>(wgt))
		drawWidget(btxt->rect(), btxt->color, btxt->text());
	else if (ButtonImage* bimg = dynamic_cast<ButtonImage*>(wgt))
		drawImage(bimg->getCurTex(), bimg->rect());
	else if (LineEditor* edt = dynamic_cast<LineEditor*>(wgt))
		drawWidget(edt);
	else if (ScrollAreaItems* box = dynamic_cast<ScrollAreaItems*>(wgt))
		drawWidget(box);
	else if (ReaderBox* box = dynamic_cast<ReaderBox*>(wgt))
		drawWidget(box);
	else
		drawRect(wgt->rect(), wgt->color);
}

void DrawSys::drawWidget(const SDL_Rect& bg, EColor bgColor, const Text& text) {
	drawRect(bg, bgColor);
	drawText(text, bg);
}

void DrawSys::drawWidget(LineEditor* wgt) {
	SDL_Rect bg = wgt->rect();
	drawRect(bg, wgt->color);
	drawText(wgt->text(), bg);

	if (World::inputSys()->getCaptured() == wgt)
		drawRect(wgt->caretRect(), EColor::highlighted);
}

void DrawSys::drawWidget(ScrollAreaItems* wgt) {
	// draw scroll are background
	SDL_Rect bg = wgt->rect();
	drawRect(bg, wgt->color);

	// get index interval of items on screen and draw items
	vec2t interval = wgt->visibleItems();
	for (size_t i=interval.x; i<=interval.y; i++) {
		ListItem* it = wgt->item(i);

		// draw background rect
		SDL_Rect rect = wgt->itemRect(i);
		vec2i textPos(rect.x + Default::textOffset, rect.y);
		cropRect(rect, bg);
		EColor color = (it == wgt->selectedItem) ? EColor::highlighted : EColor::rectangle;
		drawRect(rect, color);

		// draw label
		Text txt(it->label, textPos, Default::itemHeight);
		drawText(txt, rect);

		// draw the rest specific to the derived class
		if (!it->label.empty())
			textPos.x += txt.size().x + Default::lineEditOffset;
		passDrawItem(it, rect, textPos);
	}
	// draw scroll bar
	drawRect(wgt->barRect(), EColor::darkened);
	drawRect(wgt->sliderRect(), EColor::highlighted);
}

void DrawSys::drawWidget(ReaderBox* wgt) {
	// draw scroll are background
	SDL_Rect bg = wgt->rect();
	drawRect(bg, wgt->color);

	// get index interval of pictures on screen and draw pictures
	vec2t interval = wgt->visiblePictures();
	for (size_t i=interval.x; i<=interval.y; i++)
		drawImage(wgt->image(i), bg);

	if (wgt->showSlider()) {
		drawRect(wgt->barRect(), EColor::darkened);
		drawRect(wgt->sliderRect(), EColor::highlighted);
	}
	if (wgt->showList()) {
		drawRect(wgt->listRect(), EColor::darkened);
		for (Widget* it : wgt->getListWidgets())
			passDrawWidget(it);
	}
	if (wgt->showPlayer()) {
		drawRect(wgt->playerRect(), EColor::darkened);
		for (Widget* it : wgt->getPlayerWidgets())
			passDrawWidget(it);
	}
}

void DrawSys::passDrawItem(ListItem* item, const SDL_Rect& rect, const vec2i& textPos) {
	if (Checkbox* wgt = dynamic_cast<Checkbox*>(item))
		drawItem(wgt, rect, textPos);
	else if (Switchbox* wgt = dynamic_cast<Switchbox*>(item))
		drawItem(wgt, rect, textPos);
	else if (LineEdit* wgt = dynamic_cast<LineEdit*>(item))
		drawItem(wgt, rect, textPos);
	else if (KeyGetter* wgt = dynamic_cast<KeyGetter*>(item))
		drawItem(wgt, rect, textPos);
}

void DrawSys::drawItem(Checkbox* item, const SDL_Rect& rect, const vec2i& textPos) {
	EColor clr = item->getOn() ? EColor::highlighted : EColor::darkened;
	int size = Default::itemHeight - Default::checkboxSpacing*2;
	SDL_Rect box = {textPos.x+Default::checkboxSpacing, textPos.y+Default::checkboxSpacing, size, size};
	cropRect(box, rect);
	drawRect(box, clr);
}

void DrawSys::drawItem(Switchbox* item, const SDL_Rect& rect, const vec2i& textPos) {
	drawText(Text(item->getCurOption(), textPos, Default::itemHeight), rect);
}

void DrawSys::drawItem(LineEdit* item, const SDL_Rect& rect, const vec2i& textPos) {
	drawText(Text(item->getEditor().getText(), vec2i(textPos.x-item->getTextPos(), textPos.y), Default::itemHeight), {textPos.x, rect.y, rect.w-textPos.x+rect.x, rect.h});

	if (World::inputSys()->getCaptured() == item) {
		int pos = textPos.x + Text(item->getEditor().getText().substr(0, item->getEditor().getCaretPos()), 0, Default::itemHeight).size().x - item->getTextPos();
		drawRect({pos, rect.y, Default::caretWidth, rect.h}, EColor::highlighted);
	}
}

void DrawSys::drawItem(KeyGetter* item, const SDL_Rect& rect, const vec2i& textPos) {
	EColor clr = (World::inputSys()->getCaptured() == item) ? EColor::highlighted : EColor::text;
	Text text(item->text(), textPos, Default::itemHeight, clr);
	drawText(text, rect);
}

void DrawSys::drawRect(const SDL_Rect& rect, EColor color) {
	SDL_Color clr = World::winSys()->getSettings().colors.at(color);
	SDL_SetRenderDrawColor(renderer, clr.r/colorDim.r, clr.g/colorDim.g, clr.b/colorDim.b, clr.a/colorDim.a);
	SDL_RenderFillRect(renderer, &rect);
}

void DrawSys::drawImage(const Image& img, const SDL_Rect& frame) {
	// get destination rect and crop
	SDL_Rect dst = img.rect();
	SDL_Rect crop = cropRect(dst, frame);

	// get cropped source rect
	vec2f factor(float(img.tex->getRes().x) / float(img.size.x), float(img.tex->getRes().y) / float(img.size.y));
	SDL_Rect src = {float(crop.x) * factor.x, float(crop.y) * factor.y, img.size.x - int(float(crop.w) * factor.x), img.size.y - int(float(crop.h) * factor.y)};

	SDL_RenderCopy(renderer, img.tex->tex, &src, &dst);
}

void DrawSys::drawText(const Text& txt, const SDL_Rect& frame) {
	if (txt.text.empty())
		return;

	// get text texture
	SDL_Color clr = World::winSys()->getSettings().colors.at(txt.color);
	SDL_Surface* surf = TTF_RenderUTF8_Blended(World::library()->getFonts().getFont(txt.height), txt.text.c_str(), {clr.r/colorDim.r, clr.g/colorDim.g, clr.b/colorDim.b, clr.a/colorDim.a});
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);

	// crop destination rect and original texture rect
	SDL_Rect dst = {txt.pos.x, txt.pos.y, surf->w, surf->h};
	SDL_Rect crop = cropRect(dst, frame);
	SDL_Rect src = {crop.x, crop.y, surf->w - crop.w, surf->h - crop.h};

	// draw and cleanup
	SDL_RenderCopy(renderer, tex, &src, &dst);
	SDL_DestroyTexture(tex);
	SDL_FreeSurface(surf);
}
