#include "world.h"

// FONT SET

FontSet::~FontSet() {
	for (const pair<const int, TTF_Font*>& it : fonts)
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
	for (const pair<const int, TTF_Font*>& it : fonts)
		TTF_CloseFont(it.second);
	fonts.clear();
}

TTF_Font* FontSet::addSize(int size) {
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.insert(std::make_pair(size, font));
	return font;
}

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
	try {	// load font if it hasn't been loaded yet
		return fonts.at(height);
	} catch (const std::out_of_range&) {
		return addSize(height);
	}
}

int FontSet::length(const string& text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, int driverIndex) {
	// create and set up renderer
	if (!(renderer = SDL_CreateRenderer(window, driverIndex, Default::rendererFlags)))
		throw std::runtime_error("Couldn't create renderer:\n" + string(SDL_GetError()));
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// load default textures with colors and initialize fonts and translations
	for (string& it : FileSys::listDir(World::fileSys()->getDirTexs(), FTYPE_FILE)) {
		if (string file = World::fileSys()->getDirTexs() + it; SDL_Texture* tex = IMG_LoadTexture(renderer, file.c_str()))
			texes.insert(std::make_pair(delExt(it), tex));
		else
			std::cerr << "Couldn't load texture " << file << '\n' << IMG_GetError() << std::endl;
	}
	setTheme(World::sets()->getTheme());
	setFont(World::sets()->getFont());
	setLanguage(World::sets()->getLang());
}

DrawSys::~DrawSys() {
	for (const pair<string, SDL_Texture*>& it : texes)
		SDL_DestroyTexture(it.second);
	SDL_DestroyRenderer(renderer);
}

void DrawSys::setTheme(const string& name) {
	colors = World::fileSys()->loadColors(World::sets()->setTheme(name));
	SDL_Color clr = colors[uint8(Color::texture)];

	for (const pair<string, SDL_Texture*>& it : texes) {
		SDL_SetTextureColorMod(it.second, clr.r, clr.g, clr.b);
		SDL_SetTextureAlphaMod(it.second, clr.a);
	}
}

void DrawSys::setFont(const string& font) {
	fonts.init(World::sets()->setFont(font));
}

string DrawSys::translation(const string& line, bool firstCapital) const {
	string str = trans.count(line) ? trans.at(line) : line;
	if (firstCapital && str.size())
		str[0] = char(toupper(str[0]));
	return str;
}

SDL_Texture* DrawSys::texture(const string& name) const {
	try {
		return texes.at(name);
	} catch (const std::out_of_range&) {
		std::cerr << "Texture " << name << " doesn't exist." << std::endl;
		return nullptr;
	}
}

void DrawSys::setLanguage(const string& lang) {
	trans = World::fileSys()->loadTranslations(World::sets()->setLang(lang));
}

void DrawSys::drawWidgets() {
	// clear screen
	SDL_Color bgcolor = colors[uint8(Color::background)];
	SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	SDL_RenderClear(renderer);

	// draw main widgets and visible overlays
	World::scene()->getLayout()->drawSelf();
	if (World::scene()->getOverlay() && World::scene()->getOverlay()->on)
		World::scene()->getOverlay()->drawSelf();

	// draw popup if exists and dim main widgets
	if (World::scene()->getPopup()) {
		Rect view = viewport();
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
		drawRect(wgt->rect().getOverlap(wgt->frame()), wgt->color());
	if (wgt->tex)
		drawImage(wgt->tex, wgt->texRect(), wgt->frame());
}

void DrawSys::drawCheckBox(CheckBox* wgt) {
	Rect frame = wgt->frame();
	drawButton(wgt);												// draw background
	drawRect(wgt->boxRect().getOverlap(frame), wgt->boxColor());	// draw checkbox
}

void DrawSys::drawSlider(Slider* wgt) {
	Rect frame = wgt->frame();
	drawButton(wgt);												// draw background
	drawRect(wgt->barRect().getOverlap(frame), Color::dark);		// draw bar
	drawRect(wgt->sliderRect().getOverlap(frame), Color::light);	// draw slider
}

void DrawSys::drawLabel(Label* wgt) {
	Rect rect = wgt->rect().getOverlap(wgt->frame());
	if (wgt->showBG)	// draw background
		drawRect(rect, wgt->color());
	if (wgt->tex)		// draw left icon
		drawImage(wgt->tex, wgt->texRect(), rect);
	if (wgt->textTex)	// draw text
		drawText(wgt->textTex, wgt->textRect(), wgt->textFrame());
}

void DrawSys::drawScrollArea(ScrollArea* box) {
	vec2t vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizt i = vis.b; i < vis.t; i++)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(ReaderBox* box) {
	vec2t vis = box->visibleWidgets();
	for (sizt i = vis.b; i < vis.t; i++)
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

void DrawSys::drawRect(const Rect& rect, Color color) {
	SDL_Color clr = colors[uint8(color)];
	SDL_SetRenderDrawColor(renderer, clr.r, clr.g, clr.b, clr.a);
	SDL_RenderFillRect(renderer, &rect);
}

void DrawSys::drawText(SDL_Texture* tex, const Rect& rect, const Rect& frame) {
	// crop destination rect and original texture rect
	Rect dst = rect;
	Rect crop = dst.crop(frame);
	Rect src(crop.pos(), rect.size() - crop.size());

	SDL_RenderCopy(renderer, tex, &src, &dst);
}

void DrawSys::drawImage(SDL_Texture* tex, const Rect& rect, const Rect& frame) {
	// get destination rect and crop
	Rect dst = rect;
	Rect crop = dst.crop(frame);

	// get cropped source rect
	vec2i res = texSize(tex);
	vec2f factor(vec2f(res) / vec2f(rect.size()));
	Rect src(vec2f(crop.pos()) * factor, res - vec2i(vec2f(crop.size()) * factor));

	SDL_RenderCopy(renderer, tex, &src, &dst);
}

SDL_Texture* DrawSys::renderText(const string& text, int height) {
	if (text.empty())
		return nullptr;
	
	SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colors[uint8(Color::text)]);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
	return tex;
}

vector<Texture> DrawSys::loadTexturesDirectory(string drc) {
	vector<Texture> pics;
	for (string& it : FileSys::listDir(drc, FTYPE_FILE))
		if (SDL_Texture* tex = IMG_LoadTexture(renderer, childPath(drc, it).c_str()))
			pics.push_back(Texture(it, tex));
	return pics;
}

vector<Texture> DrawSys::loadTexturesArchive(const string& arc) {
	vector<Texture> pics;
	archive* arch = openArchive(arc);
	if (!arch)
		return pics;

	for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
		if (SDL_RWops* io = readArchiveEntry(arch, entry))
			if (SDL_Texture* tex = IMG_LoadTexture_RW(renderer, io, SDL_TRUE))
				pics.push_back(Texture(archive_entry_pathname(entry), tex));

	archive_read_free(arch);
	std::sort(pics.begin(), pics.end(), [](const Texture& a, const Texture& b) -> bool { return strnatless(a.name, b.name); });
	return pics;
}
