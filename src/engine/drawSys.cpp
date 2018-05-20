#include "world.h"

// FONT SET

FontSet::FontSet(const string& FILE) {
	init(FILE);
}

FontSet::~FontSet() {
	for (const pair<int, TTF_Font*>& it : fonts)
		TTF_CloseFont(it.second);
}

void FontSet::init(const string& path) {
	clear();
	file = path;

	// get approximate height scale factor
	int size;
	TTF_Font* tmp = TTF_OpenFont(file.c_str(), Default::fontTestHeight);
	TTF_SizeUTF8(tmp, Default::fontTestString, nullptr, &size);
	heightScale = float(Default::fontTestHeight) / float(size);
	TTF_CloseFont(tmp);
}

void FontSet::clear() {
	for (const pair<int, TTF_Font*>& it : fonts)
		TTF_CloseFont(it.second);
	fonts.clear();
}

TTF_Font* FontSet::addSize(int size) {
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.insert(make_pair(size, font));
	return font;
}

TTF_Font* FontSet::getFont(int height) {
	height = float(height) * heightScale;
	return fonts.count(height) ? fonts.at(height) : addSize(height);	// load font if it hasn't been loaded yet
}

int FontSet::length(const string& text, int height) {
	int len = 0;
	TTF_Font* font = getFont(height);
	if (font)
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, int driverIndex) :
	colors(Filer::getColors(World::winSys()->sets.getTheme())),
	fonts(Filer::findFont(World::winSys()->sets.getFont())),
	trans(Filer::getTranslations(World::winSys()->sets.getLang()))
{
	renderer = SDL_CreateRenderer(window, driverIndex, Default::rendererFlags);
	if (!renderer)
		throw "Couldn't create renderer:\n" + string(SDL_GetError());
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

DrawSys::~DrawSys() {
	SDL_DestroyRenderer(renderer);
}

SDL_Rect DrawSys::viewport() const {
	SDL_Rect view;
	SDL_RenderGetViewport(renderer, &view);
	return view;
}

void DrawSys::setTheme(const string& name) {
	colors = Filer::getColors(World::winSys()->sets.setTheme(name));
}

void DrawSys::setFont(const string& font) {
	fonts.init(World::winSys()->sets.setFont(font));
}

string DrawSys::translation(const string& line, bool firstCapital) const {
	string str = trans.count(line) ? trans.at(line) : line;
	if (firstCapital && str.size())
		str[0] = toupper(str[0]);
	return str;
}

void DrawSys::setLanguage(const string& lang) {
	trans = Filer::getTranslations(World::winSys()->sets.setLang(lang));
}

void DrawSys::drawWidgets() {
	// clear screen
	SDL_Color bgcolor = colors[static_cast<uint8>(Color::background)];
	SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	SDL_RenderClear(renderer);

	// draw main widgets and visible overlays
	World::scene()->getLayout()->drawSelf();
	if (World::scene()->getOverlay() && World::scene()->getOverlay()->on)
		World::scene()->getOverlay()->drawSelf();

	// draw popup if exists and dim main widgets
	if (World::scene()->getPopup()) {
		SDL_Rect view = viewport();
		SDL_SetRenderDrawColor(renderer, Default::colorPopupDim.r, Default::colorPopupDim.g, Default::colorPopupDim.b, Default::colorPopupDim.a);
		SDL_RenderFillRect(renderer, &view);

		World::scene()->getPopup()->drawSelf();
	}

	// draw caret if capturing LineEdit
	if (LineEdit* let = dynamic_cast<LineEdit*>(World::scene()->capture))
		drawRect(let->caretRect(), Color::light);

	SDL_RenderPresent(renderer);
}

void DrawSys::drawButton(Button* wgt) {
	drawRect(overlapRect(wgt->rect(), wgt->parentFrame()), (World::scene()->select == wgt) ? Color::select : Color::normal);
}

void DrawSys::drawCheckBox(CheckBox* wgt) {
	SDL_Rect frame = wgt->parentFrame();
	drawRect(overlapRect(wgt->rect(), frame), (World::scene()->select == wgt) ? Color::select : Color::normal);	// draw background
	drawRect(overlapRect(wgt->boxRect(), frame), wgt->on ? Color::light : Color::dark);	// draw checkbox
}

void DrawSys::drawSlider(Slider* wgt) {
	SDL_Rect frame = wgt->parentFrame();
	drawRect(overlapRect(wgt->rect(), frame), (World::scene()->select == wgt) ? Color::select : Color::normal);	// draw background
	drawRect(overlapRect(wgt->barRect(), frame), Color::dark);		// draw bar
	drawRect(overlapRect(wgt->sliderRect(), frame), Color::light);	// draw slider
}

void DrawSys::drawPicture(Picture* wgt) {
	drawImage(wgt->tex, wgt->getRes(), wgt->rect(), wgt->parentFrame());
}

void DrawSys::drawLabel(Label* wgt) {
	SDL_Rect rect = overlapRect(wgt->rect(), wgt->parentFrame());
	drawRect(rect, (World::scene()->select == wgt) ? Color::select : Color::normal);	// draw background

	if (wgt->tex) {		// modify frame and draw text if exists
		rect.x += Default::textOffset;
		rect.w -= Default::textOffset * 2;
		drawText(wgt->tex, wgt->textRect(), rect);
	}
}

void DrawSys::drawScrollArea(ScrollArea* box) {
	vec2t vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizt i=vis.l; i<vis.u; i++)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(ReaderBox* box) {
	vec2t vis = box->visibleWidgets();
	for (sizt i=vis.l; i<vis.u; i++)
		box->getWidget(i)->drawSelf();

	if (box->showBar()) {
		drawRect(box->barRect(), Color::dark);
		drawRect(box->sliderRect(), Color::light);
	}
}

void DrawSys::drawPopup(Popup* box) {
	drawRect(box->rect(), Color::normal);	// draw background
	for (Widget* it : box->getWidgets())	// draw children
		it->drawSelf();
}

void DrawSys::drawRect(const SDL_Rect& rect, Color color) {
	SDL_Color clr = colors[static_cast<uint8>(color)];
	SDL_SetRenderDrawColor(renderer, clr.r, clr.g, clr.b, clr.a);
	SDL_RenderFillRect(renderer, &rect);
}

void DrawSys::drawText(SDL_Texture* tex, const SDL_Rect& rect, const SDL_Rect& frame) {
	// crop destination rect and original texture rect
	SDL_Rect dst = rect;
	SDL_Rect crop = cropRect(dst, frame);
	SDL_Rect src = {crop.x, crop.y, rect.w - crop.w, rect.h - crop.h};

	SDL_RenderCopy(renderer, tex, &src, &dst);
}

void DrawSys::drawImage(SDL_Texture* tex, const vec2i& res, const SDL_Rect& rect, const SDL_Rect& frame) {
	// get destination rect and crop
	SDL_Rect dst = rect;
	SDL_Rect crop = cropRect(dst, frame);

	// get cropped source rect
	vec2f factor(float(res.x) / float(rect.w), float(res.y) / float(rect.h));
	SDL_Rect src = {float(crop.x) * factor.x, float(crop.y) * factor.y, res.x - int(float(crop.w) * factor.x), res.y - int(float(crop.h) * factor.y)};

	SDL_RenderCopy(renderer, tex, &src, &dst);
}

SDL_Texture* DrawSys::renderText(const string& text, int height, vec2i& size) {
	if (text.empty()) {
		size = 0;
		return nullptr;
	}
	SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colors[static_cast<uint8>(Color::text)]);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	size = vec2i(surf->w, surf->h);
	SDL_FreeSurface(surf);
	return tex;
}

SDL_Texture* DrawSys::loadTexture(const string& file, vec2i& res) {
	SDL_Texture* tex = IMG_LoadTexture(renderer, file.c_str());
	if (tex)
		SDL_QueryTexture(tex, nullptr, nullptr, &res.x, &res.y);
	else {
		cerr << "Couldn't load texture " << file << endl << IMG_GetError << endl;
		res = 0;
	}
	return tex;
}
