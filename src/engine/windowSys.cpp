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

int WindowSys::start(vector<string>&& cmdVals,  uset<string>&& cmdFlags) {
	fileSys = nullptr;
	inputSys = nullptr;
	program = nullptr;
	scene = nullptr;
	sets = nullptr;
	int rc = EXIT_SUCCESS;
	try {
		init(std::move(cmdVals), std::move(cmdFlags));
		exec();
	} catch (const std::runtime_error& e) {
		logError(e.what());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", e.what(), !windows.empty() ? windows.begin()->second : nullptr);
		rc = EXIT_FAILURE;
#ifdef NDEBUG
	} catch (...) {
		logError("Unknown fatal error");
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Unknown fatal error", !windows.empty() ? windows.begin()->second : nullptr);
		rc = EXIT_FAILURE;
#endif
	}
	delete program;
	delete scene;
	delete inputSys;
	destroyWindows();
	delete fileSys;
	delete sets;

	IMG_Quit();
	SDL_Quit();
	return rc;
}

void WindowSys::init(vector<string>&& cmdVals, uset<string>&& cmdFlags) {
#if SDL_VERSION_ATLEAST(2, 0, 22)
	SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
	SDL_SetHint(SDL_HINT_APP_NAME, "VertiRead");
#endif
#if SDL_VERSION_ATLEAST(2, 0, 10)
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#else
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#ifndef _WIN32
	if (cmdFlags.contains(Settings::flagCompositor))
		SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(SDL_GetError());
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP | IMG_INIT_JXL | IMG_INIT_AVIF;
#else
	int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP;
#endif
	if (IMG_Init(imgFlags) != imgFlags) {
		const char* err = IMG_GetError();
		logError(strfilled(err) ? err : "Failed to initialize all image formats");
	}
	SDL_EventState(SDL_LOCALECHANGED, SDL_DISABLE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
	SDL_EventState(SDL_KEYUP, SDL_DISABLE);
	SDL_EventState(SDL_KEYMAPCHANGED, SDL_DISABLE);
	SDL_EventState(SDL_JOYBALLMOTION, SDL_DISABLE);
	SDL_EventState(SDL_JOYBUTTONUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERDEVICEREMAPPED, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADDOWN, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADMOTION, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERTOUCHPADUP, SDL_DISABLE);
	SDL_EventState(SDL_CONTROLLERSENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_DOLLARRECORD, SDL_DISABLE);
	SDL_EventState(SDL_MULTIGESTURE, SDL_DISABLE);
	SDL_EventState(SDL_CLIPBOARDUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_DROPBEGIN, SDL_DISABLE);
	SDL_EventState(SDL_DROPCOMPLETE, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEADDED, SDL_DISABLE);
	SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_DISABLE);
	SDL_EventState(SDL_SENSORUPDATE, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_TARGETS_RESET, SDL_DISABLE);
	SDL_EventState(SDL_RENDER_DEVICE_RESET, SDL_DISABLE);
	if (SDL_RegisterEvents(SDL_EventType(SDL_USEREVENT_MAX) - SDL_USEREVENT) == UINT32_MAX)
		throw std::runtime_error(SDL_GetError());
	SDL_StopTextInput();

	fileSys = new FileSys(cmdFlags);
	sets = fileSys->loadSettings(&cmdFlags);
	createWindow();
	inputSys = new InputSys;
	scene = new Scene;
	program = new Program;
	program->start(cmdVals);

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_USEREVENT - 1);
}

void WindowSys::exec() {
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		uint32 newTime = SDL_GetTicks();
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
	fileSys->saveSettings(sets);
	fileSys->saveBindings(inputSys->getBindings());
}

void WindowSys::createWindow() {
	if (sets->screen == Settings::Screen::multiFullscreen && sets->displays.empty())
		sets->screen = Settings::Screen::fullscreen;
	uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI;
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

	vector<Settings::Renderer> renderers;
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
			logError(err.what());
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
		if (SDL_GL_LoadLibrary(nullptr))
			throw std::runtime_error(SDL_GetError());
		return SDL_WINDOW_OPENGL; }
#endif
#ifdef WITH_VULKAN
	case vulkan:
		if (SDL_Vulkan_LoadLibrary(nullptr))
			throw std::runtime_error(SDL_GetError());
		return SDL_WINDOW_VULKAN;
#endif
	}
	return 0;
}

void WindowSys::createSingleWindow(uint32 flags, SDL_Surface* icon) {
	sets->resolution = glm::clamp(sets->resolution, windowMinSize, displayResolution());
	SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, flags);
	if (!win)
		throw std::runtime_error(std::format("Failed to create window:" LINEND "{}", SDL_GetError()));
	if (sets->screen == Settings::Screen::windowed && sets->maximized)
		SDL_MaximizeWindow(win);	// workaround for SDL_WINDOW_MAXIMIZED causing the window to not report the correct size
	windows.emplace(Renderer::singleDspId, win);
	drawSys = new DrawSys(windows);
	SDL_SetWindowIcon(win, icon);
	SDL_SetWindowMinimumSize(win, windowMinSize.x, windowMinSize.y);
}

void WindowSys::createMultiWindow(uint32 flags, SDL_Surface* icon) {
	windows.reserve(sets->displays.size());
	int minId = rng::min_element(sets->displays, [](const pair<const int, Recti>& a, const pair<const int, Recti>& b) -> bool { return a.first < b.first; })->first;
	for (const auto& [id, rect] : sets->displays) {
		SDL_Window* win = windows.emplace(id, SDL_CreateWindow(std::format("{} {}", title, id).data(), SDL_WINDOWPOS_CENTERED_DISPLAY(id), SDL_WINDOWPOS_CENTERED_DISPLAY(id), rect.w, rect.h, id != minId ? flags : flags & ~SDL_WINDOW_SKIP_TASKBAR)).first->second;
		if (!win)
			throw std::runtime_error(std::format("Failed to create window:" LINEND "{}", SDL_GetError()));
		SDL_SetWindowIcon(win, icon);
	}
	drawSys = new DrawSys(windows);
}

void WindowSys::destroyWindows() {
	delete drawSys;
	drawSys = nullptr;
	for (auto [id, win] : windows)
		SDL_DestroyWindow(win);
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
	case SDL_DISPLAYEVENT:
		eventDisplay(event.display);
		break;
	case SDL_WINDOWEVENT:
		eventWindow(event.window);
		break;
	case SDL_KEYDOWN:
		inputSys->eventKeypress(event.key);
		break;
	case SDL_TEXTEDITING:
		scene->onCompose(event.edit.text);
		break;
	case SDL_TEXTINPUT:
		scene->onText(event.text.text);
		break;
#if SDL_VERSION_ATLEAST(2, 0, 22)
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
	case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
		inputSys->reloadControllers();
		break;
	case SDL_CONTROLLERAXISMOTION:
		inputSys->eventGamepadAxis(event.caxis);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		inputSys->eventGamepadButton(event.cbutton);
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
		program->getState()->eventFileDrop(event.drop.file);
		SDL_free(event.drop.file);
		break;
	case SDL_DROPTEXT:
		scene->onText(event.drop.file);
		SDL_free(event.drop.file);
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
	case SDL_USEREVENT_THREAD_ARCHIVE_FINISHED:
		program->eventArchiveFinished(event.user);
		break;
	case SDL_USEREVENT_THREAD_PREVIEW:
		program->handleThreadPreviewEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_READER:
		program->handleThreadReaderEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_MOVE:
		program->handleThreadMoveEvent(event.user);
		break;
	case SDL_USEREVENT_THREAD_FONTS_FINISHED:
		program->eventFontsFinished(event.user);
	}
}

void WindowSys::eventWindow(const SDL_WindowEvent& winEvent) {
	switch (winEvent.event) {
	case SDL_WINDOWEVENT_RESIZED:
		if (windows.size() == 1)
			if (uint32 flags = SDL_GetWindowFlags(windows.begin()->second); !(flags & SDL_WINDOW_FULLSCREEN_DESKTOP))
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
		if (sets->screen == Settings::Screen::multiFullscreen && rng::any_of(windows, [](const pair<const int, SDL_Window*>& it) -> bool { return SDL_GetWindowFlags(it.second) & SDL_WINDOW_MINIMIZED; })) {
			for (auto [id, win] : windows)
				SDL_MaximizeWindow(win);
			SDL_FlushEvent(SDL_WINDOWEVENT);
		}
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (sets->screen == Settings::Screen::multiFullscreen && rng::none_of(windows, [](const pair<const int, SDL_Window*>& it) -> bool { return SDL_GetWindowFlags(it.second) & SDL_WINDOW_INPUT_FOCUS; })) {
			for (auto [id, win] : windows)
				SDL_MinimizeWindow(win);
			SDL_FlushEvent(SDL_WINDOWEVENT);
		}
		break;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED:
		if (Renderer::isSingleWindow(windows) && drawSys->updateDpi(winEvent.data1))
#else
	case SDL_WINDOWEVENT_MOVED:
		if (Renderer::isSingleWindow(windows) && drawSys->updateDpi(SDL_GetWindowDisplayIndex(SDL_GetWindowFromID(winEvent.windowID))))
#endif
			scene->onResize();
	}
}

void WindowSys::eventDisplay(const SDL_DisplayEvent& dspEvent) {
#if SDL_VERSION_ATLEAST(2, 28, 0)
	constexpr uint32 last = SDL_DISPLAYEVENT_MOVED;
#else
	constexpr uint32 last = SDL_DISPLAYEVENT_DISCONNECTED;
#endif
	for (uint32 i = SDL_DISPLAYEVENT_ORIENTATION; i <= last; ++i)
		if (dspEvent.type == i) {
			sets->unionDisplays();
			if (windows.size() > 1)
				recreateWindows();
			scene->onDisplayChange();
			break;
		}
}

ivec2 WindowSys::winViewOffset(uint32 wid) {
	if (SDL_Window* win = SDL_GetWindowFromID(wid))
		if (umap<int, SDL_Window*>::const_iterator it = rng::find_if(windows, [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; }); it != windows.end())
			return drawSys->getRenderer()->getViews().at(it->first)->rect.pos();
	return ivec2(INT_MIN);
}

ivec2 WindowSys::mousePos() {
	ivec2 mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	SDL_Window* win = SDL_GetMouseFocus();
	umap<int, SDL_Window*>::const_iterator it = rng::find_if(windows, [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; });
	return it != windows.end() ? mp + drawSys->getRenderer()->getViews().at(it->first)->rect.pos() : mp;
}

void WindowSys::moveCursor(ivec2 mov) {
	SDL_Window* win = SDL_GetMouseFocus();
	if (umap<int, SDL_Window*>::iterator it = rng::find_if(windows, [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; }); it != windows.end()) {
		ivec2 wpos;
		SDL_GetMouseState(&wpos.x, &wpos.y);
		wpos += drawSys->getRenderer()->getViews().at(it->first)->rect.pos() + mov;
		int id = drawSys->findPointInView(wpos);
		if (it = windows.find(id); it != windows.end()) {
			ivec2 vpos = drawSys->getRenderer()->getViews().at(id)->rect.pos();
			SDL_WarpMouseInWindow(it->second, wpos.x - vpos.x, wpos.y - vpos.y);
		}
	}
}

void WindowSys::toggleOpacity() {
	for (auto [id, win] : windows) {
		if (float val; !SDL_GetWindowOpacity(win, &val))
			SDL_SetWindowOpacity(win, val < 1.f ? 1.f : 0.f);
		else
			SDL_MinimizeWindow(win);
	}
}

void WindowSys::setScreenMode(Settings::Screen sm) {
	bool changeFlag = sets->screen != Settings::Screen::multiFullscreen && sm != Settings::Screen::multiFullscreen;
	sets->screen = sm;
	if (changeFlag)
		SDL_SetWindowFullscreen(windows.begin()->second, sm == Settings::Screen::fullscreen ? SDL_GetWindowFlags(windows.begin()->second) | SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_GetWindowFlags(windows.begin()->second) & ~SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		recreateWindows();
}

ivec2 WindowSys::displayResolution() const {
	SDL_DisplayMode mode{};
	if (!windows.empty() && !SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows.begin()->second), &mode))
		return ivec2(mode.w, mode.h);

	ivec2 res(0);
	for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
		if (!SDL_GetDesktopDisplayMode(i, &mode))
			res = glm::max(res, ivec2(mode.w, mode.h));
	return res;
}

void WindowSys::resetSettings() {
	*sets = Settings(fileSys->getDirSets(), fileSys->getAvailableThemes());
	recreateWindows();
}
