#pragma once

#include "browser.h"
#include "downloader.h"

// handles the front-end
class Program {
private:
	static constexpr float zoomFactor = 1.2f;
	static constexpr float resModeBorder = 0.85f;
	static constexpr float resModeRatio = 0.75f;

	Downloader downloader;
	ProgState* state = nullptr;
	uptr<Browser> browser;
	std::thread thread;
	std::atomic_bool threadRunning;

public:
	~Program();

	void start();

	// books
	void eventOpenBookList(Button* but = nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenBookContext(Button* but);
	void eventOpenLastPage(Button* but = nullptr);
	void eventDeleteBook(Button* but = nullptr);
	bool openFile(const fs::path& file);

	// browser
	void eventBrowserGoUp(Button* but = nullptr);
	void eventBrowserGoIn(Button* but);
	void eventBrowserGoFile(Button* but);
	void eventBrowserGoTo(Button* but);
	void eventPreviewProgress(const SDL_UserEvent& user);
	void eventExitBrowser(Button* but = nullptr);

	// reader
	void eventStartLoadingReader(const string& first, bool fwd = true);
	void eventReaderLoadingCancelled(Button* but = nullptr);
	void eventReaderProgress(const SDL_UserEvent& user);
	void eventReaderFinished(const SDL_UserEvent& user);
	void eventZoomIn(Button* but = nullptr);
	void eventZoomOut(Button* but = nullptr);
	void eventZoomReset(Button* but = nullptr);
	void eventCenterView(Button* but = nullptr);
	void eventNextDir(Button* but = nullptr);
	void eventPrevDir(Button* but = nullptr);
	void eventExitReader(Button* but = nullptr);

#ifdef DOWNLOADER
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
	void eventDownloadProgress();
	void eventDownloadNext();
	void eventDownloadFinish();
	void eventDownloadDelete(Button* but);
	void eventResumeDownloads(Button* but = nullptr);
	void eventStopDownloads(Button* but = nullptr);
	void eventClearDownloads(Button* but = nullptr);
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
	void eventMoveProgress(const SDL_UserEvent& user);
	void eventMoveFinished();
	void eventSetScreenMode(Button* but);
	void eventSetRenderer(Button* but);
	void eventSetDevice(Button* but);
	void eventSetCompression(Button* but);
	void eventSetVsync(Button* but);
	void eventSetGpuSelecting(Button* but);
	void eventSetMultiFullscreen(Button* but);
	void eventSetPreview(Button* but);
	void eventSetHide(Button* but);
	void eventSetTooltips(Button* but);
	void eventSetTheme(Button* but);
	void eventSetFont(Button* but);
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
	void eventCloseContext(Button* but = nullptr);
	void eventResizeComboContext(Layout* lay = nullptr);
	void eventTryExit(Button* but = nullptr);
	void eventForceExit(Button* but = nullptr);

	Downloader* getDownloader();
	ProgState* getState();
	Browser* getBrowser();

private:
	void switchPictures(bool fwd, string_view picname);
	void offerMoveBooks(fs::path&& oldLib);
	static sizet finishComboBox(Button* but);
	template <class T, class... A> void setState(A&&... args);
	void reposizeWindow(ivec2 dres, ivec2 wsiz);
};

inline ProgState* Program::getState() {
	return state;
}

inline Browser* Program::getBrowser() {
	return browser.get();
}

inline Downloader* Program::getDownloader() {
	return &downloader;
}
