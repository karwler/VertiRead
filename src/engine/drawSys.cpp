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
		logError("Failed to load font:", linend, TTF_GetError());
#endif
	return font;
}

int FontSet::length(const char* text, int height) {
	int len = 0;
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	return len;
}

int FontSet::length(char* text, size_t length, int height) {
	int len = 0;
	char tmp = text[length];
	text[length] = '\0';
	TTF_SizeUTF8(getFont(height), text, &len, nullptr);
	text[length] = tmp;
	return len;
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
	if (blank = renderer->texFromIcon(white); !blank)
		throw std::runtime_error("Failed to create blank texture");
	texes.emplace(string(), blank);

	for (const fs::directory_entry& it : fs::directory_iterator(fileSys->dirIcons(), fs::directory_options::skip_permission_denied))
		if (Texture* tex = renderer->texFromIcon(loadIcon(it.path().u8string(), iconSize)))
			texes.emplace(it.path().stem().u8string(), tex);
	setFont(sets->font, sets, fileSys);
	renderer->synchTransfer();
}

DrawSys::~DrawSys() {
	if (renderer)
		for (auto& [name, tex] : texes)
			renderer->freeTexture(tex);
	delete renderer;
}

SDL_Surface* DrawSys::loadIcon(const string& path, int size) {
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (SDL_RWops* ifh = SDL_RWFromFile(path.c_str(), "rb")) {
		if (SDL_Surface* img = IMG_LoadSizedSVG_RW(ifh, 0, size)) {
			SDL_RWclose(ifh);
			return img;
		}
		if (SDL_RWseek(ifh, 0, RW_SEEK_SET) == 0) {
			if (SDL_Surface* img = IMG_Load_RW(ifh, SDL_TRUE))
				return img;
		} else
			SDL_RWclose(ifh);
	}
	return nullptr;
#else
	return IMG_Load(path.c_str());
#endif
}

int DrawSys::findPointInView(ivec2 pos) const {
	umap<int, Renderer::View*>::const_iterator vit = findViewForPoint(pos);
	return vit != renderer->getViews().end() ? vit->first : Renderer::singleDspId;
}

void DrawSys::setTheme(string_view name, Settings* sets, const FileSys* fileSys) {
	colors = fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
	renderer->setClearColor(colors[uint8(Color::background)]);
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

pair<Texture*, bool> DrawSys::texture(const string& name) const {
	try {
		return pair(texes.at(name), false);
	} catch (const std::out_of_range&) {
		logError("Texture ", name, " doesn't exist");
	}
	return pair(blank, false);
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
		if (wgt->getTex())
			renderer->drawRect(wgt->getTex(), wgt->texRect(), frame, colors[uint8(Color::texture)]);
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
	for (size_t i = vis.x; i < vis.y; ++i)
		box->getWidget(i)->drawSelf(view);

	if (Recti bar = box->barRect(); bar.overlaps(view)) {
		Recti frame = box->frame();
		renderer->drawRect(blank, bar, frame, colors[uint8(Color::dark)]);					// draw scroll bar
		renderer->drawRect(blank, box->sliderRect(), frame, colors[uint8(Color::light)]);	// draw scroll slider
	}
}

void DrawSys::drawReaderBox(const ReaderBox* box, const Recti& view) {
	mvec2 vis = box->visibleWidgets();
	for (size_t i = vis.x; i < vis.y; ++i)
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
