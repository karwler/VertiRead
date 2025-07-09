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
#if defined(WITH_SDL3) && defined( __linux__)
		isWayland = !SDL_strcasecmp(coalesce(SDL_getenv("XDG_SESSION_TYPE"), ""), "wayland");
#endif
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
	tick_t eventTimeout, loopTimeout;
	SDL_Event event;
	double perfHz = SDL_GetPerformanceFrequency();
	for (uint64 oldTime = SDL_GetPerformanceCounter(); run;) {
		uint64 newTime = SDL_GetPerformanceCounter();
		dSec = double(newTime - oldTime) / perfHz;
		oldTime = newTime;
		tick_t tickBase = SDL_GetTicks();

		if (active) {
			drawSys->drawWidgets(inputSys->mouseWin.has_value());
			inputSys->tick();

			eventTimeout = tickBase + eventCheckTimeout;
			loopTimeout = tickBase + refreshMs;
		} else
			loopTimeout = eventTimeout = tickBase + eventCheckTimeout * 4;
		scene->tick(dSec);
		program->tick();

		while (SDL_PollEvent(&event)) {
			handleEvent(event);
			if (SDL_TICKS_PASSED(SDL_GetTicks(), eventTimeout))
				break;
		}
		if (stick_t rest = loopTimeout - SDL_GetTicks(); rest > 0)
			SDL_Delay(rest);
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
	if (!numWindows)
		throw std::runtime_error("Failed to initialize a working renderer");
	setRefreshTime();
}

#ifdef WITH_SDL3
SDL_PropertiesID WindowSys::initWindow(uint8 wincnt, const array<vec4, Settings::defaultColors.size()>& colors) {
	const char* graphicsProp = nullptr;
#else
uint32 WindowSys::initWindow(uint8 wincnt) {
	uint32 windowFlags = SDL_WINDOW_ALLOW_HIGHDPI;
#endif
	windows = std::make_unique<SDL_Window*[]>(wincnt);
	numWindows = wincnt;

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
#ifdef WITH_SDL3
		graphicsProp = SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN;
#else
		windowFlags |= SDL_WINDOW_OPENGL;
#endif
		break; }
#endif
#ifdef WITH_VULKAN
	case vulkan:
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
	SDL_SetBooleanProperty(windowProps, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, sets->maximized);
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
	ivec2 vofs[2] = { ivec2(0), ivec2(0) };
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
	drawSys = new DrawSys(windows.get(), numWindows, colors, vofs);
}

void WindowSys::createMultiWindow(SDL_Surface* icon, const array<vec4, Settings::defaultColors.size()>& colors) {
#ifdef WITH_SDL3
	sthandle<SDL_PropertiesID> props = initWindow(sets->displays.size(), colors);
#else
	uint32 flags = initWindow(sets->displays.size());
#endif
	uptr<ivec2[]> vofs = std::make_unique_for_overwrite<ivec2[]>(numWindows + 1);
	vofs[numWindows] = ivec2(INT_MAX);

	for (uint8 i = 0; i < numWindows; ++i) {
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
		vofs[numWindows] = glm::min(vofs[numWindows], vofs[i]);
	}
	drawSys = new DrawSys(windows.get(), numWindows, colors, vofs.get());
}

void WindowSys::destroyWindows() noexcept {
	delete drawSys;
	drawSys = nullptr;
#ifdef WITH_SDL3
	if (numWindows)
		SDL_DestroyWindow(windows[0]);
#else
	for (uint8 i = 0; i < numWindows; ++i)
		SDL_DestroyWindow(windows[i]);
#endif
	windows.reset();
	numWindows = 0;
#ifdef WITH_DIRECT3D
	if (sets->renderer == Settings::Renderer::direct3d11)
		closeD3d11();
#endif
}

void WindowSys::recreateWindows() {
	scene->clearLayouts();
	destroyWindows();
	createWindow();
	scene->setLayouts();
}

void WindowSys::handleEvent(SDL_Event& event) {
	switch (event.type) {
	case SDL_QUIT:
		program->eventExit();
		break;
#ifndef WITH_SDL3
	case SDL_DISPLAYEVENT:
		eventDisplay(event.display);
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
#if defined(WITH_SDL3) && defined( __linux__)
		if (isWayland)
			if (float scale = SDL_GetWindowDisplayScale(SDL_GetWindowFromID(event.motion.windowID)); scale > 0.f) {
				event.motion.x *= scale;
				event.motion.y *= scale;
				event.motion.xrel *= scale;
				event.motion.yrel *= scale;
			}
#endif
		inputSys->eventMouseMotion(event.motion);
		break;
	case SDL_MOUSEBUTTONDOWN:
#if defined(WITH_SDL3) && defined( __linux__)
		if (isWayland)
			if (float scale = SDL_GetWindowDisplayScale(SDL_GetWindowFromID(event.motion.windowID)); scale > 0.f) {
				event.button.x *= scale;
				event.button.y *= scale;
			}
#endif
		inputSys->eventMouseButtonDown(event.button);
		break;
	case SDL_MOUSEBUTTONUP:
#if defined(WITH_SDL3) && defined( __linux__)
		if (isWayland)
			if (float scale = SDL_GetWindowDisplayScale(SDL_GetWindowFromID(event.motion.windowID)); scale > 0.f) {
				event.button.x *= scale;
				event.button.y *= scale;
			}
#endif
		inputSys->eventMouseButtonUp(event.button);
		break;
	case SDL_MOUSEWHEEL:
#if defined(WITH_SDL3) && defined( __linux__)
		if (isWayland)
			if (float scale = SDL_GetWindowDisplayScale(SDL_GetWindowFromID(event.motion.windowID)); scale > 0.f) {
				event.wheel.mouse_x *= scale;
				event.wheel.mouse_y *= scale;
			}
#endif
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
			eventDisplay(event.display);
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
	case SDL_WINDOWEVENT_RESIZED:	// should only happen when single window
#ifdef WITH_SDL3
		if (SDL_WindowFlags flags = SDL_GetWindowFlags(windows[0]); !(flags & SDL_WINDOW_FULLSCREEN))
#else
		if (uint32 flags = SDL_GetWindowFlags(windows[0]); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
#endif
			if (sets->maximized = flags & SDL_WINDOW_MAXIMIZED; !sets->maximized)
				sets->resolution = ivec2(winEvent.data1, winEvent.data2);
		break;
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		if (drawSys->updateView())
			scene->onResize();
		break;
	case SDL_WINDOWEVENT_MINIMIZED:
		active = false;
		break;
	case SDL_WINDOWEVENT_ENTER:
		active = true;
		break;
	case SDL_WINDOWEVENT_LEAVE:
		scene->onMouseLeave();
		if (numWindows == 1 && !(SDL_GetWindowFlags(windows[0]) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS)))
			active = false;
		break;
	case SDL_WINDOWEVENT_FOCUS_GAINED:
#ifndef WITH_SDL3
		if (numWindows > 1 && !active) {
			for (uint8 i = 0; i < numWindows; ++i)
				if (SDL_WindowID wid = SDL_GetWindowID(windows[i]); wid != winEvent.windowID)
					SDL_RaiseWindow(windows[i]);	// TODO: what does SDL_RestoreWindow do on fullscreen?
			SDL_FlushEvent(SDL_WINDOWEVENT);
		}
#endif
		active = true;
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (numWindows == 1) {
			if (!(SDL_GetWindowFlags(windows[0]) & (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS)))
				active = false;
		}
#ifndef WITH_SDL3
		else if (std::none_of(windows.get(), windows.get() + numWindows, [](SDL_Window* it) -> bool { return SDL_GetWindowFlags(it) & SDL_WINDOW_INPUT_FOCUS; })) {
			for (uint8 i = 0; i < numWindows; ++i)
				SDL_MinimizeWindow(windows[i]);	// TODO: does this work?
			SDL_FlushEvent(SDL_WINDOWEVENT);
			active = false;
		}
#endif
		break;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED:
		setRefreshTime();
		drawSys->updateUiScale();
#endif
	}
}

void WindowSys::eventDisplay(const SDL_DisplayEvent& dspEvent) {
#ifdef WITH_SDL3
	bool affected = std::any_of(windows.get(), windows.get() + numWindows, [&dspEvent](SDL_Window* it) -> bool { return SDL_GetDisplayForWindow(it) == dspEvent.displayID; });
	switch (dspEvent.type) {
#else
	bool affected = std::any_of(windows.get(), windows.get() + numWindows, [&dspEvent](SDL_Window* it) -> bool { return SDL_GetWindowDisplayIndex(it) == int(dspEvent.display); });
	switch (dspEvent.event) {
#endif
	case SDL_DISPLAYEVENT_ORIENTATION: case SDL_DISPLAYEVENT_CONNECTED: case SDL_DISPLAYEVENT_DISCONNECTED: case SDL_DISPLAYEVENT_MOVED:
		sets->unionDisplays();
		if (numWindows == 1 || !affected)
			scene->onDisplayChange();
		else
			recreateWindows();
		break;
#ifdef WITH_SDL3
	case SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED: case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED:
		if (affected)
			setRefreshTime();
	case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
		if (affected)
			drawSys->updateUiScale();
#endif
	}
}

ivec2 WindowSys::winViewOffset(SDL_WindowID wid) const noexcept {
	if (SDL_Window* win = SDL_GetWindowFromID(wid))
		if (Renderer::View* view = drawSys->getRenderer()->findView(win))
			return view->rect.pos();
	return ivec2(0);
}

ivec2 WindowSys::mousePos() const noexcept {
	mpvec2 mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	if (SDL_Window* win = SDL_GetMouseFocus())
		if (Renderer::View* view = drawSys->getRenderer()->findView(win)) {
#if defined(WITH_SDL3) && defined( __linux__)
			if (isWayland)
				if (float scale = SDL_GetWindowDisplayScale(win); scale > 0.f)
					mp *= scale;
			return ivec2(mp) + view->rect.pos();
#else
			return vec2(mp) + vec2(view->rect.pos());
#endif
		}
	return mp;
}

void WindowSys::moveCursor(ivec2 mov) noexcept {
	if (SDL_Window* win = SDL_GetMouseFocus())
		if (Renderer::View* vsrc = drawSys->getRenderer()->findView(win)) {
			mpvec2 wpos;
			SDL_GetMouseState(&wpos.x, &wpos.y);
#if defined(WITH_SDL3) && defined( __linux__)
			if (isWayland)
				if (float scale = SDL_GetWindowDisplayScale(win); scale > 0.f)
					wpos *= scale;	// TODO: test this
#endif
			wpos += vsrc->rect.pos() + mov;
			if (Renderer::View* vdst = drawSys->getRenderer()->findView(wpos))
				SDL_WarpMouseInWindow(vdst->win, wpos.x - vdst->rect.x, wpos.y - vdst->rect.y);
		}
}

void WindowSys::toggleOpacity() noexcept {
	for (uint8 i = 0; i < numWindows; ++i) {
#ifdef WITH_SDL3
		SDL_SetWindowOpacity(windows[i], SDL_GetWindowOpacity(windows[i]) < 1.f ? 1.f : 0.f);
#else
		if (float val; !SDL_GetWindowOpacity(windows[i], &val))
			SDL_SetWindowOpacity(windows[i], val < 1.f ? 1.f : 0.f);
		else
			SDL_MinimizeWindow(windows[i]);
#endif
	}
}

void WindowSys::setScreenMode(Settings::Screen sm) {
	bool changeFlag = sets->screen != Settings::Screen::multiFullscreen && sm != Settings::Screen::multiFullscreen;
	sets->screen = sm;
	if (changeFlag)
		SDL_SetWindowFullscreen(windows[0], sm == Settings::Screen::fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	else
		recreateWindows();
}

void WindowSys::setRefreshTime() noexcept {
	int rate = 0;
#ifdef WITH_SDL3
	for (uint8 i = 0; i < numWindows; ++i)
		if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(windows[i])))
			rate = std::max(rate, int(mode->refresh_rate));
#else
	SDL_DisplayMode mode{};
	for (uint8 i = 0; i < numWindows; ++i)
		if (!SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows[i]), &mode))
			rate = std::max(rate, mode.refresh_rate);
#endif
	refreshMs = rate ? 1000 / uint(rate) : 0;
}

array<vec4, Settings::defaultColors.size()> WindowSys::loadColors(string_view name) {
	return fileSys->loadColors(sets->setTheme(name, fileSys->getAvailableThemes()));
}

void WindowSys::resetSettings() {
	*sets = fileSys->getAvailableThemes();
	recreateWindows();
}
