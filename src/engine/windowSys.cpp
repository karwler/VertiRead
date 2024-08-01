#include "windowSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "optional/d3d.h"
#include "prog/program.h"
#include "prog/progs.h"
#include <SDL_image.h>
#include <SDL_vulkan.h>

void WindowSys::init() {
	try {
		fileSys = new FileSys();
		sets = fileSys->loadSettings();
		createWindow();
		inputSys = new InputSys;
		scene = new Scene;
		program = new Program;
		program->start();
	} catch (...) {
		cleanup();
		throw;
	}
}

void WindowSys::cleanup() noexcept {
	delete program;
	delete scene;
	delete inputSys;
	if (sets)
		destroyWindows();
	delete fileSys;
}

void WindowSys::exec() {
	for (tick_t oldTime = SDL_GetTicks(); run;) {
		tick_t newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / ticksPerSec;
		oldTime = newTime;

		drawSys->drawWidgets(inputSys->mouseWin.has_value());
		inputSys->tick();
		scene->tick(dSec);
		program->tick();

		SDL_Event event;
		uint32 timeout = SDL_GetTicks() + eventCheckTimeout;
		do {
			if (!SDL_PollEvent(&event))
				break;
			handleEvent(event);
		} while (!SDL_TICKS_PASSED(SDL_GetTicks(), timeout));
	}
	fileSys->saveSettings(sets.get());
	fileSys->saveBindings(inputSys->getBindings());
}

void WindowSys::createWindow() {
	if (sets->screen == Settings::Screen::multiFullscreen && sets->displays.empty())
		sets->screen = Settings::Screen::fullscreen;
	SDL_WindowFlags flags = SDL_WINDOW_ALLOW_HIGHDPI;
	switch (sets->screen) {
	using enum Settings::Screen;
	case windowed:
		flags |= SDL_WINDOW_RESIZABLE;
		break;
	case fullscreen:
		flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
	case multiFullscreen:
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SKIP_TASKBAR;
	}
	SDL_Surface* icon = IMG_Load((fromPath(fileSys->dirIcons()) / DrawSys::iconName(DrawSys::Tex::vertiread)).data());

	stvector<Settings::Renderer, Settings::rendererNames.size()> renderers;
	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		renderers = {
			direct3d11,
#ifdef WITH_OPENGL
			opengl3, opengl1,
#endif
#ifdef WITH_VULKAN
			vulkan,
#endif
			software
		};
		break;
#endif
#ifdef WITH_OPENGL
	case opengl1:
		renderers = {
			opengl1, opengl3,
#ifndef _WIN32
			opengles3,
#endif
#ifdef WITH_DIRECT3D
			direct3d11,
#endif
#ifdef WITH_VULKAN
			vulkan,
#endif
			software
		};
		break;
	case opengl3:
		renderers = {
			opengl3, opengl1,
#ifndef _WIN32
			opengles3,
#endif
#ifdef WITH_DIRECT3D
			direct3d11,
#endif
#ifdef WITH_VULKAN
			vulkan,
#endif
			software
		};
		break;
	case opengles3:
		renderers = {
#ifndef _WIN32
			opengles3,
#endif
			opengl3, opengl1,
#ifdef WITH_DIRECT3D
			direct3d11,
#endif
#ifdef WITH_VULKAN
			vulkan,
#endif
			software
		};
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderers = {
			vulkan,
#ifdef WITH_OPENGL
			opengl3, opengl1,
#ifndef _WIN32
			opengles3,
#endif
#endif
#ifdef WITH_DIRECT3D
			direct3d11,
#endif
			software
		};
		break;
#endif
	case software:
		renderers = {
			software,
#ifdef WITH_OPENGL
			opengl1, opengl3,
#endif
#ifdef WITH_DIRECT3D
			direct3d11,
#endif
#ifdef WITH_VULKAN
			vulkan
#endif
		};
	}
	for (Settings::Renderer rnd : renderers) {
		try {
			sets->renderer = rnd;
			if (sets->screen != Settings::Screen::multiFullscreen)
				createSingleWindow(flags | initWindow(false), icon);
			else
				createMultiWindow(flags | initWindow(true), icon);
			break;
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err.what(), nullptr);
			destroyWindows();
		}
	}
	SDL_FreeSurface(icon);
	if (windows.empty())
		throw std::runtime_error("Failed to initialize a working renderer");
}

uint32 WindowSys::initWindow(bool shared) {
	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		if (!symD3d11())
			throw std::runtime_error("Failed to load D3D11 libraries");
		break;
#endif
#ifdef WITH_OPENGL
	case opengl1: case opengl3: case opengles3: {
		bool core = sets->renderer <= opengl3;
		int flags = 0;
#ifndef NDEBUG
		flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
		SDL_GL_ResetAttributes();
		if (sets->renderer == opengl1) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		} else {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			if (core)
				flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
		}
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, core ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, flags);
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, shared);
		if (sdlFailed(SDL_GL_LoadLibrary(nullptr)))
			throw std::runtime_error(SDL_GetError());
		return SDL_WINDOW_OPENGL; }
#endif
#ifdef WITH_VULKAN
	case vulkan:
		if (sdlFailed(SDL_Vulkan_LoadLibrary(nullptr)))
			throw std::runtime_error(SDL_GetError());
		return SDL_WINDOW_VULKAN;
#endif
	}
	return 0;
}

void WindowSys::createSingleWindow(uint32 flags, SDL_Surface* icon) {
	sets->resolution = glm::clamp(sets->resolution, windowMinSize, displayResolution());
	windows.resize(1);
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (windows[0] = SDL_CreateWindow(title, sets->resolution.x, sets->resolution.y, flags); !windows[0])
#else
	if (windows[0] = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, flags); !windows[0])
#endif
		throw std::runtime_error(SDL_GetError());
	if (sets->screen == Settings::Screen::windowed && sets->maximized)
		SDL_MaximizeWindow(windows[0]);	// workaround for SDL_WINDOW_MAXIMIZED causing the window to not report the correct size	// SDL3 TODO: does this still happen?
	SDL_SetWindowIcon(windows[0], icon);
	SDL_SetWindowMinimumSize(windows[0], windowMinSize.x, windowMinSize.y);
	drawSys = new DrawSys(windows);
}

void WindowSys::createMultiWindow(uint32 flags, SDL_Surface* icon) {
	windows.resize(sets->displays.size());
	uptr<ivec2[]> vofs = std::make_unique_for_overwrite<ivec2[]>(windows.size() + 1);
	vofs[windows.size()] = ivec2(INT_MAX);
	for (size_t i = 0; i < windows.size(); ++i) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
		string name = std::format("{} {}", title, i);
		SDL_PropertiesID props = SDL_CreateProperties();
		if (!props)
			throw std::runtime_error(SDL_GetError());
		SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, name.data());
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, sets->displays[i].rect.w);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, sets->displays[i].rect.h);
		SDL_SetNumberProperty(props, "flags", flags);
		windows[i] = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);
#else
		windows[i] = SDL_CreateWindow(std::format("{} {}", title, i).data(), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did), sets->displays[i].rect.w, sets->displays[i].rect.h, flags);
#endif
		if (!windows[i])
			throw std::runtime_error(SDL_GetError());
		SDL_SetWindowIcon(windows[i], icon);
		vofs[i] = sets->displays[i].rect.pos();
		vofs[windows.size()] = glm::min(vofs[windows.size()], vofs[i]);
	}
	drawSys = new DrawSys(windows, vofs.get());
}

void WindowSys::destroyWindows() noexcept {
	delete drawSys;
	drawSys = nullptr;
	for (SDL_Window* it : windows)
		SDL_DestroyWindow(it);
	windows.clear();

	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		closeD3d11();
		break;
#endif
#ifdef WITH_OPENGL
	case opengl1: case opengl3: case opengles3:
		SDL_GL_UnloadLibrary();
		break;
#endif
#ifdef WITH_VULKAN
	case vulkan:
		SDL_Vulkan_UnloadLibrary();
#endif
	}
}

void WindowSys::recreateWindows() {
	scene->clearLayouts();
	destroyWindows();
	createWindow();
	scene->setLayouts();
}

void WindowSys::handleEvent(const SDL_Event& event) {
	switch (event.type) {
	case SDL_QUIT:
		program->eventExit();
		break;
#if !SDL_VERSION_ATLEAST(3, 0, 0)
	case SDL_DISPLAYEVENT:
		eventDisplay();
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
#endif
	case SDL_KEYDOWN:
		inputSys->eventKeypress(event.key);
		break;
	case SDL_TEXTEDITING:
		scene->onCompose(event.edit.text);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
#if SDL_VERSION_ATLEAST(2, 0, 22) && !SDL_VERSION_ATLEAST(3, 0, 0)
	case SDL_TEXTEDITING_EXT:
		scene->onCompose(event.editExt.text);
		SDL_free(event.editExt.text);
		break;
#endif
	case SDL_MOUSEMOTION:
		inputSys->eventMouseMotion(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
		inputSys->eventMouseButtonDown(event.button);
		break;
	case SDL_MOUSEBUTTONUP:
		inputSys->eventMouseButtonUp(event.button);
		break;
	case SDL_MOUSEWHEEL:
		inputSys->eventMouseWheel(event.wheel);
		break;
	case SDL_JOYAXISMOTION:
		inputSys->eventJoystickAxis(event.jaxis);
		break;
	case SDL_JOYHATMOTION:
		inputSys->eventJoystickHat(event.jhat);
		break;
	case SDL_JOYBUTTONDOWN:
		inputSys->eventJoystickButton(event.jbutton);
		break;
	case SDL_JOYDEVICEADDED:
		inputSys->addJoystick(event.jdevice.which);
		break;
	case SDL_JOYDEVICEREMOVED:
		inputSys->delJoystick(event.jdevice.which);
		break;
	case SDL_CONTROLLERAXISMOTION:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		inputSys->eventGamepadAxis(event.gaxis);
#else
		inputSys->eventGamepadAxis(event.caxis);
#endif
		break;
	case SDL_CONTROLLERBUTTONDOWN:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		inputSys->eventGamepadButton(event.gbutton);
#else
		inputSys->eventGamepadButton(event.cbutton);
#endif
		break;
	case SDL_CONTROLLERDEVICEADDED:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		inputSys->addGamepad(event.gdevice.which);
#else
		inputSys->addGamepad(event.cdevice.which);
#endif
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		inputSys->delGamepad(event.gdevice.which);
#else
		inputSys->delGamepad(event.cdevice.which);
#endif
		break;
	case SDL_FINGERDOWN:
		inputSys->eventFingerDown(event.tfinger);
		break;
	case SDL_FINGERUP:
		inputSys->eventFingerUp(event.tfinger);
		break;
	case SDL_FINGERMOTION:
		inputSys->eventFingerMove(event.tfinger);
		break;
	case SDL_DROPFILE:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		program->getState()->eventFileDrop(event.drop.data);
#else
		program->getState()->eventFileDrop(event.drop.file);
		SDL_free(event.drop.file);
#endif
		break;
	case SDL_DROPTEXT:
#if SDL_VERSION_ATLEAST(3, 0, 0)
		scene->onText(event.drop.data);
#else
		scene->onText(event.drop.file);
		SDL_free(event.drop.file);
#endif
		break;
	case SDL_USEREVENT_GENERAL:
		program->handleGeneralEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_BOOKS:
		program->handleProgBooksEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_FILE_EXPLORER:
		program->handleProgFileExplorerEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_PAGE_BROWSER:
		program->handleProgPageBrowserEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_READER:
		program->handleProgReaderEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_SETTINGS:
		program->handleProgSettingsEvent(event.user);
		break;
	case SDL_USEREVENT_PROG_SEARCH_DIR:
		program->handleProgSearchDirEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_LIST_FINISHED:
		program->eventListFinished(event.user);
		break;
	case SDL_USEREVENT_THREAD_DELETE_FINISHED:
		program->eventDeleteFinished(event.user);
		break;
	case SDL_USEREVENT_THREAD_ARCHIVE_FINISHED:
		program->eventArchiveFinished(event.user);
		break;
	case SDL_USEREVENT_THREAD_PREVIEW:
		program->handleThreadPreviewEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_READER:
		program->handleThreadReaderEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_GO_NEXT_FINISHED:
		program->eventGoNextFinished(event.user);
		break;
	case SDL_USEREVENT_THREAD_MOVE:
		program->handleThreadMoveEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_FONTS_FINISHED:
		program->eventFontsFinished(event.user);
		break;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	default:
		if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
			eventWindow(event.window);
		else if (event.type >= SDL_EVENT_DISPLAY_FIRST && event.type <= SDL_EVENT_DISPLAY_LAST)
			eventDisplay();
#endif
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
	switch (winEvent.type) {
#else
	switch (winEvent.event) {
#endif
	case SDL_WINDOWEVENT_RESIZED:
		if (windows.size() == 1)
			if (uint32 flags = SDL_GetWindowFlags(windows[0]); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
				if (sets->maximized = flags & SDL_WINDOW_MAXIMIZED; !sets->maximized)
					sets->resolution = ivec2(winEvent.data1, winEvent.data2);
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		drawSys->updateView();
		scene->onResize();
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		if (sets->screen == Settings::Screen::multiFullscreen && rng::any_of(windows, [](SDL_Window* it) -> bool { return SDL_GetWindowFlags(it) & SDL_WINDOW_MINIMIZED; })) {
			for (SDL_Window* it : windows)
				SDL_MaximizeWindow(it);
#if SDL_VERSION_ATLEAST(3, 0, 0)
			SDL_FlushEvents(SDL_EVENT_WINDOW_FOCUS_GAINED, SDL_EVENT_WINDOW_FOCUS_LOST);
#else
			SDL_FlushEvent(SDL_WINDOWEVENT);
#endif
		}
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (sets->screen == Settings::Screen::multiFullscreen && rng::none_of(windows, [](SDL_Window* it) -> bool { return SDL_GetWindowFlags(it) & SDL_WINDOW_INPUT_FOCUS; })) {
			for (SDL_Window* it : windows)
				SDL_MinimizeWindow(it);
#if SDL_VERSION_ATLEAST(3, 0, 0)
			SDL_FlushEvents(SDL_EVENT_WINDOW_FOCUS_GAINED, SDL_EVENT_WINDOW_FOCUS_LOST);
#else
			SDL_FlushEvent(SDL_WINDOWEVENT);
#endif
		}
		break;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED:
#else
	case SDL_WINDOWEVENT_MOVED:
#endif
		if (windows.size() == 1 && drawSys->updateDpi())
			scene->onResize();
	}
}

void WindowSys::eventDisplay() {
	sets->unionDisplays();
	if (windows.size() == 1)
		drawSys->updateDpi();
	else
		recreateWindows();
	scene->onDisplayChange();
}

ivec2 WindowSys::winViewOffset(uint32 wid) const noexcept {
	if (SDL_Window* win = SDL_GetWindowFromID(wid))
		if (Renderer::View* view = drawSys->getRenderer()->findView(win))
			return view->rect.pos();
	return ivec2(INT_MIN);
}

ivec2 WindowSys::mousePos() const noexcept {
	mpvec2 mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	if (SDL_Window* win = SDL_GetMouseFocus())
		if (Renderer::View* view = drawSys->getRenderer()->findView(win))
#if SDL_VERSION_ATLEAST(3, 0, 0)
			return ivec2(mp) + view->rect.pos();
#else
			return mp + view->rect.pos();
#endif
	return mp;
}

void WindowSys::moveCursor(ivec2 mov) noexcept {
	if (SDL_Window* win = SDL_GetMouseFocus())
		if (Renderer::View* vsrc = drawSys->getRenderer()->findView(win)) {
			mpvec2 wpos;
			SDL_GetMouseState(&wpos.x, &wpos.y);
			wpos += vsrc->rect.pos() + mov;
			if (Renderer::View* vdst = drawSys->getRenderer()->findView(wpos))
				SDL_WarpMouseInWindow(vdst->win, wpos.x - vdst->rect.x, wpos.y - vdst->rect.y);
		}
}

void WindowSys::toggleOpacity() noexcept {
	for (SDL_Window* it : windows) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
		SDL_SetWindowOpacity(it, SDL_GetWindowOpacity(it) < 1.f ? 1.f : 0.f);
#else
		if (float val; !SDL_GetWindowOpacity(it, &val))
			SDL_SetWindowOpacity(it, val < 1.f ? 1.f : 0.f);
		else
			SDL_MinimizeWindow(it);
#endif
	}
}

void WindowSys::setScreenMode(Settings::Screen sm) {
	bool changeFlag = sets->screen != Settings::Screen::multiFullscreen && sm != Settings::Screen::multiFullscreen;
	sets->screen = sm;
	if (changeFlag)
		SDL_SetWindowFullscreen(windows[0], sm == Settings::Screen::fullscreen ? SDL_GetWindowFlags(windows[0]) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(windows[0]) & ~SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		recreateWindows();
}

ivec2 WindowSys::displayResolution() const noexcept {
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (!windows.empty())
		if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows[0])))
			return ivec2(mode->w, mode->h);
#else
	SDL_DisplayMode mode{};
	if (!windows.empty() && !SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows[0]), &mode))
		return ivec2(mode.w, mode.h);
#endif

	ivec2 res(0);
#if SDL_VERSION_ATLEAST(3, 0, 0)
	if (int cnt; SDL_DisplayID* dids = SDL_GetDisplays(&cnt)) {
		for (int i = 0; i < cnt; ++i)
			if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(dids[i]))
				res = glm::max(res, ivec2(mode->w, mode->h));
		SDL_free(dids);
	}
#else
	for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
		if (!SDL_GetDesktopDisplayMode(i, &mode))
			res = glm::max(res, ivec2(mode.w, mode.h));
#endif
	return res;
}

void WindowSys::resetSettings() {
	*sets = Settings(fileSys->getDirSets(), fileSys->getAvailableThemes());
	recreateWindows();
}
