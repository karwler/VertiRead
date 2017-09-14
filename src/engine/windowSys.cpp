#include "world.h"
#include "filer.h"

WindowSys::WindowSys(const VideoSettings& SETS) :
	window(nullptr),
	redraw(false),
	sets(SETS)
{
	setShowMouse(true);
}

WindowSys::~WindowSys() {
	destroyWindow();
}

void WindowSys::createWindow() {
	destroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	uint32 flags = Default::windowFlags;
	if (sets.maximized)
		flags |= SDL_WINDOW_MAXIMIZED;
	if (sets.fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow(Default::titleDefault, Default::windowPos.x, Default::windowPos.y, sets.resolution.x, sets.resolution.y, flags);
	if (!window)
		throw Exception("couldn't create window\n" + string(SDL_GetError()), 3);

	// set icon
	SDL_Surface* icon = IMG_Load(string(Filer::dirExec + Default::fileIcon).c_str());
	if (icon) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}

	// set up renderer
	drawSys.createRenderer(window, sets.getRenderDriverIndex());
	setRedrawNeeded();
}

void WindowSys::destroyWindow() {
	drawSys.destroyRenderer();
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

void WindowSys::drawWidgets(const vector<Widget*>& widgets, const Popup* popup) {
	if (redraw) {
		redraw = false;
		drawSys.drawWidgets(widgets, popup);
	}
}

bool WindowSys::getShowMouse() const {
	return showMouse;
}

void WindowSys::setShowMouse(bool on) {
	showMouse = on;
	SDL_ShowCursor(showMouse ? SDL_ENABLE : SDL_DISABLE);
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	if (winEvent.event == SDL_WINDOWEVENT_EXPOSED)
		setRedrawNeeded();
	else if (winEvent.event == SDL_WINDOWEVENT_RESIZED) {
		// update settings if needed
		uint32 flags = SDL_GetWindowFlags(window);
		if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			sets.maximized = flags & SDL_WINDOW_MAXIMIZED;
			if (!sets.maximized)
				sets.resolution = resolution();
		}
	} else if (winEvent.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		World::scene()->resizeMenu();
	else if (winEvent.event == SDL_WINDOWEVENT_LEAVE)
		World::scene()->onMouseLeave();
	else if (winEvent.event == SDL_WINDOWEVENT_CLOSE)
		World::base()->close();
}

void WindowSys::setRedrawNeeded() {
	redraw = true;
}

vec2i WindowSys::displayResolution() {
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);
	return vec2i(mode.w, mode.h);
}

vec2i WindowSys::resolution() const {
	vec2i res;
	SDL_GetWindowSize(window, &res.x, &res.y);
	return res;
}

vec2i WindowSys::position() const {
	vec2i pos;
	SDL_GetWindowPosition(window, &pos.x, &pos.y);
	return pos;
}

DrawSys* WindowSys::getDrawSys() {
	return &drawSys;
}

const VideoSettings& WindowSys::getSettings() const {
	return sets;
}

void WindowSys::setRenderer(const string& name) {
	sets.renderer = name;
	createWindow();
}

void WindowSys::setFullscreen(bool on) {
	sets.fullscreen = on;
	createWindow();
}

void WindowSys::setFont(const string& font) {
	sets.setFont(font);
	World::library()->getFonts().init(sets.getFontpath());

	setRedrawNeeded();
}

void WindowSys::setTheme(const string& theme) {
	sets.setDefaultTheme();
	sets.theme = theme;
	Filer::getColors(sets.colors, sets.theme);

	setRedrawNeeded();
}
