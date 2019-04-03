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
	infiLock(rendLock);

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

int DrawSys::loadTexturesDirectoryThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	vector<string> files = FileSys::listDir(World::browser()->getCurDir(), FTYPE_REG, World::sets()->showHidden);
	uptrt lim, mem;	// picture count limit, picture size limit
	bool fwd = initLoadLimits(files, thread, lim, mem);
	sizet sizMag = memSizeMag(mem);
	string limStr = limitToStr(lim, lim, mem, sizMag);
	vector<Texture>* pics = new vector<Texture>;

	// iterate over files utils one of the limits is hit (it should be the one associated with the setting)
	for (uptrt mov = btom<uptrt>(fwd), i = fwd ? 0 : files.size() - 1, c = 0, m = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (!thread->getRun()) {
			clearTexVec(*pics);
			delete pics;
			return 1;
		}
		World::winSys()->pushEvent(UserCode::readerProgress, new string(limitToStr(fwd ? i : files.size() - i - 1, c, m, sizMag)), new string(limStr));

		infiLock(World::drawSys()->rendLock);
		SDL_Texture* tex = IMG_LoadTexture(World::drawSys()->renderer, childPath(World::browser()->getCurDir(), files[i]).c_str());
		SDL_UnlockMutex(World::drawSys()->rendLock);
		if (tex) {
			pics->emplace_back(files[i], tex);
			m += texMemory(tex);
			c++;
		}
	}
	if (!fwd)
		std::reverse(pics->begin(), pics->end());

	World::winSys()->pushEvent(UserCode::readerFinished, pics);
	return 0;
}

int DrawSys::loadTexturesArchiveThreaded(void* data) {
	Thread* thread = static_cast<Thread*>(data);
	uptrt start, end, lim, mem;	// start must be less than end (end does not get iterated over, unlike start)
	mapFiles files = initLoadLimits(thread, start, end, lim, mem);
	sizet sizMag = memSizeMag(mem);
	string limStr = limitToStr(lim, lim, mem, sizMag);
	archive* arch = FileSys::openArchive(World::browser()->getCurDir());
	if (!arch)
		return -1;

	uptrt c = 0, m = 0;
	vector<Texture>* pics = new vector<Texture>;
	for (archive_entry* entry; !archive_read_next_header(arch, &entry) && c < lim && m < mem;) {
		if (!thread->getRun()) {
			clearTexVec(*pics);
			delete pics;
			archive_read_free(arch);
			return 1;
		}
		World::winSys()->pushEvent(UserCode::readerProgress, new string(limitToStr(c, c, m, sizMag)), new string(limStr));

		const char* ename = archive_entry_pathname(entry);
		if (pair<sizet, uptrt>& ent = files[ename]; ent.first >= start && ent.first < end)
			if (SDL_Texture* tex = World::drawSys()->loadArchiveTexture(arch, entry)) {
				pics->emplace_back(ename, tex);
				m += ent.second;
				c++;
			}
	}
	archive_read_free(arch);
	std::sort(pics->begin(), pics->end(), [&files](const Texture& a, const Texture& b) -> bool { return files[a.name].first < files[b.name].first; });

	World::winSys()->pushEvent(UserCode::readerFinished, pics);
	return 0;
}

SDL_Texture* DrawSys::loadArchiveTexture(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	uint8* buffer = new uint8[sizet(bsiz)];
	int64 size = archive_read_data(arch, buffer, sizet(bsiz));
	infiLock(rendLock);
	SDL_Texture* tex = size > 0 ? IMG_LoadTexture_RW(renderer, SDL_RWFromMem(buffer, int(size)), SDL_TRUE) : nullptr;
	SDL_UnlockMutex(rendLock);
	delete[] buffer;
	return tex;
}

bool DrawSys::initLoadLimits(vector<string>& files, Thread* thread, uptrt& lim, uptrt& mem) {
	auto [first, fwd] = thread->pop<std::tuple<string, bool>>();
	if (World::sets()->picLim.type != PicLim::Type::none)
		if (vector<string>::iterator it = std::find(files.begin(), files.end(), first); it != files.end())
			fwd ? files.erase(files.begin(), it) : files.erase(it + 1, files.end());

	switch (World::sets()->picLim.type) {
	case PicLim::Type::none:
		mem = UINT64_MAX;
		lim = files.size();
		break;
	case PicLim::Type::count:
		mem = UINT64_MAX;
		lim = World::sets()->picLim.getCount() <= files.size() ? World::sets()->picLim.getCount() : files.size();
		break;
	case PicLim::Type::size:
		mem = World::sets()->picLim.getSize();
		lim = files.size();
	}
	return fwd;
}

mapFiles DrawSys::initLoadLimits(Thread* thread, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem) {
	vector<string> names;
	mapFiles files = FileSys::listArchivePictures(World::browser()->getCurDir(), names);
	auto [first, fwd] = thread->pop<std::tuple<string, bool>>();
	start = 0;
	if (World::sets()->picLim.type != PicLim::Type::none)
		if (mapFiles::iterator it = files.find(first); it != files.end())
			start = it->second.first;

	switch (World::sets()->picLim.type) {
	case PicLim::Type::none:
		mem = UINT64_MAX;
		lim = files.size();
		end = lim;
		break;
	case PicLim::Type::count:
		mem = UINT64_MAX;
		if (fwd) {
			lim = World::sets()->picLim.getCount() + start <= files.size() ? World::sets()->picLim.getCount() : files.size() - start;
			end = start + lim;
		} else {
			lim = World::sets()->picLim.getCount() <= start + 1 ? World::sets()->picLim.getCount() : start + 1;
			end = start + 1;
			if (start -= lim - 1; start > end)
				start = 0;
		}
		break;
	case PicLim::Type::size:
		mem = World::sets()->picLim.getSize();
		lim = files.size();
		if (fwd) {
			end = start;
			for (uptrt m = 0; end < lim && m < mem; m += files[names[end]].second, end++);
		} else {
			end = start + 1;
			for (uptrt m = 0; start > 0 && m < mem; m += files[names[start]].second, start--);
		}
	}
	return files;
}

string DrawSys::limitToStr(uptrt i, uptrt c, uptrt m, sizet mag) {
	switch (World::sets()->picLim.type) {
	case PicLim::Type::none:
		return to_string(i);
	case PicLim::Type::count:
		return to_string(c);
	case PicLim::Type::size:
		return memoryString(m, mag);
	}
	return emptyStr;
}
