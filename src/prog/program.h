#pragma once

#include "browser.h"
#include "progs.h"

// handles the frontend
class Program {
public:
	Program();

	// books
	void eventOpenBookList(Button* but=nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenReader(Button* but);
	void eventOpenLastPage(Button* but);

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
};
