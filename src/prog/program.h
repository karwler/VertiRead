#pragma once

#include "browser.h"
#include "progs.h"

// handles the frontend
class Program {
private:
	static constexpr float zoomFactor = 1.2f;
	static constexpr float resModeBorder = 0.85f;
	static constexpr float resModeRatio = 0.75f;

	Downloader downloader;
	uptr<ProgState> state;
	uptr<Browser> browser;
	uptr<Thread> thread;

public:
	Program();

	void start();
	void eventUser(const SDL_UserEvent& user);

	// books
	void eventOpenBookList(Button* but = nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenLastPage(Button* but);
	bool openFile(const string& file);

	// browser
	void eventBrowserGoUp(Button* but = nullptr);
	void eventBrowserGoIn(Button* but);
	void eventBrowserGoFile(Button* but);
	void eventBrowserGoTo(Button* but);
	void eventExitBrowser(Button* but = nullptr);

	// reader
	void eventStartLoadingReader(const string& first, bool fwd = true);
	void eventReaderLoadingProgress(string* prg, string* lim);
	void eventReaderLoadingCancelled(Button* but = nullptr);
	void eventReaderLoadingFinished(vector<Texture>* pics);
	void eventZoomIn(Button* but = nullptr);
	void eventZoomOut(Button* but = nullptr);
	void eventZoomReset(Button* but = nullptr);
	void eventCenterView(Button* but = nullptr);
	void eventNextDir(Button* but = nullptr);
	void eventPrevDir(Button* but = nullptr);
	void eventExitReader(Button* but = nullptr);
#ifdef BUILD_DOWNLOADER
	// downloader
	void eventOpenDownloader(Button* but = nullptr);
	void eventSwitchSource(Button* but = nullptr);
	void eventQuery(Button* but = nullptr);
	void eventShowComicInfo(Button* but = nullptr);
	void eventSelectAllChapters(Button* but);
	void eventSelectChapter(Button* but);
	void eventDownloadAllChapters(Button* but = nullptr);
	void eventDownloadChapter(Button* but);
	void eventDownloadComic(Button* but);

	// downloads
	void eventOpenDownloadList(Button* but = nullptr);
	void eventDownloadListProgress();
	void eventDownloadListNext();
	void eventDownloadListFinish();
	void eventDownloadDelete(Button* but);
	void eventResumeDownloads(Button* but = nullptr);
	void eventStopDownloads(Button* but = nullptr);
	void eventClearDownloads(Button* but = nullptr);
#else
	void eventDownloadListProgress() {}
	void eventDownloadListNext() {}
	void eventDownloadListFinish() {}
#endif
	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventSwitchDirection(Button* but);
	void eventSetZoom(Button* but);
	void eventSetSpacing(Button* but);
	void eventSetLibraryDirLE(Button* but);
	void eventSetLibraryDirBW(Button* but);
	void eventOpenLibDirBrowser(Button* but = nullptr);
	void eventMoveComics(Button* but = nullptr);
	void eventDontMoveComics(Button* but = nullptr);
	void eventMoveProgress(uptrt prg, uptrt lim);
	void eventMoveFinished();
	void eventSetFullscreen(Button* but);
	void eventSetHide(Button* but);
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
	void eventSetPicLimitType(Button* but);
	void eventSetPicLimCount(Button* but);
	void eventSetPicLimSize(Button* but);
	void eventResetSettings(Button* but);

	// other
	bool tryClosePopupThread();
	void eventClosePopup(Button* but = nullptr);
	void eventTryExit(Button* but = nullptr);
	void eventForceExit(Button* but = nullptr);
	
	Downloader* getDownloader();
	ProgState* getState();
	Browser* getBrowser();

private:
	void switchPictures(bool fwd, const string& picname);
	void offerMoveBooks(const string& oldLib);
	void setState(ProgState* newState);
	void reposizeWindow(vec2i dres, vec2i wsiz);
};

inline Program::Program() :
	state(new ProgState)	// necessary as a placeholder to prevent nullptr exceptions
{}

inline void Program::eventOpenBookList(Button*) {
	setState(new ProgBooks);
}

inline void Program::eventOpenSettings(Button*) {
	setState(new ProgSettings);
}

inline ProgState* Program::getState() {
	return state.get();
}

inline Browser* Program::getBrowser() {
	return browser.get();
}

inline Downloader* Program::getDownloader() {
	return &downloader;
}
