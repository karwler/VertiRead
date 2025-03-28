#include "windowSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "optional/d3d.h"
#include "prog/program.h"
#include "prog/progs.h"
#ifdef WITH_SDL3
#include <SDL3/SDL_vulkan.h>
#include <SDL3_image/SDL_image.h>
#else
#include <SDL_image.h>
#include <SDL_vulkan.h>
#endif

void WindowSys::init() {
	try {
		fileSys = new FileSys();
		sets = fileSys->loadSettings();
		createWindow();
		inputSys = new InputSys;
		scene = new Scene;
		program = new Program;
		program->start();
	} catch (const std::exception&) {
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
	double perfHz = SDL_GetPerformanceFrequency();
	for (uint64 oldTime = SDL_GetPerformanceCounter(); run;) {
		uint64 newTime = SDL_GetPerformanceCounter();
		dSec = double(newTime - oldTime) / perfHz;
		oldTime = newTime;

		if (active) {
			drawSys->drawWidgets(inputSys->mouseWin.has_value());
			inputSys->tick();
		}
		scene->tick(dSec);
		program->tick();

		SDL_Event event;
		tick_t timeout = SDL_GetTicks() + eventCheckTimeout;
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

	uptr<SDL_Surface> icon(IMG_Load((fileSys->dirIcons() / DrawSys::iconName(DrawSys::Tex::vertiread)).data()));
	array<vec4, Settings::defaultColors.size()> colors = loadColors(sets->getTheme());
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
#if !defined(__arm__) && !defined(__aarch64__)
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
#endif
#ifndef _WIN32
	case opengles3:
		renderers = {
			opengles3,
#if !defined(__arm__) && !defined(__aarch64__)
			opengl3, opengl1,
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
#endif
#endif
#ifdef WITH_VULKAN
	case vulkan:
		renderers = {
			vulkan,
#ifdef WITH_OPENGL
#if !defined(__arm__) && !defined(__aarch64__)
			opengl3, opengl1,
#endif
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
#if !defined(__arm__) && !defined(__aarch64__)
			opengl1, opengl3,
#endif
#ifndef _WIN32
			opengles3,
#endif
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
				createSingleWindow(icon.get(), colors);
			else
				createMultiWindow(icon.get(), colors);
			break;
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err.what(), nullptr);
			destroyWindows();
		}
	}
	if (windows.empty())
		throw std::runtime_error("Failed to initialize a working renderer");
}

#ifdef WITH_SDL3
SDL_PropertiesID WindowSys::initWindow(size_t numWindows, const array<vec4, Settings::defaultColors.size()>& colors) {
	const char* graphicsProp = nullptr;
#else
uint32 WindowSys::initWindow(size_t numWindows) {
	uint32 windowFlags = SDL_WINDOW_ALLOW_HIGHDPI;
#endif
	windows.resize(numWindows);

	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		if (!symD3d11())
			throw std::runtime_error("Failed to load D3D11 libraries");
#ifdef WITH_SDL3
		graphicsProp = SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN;
#endif
		break;
#endif
#ifdef WITH_OPENGL
#ifdef _WIN32
	case opengl1: case opengl3: {
		bool core = true;
#elif defined(__arm__)
	case opengles3: {
		bool core = true;
#elif defined(__aarch64__)
	case opengles3: {
		bool core = false;
#else
	case opengl1: case opengl3: case opengles3: {
		bool core = sets->renderer <= opengl3;
#endif
		int flags = 0;
#ifndef NDEBUG
		flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
		SDL_GL_ResetAttributes();
		if (sets->renderer == opengl1) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		} else {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, sets->gammaType == Settings::Gamma::srgb);
			if (core)
				flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
		}
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, core ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, flags);
		SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, numWindows > 1);
		if (sdlFailed(SDL_GL_LoadLibrary(nullptr)))
			throw std::runtime_error(SDL_GetError());
#ifdef WITH_SDL3
		graphicsProp = SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN;
#else
		windowFlags |= SDL_WINDOW_OPENGL;
#endif
		break; }
#endif
#ifdef WITH_VULKAN
	case vulkan:
		if (sdlFailed(SDL_Vulkan_LoadLibrary(nullptr)))
			throw std::runtime_error(SDL_GetError());
#ifdef WITH_SDL3
		graphicsProp = SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN;
#else
		windowFlags |= SDL_WINDOW_VULKAN;
#endif
#endif
	}
#ifdef WITH_SDL3
	SDL_PropertiesID windowProps = SDL_CreateProperties();
	if (!windowProps)
		throw std::runtime_error(SDL_GetError());
	if (graphicsProp)
		SDL_SetBooleanProperty(windowProps, graphicsProp, true);
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, sets->maximized);	// TODO: does this work or do we need the workaround?
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, sets->screen <= Settings::Screen::fullscreen);
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, sets->screen >= Settings::Screen::fullscreen);
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN, sets->needsTransparentWindow(colors));
	return windowProps;
#else
	switch (sets->screen) {
	using enum Settings::Screen;
	case windowed:
		windowFlags |= SDL_WINDOW_RESIZABLE;
		break;
	case fullscreen:
		windowFlags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;
		break;
	case multiFullscreen:
		windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	return windowFlags;
#endif
}

void WindowSys::createSingleWindow(SDL_Surface* icon, const array<vec4, Settings::defaultColors.size()>& colors) {
	sets->resolution = glm::clamp(sets->resolution, windowMinSize, displayResolution());
#ifdef WITH_SDL3
	sthandle<SDL_PropertiesID> props = initWindow(1, colors);
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, sets->resolution.x);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, sets->resolution.y);
	if (windows[0] = SDL_CreateWindowWithProperties(props); !windows[0])
		throw std::runtime_error(SDL_GetError());
#else
	if (windows[0] = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, initWindow(1)); !windows[0])
		throw std::runtime_error(SDL_GetError());
	if (sets->screen == Settings::Screen::windowed && sets->maximized)
		SDL_MaximizeWindow(windows[0]);	// workaround for SDL_WINDOW_MAXIMIZED causing the window to not report the correct size
#endif
	SDL_SetWindowIcon(windows[0], icon);
	SDL_SetWindowMinimumSize(windows[0], windowMinSize.x, windowMinSize.y);
	drawSys = new DrawSys(windows, colors);
}

void WindowSys::createMultiWindow(SDL_Surface* icon, const array<vec4, Settings::defaultColors.size()>& colors) {
	uptr<ivec2[]> vofs = std::make_unique_for_overwrite<ivec2[]>(windows.size() + 1);
	vofs[windows.size()] = ivec2(INT_MAX);
#ifdef WITH_SDL3
	sthandle<SDL_PropertiesID> props = initWindow(sets->displays.size(), colors);
#else
	uint32 flags = initWindow(sets->displays.size());
#endif
	for (size_t i = 0; i < windows.size(); ++i) {
#ifdef WITH_SDL3
		string name = i ? fmt::format("{} {}", title, i) : title;
		SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, name.data());
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did));
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, sets->displays[i].rect.w);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, sets->displays[i].rect.h);
		if (windows[i] = SDL_CreateWindowWithProperties(props); !windows[i])
			throw std::runtime_error(SDL_GetError());
		if (!i)
			SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, windows[0]);
#else
		if (windows[i] = SDL_CreateWindow(i ? fmt::format("{} {}", title, i).data() : title, SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did), SDL_WINDOWPOS_CENTERED_DISPLAY(sets->displays[i].did), sets->displays[i].rect.w, sets->displays[i].rect.h, flags); !windows[i])
			throw std::runtime_error(SDL_GetError());
		flags |= SDL_WINDOW_SKIP_TASKBAR;
#endif
		SDL_SetWindowIcon(windows[i], icon);
		vofs[i] = sets->displays[i].rect.pos();
		vofs[windows.size()] = glm::min(vofs[windows.size()], vofs[i]);
	}
	drawSys = new DrawSys(windows, colors, vofs.get());
}

void WindowSys::destroyWindows() noexcept {
	delete drawSys;
	drawSys = nullptr;
#ifdef WITH_SDL3
	if (!windows.empty()) {
		SDL_DestroyWindow(windows[0]);
		windows.clear();
	}
#else
	for (SDL_Window* it : windows)
		SDL_DestroyWindow(it);
	windows.clear();
#endif

	switch (sets->renderer) {
	using enum Settings::Renderer;
#ifdef WITH_DIRECT3D
	case direct3d11:
		closeD3d11();
		break;
#endif
#ifdef WITH_OPENGL
#if !defined(__arm__) && !defined(__aarch64__)
	case opengl1: case opengl3:
#endif
#ifndef _WIN32
	case opengles3:
#endif
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
#ifndef WITH_SDL3
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
#if SDL_VERSION_ATLEAST(2, 0, 22) && !defined(WITH_SDL3)
	case SDL_TEXTEDITING_EXT:
		scene->onCompose(uptr<char[], SdlFreePtr>(event.editExt.text).get());
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
#ifdef WITH_SDL3
		inputSys->eventGamepadAxis(event.gaxis);
#else
		inputSys->eventGamepadAxis(event.caxis);
#endif
		break;
	case SDL_CONTROLLERBUTTONDOWN:
#ifdef WITH_SDL3
		inputSys->eventGamepadButton(event.gbutton);
#else
		inputSys->eventGamepadButton(event.cbutton);
#endif
		break;
	case SDL_CONTROLLERDEVICEADDED:
#ifdef WITH_SDL3
		inputSys->addGamepad(event.gdevice.which);
#else
		inputSys->addGamepad(event.cdevice.which);
#endif
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
#ifdef WITH_SDL3
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
#ifdef WITH_SDL3
		program->getState()->eventFileDrop(event.drop.data);
#else
		program->getState()->eventFileDrop(uptr<char[], SdlFreePtr>(event.drop.file).get());
#endif
		break;
	case SDL_DROPTEXT:
#ifdef WITH_SDL3
		scene->onText(event.drop.data);
#else
		scene->onText(uptr<char[], SdlFreePtr>(event.drop.file).get());
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
#ifdef WITH_SDL3
	default:
		if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
			eventWindow(event.window);
		else if (event.type >= SDL_EVENT_DISPLAY_FIRST && event.type <= SDL_EVENT_DISPLAY_LAST)
			eventDisplay();
#endif
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
#ifdef WITH_SDL3
	switch (winEvent.type) {
#else
	switch (winEvent.event) {
#endif
	case SDL_WINDOWEVENT_EXPOSED:
		active = true;
		break;
	case SDL_WINDOWEVENT_RESIZED:
		if (!sets->maximized)	// should only happen when single window
			sets->resolution = ivec2(winEvent.data1, winEvent.data2);
		break;
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		if (drawSys->updateView())
			scene->onResize();
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		active = false;
		break;
	case SDL_WINDOWEVENT_MAXIMIZED:
		sets->maximized = true;
		break;
	case SDL_WINDOWEVENT_RESTORED:
		sets->maximized = false;	// TODO: test if maximized and resolution get saved properly
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		break;
#ifndef WITH_SDL3
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		if (sets->screen == Settings::Screen::multiFullscreen && !active) {
			for (SDL_Window* it : windows)
				if (SDL_WindowID wid = SDL_GetWindowID(it); wid != winEvent.windowID)
					SDL_RaiseWindow(it);	// TODO: what does SDL_RestoreWindow do on fullscreen?
			SDL_FlushEvent(SDL_WINDOWEVENT);
			active = true;
		}
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (sets->screen == Settings::Screen::multiFullscreen && rng::none_of(windows, [](SDL_Window* it) -> bool { return SDL_GetWindowFlags(it) & SDL_WINDOW_INPUT_FOCUS; }))
			active = false;
		break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED:
#ifdef WITH_SDL3
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:	// TODO: check if SDL_EVENT_WINDOW_DISPLAY_CHANGED is still needed in this case and handle for multi window
#endif
		if (windows.size() == 1 && drawSys->updateDpi())
			scene->onResize();
#endif
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
#ifdef WITH_SDL3
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
#ifdef WITH_SDL3
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
#ifdef WITH_SDL3
	if (!windows.empty())
		if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows[0])))
			return ivec2(mode->w, mode->h);
#else
	SDL_DisplayMode mode{};
	if (!windows.empty() && !SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows[0]), &mode))
		return ivec2(mode.w, mode.h);
#endif

	ivec2 res(0);
#ifdef WITH_SDL3
	int cnt;
	if (uptr<SDL_DisplayID[], SdlFreePtr> dids(SDL_GetDisplays(&cnt)); dids)
		for (int i = 0; i < cnt; ++i)
			if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(dids[i]))
				res = glm::max(res, ivec2(mode->w, mode->h));
#else
	for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
		if (!SDL_GetDesktopDisplayMode(i, &mode))
			res = glm::max(res, ivec2(mode.w, mode.h));
#endif
	return res;
}

array<vec4, Settings::defaultColors.size()> WindowSys::loadColors(string_view name) {
	return fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
}

void WindowSys::resetSettings() {
	*sets = fileSys->getAvailableThemes();
	recreateWindows();
}
