#include "windowSys.h"
#include "drawSys.h"
#include "fileSys.h"
#include "inputSys.h"
#include "scene.h"
#include "prog/program.h"
#include "prog/progs.h"
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif

int WindowSys::start() {
	fileSys = nullptr;
	inputSys = nullptr;
	program = nullptr;
	scene = nullptr;
	sets = nullptr;
	int rc = EXIT_SUCCESS;
	try {
		init();
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
	TTF_Quit();
	SDL_Quit();
	return rc;
}

void WindowSys::init() {
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
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
		throw std::runtime_error(SDL_GetError());
	if (TTF_Init())
		throw std::runtime_error(TTF_GetError());
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP | IMG_INIT_JXL | IMG_INIT_AVIF);
#else
	int flags = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
#endif
	if (!(flags & IMG_INIT_JPG))
		logError(IMG_GetError());
	if (!(flags & IMG_INIT_PNG))
		logError(IMG_GetError());
	if (!(flags & IMG_INIT_TIF))
		logError(IMG_GetError());
	if (!(flags & IMG_INIT_WEBP))
		logError(IMG_GetError());
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
	if (!(flags & IMG_INIT_JXL))
		logError(IMG_GetError());
	if (!(flags & IMG_INIT_AVIF))
		logError(IMG_GetError());
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#ifdef OPENGLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#ifdef NDEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG | SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
#endif
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
	if (SDL_RegisterEvents(SDL_USEREVENT_MAX - SDL_USEREVENT) == UINT32_MAX)
		throw std::runtime_error(SDL_GetError());
	SDL_StopTextInput();

	fileSys = new FileSys;
	sets = fileSys->loadSettings();
	createWindow();
	inputSys = new InputSys;
	scene = new Scene;
	program = new Program;
	program->start();

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_USEREVENT - 1);
}

void WindowSys::exec() {
	for (uint32 oldTime = SDL_GetTicks(); run;) {
		uint32 newTime = SDL_GetTicks();
		dSec = float(newTime - oldTime) / ticksPerSec;
		oldTime = newTime;

		drawSys->drawWidgets(scene, inputSys->mouseWin.has_value());
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
	if (sets->screen != Settings::Screen::multiFullscreen)
		flags |= SDL_WINDOW_RESIZABLE | (sets->maximized ? SDL_WINDOW_MAXIMIZED : 0) | (sets->screen != Settings::Screen::windowed ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	else
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SKIP_TASKBAR;
	SDL_Surface* icon = IMG_Load((fileSys->dirIcons() / "vertiread.svg").u8string().c_str());

	array<pair<Settings::Renderer, uint32>, size_t(Settings::Renderer::max)> renderers;
	switch (sets->renderer) {
#ifdef WITH_DIRECTX
	case Settings::Renderer::directx:
		renderers = {
			pair(Settings::Renderer::directx, 0),
#ifdef WITH_OPENGL
			pair(Settings::Renderer::opengl, SDL_WINDOW_OPENGL),
#endif
#ifdef WITH_VULKAN
			pair(Settings::Renderer::vulkan, SDL_WINDOW_VULKAN)
#endif
		};
		break;
#endif
#ifdef WITH_OPENGL
	case Settings::Renderer::opengl:
		renderers = {
			pair(Settings::Renderer::opengl, SDL_WINDOW_OPENGL),
#ifdef WITH_DIRECTX
			pair(Settings::Renderer::directx, 0),
#endif
#ifdef WITH_VULKAN
			pair(Settings::Renderer::vulkan, SDL_WINDOW_VULKAN)
#endif
		};
		break;
#endif
#ifdef WITH_VULKAN
	case Settings::Renderer::vulkan:
		renderers = {
			pair(Settings::Renderer::vulkan, SDL_WINDOW_VULKAN),
#ifdef WITH_OPENGL
			pair(Settings::Renderer::opengl, SDL_WINDOW_OPENGL),
#endif
#ifdef WITH_DIRECTX
			pair(Settings::Renderer::directx, 0)
#endif
		};
#endif
	}
	for (auto [rnd, rfl] : renderers) {
		try {
			sets->renderer = rnd;
			if (sets->screen != Settings::Screen::multiFullscreen)
				createSingleWindow(flags | rfl, icon);
			else
				createMultiWindow(flags | rfl, icon);
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

void WindowSys::createSingleWindow(uint32 flags, SDL_Surface* icon) {
	sets->resolution = glm::clamp(sets->resolution, windowMinSize, displayResolution());
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, SDL_DISABLE);
	SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sets->resolution.x, sets->resolution.y, flags);
	if (!win)
		throw std::runtime_error("Failed to create window:"s + linend + SDL_GetError());
	windows.emplace(Renderer::singleDspId, win);

	drawSys = new DrawSys(windows, sets, fileSys, int(128.f / fallbackDpi * winDpi));
	SDL_SetWindowIcon(win, icon);
	SDL_SetWindowMinimumSize(win, windowMinSize.x, windowMinSize.y);
	if (SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(windows.begin()->second), nullptr, nullptr, &winDpi))
		winDpi = fallbackDpi;
}

void WindowSys::createMultiWindow(uint32 flags, SDL_Surface* icon) {
	windows.reserve(sets->displays.size());
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, SDL_ENABLE);
	int minId = std::min_element(sets->displays.begin(), sets->displays.end(), [](const pair<const int, Recti>& a, const pair<const int, Recti>& b) -> bool { return a.first < b.first; })->first;
	for (const auto& [id, rect] : sets->displays) {
		SDL_Window* win = windows.emplace(id, SDL_CreateWindow((title + toStr(id)).c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(id), SDL_WINDOWPOS_CENTERED_DISPLAY(id), rect.w, rect.h, id != minId ? flags : flags & ~SDL_WINDOW_SKIP_TASKBAR)).first->second;
		if (!win)
			throw std::runtime_error("Failed to create window:"s + linend + SDL_GetError());
		SDL_SetWindowIcon(win, icon);
	}

	drawSys = new DrawSys(windows, sets, fileSys, int(128.f / fallbackDpi * winDpi));
	if (SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(windows.begin()->second), nullptr, nullptr, &winDpi))
		winDpi = fallbackDpi;
}

void WindowSys::destroyWindows() {
	delete drawSys;
	drawSys = nullptr;
	for (auto [id, win] : windows)
		SDL_DestroyWindow(win);
	windows.clear();
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
		program->eventTryExit();
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
		program->getState()->eventFileDrop(fs::u8path(event.drop.file));
		SDL_free(event.drop.file);
		break;
	case SDL_DROPTEXT:
		scene->onText(event.drop.file);
		SDL_free(event.drop.file);
		break;
	case SDL_USEREVENT_READER_PROGRESS:
		program->eventReaderProgress(event.user);
		break;
	case SDL_USEREVENT_READER_FINISHED:
		program->eventReaderFinished(event.user);
		break;
	case SDL_USEREVENT_PREVIEW_PROGRESS:
		program->eventPreviewProgress(event.user);
		break;
	case SDL_USEREVENT_PREVIEW_FINISHED:
		program->eventPreviewFinished();
		break;
	case SDL_USEREVENT_ARCHIVE_PROGRESS:
		program->eventArchiveProgress(event.user);
		break;
	case SDL_USEREVENT_ARCHIVE_FINISHED:
		program->eventArchiveFinished(event.user);
		break;
#ifdef DOWNLOADER
	case SDL_USEREVENT_DOWNLOAD_PROGRESS:
		program->eventDownloadProgress();
		break;
	case SDL_USEREVENT_DOWNLOAD_NEXT:
		program->eventDownloadNext();
		break;
	case SDL_USEREVENT_DOWNLOAD_FINISHED:
		program->eventDownloadFinish();
		break;
#endif
	case SDL_USEREVENT_MOVE_PROGRESS:
		program->eventMoveProgress(event.user);
		break;
	case SDL_USEREVENT_MOVE_FINISHED:
		program->eventMoveFinished(event.user);
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
		if (sets->screen == Settings::Screen::multiFullscreen && std::any_of(windows.begin(), windows.end(), [](const pair<const int, SDL_Window*>& it) -> bool { return SDL_GetWindowFlags(it.second) & SDL_WINDOW_MINIMIZED; })) {
			for (auto [id, win] : windows)
				SDL_MaximizeWindow(win);
			SDL_FlushEvent(SDL_WINDOWEVENT);
		}
		break;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (sets->screen == Settings::Screen::multiFullscreen && std::none_of(windows.begin(), windows.end(), [](const pair<const int, SDL_Window*>& it) -> bool { return SDL_GetWindowFlags(it.second) & SDL_WINDOW_INPUT_FOCUS; })) {
			for (auto [id, win] : windows)
				SDL_MinimizeWindow(win);
			SDL_FlushEvent(SDL_WINDOWEVENT);
		}
		break;
#if SDL_VERSION_ATLEAST(2, 0, 18)
	case SDL_WINDOWEVENT_DISPLAY_CHANGED: {
		float newDpi = fallbackDpi;
		if (int rc = SDL_GetDisplayDPI(winEvent.data1, nullptr, nullptr, &newDpi); !rc && newDpi != winDpi) {
#else
	case SDL_WINDOWEVENT_MOVED: {
		float newDpi = fallbackDpi;
		if (int rc = SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(SDL_GetWindowFromID(winEvent.windowID)), nullptr, nullptr, &newDpi); !rc && newDpi != winDpi) {
#endif
			winDpi = newDpi;
			scene->onResize();
		}
	} }
}

void WindowSys::eventDisplay(const SDL_DisplayEvent& dspEvent) {
	if (dspEvent.type == SDL_DISPLAYEVENT_ORIENTATION || dspEvent.type == SDL_DISPLAYEVENT_CONNECTED || dspEvent.type == SDL_DISPLAYEVENT_DISCONNECTED) {
		sets->unionDisplays();
		if (windows.size() > 1)
			recreateWindows();
		scene->onDisplayChange();
	}
}

ivec2 WindowSys::winViewOffset(uint32 wid) {
	if (SDL_Window* win = SDL_GetWindowFromID(wid))
		if (umap<int, SDL_Window*>::const_iterator it = std::find_if(windows.begin(), windows.end(), [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; }); it != windows.end())
			return drawSys->getRenderer()->getViews().at(it->first)->rect.pos();
	return ivec2(INT_MIN);
}

ivec2 WindowSys::mousePos() {
	ivec2 mp;
	SDL_GetMouseState(&mp.x, &mp.y);
	SDL_Window* win = SDL_GetMouseFocus();
	umap<int, SDL_Window*>::const_iterator it = std::find_if(windows.begin(), windows.end(), [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; });
	return it != windows.end() ? mp + drawSys->getRenderer()->getViews().at(it->first)->rect.pos() : mp;
}

void WindowSys::moveCursor(ivec2 mov) {
	SDL_Window* win = SDL_GetMouseFocus();
	if (umap<int, SDL_Window*>::iterator it = std::find_if(windows.begin(), windows.end(), [win](const pair<int, SDL_Window*>& p) -> bool { return p.second == win; }); it != windows.end()) {
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

void WindowSys::setWindowPos(ivec2 pos) {
	if (sets->screen != Settings::Screen::multiFullscreen)
		SDL_SetWindowPosition(windows.begin()->second, pos.x, pos.y);
}

void WindowSys::setResolution(ivec2 res) {
	sets->resolution = glm::clamp(res, windowMinSize, displayResolution());
	if (sets->screen != Settings::Screen::multiFullscreen)
		SDL_SetWindowSize(windows.begin()->second, res.x, res.y);
}

ivec2 WindowSys::displayResolution() const {
	SDL_DisplayMode mode;
	if (!windows.empty() && !SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(windows.begin()->second), &mode))
		return ivec2(mode.w, mode.h);

	ivec2 res(0);
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
		if (!SDL_GetDesktopDisplayMode(i, &mode))
			res = glm::max(res, ivec2(mode.w, mode.h));
	return res;
}

void WindowSys::resetSettings() {
	delete sets;
	sets = new Settings(fileSys->getDirSets(), fileSys->getAvailableThemes());
	recreateWindows();
}
