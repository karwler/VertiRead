#include "world.h"

WindowSys::WindowSys(const VideoSettings& SETS) :
	window(nullptr),
	redraw(false),
	sets(SETS)
{
	ShowMouse(true);
}

WindowSys::~WindowSys() {
	DestroyWindow();	// scene cleans up popup window
}

void WindowSys::CreateWindow() {
	CreateWindow(SDL_WINDOWPOS_UNDEFINED, sets.resolution);
}

void WindowSys::CreateWindow(const vec2i& pos, const vec2i& res) {
	DestroyWindow();	// make sure old window (if exists) is destroyed

	// create new window
	uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
	if (sets.maximized)
		flags |= SDL_WINDOW_MAXIMIZED;
	if (sets.fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow("VertiRead", pos.x, pos.y, res.x, res.y, flags);
	if (!window)
		throw Exception("couldn't create window\n" + string(SDL_GetError()), 3);

	// set icon
	SDL_Surface* icon = IMG_Load(string(Filer::dirExec + "icon.ico").c_str());
	if (icon) {
		SDL_SetWindowIcon(window, icon);
		SDL_FreeSurface(icon);
	}

	shaderSys.CreateRenderer(window, sets.GetRenderDriverIndex());
	SetRedrawNeeded();
}

void WindowSys::DestroyWindow() {
	shaderSys.DestroyRenderer();
	if (window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
}

void WindowSys::DrawObjects(const vector<Object*>& objects, const Popup* popup) {
	if (redraw) {
		redraw = false;
		shaderSys.DrawObjects(objects, popup);
	}
}

bool WindowSys::ShowMouse() const {
	return showMouse;
}

void WindowSys::ShowMouse(bool on) {
	showMouse = on;
	SDL_ShowCursor(showMouse ? SDL_ENABLE : SDL_DISABLE);
}

void WindowSys::MoveMouse(const vec2i& mPos) {
	SDL_WarpMouseInWindow(window, mPos.x, mPos.y);
}

void WindowSys::WindowEvent(const SDL_WindowEvent& winEvent) {
	if (winEvent.event == SDL_WINDOWEVENT_EXPOSED)
		SetRedrawNeeded();
	else if (winEvent.event == SDL_WINDOWEVENT_RESIZED) {
		// update settings if needed
		uint32 flags = SDL_GetWindowFlags(window);
		if (!(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			sets.maximized = flags & SDL_WINDOW_MAXIMIZED;
			if (!sets.maximized)
				sets.resolution = Resolution();
		}
	} else if (winEvent.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		World::scene()->ResizeMenu();
}

void WindowSys::SetRedrawNeeded() {
	redraw = true;
}

vec2i WindowSys::DesktopResolution() {
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);
	return vec2i(mode.w, mode.h);
}

vec2i WindowSys::Resolution() const {
	vec2i res;
	SDL_GetWindowSize(window, &res.x, &res.y);
	return res;
}

vec2i WindowSys::Position() const {
	vec2i pos;
	SDL_GetWindowPosition(window, &pos.x, &pos.y);
	return pos;
}

const VideoSettings& WindowSys::Settings() const {
	return sets;
}

void WindowSys::Renderer(const string& name) {
	sets.renderer = name;
	CreateWindow(Position(), Resolution());
}

void WindowSys::Fullscreen(bool on) {
	sets.fullscreen = on;
	CreateWindow(Position(), Resolution());
}

void WindowSys::Font(const string& font) {
	sets.SetFont(font);
	World::library()->LoadFont(sets.Fontpath());

	SetRedrawNeeded();
}

void WindowSys::Theme(const string& theme) {
	sets.SetDefaultTheme();
	sets.theme = theme;
	Filer::GetColors(sets.colors, sets.theme);

	SetRedrawNeeded();
}
