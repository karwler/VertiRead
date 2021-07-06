#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "utils/layouts.h"
#include <archive.h>
#include <archive_entry.h>

// FONT SET

void FontSet::init(const fs::path& path) {
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	TTF_CloseFont(font);
#else
	clear();
	file = path;
#endif
	TTF_Font* tmp = TTF_OpenFont(path.u8string().c_str(), fontTestHeight);
	if (!tmp)
		throw std::runtime_error(TTF_GetError());

	// get approximate height scale factor
	int size;
	heightScale = !TTF_SizeUTF8(tmp, fontTestString, nullptr, &size) ? float(fontTestHeight) / float(size) : 1.f;
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	font = tmp;
#else
	TTF_CloseFont(tmp);
#endif
}

#if !SDL_TTF_VERSION_ATLEAST(2, 0, 18)
void FontSet::clear() {
	for (auto [size, font] : fonts)
		TTF_CloseFont(font);
	fonts.clear();
}
#endif

TTF_Font* FontSet::getFont(int height) {
	height = int(float(height) * heightScale);
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	if (TTF_SetFontSize(font, height)) {
		std::cerr << TTF_GetError() << std::endl;
		return nullptr;
	}
#else
	try {
		return fonts.at(height);
	} catch (const std::out_of_range&) {}

	TTF_Font* font = TTF_OpenFont(file.u8string().c_str(), size);
	if (font)
		fonts.emplace(size, font);
	else
		std::cerr << "failed to load font:" << linend << TTF_GetError() << std::endl;
#endif
	return font;
}

int FontSet::length(const char* text, int height) {
	int len = 0;
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	return len;
}

int FontSet::length(char* text, sizet length, int height) {
	int len = 0;
	char tmp = text[length];
	text[length] = '\0';
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	text[length] = tmp;
	return len;
}

// PICTURE LOADER

PictureLoader::PictureLoader(fs::path cdrc, string pfirst, const PicLim& plim, bool forward, bool hidden) :
	curDir(std::move(cdrc)),
	firstPic(std::move(pfirst)),
	picLim(plim),
	fwd(forward),
	showHidden(hidden)
{}

PictureLoader::~PictureLoader() {
	for (pair<string, SDL_Surface*>& it : pics)
		SDL_FreeSurface(it.second);
}

string PictureLoader::limitToStr(uptrt i, uptrt c, uptrt m, sizet mag) const {
	switch (picLim.type) {
	case PicLim::Type::none:
		return toStr(i);
	case PicLim::Type::count:
		return toStr(c);
	case PicLim::Type::size:
		return PicLim::memoryString(m, mag);
	}
	throw std::runtime_error("Invalid picture limit type: " + toStr(picLim.type));
}

// DRAW SYS

DrawSys::DrawSys(SDL_Window* window, pair<int, uint32> info, Settings* sets, const FileSys* fileSys) {
	// create and set up renderer
	if (renderer = SDL_CreateRenderer(window, info.first, info.second); !renderer)
		throw std::runtime_error("Failed to create renderer:\n"s + SDL_GetError());
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// load default textures with colors and initialize fonts
	for (const fs::directory_entry& it : fs::directory_iterator(fileSys->dirIcons(), fs::directory_options::skip_permission_denied)) {
		if (SDL_Texture* tex = IMG_LoadTexture(renderer, it.path().u8string().c_str()))
			texes.emplace(it.path().stem().u8string(), tex);
		else
			std::cerr << "failed to load texture " << it.path().filename() << '\n' << IMG_GetError() << std::endl;
	}
	setTheme(sets->getTheme(), sets, fileSys);
	setFont(sets->font, sets, fileSys);
}

DrawSys::~DrawSys() {
	for (auto& [name, tex] : texes)
		SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
}

void DrawSys::setTheme(string_view name, Settings* sets, const FileSys* fileSys) {
	colors = fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
	SDL_Color clr = colors[uint8(Color::texture)];

	for (auto& [ts, tex] : texes) {
		SDL_SetTextureColorMod(tex, clr.r, clr.g, clr.b);
		SDL_SetTextureAlphaMod(tex, clr.a);
	}
}

void DrawSys::setFont(string_view font, Settings* sets, const FileSys* fileSys) {
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

vector<Texture> DrawSys::transferPictures(vector<pair<string, SDL_Surface*>>& pics) {
	vector<Texture> texs(pics.size());
	size_t j = 0;
	for (size_t i = 0; i < pics.size(); ++i) {
		if (SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, pics[i].second))
			texs[j++] = Texture(std::move(pics[i].first), tex);
		else
			std::cerr << SDL_GetError() << std::endl;
	}
	texs.resize(j);
	return texs;
}

void DrawSys::drawWidgets(Scene* scene, bool mouseLast) {
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

	// draw context menu
	if (scene->getContext())
		scene->getContext()->drawSelf();

	// draw caret if capturing LineEdit
	if (LabelEdit* let = dynamic_cast<LabelEdit*>(scene->getCapture()))
		drawRect(let->caretRect(), Color::light);
	if (Button* but = dynamic_cast<Button*>(scene->select); mouseLast && but && but->getTooltip())
		drawTooltip(but);

	SDL_RenderPresent(renderer);
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
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf();

	drawRect(box->barRect(), Color::dark);		// draw scroll bar
	drawRect(box->sliderRect(), Color::light);	// draw scroll slider
}

void DrawSys::drawReaderBox(const ReaderBox* box) {
	mvec2 vis = box->visibleWidgets();
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf();

	if (box->showBar()) {
		drawRect(box->barRect(), Color::dark);
		drawRect(box->sliderRect(), Color::light);
	}
}

void DrawSys::drawPopup(const Popup* box) {
	drawRect(box->rect(), box->bgColor);	// draw background
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

void DrawSys::loadTexturesDirectoryThreaded(bool* running, uptr<PictureLoader> pl) {
	vector<fs::path> files = FileSys::listDir(pl->curDir, true, false, pl->showHidden);
	uptrt lim, mem;	// picture count limit, picture size limit
	sizet sizMag = initLoadLimits(pl.get(), files, lim, mem);
	pl->progLim = pl->limitToStr(lim, lim, mem, sizMag);

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	for (uptrt mov = btom<uptrt>(pl->fwd), i = pl->fwd ? 0 : files.size() - 1, c = 0, m = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (!*running)
			return;
		pl->progVal = pl->limitToStr(pl->fwd ? i : files.size() - i - 1, c, m, sizMag);
		pushEvent(UserCode::readerProgress, pl.get());

		SDL_Surface* img = IMG_Load((pl->curDir / files[i]).u8string().c_str());
		if (img) {
			pl->pics.emplace_back(files[i].u8string(), img);
			m += uptrt(img->w) * uptrt(img->h) * img->format->BytesPerPixel;
			++c;
		}
	}
	if (!pl->fwd)
		std::reverse(pl->pics.begin(), pl->pics.end());

	pushEvent(UserCode::readerFinished, pl.release());
	*running = false;
}

void DrawSys::loadTexturesArchiveThreaded(bool* running, uptr<PictureLoader> pl) {
	archive* arch = FileSys::openArchive(pl->curDir);
	if (!arch)
		return;
	uptrt start, end, lim, mem;	// start must be less than end (end does not get iterated over, unlike start)
	mapFiles files = initLoadLimits(pl.get(), start, end, lim, mem);
	sizet sizMag = PicLim::memSizeMag(mem);
	uptrt c = 0, m = 0;
	pl->progLim = pl->limitToStr(lim, lim, mem, sizMag);

	for (archive_entry* entry; !archive_read_next_header(arch, &entry) && c < lim && m < mem;) {
		if (!*running) {
			archive_read_free(arch);
			return;
		}
		pl->progVal = pl->limitToStr(c, c, m, sizMag);
		pushEvent(UserCode::readerProgress, pl.get());

		string pname = archive_entry_pathname_utf8(entry);
		if (pair<sizet, uptrt>& ent = files[pname]; ent.first >= start && ent.first < end)
			if (SDL_Surface* img = FileSys::loadArchivePicture(arch, entry)) {
				pl->pics.emplace_back(std::move(pname), img);
				m += ent.second;
				++c;
			}
	}
	archive_read_free(arch);
	std::sort(pl->pics.begin(), pl->pics.end(), [&files](const pair<string, SDL_Surface*>& a, const pair<string, SDL_Surface*>& b) -> bool { return files[a.first].first < files[b.first].first; });

	pushEvent(UserCode::readerFinished, pl.release());
	*running = false;
}

sizet DrawSys::initLoadLimits(const PictureLoader* pl, vector<fs::path>& files, uptrt& lim, uptrt& mem) {
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
		break;
	default:
		throw std::runtime_error("Invalid picture limit type: " + toStr(pl->picLim.type));
	}
	return PicLim::memSizeMag(mem);
}

mapFiles DrawSys::initLoadLimits(const PictureLoader* pl, uptrt& start, uptrt& end, uptrt& lim, uptrt& mem) {
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
			for (uptrt m = 0; end < lim && m < mem; m += files[names[end]].second, ++end);
		} else {
			end = start + 1;
			for (uptrt m = 0; start > 0 && m < mem; m += files[names[start]].second, --start);
		}
		break;
	default:
		throw std::runtime_error("Invalid picture limit type: " + toStr(pl->picLim.type));
	}
	return files;
}
