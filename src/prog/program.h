#pragma once

#include "browser.h"
#include "progs.h"

// handles the frontend
class Program {
public:
	Program();

	void init();

	// books
	void eventOpenBookList(Button* but=nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenReader(Button* but);
	void eventOpenLastPage(Button* but);
	bool openFile(string file);

	// browser
	void eventBrowserGoUp(Button* but=nullptr);
	void eventBrowserGoIn(Button* but);
	void eventExitBrowser(Button* but=nullptr);

	// reader
	void eventZoomIn(Button* but=nullptr);
	void eventZoomOut(Button* but=nullptr);
	void eventZoomReset(Button* but=nullptr);
	void eventCenterView(Button* but=nullptr);
	void eventNextDir(Button* but=nullptr);
	void eventPrevDir(Button* but=nullptr);
	void eventExitReader(Button* but=nullptr);

	// settings
	void eventOpenSettings(Button* but=nullptr);
	void eventSwitchDirection(Button* but);
	void eventSwitchLanguage(Button* but);
	void eventSetLibraryDirLE(Button* but);
	void eventSetLibraryDirBW(Button* but);
	void eventOpenLibDirBrowser(Button* but=nullptr);
	void eventSwitchFullscreen(Button* but);
	void eventSetTheme(Button* but);
	void eventSetFont(Button* but);
	void eventSetRenderer(Button* but);
	void eventSetScrollSpeed(Button* but);
	void eventSetDeadzoneSL(Button* but);
	void eventSetDeadzoneLE(Button* but);
	void eventSetPortrait(Button* but);
	void eventSetLandscape(Button* but);
	void eventSetSquare(Button* but);
	void eventSetFill(Button* but);
	void eventResetSettings(Button* but);

	// other
	void eventClosePopup(Button* but=nullptr);
	void eventExit(Button* but=nullptr);
	
	ProgState* getState() { return state.get(); }
	Browser* getBrowser() { return browser.get(); }

private:
	uptr<ProgState> state;
	uptr<Browser> browser;

	void setState(ProgState* newState);
	bool startReader(const string& picname);
	void reposizeWindow(const vec2i& dres, const vec2i& wsiz);
};
