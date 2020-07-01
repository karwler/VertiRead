#include "drawSys.h"
#include "scene.h"

// FONT SET

FontSet::~FontSet() {
	for (const auto& [size, font] : fonts)
		TTF_CloseFont(font);
}

void FontSet::init(const fs::path& path) {
	clear();
	file = path;
	TTF_Font* tmp = TTF_OpenFont(file.u8string().c_str(), fontTestHeight);
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
	TTF_Font* font = TTF_OpenFont(file.u8string().c_str(), size);
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

int FontSet::length(const char* text, int height) {
	int len = 0;
	if (TTF_Font* font = getFont(height))
		TTF_SizeUTF8(font, text, &len, nullptr);
	return len;
}

// PICTURE LOADER

PictureLoader::PictureLoader(DrawSys* drawer, fs::path cdrc, string pfirst, const PicLim& plim, bool forward, bool hidden) :
	drawSys(drawer),
	curDir(std::move(cdrc)),
	firstPic(std::move(pfirst)),
	picLim(plim),
	fwd(forward),
	showHidden(hidden)
{}

string PictureLoader::limitToStr(uptrt i, uptrt c, uptrt m, sizet mag) const {
	switch (picLim.type) {
	case PicLim::Type::none:
		return to_string(i);
	case PicLim::Type::count:
		return to_string(c);
	case PicLim::Type::size:
		return memoryString(m, mag);
	}
	return string();
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, int driverIndex, Settings* sets, const FileSys* fileSys) :
	rendLock(SDL_CreateMutex())
{
	// create and set up renderer
	if (!(renderer = SDL_CreateRenderer(window, driverIndex, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED)))
		throw std::runtime_error(string("Failed to create renderer:\n") + SDL_GetError());
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// load default textures with colors and initialize fonts
	for (const fs::path& file : FileSys::listDir(FileSys::dirTexs, true, false)) {
		if (fs::path path = FileSys::dirTexs / file; SDL_Texture* tex = IMG_LoadTexture(renderer, path.u8string().c_str()))
			texes.emplace(file.stem().u8string(), tex);
		else
			std::cerr << "failed to load texture " << file << '\n' << IMG_GetError() << std::endl;
	}
	setTheme(sets->getTheme(), sets, fileSys);
	setFont(sets->font, sets, fileSys);
}

DrawSys::~DrawSys() {
	SDL_DestroyMutex(rendLock);
	for (const auto& [name, tex] : texes)
		SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
}

void DrawSys::setTheme(const string& name, Settings* sets, const FileSys* fileSys) {
	colors = fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
	SDL_Color clr = colors[uint8(Color::texture)];

	for (const auto& [ts, tex] : texes) {
		SDL_SetTextureColorMod(tex, clr.r, clr.g, clr.b);
		SDL_SetTextureAlphaMod(tex, clr.a);
	}
}

void DrawSys::setFont(const string& font, Settings* sets, const FileSys* fileSys) {
	fs::path path = fileSys->findFont(font);
	if (FileSys::isFont(path))
		sets->font = font;
	else {
		sets->font = Settings::defaultFont;
		path = fileSys->findFont(Settings::defaultFont);
	}
	fonts.init(path);
}

SDL_Texture* DrawSys::texture(const string& name) const {
	try {
		return texes.at(name);
	} catch (const std::out_of_range&) {
		std::cerr << "texture " << name << " doesn't exist" << std::endl;
	}
	return nullptr;
}

void DrawSys::drawWidgets(Scene* scene, bool mouseLast) {
	SDL_LockMutex(rendLock);

	// clear screen
	SDL_Color bgcolor = colors[uint8(Color::background)];
	SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	SDL_RenderClear(renderer);

	// draw main widgets and visible overlays
	scene->getLayout()->drawSelf();
	if (scene->getOverlay() && scene->getOverlay()->on)
		scene->getOverlay()->drawSelf();

	// draw popup if exists and dim main widgets
	if (scene->getPopup()) {
		Rect view = viewport();
		SDL_SetRenderDrawColor(renderer, colorPopupDim.r, colorPopupDim.g, colorPopupDim.b, colorPopupDim.a);
		SDL_RenderFillRect(renderer, &view);

		scene->getPopup()->drawSelf();
	}

	// draw caret if capturing LineEdit
	if (LabelEdit* let = dynamic_cast<LabelEdit*>(scene->capture))
		drawRect(let->caretRect(), Color::light);
	if (Button* but = dynamic_cast<Button*>(scene->select); mouseLast && but && but->getTooltip())
		drawTooltip(but);

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
	drawPicture(wgt);											// draw background
	drawRect(wgt->barRect().intersect(frame), Color::dark);		// draw bar
	drawRect(wgt->sliderRect().intersect(frame), Color::light);	// draw slider
}

void DrawSys::drawProgressBar(const ProgressBar* wgt) {
	drawRect(wgt->rect(), Color::normal);							// draw background
	drawRect(wgt->barRect().intersect(wgt->frame()), Color::light);	// draw bar
}

void DrawSys::drawLabel(const Label* wgt) {
	drawPicture(wgt);
	if (wgt->textTex)
		drawText(wgt->textTex, wgt->textRect(), wgt->textFrame());
}

void DrawSys::drawScrollArea(const ScrollArea* box) {
	mvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; i++)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(const ReaderBox* box) {
	mvec2 vis = box->visibleWidgets();
	for (sizet i = vis.x; i < vis.y; i++)
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

void DrawSys::drawTooltip(Button* but) {
	ivec2 res;
	Rect rct = but->tooltipRect(res);
	drawRect(rct, Color::tooltip);

	rct = Rect(rct.pos() + Button::tooltipMargin, res);
	SDL_RenderCopy(renderer, but->getTooltip(), nullptr, &rct);
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
	ivec2 res = texSize(tex);
	vec2 factor(vec2(res) / vec2(rect.size()));
	Rect src(vec2(crop.pos()) * factor, res - ivec2(vec2(crop.size()) * factor));
	SDL_RenderCopy(renderer, tex, &src, &dst);
}

SDL_Texture* DrawSys::renderText(const char* text, int height) {
	if (SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts.getFont(height), text, colors[uint8(Color::text)])) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_FreeSurface(surf);
		return tex;
	}
	return nullptr;
}

SDL_Texture* DrawSys::renderText(const char* text, int height, uint length) {
	if (SDL_Surface* surf = TTF_RenderUTF8_Blended_Wrapped(fonts.getFont(height), text, colors[uint8(Color::text)], length)) {
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_FreeSurface(surf);
		return tex;
	}
	return nullptr;
}

int DrawSys::loadTexturesDirectoryThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	PictureLoader* pl = static_cast<PictureLoader*>(thread->data);
	vector<fs::path> files = FileSys::listDir(pl->curDir, true, false, pl->showHidden);
	uptrt lim, mem;	// picture count limit, picture size limit
	sizet sizMag = initLoadLimits(pl, files, lim, mem);
	pl->progLim = pl->limitToStr(lim, lim, mem, sizMag);

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	for (uptrt mov = btom<uptrt>(pl->fwd), i = pl->fwd ? 0 : files.size() - 1, c = 0, m = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (!thread->getRun()) {
			Texture::clearVec(pl->pics);
			delete pl;
			return 1;
		}
		pl->progVal = pl->limitToStr(pl->fwd ? i : files.size() - i - 1, c, m, sizMag);
		pushEvent(UserCode::readerProgress, pl);

		SDL_LockMutex(pl->drawSys->rendLock);
		SDL_Texture* tex = IMG_LoadTexture(pl->drawSys->renderer, (pl->curDir / files[i]).u8string().c_str());
		SDL_UnlockMutex(pl->drawSys->rendLock);
		if (tex) {
			pl->pics.emplace_back(files[i].u8string(), tex);
			m += texMemory(tex);
			c++;
		}
	}
	if (!pl->fwd)
		std::reverse(pl->pics.begin(), pl->pics.end());

	pushEvent(UserCode::readerFinished, pl);
	return 0;
}

int DrawSys::loadTexturesArchiveThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	PictureLoader* pl = static_cast<PictureLoader*>(thread->data);
	archive* arch = FileSys::openArchive(pl->curDir);
	if (!arch) {
		delete pl;
		return -1;
	}

	uptrt start, end, lim, mem;	// start must be less than end (end does not get iterated over, unlike start)
	mapFiles files = initLoadLimits(pl, start, end, lim, mem);
	sizet sizMag = memSizeMag(mem);
	uptrt c = 0, m = 0;
	pl->progLim = pl->limitToStr(lim, lim, mem, sizMag);

	for (archive_entry* entry; !archive_read_next_header(arch, &entry) && c < lim && m < mem;) {
		if (!thread->getRun()) {
			Texture::clearVec(pl->pics);
			delete pl;
			archive_read_free(arch);
			return 1;
		}
		pl->progVal = pl->limitToStr(c, c, m, sizMag);
		pushEvent(UserCode::readerProgress, pl);

		string pname = archive_entry_pathname_utf8(entry);
		if (pair<sizet, uptrt>& ent = files[pname]; ent.first >= start && ent.first < end)
			if (SDL_Texture* tex = pl->drawSys->loadArchiveTexture(arch, entry)) {
				pl->pics.emplace_back(std::move(pname), tex);
				m += ent.second;
				c++;
			}
	}
	archive_read_free(arch);
	std::sort(pl->pics.begin(), pl->pics.end(), [&files](const Texture& a, const Texture& b) -> bool { return files[a.name].first < files[b.name].first; });

	pushEvent(UserCode::readerFinished, pl);
	return 0;
}

SDL_Texture* DrawSys::loadArchiveTexture(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	uint8* buffer = new uint8[sizet(bsiz)];
	int64 size = archive_read_data(arch, buffer, sizet(bsiz));
	SDL_LockMutex(rendLock);
	SDL_Texture* tex = size > 0 ? IMG_LoadTexture_RW(renderer, SDL_RWFromMem(buffer, int(size)), SDL_TRUE) : nullptr;
	SDL_UnlockMutex(rendLock);
	delete[] buffer;
	return tex;
}

sizet DrawSys::initLoadLimits(PictureLoader* pl, vector<fs::path>& files, uptrt& lim, uptrt& mem) {
	if (pl->picLim.type != PicLim::Type::none)
		if (vector<fs::path>::iterator it = std::find(files.begin(), files.end(),fs::u8path(pl->firstPic)); it != files.end())
			pl->fwd ? files.erase(files.begin(), it) : files.erase(it + 1, files.end());

	switch (pl->picLim.type) {
	case PicLim::Type::none:
		mem = UINTPTR_MAX;
		lim = files.size();
		break;
	case PicLim::Type::count:
		mem = UINTPTR_MAX;
		lim = pl->picLim.getCount() <= files.size() ? pl->picLim.getCount() : files.size();
		break;
	case PicLim::Type::size:
		mem = pl->picLim.getSize();
		lim = files.size();
	}
	return memSizeMag(mem);
}

mapFiles DrawSys::initLoadLimits(PictureLoader* pl, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem) {
	vector<string> names;
	mapFiles files = FileSys::listArchivePictures(pl->curDir, names);
	start = 0;
	if (pl->picLim.type != PicLim::Type::none)
		if (mapFiles::iterator it = files.find(pl->firstPic); it != files.end())
			start = it->second.first;

	switch (pl->picLim.type) {
	case PicLim::Type::none:
		mem = UINTPTR_MAX;
		lim = files.size();
		end = lim;
		break;
	case PicLim::Type::count:
		mem = UINTPTR_MAX;
		if (pl->fwd) {
			lim = pl->picLim.getCount() + start <= files.size() ? pl->picLim.getCount() : files.size() - start;
			end = start + lim;
		} else {
			lim = pl->picLim.getCount() <= start + 1 ? pl->picLim.getCount() : start + 1;
			end = start + 1;
			if (start -= lim - 1; start > end)
				start = 0;
		}
		break;
	case PicLim::Type::size:
		mem = pl->picLim.getSize();
		lim = files.size();
		if (pl->fwd) {
			end = start;
			for (uptrt m = 0; end < lim && m < mem; m += files[names[end]].second, end++);
		} else {
			end = start + 1;
			for (uptrt m = 0; start > 0 && m < mem; m += files[names[start]].second, start--);
		}
	}
	return files;
}

uptrt DrawSys::texMemory(SDL_Texture* tex) {
	uint32 format;
	int width, height;
	return !SDL_QueryTexture(tex, &format, nullptr, &width,&height) ? uptrt(width) * uptrt(height) * SDL_BYTESPERPIXEL(format) : 0;
}
