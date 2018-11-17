#pragma once

#include "browser.h"
#include "progs.h"

// handles the frontend
class Program {
public:
	Program();

	void start();

	// books
	void eventOpenBookList(Button* = nullptr) { setState(new ProgBooks); }
	void eventOpenPageBrowser(Button* but);
	void eventOpenReader(Button* but);
	void eventOpenLastPage(Button* but);
	bool openFile(const string& file);

	// browser
	void eventBrowserGoUp(Button* = nullptr);
	void eventBrowserGoIn(Button* but);
	void eventBrowserGoTo(Button* but);
	void eventExitBrowser(Button* = nullptr);

	// reader
	void eventZoomIn(Button* = nullptr);
	void eventZoomOut(Button* = nullptr);
	void eventZoomReset(Button* = nullptr);
	void eventCenterView(Button* = nullptr);
	void eventNextDir(Button* = nullptr);
	void eventPrevDir(Button* = nullptr);
	void eventExitReader(Button* = nullptr);

	// settings
	void eventOpenSettings(Button* = nullptr) { setState(new ProgSettings); }
	void eventSwitchDirection(Button* but);
	void eventSetZoom(Button* but);
	void eventSetSpacing(Button* but);
	void eventSwitchLanguage(Button* but);
	void eventSetLibraryDirLE(Button* but);
	void eventSetLibraryDirBW(Button*);
	void eventOpenLibDirBrowser(Button* = nullptr);
	void eventSwitchFullscreen(Button* but);
	void eventSetTheme(Button* but);
	void eventSetFont(Button* but);
	void eventSetRenderer(Button* but);
	void eventSetScrollSpeed(Button* but);
	void eventSetDeadzoneSL(Button* but);
	void eventSetDeadzoneLE(Button* but);
	void eventSetPortrait(Button*);
	void eventSetLandscape(Button*);
	void eventSetSquare(Button*);
	void eventSetFill(Button*);
	void eventResetSettings(Button*);

	// other
	void eventClosePopup(Button* = nullptr);
	void eventExit(Button* = nullptr);
	
	ProgState* getState() { return state.get(); }
	Browser* getBrowser() { return browser.get(); }

private:
	uptr<ProgState> state;
	uptr<Browser> browser;

	void setState(ProgState* newState);
	void reposizeWindow(const vec2i& dres, const vec2i& wsiz);
};
