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
		logError(TTF_GetError());
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
		logError("failed to load font:", linend, TTF_GetError());
#endif
	return font;
}

int FontSet::length(const char* text, int height) {
	int len = 0;
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	return len;
}

int FontSet::length(const char* text, sizet length, int height) {
	int len = 0;
	char tmp = text[length];
	const_cast<char*>(text)[length] = '\0';
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	const_cast<char*>(text)[length] = tmp;
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

DrawSys::DrawSys(const umap<int, SDL_Window*>& windows, Settings* sets, const FileSys* fileSys, int iconSize) :
	colors(fileSys->loadColors(sets->setTheme(sets->getTheme(), fileSys->getAvailableThemes())))
{
	switch (sets->renderer) {
#ifdef WITH_DIRECTX
	case Settings::Renderer::directx:
		try {
			renderer = new RendererDx(windows, sets, viewRes, colors[uint8(Color::background)]);	// TODO: exceptions here rely on the destructor being called
			break;
		} catch (const std::runtime_error& err) {
			logError(err.what());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err.what(), !windows.empty() ? windows.begin()->second : nullptr);
		}
#endif
	default:
		renderer = new RendererGl(windows, sets, viewRes, colors[uint8(Color::background)]);
	}

	blank = texes.emplace(string(), renderer->texFromColor(u8vec4(255))).first->second;
	for (const fs::directory_entry& it : fs::directory_iterator(fileSys->dirIcons(), fs::directory_options::skip_permission_denied)) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		if (SDL_RWops* ifh = SDL_RWFromFile(it.path().u8string().c_str(), "rb")) {
			if (Texture* atex = renderer->texFromIcon(IMG_LoadSizedSVG_RW(ifh, iconSize, iconSize)); atex)
				texes.emplace(it.path().stem().u8string(), atex);
			else {
				SDL_RWseek(ifh, 0, RW_SEEK_SET);
				if (Texture* btex = renderer->texFromIcon(IMG_Load_RW(ifh, SDL_FALSE)); btex)
					texes.emplace(it.path().stem().u8string(), btex);
				else
					logError("failed to load texture ", it.path().filename(), linend, IMG_GetError());
			}
			SDL_RWclose(ifh);
		} else
			logError("failed to open texture ", it.path().filename(), linend, SDL_GetError());
#else
		if (Texture* tex = renderer->texFromIcon(IMG_Load(it.path().u8string().c_str())); tex)
			texes.emplace(it.path().stem().u8string(), tex);
		else
			logError("failed to load texture ", it.path().filename(), linend, IMG_GetError());
#endif
	}
	setFont(sets->font, sets, fileSys);
}

DrawSys::~DrawSys() {
	for (auto& [name, tex] : texes)
		tex->free();
}

int DrawSys::findPointInView(ivec2 pos) const {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(pos);
	return vit != renderer->getViews().end() ? vit->first : Renderer::singleDspId;
}

void DrawSys::setTheme(string_view name, Settings* sets, const FileSys* fileSys) {
	colors = fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
	renderer->setClearColor(colors[uint8(Color::background)]);
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

const Texture* DrawSys::texture(const string& name) const {
	try {
		return texes.at(name);
	} catch (const std::out_of_range&) {
		logError("texture ", name, " doesn't exist");
	}
	return texes.at(string());
}

vector<pair<string, Texture*>> DrawSys::transferPictures(vector<pair<string, SDL_Surface*>>& pics) {
	vector<pair<string, Texture*>> texs(pics.size());
	size_t j = 0;
	for (size_t i = 0; i < pics.size(); ++i) {
		if (Texture* tex = renderer->texFromIcon(pics[i].second))
			texs[j++] = pair(std::move(pics[i].first), tex);
		else
			logError(SDL_GetError());
	}
	texs.resize(j);
	return texs;
}

void DrawSys::drawWidgets(Scene* scene, bool mouseLast) {
	for (auto [id, view] : renderer->getViews()) {
		renderer->startDraw(view);

		// draw main widgets and visible overlays
		scene->getLayout()->drawSelf(view->rect);
		if (scene->getOverlay() && scene->getOverlay()->on)
			scene->getOverlay()->drawSelf(view->rect);

		// draw popup if exists and dim main widgets
		if (scene->getPopup()) {
			renderer->drawRect(blank, view->rect, view->rect, colorPopupDim);
			scene->getPopup()->drawSelf(view->rect);
		}

		// draw context menu
		if (scene->getContext())
			scene->getContext()->drawSelf(view->rect);

		// draw extra stuff on top
		if (scene->getCapture())
			scene->getCapture()->drawTop(view->rect);
		else if (Button* but = dynamic_cast<Button*>(scene->select); mouseLast && but)
			drawTooltip(but, view->rect);

		renderer->finishDraw(view);
	}
}

bool DrawSys::drawPicture(const Picture* wgt, const Rect& view) {
	if (Rect rect = wgt->rect(); rect.overlap(view)) {
		Rect frame = wgt->frame();
		if (wgt->showBG)
			renderer->drawRect(blank, rect, frame, colors[uint8(wgt->color())]);
		if (wgt->tex)
			renderer->drawRect(wgt->tex, wgt->texRect(), frame, colors[uint8(Color::texture)]);
		return true;
	}
	return false;
}

void DrawSys::drawCheckBox(const CheckBox* wgt, const Rect& view) {
	if (drawPicture(wgt, view))																		// draw background
		renderer->drawRect(blank, wgt->boxRect(), wgt->frame(), colors[uint8(wgt->boxColor())]);	// draw checkbox
}

void DrawSys::drawSlider(const Slider* wgt, const Rect& view) {
	if (drawPicture(wgt, view)) {															// draw background
		Rect frame = wgt->frame();
		renderer->drawRect(blank, wgt->barRect(), frame, colors[uint8(Color::dark)]);		// draw bar
		renderer->drawRect(blank, wgt->sliderRect(), frame, colors[uint8(Color::light)]);	// draw slider
	}
}

void DrawSys::drawProgressBar(const ProgressBar* wgt, const Rect& view) {
	if (Rect rect = wgt->rect(); rect.overlap(view)) {
		Rect frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[uint8(Color::normal)]);			// draw background
		renderer->drawRect(blank, wgt->barRect(), frame, colors[uint8(Color::light)]);	// draw bar
	}
}

void DrawSys::drawLabel(const Label* wgt, const Rect& view) {
	if (drawPicture(wgt, view) && wgt->getTextTex())
		renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[uint8(Color::text)]);
}

void DrawSys::drawCaret(const Rect& rect, const Rect& frame, const Rect& view) {
	if (rect.overlap(view))
		renderer->drawRect(blank, rect, frame, colors[uint8(Color::light)]);
}

void DrawSys::drawWindowArranger(const WindowArranger* wgt, const Rect& view) {
	if (Rect rect = wgt->rect(); rect.overlap(view)) {
		Rect frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[uint8(wgt->color())]);
		for (const auto& [id, dsp] : wgt->getDisps())
			if (!wgt->draggingDisp(id)) {
				auto [rct, color, text, tex] = wgt->dispRect(id, dsp);
				drawWaDisp(rct, color, text, tex, frame, view);
			}
	}
}

void DrawSys::drawWaDisp(const Rect& rect, Color color, const Rect& text, const Texture* tex, const Rect& frame, const Rect& view) {
	if (rect.overlap(view)) {
		renderer->drawRect(blank, rect, frame, colors[uint8(color)]);
		if (tex)
			renderer->drawRect(tex, text, frame, colors[uint8(Color::text)]);
	}
}

void DrawSys::drawScrollArea(const ScrollArea* box, const Rect& view) {
	mvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Rect bar = box->barRect(); bar.overlap(view)) {
		Rect frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[uint8(Color::dark)]);					// draw scroll bar
		renderer->drawRect(blank, box->sliderRect(), frame, colors[uint8(Color::light)]);	// draw scroll slider
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Rect& view) {
	mvec2 vis = box->visibleWidgets();
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Rect bar = box->barRect(); box->showBar() && bar.overlap(view)) {
		Rect frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[uint8(Color::dark)]);
		renderer->drawRect(blank, box->sliderRect(), frame, colors[uint8(Color::light)]);
	}
}

void DrawSys::drawPopup(const Popup* box, const Rect& view) {
	if (Rect rect = box->rect(); rect.overlap(view)) {
		renderer->drawRect(blank, rect, box->frame(), colors[uint8(box->bgColor)]);	// draw background
		for (const Widget* it : box->getWidgets())									// draw children
			it->drawSelf(view);
	}
}

void DrawSys::drawTooltip(Button* but, const Rect& view) {
	const Texture* tip = but->getTooltip();
	if (Rect rct = but->tooltipRect(); tip && rct.overlap(view)) {
		renderer->drawRect(blank, rct, view, colors[uint8(Color::tooltip)]);
		renderer->drawRect(tip, Rect(rct.pos() + Button::tooltipMargin, tip->getRes()), rct, colors[uint8(Color::text)]);
	}
}

Widget* DrawSys::getSelectedWidget(const Layout* box, ivec2 mPos) {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(mPos);
	if (vit == renderer->getViews().end())
		return nullptr;

	renderer->startSelDraw(vit->second, mPos);
	box->drawAddr(vit->second->rect);
	return renderer->finishSelDraw(vit->second);
}

void DrawSys::drawPictureAddr(const Picture* wgt, const Rect& view) {
	if (Rect rect = wgt->rect(); rect.overlap(view))
		renderer->drawSelRect(wgt, rect, wgt->frame());
}

void DrawSys::drawLayoutAddr(const Layout* wgt, const Rect& view) {
	renderer->drawSelRect(wgt, wgt->rect(), wgt->frame());	// invisible background to set selection area
	for (const Widget* it : wgt->getWidgets())
		it->drawAddr(view);
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
