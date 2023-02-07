#include "rendererDx.h"
#include "rendererGl.h"
#include "rendererVk.h"
#include "drawSys.h"
#include "fileSys.h"
#include "scene.h"
#include "utils/layouts.h"
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
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
	for (auto [id, img] : pics)
		SDL_FreeSurface(img);
}

vector<pair<sizet, SDL_Surface*>> PictureLoader::extractPics() {
	vector<pair<sizet, SDL_Surface*>> out = std::move(pics);
	pics.clear();
	return out;
}

string PictureLoader::limitToStr(uptrt c, uptrt m, sizet mag) const {
	switch (picLim.type) {
	case PicLim::Type::none: case PicLim::Type::count:
		return toStr(c);
	case PicLim::Type::size:
		return PicLim::memoryString(m, mag);
	}
	throw std::runtime_error("Invalid picture limit type: " + toStr(picLim.type));
}

char* PictureLoader::progressText(string_view val, string_view lim) {
	char* text = new char[val.length() + lim.length() + 2];
	std::copy(val.begin(), val.end(), text);
	text[val.length()] = '/';
	std::copy(lim.begin(), lim.end(), text + val.length() + 1);
	text[val.length() + 1 + lim.length()] = '\0';
	return text;
}

// DRAW SYS

DrawSys::DrawSys(const umap<int, SDL_Window*>& windows, Settings* sets, const FileSys* fileSys, int iconSize) :
	colors(fileSys->loadColors(sets->setTheme(sets->getTheme(), fileSys->getAvailableThemes())))
{
	ivec2 origin(INT_MAX);
	for (auto [id, rct] : sets->displays)
		origin = glm::min(origin, rct.pos());

	switch (sets->renderer) {
#ifdef WITH_DIRECTX
	case Settings::Renderer::directx:
		renderer = new RendererDx(windows, sets, viewRes, origin, colors[uint8(Color::background)]);
		break;
#endif
#ifdef WITH_OPENGL
	case Settings::Renderer::opengl:
		renderer = new RendererGl(windows, sets, viewRes, origin, colors[uint8(Color::background)]);
		break;
#endif
#ifdef WITH_VULKAN
	case Settings::Renderer::vulkan:
		renderer = new RendererVk(windows, sets, viewRes, origin, colors[uint8(Color::background)]);
#endif
	}

	SDL_Surface* white = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
	if (!white)
		throw std::runtime_error("Failed to create blank texture: "s + SDL_GetError());
	SDL_FillRect(white, nullptr, 0xFFFFFFFF);
	Texture* tex = renderer->texFromImg(white);
	if (!tex)
		throw std::runtime_error("Failed to create blank texture");
	texes.emplace(string(), tex);
	blank = tex;

	for (const fs::directory_entry& it : fs::directory_iterator(fileSys->dirIcons(), fs::directory_options::skip_permission_denied)) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		if (SDL_RWops* ifh = SDL_RWFromFile(it.path().u8string().c_str(), "rb")) {
			if (SDL_Surface* aimg = IMG_LoadSizedSVG_RW(ifh, iconSize, iconSize))
				texes.emplace(it.path().stem().u8string(), renderer->texFromImg(aimg));
			else if (SDL_RWseek(ifh, 0, RW_SEEK_SET); SDL_Surface* bimg = IMG_Load_RW(ifh, SDL_FALSE))
				texes.emplace(it.path().stem().u8string(), renderer->texFromImg(bimg));
			SDL_RWclose(ifh);
		}
#else
		if (SDL_Surface* img = IMG_Load(it.path().u8string().c_str()))
			texes.emplace(it.path().stem().u8string(), renderer->texFromImg(img));
#endif
	}
	setFont(sets->font, sets, fileSys);
}

DrawSys::~DrawSys() {
	if (renderer)
		for (auto& [name, tex] : texes)
			renderer->freeTexture(tex);
	delete renderer;
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

vector<pair<string, Texture*>> DrawSys::transferPictures(PictureLoader* pl) {
	vector<pair<sizet, SDL_Surface*>> refs = pl->extractPics();
	std::sort(refs.begin(), refs.end(), [](const pair<sizet, SDL_Surface*>& a, const pair<sizet, SDL_Surface*>& b) -> bool { return a.first < b.first; });

	vector<pair<string, Texture*>> ptxv(refs.size());
	for (sizet i = 0; i < ptxv.size(); ++i)
		ptxv[i] = pair(std::move(pl->names[refs[i].first]), renderer->texFromImg(refs[i].second));
	return ptxv;
}

void DrawSys::drawWidgets(Scene* scene, bool mouseLast) {
	for (auto [id, view] : renderer->getViews()) {
		try {
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
		} catch (const Renderer::ErrorSkip&) {}
	}
	renderer->finishRender();
}

bool DrawSys::drawPicture(const Picture* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		if (wgt->showBG)
			renderer->drawRect(blank, rect, frame, colors[uint8(wgt->color())]);
		if (wgt->tex)
			renderer->drawRect(wgt->tex, wgt->texRect(), frame, colors[uint8(Color::texture)]);
		return true;
	}
	return false;
}

void DrawSys::drawCheckBox(const CheckBox* wgt, const Recti& view) {
	if (drawPicture(wgt, view))																		// draw background
		renderer->drawRect(blank, wgt->boxRect(), wgt->frame(), colors[uint8(wgt->boxColor())]);	// draw checkbox
}

void DrawSys::drawSlider(const Slider* wgt, const Recti& view) {
	if (drawPicture(wgt, view)) {															// draw background
		Recti frame = wgt->frame();
		renderer->drawRect(blank, wgt->barRect(), frame, colors[uint8(Color::dark)]);		// draw bar
		renderer->drawRect(blank, wgt->sliderRect(), frame, colors[uint8(Color::light)]);	// draw slider
	}
}

void DrawSys::drawProgressBar(const ProgressBar* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[uint8(Color::normal)]);			// draw background
		renderer->drawRect(blank, wgt->barRect(), frame, colors[uint8(Color::light)]);	// draw bar
	}
}

void DrawSys::drawLabel(const Label* wgt, const Recti& view) {
	if (drawPicture(wgt, view) && wgt->getTextTex())
		renderer->drawRect(wgt->getTextTex(), wgt->textRect(), wgt->textFrame(), colors[uint8(Color::text)]);
}

void DrawSys::drawCaret(const Recti& rect, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view))
		renderer->drawRect(blank, rect, frame, colors[uint8(Color::light)]);
}

void DrawSys::drawWindowArranger(const WindowArranger* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view)) {
		Recti frame = wgt->frame();
		renderer->drawRect(blank, rect, frame, colors[uint8(wgt->color())]);
		for (const auto& [id, dsp] : wgt->getDisps())
			if (!wgt->draggingDisp(id)) {
				auto [rct, color, text, tex] = wgt->dispRect(id, dsp);
				drawWaDisp(rct, color, text, tex, frame, view);
			}
	}
}

void DrawSys::drawWaDisp(const Recti& rect, Color color, const Recti& text, const Texture* tex, const Recti& frame, const Recti& view) {
	if (rect.overlaps(view)) {
		renderer->drawRect(blank, rect, frame, colors[uint8(color)]);
		if (tex)
			renderer->drawRect(tex, text, frame, colors[uint8(Color::text)]);
	}
}

void DrawSys::drawScrollArea(const ScrollArea* box, const Recti& view) {
	mvec2 vis = box->visibleWidgets();	// get index interval of items on screen and draw children
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[uint8(Color::dark)]);					// draw scroll bar
		renderer->drawRect(blank, box->sliderRect(), frame, colors[uint8(Color::light)]);	// draw scroll slider
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) {
	mvec2 vis = box->visibleWidgets();
	for (sizet i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); box->showBar() && bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[uint8(Color::dark)]);
		renderer->drawRect(blank, box->sliderRect(), frame, colors[uint8(Color::light)]);
	}
}

void DrawSys::drawPopup(const Popup* box, const Recti& view) {
	if (Recti rect = box->rect(); rect.overlaps(view)) {
		renderer->drawRect(blank, rect, box->frame(), colors[uint8(box->bgColor)]);	// draw background
		for (Widget* it : box->getWidgets())										// draw children
			it->drawSelf(view);
	}
}

void DrawSys::drawTooltip(Button* but, const Recti& view) {
	const Texture* tip = but->getTooltip();
	if (Recti rct = but->tooltipRect(); tip && rct.overlaps(view)) {
		renderer->drawRect(blank, rct, view, colors[uint8(Color::tooltip)]);
		renderer->drawRect(tip, Recti(rct.pos() + Button::tooltipMargin, tip->getRes()), rct, colors[uint8(Color::text)]);
	}
}

Widget* DrawSys::getSelectedWidget(Layout* box, ivec2 mPos) {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(mPos);
	if (vit == renderer->getViews().end())
		return nullptr;

	renderer->startSelDraw(vit->second, mPos - vit->second->rect.pos());
	box->drawAddr(vit->second->rect);
	return renderer->finishSelDraw(vit->second);
}

void DrawSys::drawPictureAddr(const Picture* wgt, const Recti& view) {
	if (Recti rect = wgt->rect(); rect.overlaps(view))
		renderer->drawSelRect(wgt, rect, wgt->frame());
}

void DrawSys::drawLayoutAddr(const Layout* wgt, const Recti& view) {
	renderer->drawSelRect(wgt, wgt->rect(), wgt->frame());	// invisible background to set selection area
	for (Widget* it : wgt->getWidgets())
		it->drawAddr(view);
}

void DrawSys::loadTexturesDirectoryThreaded(std::atomic_bool& running, uptr<PictureLoader> pl) {
	vector<fs::path> files = FileSys::listDir(pl->curDir, true, false, pl->showHidden);
	auto [lim, mem, sizMag] = initLoadLimits(pl.get(), files);	// picture count limit, picture size limit, magnitude index
	string progLim = pl->limitToStr(lim, mem, sizMag);
	pl->names.resize(files.size());
	std::transform(files.begin(), files.end(), pl->names.begin(), [](const fs::path& it) -> string { return it.u8string(); });

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	uptrt m = 0;
	for (sizet mov = btom<uptrt>(pl->fwd), i = pl->fwd ? 0 : files.size() - 1, c = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (!running)
			return;
		pushEvent(SDL_USEREVENT_READER_PROGRESS, PictureLoader::progressText(pl->limitToStr(c, m, sizMag), progLim));

		if (SDL_Surface* img = IMG_Load((pl->curDir / files[i]).u8string().c_str())) {
			pl->pics.emplace_back(i, img);
			m += uptrt(img->w) * uptrt(img->h) * uptrt(img->format->BytesPerPixel);
			++c;
		}
	}
	pushEvent(SDL_USEREVENT_READER_FINISHED, pl.release());
	running = false;
}

void DrawSys::loadTexturesArchiveThreaded(std::atomic_bool& running, uptr<PictureLoader> pl) {
	mapFiles files = FileSys::listArchivePictures(pl->curDir, pl->names);
	auto [start, end, lim, mem, sizMag] = initLoadLimits(pl.get(), files);
	string progLim = pl->limitToStr(lim, mem, sizMag);
	archive* arch = FileSys::openArchive(pl->curDir);
	if (!arch)
		return;

	sizet c = 0;
	uptrt m = 0;
	for (archive_entry* entry; !archive_read_next_header(arch, &entry) && c < lim && m < mem;) {
		if (!running) {
			archive_read_free(arch);
			return;
		}
		pushEvent(SDL_USEREVENT_READER_PROGRESS, PictureLoader::progressText(pl->limitToStr(c, m, sizMag), progLim));

		string pname = archive_entry_pathname_utf8(entry);
		if (pair<sizet, uptrt>& ent = files[pname]; ent.first >= start && ent.first < end)
			if (SDL_Surface* img = FileSys::loadArchivePicture(arch, entry)) {
				pl->pics.emplace_back(ent.first, img);
				m += ent.second;
				++c;
			}
	}
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_READER_FINISHED, pl.release());
	running = false;
}

tuple<sizet, uptrt, uint8> DrawSys::initLoadLimits(const PictureLoader* pl, vector<fs::path>& files) {
	if (pl->picLim.type != PicLim::Type::none)
		if (vector<fs::path>::iterator it = std::find(files.begin(), files.end(), fs::u8path(pl->firstPic)); it != files.end())
			pl->fwd ? files.erase(files.begin(), it) : files.erase(it + 1, files.end());

	switch (pl->picLim.type) {
	case PicLim::Type::none:
		return tuple(files.size(), UINTPTR_MAX, 0);
	case PicLim::Type::count:
		return tuple(std::min(pl->picLim.getCount(), files.size()), UINTPTR_MAX, 0);
	case PicLim::Type::size:
		return tuple(files.size(), pl->picLim.getSize(), PicLim::memSizeMag(pl->picLim.getSize()));
	}
	throw std::runtime_error("Invalid picture limit type: " + toStr(pl->picLim.type));
}

tuple<sizet, sizet, sizet, uptrt, uint8> DrawSys::initLoadLimits(PictureLoader* pl, const mapFiles& files) {
	sizet start = 0;
	if (pl->picLim.type != PicLim::Type::none)
		if (mapFiles::const_iterator it = files.find(pl->firstPic); it != files.end())
			start = it->second.first;

	switch (pl->picLim.type) {
	case PicLim::Type::none:
		return tuple(start, files.size(), files.size(), UINTPTR_MAX, 0);
	case PicLim::Type::count: {
		sizet lim, end;
		if (pl->fwd) {
			lim = pl->picLim.getCount() + start <= files.size() ? pl->picLim.getCount() : files.size() - start;
			end = start + lim;
		} else {
			lim = pl->picLim.getCount() <= start + 1 ? pl->picLim.getCount() : start + 1;
			end = start + 1;
			if (start -= lim - 1; start > end)
				start = 0;
		}
		return tuple(start, end, lim, UINTPTR_MAX, 0); }
	case PicLim::Type::size: {
		sizet end;
		if (pl->fwd) {
			end = start;
			for (uptrt m = 0; end < files.size() && m < pl->picLim.getSize(); m += files.at(pl->names[end]).second, ++end);
		} else {
			end = start + 1;
			for (uptrt m = 0; start > 0 && m < pl->picLim.getSize(); m += files.at(pl->names[start]).second, --start);
		}
		return tuple(start, end, files.size(), pl->picLim.getSize(), PicLim::memSizeMag(pl->picLim.getSize()));
	} }
	throw std::runtime_error("Invalid picture limit type: " + toStr(pl->picLim.type));
}
