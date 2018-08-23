#include "world.h"
#include <SDL2/SDL_image.h>
#include <archive.h>
#include <archive_entry.h>
#include <iostream>

// FONT SET

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
	try {	// load font if it hasn't been loaded yet
		return fonts.at(height);
	} catch (const std::out_of_range&) {
		return addSize(height);
	}
}

int FontSet::length(const string& text, int height) {
	int len = 0;
	TTF_Font* font = getFont(height);
	if (font)
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, int driverIndex) {
	// create and set up renderer
	renderer = SDL_CreateRenderer(window, driverIndex, Default::rendererFlags);
	if (!renderer)
		throw std::runtime_error("Couldn't create renderer:\n" + string(SDL_GetError()));
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// load default textures with colors and initialize fonts and translations
	for (string& it : Filer::listDir(Filer::dirTexs, FTYPE_FILE)) {
		string file = Filer::dirTexs + it;
		SDL_Texture* tex = IMG_LoadTexture(renderer, file.c_str());
		if (tex)
			texes.insert(make_pair(delExt(it), tex));
		else
			std::cerr << "Couldn't load texture " << file << std::endl << IMG_GetError << std::endl;
	}
	setTheme(World::winSys()->sets.getTheme());
	setFont(World::winSys()->sets.getFont());
	setLanguage(World::winSys()->sets.getLang());
}

DrawSys::~DrawSys() {
	for (const pair<string, SDL_Texture*>& it : texes)
		SDL_DestroyTexture(it.second);
	SDL_DestroyRenderer(renderer);
}

SDL_Rect DrawSys::viewport() const {
	SDL_Rect view;
	SDL_RenderGetViewport(renderer, &view);
	return view;
}

vec2i DrawSys::viewSize() const {
	SDL_Rect view;
	SDL_RenderGetViewport(renderer, &view);
	return vec2i(view.w, view.h);
}

void DrawSys::setTheme(const string& name) {
	colors = Filer::getColors(World::winSys()->sets.setTheme(name));
	SDL_Color clr = colors[static_cast<uint8>(Color::texture)];
	for (const pair<string, SDL_Texture*>& it : texes) {
		SDL_SetTextureColorMod(it.second, clr.r, clr.g, clr.b);
		SDL_SetTextureAlphaMod(it.second, clr.a);
	}
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
	if (wgt->showBG)
		drawRect(overlapRect(wgt->rect(), wgt->frame()), wgt->color());
	if (wgt->tex)
		drawImage(wgt->tex, wgt->texRect(), wgt->frame());
}

void DrawSys::drawCheckBox(CheckBox* wgt) {
	SDL_Rect frame = wgt->frame();
	drawButton(wgt);												// draw background
	drawRect(overlapRect(wgt->boxRect(), frame), wgt->boxColor());	// draw checkbox
}

void DrawSys::drawSlider(Slider* wgt) {
	SDL_Rect frame = wgt->frame();
	drawButton(wgt);												// draw background
	drawRect(overlapRect(wgt->barRect(), frame), Color::dark);		// draw bar
	drawRect(overlapRect(wgt->sliderRect(), frame), Color::light);	// draw slider
}

void DrawSys::drawLabel(Label* wgt) {
	SDL_Rect rect = overlapRect(wgt->rect(), wgt->frame());
	if (wgt->showBG)	// draw background
		drawRect(rect, wgt->color());
	if (wgt->tex)		// draw left icon
		drawImage(wgt->tex, wgt->texRect(), rect);
	if (wgt->textTex)	// draw text
		drawText(wgt->textTex, wgt->textRect(), wgt->textFrame());
}

void DrawSys::drawScrollArea(ScrollArea* box) {
	vec2t vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizt i = vis.l; i < vis.u; i++)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(ReaderBox* box) {
	vec2t vis = box->visibleWidgets();
	for (sizt i = vis.l; i < vis.u; i++)
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

void DrawSys::drawImage(SDL_Texture* tex, const SDL_Rect& rect, const SDL_Rect& frame) {
	// get destination rect and crop
	SDL_Rect dst = rect;
	SDL_Rect crop = cropRect(dst, frame);

	// get cropped source rect
	vec2i res;
	SDL_QueryTexture(tex, nullptr, nullptr, &res.x, &res.y);
	vec2f factor(float(res.x) / float(rect.w), float(res.y) / float(rect.h));
	SDL_Rect src = {int(float(crop.x) * factor.x), int(float(crop.y) * factor.y), res.x - int(float(crop.w) * factor.x), res.y - int(float(crop.h) * factor.y)};

	SDL_RenderCopy(renderer, tex, &src, &dst);
}

SDL_Texture* DrawSys::renderText(const string& text, int height) {
	if (text.empty())
		return nullptr;
	
	SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colors[static_cast<uint8>(Color::text)]);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
	return tex;
}

vector<pair<string, SDL_Texture*>> DrawSys::loadTexturesDirectory(string drc) {
	vector<pair<string, SDL_Texture*>> pics;
	for (string& it : Filer::listDir(drc, FTYPE_FILE))
		if (SDL_Texture* tex = IMG_LoadTexture(renderer, childPath(drc, it).c_str()))
			pics.push_back(make_pair(it, tex));
	return pics;
}

vector<pair<string, SDL_Texture*>> DrawSys::loadTexturesArchive(const string& arc) {
	struct archive* arch = archive_read_new();
	archive_read_support_filter_all(arch);
	archive_read_support_format_all(arch);

	vector<pair<string, SDL_Texture*>> pics;
	if (archive_read_open_filename(arch, arc.c_str(), Default::archiveReadBlockSize))
		return pics;

	struct archive_entry* entry;
	while (!archive_read_next_header(arch, &entry)) {
		la_int64_t bsiz = archive_entry_size(entry);
		if (bsiz <= 0)
			continue;
	
		void* buffer = malloc(bsiz);
		la_ssize_t size = archive_read_data(arch, buffer, bsiz);
		if (size < 0)
			continue;
		
		SDL_RWops* io = SDL_RWFromMem(buffer, size);
		if (SDL_Texture* tex = IMG_LoadTexture_RW(renderer, io, SDL_TRUE))
			pics.push_back(make_pair(archive_entry_pathname(entry), tex));
	}
	archive_read_free(arch);
	std::sort(pics.begin(), pics.end(), [](const pair<string, SDL_Texture*>& a, const pair<string, SDL_Texture*>& b) -> bool { return strnatless(a.first, b.first); });
	return pics;
}

SDL_Texture* DrawSys::texture(const string& name) const {
	try {
		return texes.at(name);
	} catch (const std::out_of_range&) {
		std::cerr << "Texture " << name << " doesn't exist." << std::endl;
		return nullptr;
	}
}
