#include "world.h"

// FONT SET

FontSet::~FontSet() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
}

void FontSet::init(const string& path) {
	clear();
	file = path;
	TTF_Font* tmp = TTF_OpenFont(file.c_str(), fontTestHeight);
	if (!tmp)
		throw std::runtime_error(TTF_GetError());

	// get approximate height scale factor
	int size;
	TTF_SizeUTF8(tmp, fontTestString, nullptr, &size);
	heightScale = float(fontTestHeight) / float(size);
	TTF_CloseFont(tmp);
}

void FontSet::clear() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
	fonts.clear();
}

TTF_Font* FontSet::addSize(int size) {
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.emplace(size, font);
	else
		std::cerr << "failed to load font:\n" << TTF_GetError() << std::endl;
	return font;
}

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
	try {	// load font if it hasn't been loaded yet
		return fonts.at(height);
	} catch (const std::out_of_range&) {}
	return addSize(height);
}

int FontSet::length(const string& text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text.c_str(), &len, nullptr);
	return len;
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, int driverIndex) :
	rendLock(SDL_CreateMutex())
{
	// create and set up renderer
	if (!(renderer = SDL_CreateRenderer(window, driverIndex, rendererFlags)))
		throw std::runtime_error(string("Failed to create renderer:\n") + SDL_GetError());
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// load default textures with colors and initialize fonts
	for (const string& file : FileSys::listDir(FileSys::dirTexs, FTYPE_REG, true, false)) {
		if (string path = FileSys::dirTexs + file; SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str()))
			texes.emplace(delExt(file), tex);
		else
			std::cerr << "failed to load texture " << file << '\n' << IMG_GetError() << std::endl;
	}
	setTheme(World::sets()->getTheme());
	setFont(World::sets()->getFont());
}

DrawSys::~DrawSys() {
	SDL_DestroyMutex(rendLock);
	for (const auto& [name, tex] : texes)
		SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
}

void DrawSys::setTheme(const string& name) {
	colors = World::fileSys()->loadColors(World::sets()->setTheme(name));
	SDL_Color clr = colors[uint8(Color::texture)];

	for (const auto& [name, tex] : texes) {
		SDL_SetTextureColorMod(tex, clr.r, clr.g, clr.b);
		SDL_SetTextureAlphaMod(tex, clr.a);
	}
}

void DrawSys::setFont(const string& font) {
	fonts.init(World::fileSys()->findFont(World::sets()->setFont(font)));
}

SDL_Texture* DrawSys::texture(const string& name) const {
	try {
		return texes.at(name);
	} catch (const std::out_of_range&) {
		std::cerr << "texture " << name << " doesn't exist" << std::endl;
	}
	return nullptr;
}

void DrawSys::drawWidgets() {
	while (SDL_TryLockMutex(rendLock));

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
		SDL_SetRenderDrawColor(renderer, colorPopupDim.r, colorPopupDim.g, colorPopupDim.b, colorPopupDim.a);
		SDL_RenderFillRect(renderer, &view);

		World::scene()->getPopup()->drawSelf();
	}

	// draw caret if capturing LineEdit
	if (LabelEdit* let = dynamic_cast<LabelEdit*>(World::scene()->capture))
		drawRect(let->caretRect(), Color::light);

	SDL_RenderPresent(renderer);
	SDL_UnlockMutex(rendLock);
}

void DrawSys::drawPicture(const Picture* wgt) {
	if (wgt->showBG)
		drawRect(wgt->rect().intersect(wgt->frame()), wgt->color());
	if (wgt->tex)
		drawImage(wgt->tex, wgt->texRect(), wgt->frame());
}

void DrawSys::drawCheckBox(const CheckBox* wgt) {
	drawPicture(wgt);													// draw background
	drawRect(wgt->boxRect().intersect(wgt->frame()), wgt->boxColor());	// draw checkbox
}

void DrawSys::drawSlider(const Slider* wgt) {
	Rect frame = wgt->frame();
	drawPicture(wgt);												// draw background
	drawRect(wgt->barRect().intersect(frame), Color::dark);		// draw bar
	drawRect(wgt->sliderRect().intersect(frame), Color::light);	// draw slider
}

void DrawSys::drawProgressBar(const ProgressBar* wgt) {
	drawRect(wgt->rect(), Color::normal);								// draw background
	drawRect(wgt->barRect().intersect(wgt->frame()), Color::light);	// draw bar
}

void DrawSys::drawLabel(const Label* wgt) {
	drawPicture(wgt);
	if (wgt->textTex)
		drawText(wgt->textTex, wgt->textRect(), wgt->textFrame());
}

void DrawSys::drawScrollArea(const ScrollArea* box) {
	vec2t vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.b; i < vis.t; i++)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(const ReaderBox* box) {
	vec2t vis = box->visibleWidgets();
	for (sizet i = vis.b; i < vis.t; i++)
		box->getWidget(i)->drawSelf();

	if (box->showBar()) {
		drawRect(box->barRect(), Color::dark);
		drawRect(box->sliderRect(), Color::light);
	}
}

void DrawSys::drawPopup(const Popup* box) {
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
	if (SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts.getFont(height), text.c_str(), colors[uint8(Color::text)])) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_FreeSurface(surf);
		return tex;
	}
	return nullptr;
}

vector<Texture> DrawSys::loadTexturesDirectory(const string& drc) {
	vector<Texture> pics;
	for (string& it : FileSys::listDir(drc, FTYPE_REG, World::sets()->showHidden))
		if (SDL_Texture* tex = IMG_LoadTexture(World::drawSys()->renderer, childPath(drc, it).c_str()))
			pics.emplace_back(it, tex);
	return pics;
}

vector<Texture> DrawSys::loadTexturesArchive(const string& arc) {
	vector<Texture> pics;
	archive* arch = FileSys::openArchive(arc);
	if (!arch)
		return pics;

	for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
		if (SDL_Texture* tex = loadArchiveTexture(arch, entry))
			pics.emplace_back(archive_entry_pathname(entry), tex);

	archive_read_free(arch);
	std::sort(pics.begin(), pics.end(), [](const Texture& a, const Texture& b) -> bool { return strnatless(a.name, b.name); });
	return pics;
}

int DrawSys::loadTexturesDirectoryThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	string& drc = *static_cast<string*>(thread->data);
	vector<Texture>* pics = new vector<Texture>;
	vector<string> files = FileSys::listDir(drc, FTYPE_REG, World::sets()->showHidden);

	for (sizet i = 0; i < files.size(); i++) {
		if (!thread->getRun()) {
			clearTexVec(*pics);
			delete pics;
			return 1;
		}
		while (SDL_TryLockMutex(World::drawSys()->rendLock));
		if (SDL_Texture* tex = IMG_LoadTexture(World::drawSys()->renderer, childPath(drc, files[i]).c_str()))
			pics->emplace_back(files[i], tex);

		SDL_UnlockMutex(World::drawSys()->rendLock);
		World::winSys()->pushEvent(UserCode::readerProgress, new vec2t(i, files.size()));
	}
	World::winSys()->pushEvent(UserCode::readerFinished, pics);
	return 0;
}

int DrawSys::loadTexturesArchiveThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	string& arc = *static_cast<string*>(thread->data);
	archive* arch = FileSys::openArchive(arc);
	if (!arch)
		return 1;

	sizet i = 0, numEntries = FileSys::archiveEntryCount(arc);
	vector<Texture>* pics = new vector<Texture>;
	for (archive_entry* entry; !archive_read_next_header(arch, &entry); i++) {
		if (!thread->getRun()) {
			clearTexVec(*pics);
			delete pics;
			archive_read_free(arch);
			return 1;
		}
		while (SDL_TryLockMutex(World::drawSys()->rendLock));
		if (SDL_Texture* tex = World::drawSys()->loadArchiveTexture(arch, entry))
			pics->emplace_back(archive_entry_pathname(entry), tex);
		SDL_UnlockMutex(World::drawSys()->rendLock);
		World::winSys()->pushEvent(UserCode::readerProgress, new vec2t(i, numEntries));
	}
	archive_read_free(arch);
	std::sort(pics->begin(), pics->end(), [](const Texture& a, const Texture& b) -> bool { return strnatless(a.name, b.name); });
	World::winSys()->pushEvent(UserCode::readerFinished, pics);
	return 0;
}

SDL_Texture* DrawSys::loadArchiveTexture(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	uint8* buffer = new uint8[sizet(bsiz)];
	int64 size = archive_read_data(arch, buffer, sizet(bsiz));
	SDL_Texture* tex = size > 0 ? IMG_LoadTexture_RW(renderer, SDL_RWFromMem(buffer, int(size)), SDL_TRUE) : nullptr;
	delete[] buffer;
	return tex;
}
