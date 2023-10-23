#pragma once

#include "browser.h"

// handles the front-end
class Program {
private:
	static constexpr float zoomFactor = 1.2f;
	static constexpr float resModeBorder = 0.85f;
	static constexpr float resModeRatio = 0.75f;

	ProgState* state = nullptr;
	uptr<Browser> browser;

public:
	~Program();

	void start(const vector<string>& cmdVals);
	void tick();

	// books
	void eventOpenBookList(Button* but = nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenPageBrowserGeneral(Button* but = nullptr);
	void eventOpenBookContext(Button* but);
	void eventOpenBookContextGeneral(Button* but);
	void eventOpenLastPage(Button* but = nullptr);
	void eventOpenLastPageGeneral(Button* but = nullptr);
	void eventDeleteBook(Button* but = nullptr);
	void openFile(const char* file);

	// browser
	void eventArchiveProgress(const SDL_UserEvent& user);
	void eventArchiveFinished(const SDL_UserEvent& user);
	void eventFileLoadingCancelled(Button* but = nullptr);
	void eventBrowserGoUp(Button* but = nullptr);
	void eventBrowserGoIn(Button* but);
	void eventBrowserGoFile(Button* but);
	void eventBrowserGoTo(Button* but);
	void eventPreviewProgress(const SDL_UserEvent& user);
	void eventPreviewFinished();
	void eventExitBrowser(Button* but = nullptr);

	// reader
	void eventReaderProgress(const SDL_UserEvent& user);
	void eventReaderFinished(const SDL_UserEvent& user);
	void eventZoomIn(Button* but = nullptr);
	void eventZoomOut(Button* but = nullptr);
	void eventZoomReset(Button* but = nullptr);
	void eventCenterView(Button* but = nullptr);
	void eventNextDir(Button* but = nullptr);
	void eventPrevDir(Button* but = nullptr);
	void eventExitReader(Button* but = nullptr);

	// settings
	void eventOpenSettings(Button* but = nullptr);
	void eventSwitchDirection(Button* but);
	void eventSetZoom(Button* but);
	void eventSetSpacing(Button* but);
	void eventSetLibraryDirLE(Button* but);
	void eventSetLibraryDirBW(Button* but);
	void eventOpenLibDirBrowser(Button* but = nullptr);
	void eventMoveComics(Button* but = nullptr);
	void eventMoveCancelled(Button* but = nullptr);
	void eventMoveProgress(const SDL_UserEvent& user);
	void eventMoveFinished(const SDL_UserEvent& user);
	void eventFontsFinished(const SDL_UserEvent& user);
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
	void eventSetFontCMB(Button* but);
	void eventSetFontLE(Button* but);
	void eventSetFontHinting(Button* but);
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
	void eventSetMaxPicResSL(Button* but);
	void eventSetMaxPicResLE(Button* but);
	void eventResetSettings(Button* but);

	// other
	void eventClosePopup(Button* but = nullptr);
	void eventCloseContext(Button* but = nullptr);
	void eventResizeComboContext(Layout* lay = nullptr);
	void eventExit(Button* but = nullptr);
	void setPopupLoading();

	ProgState* getState();
	Browser* getBrowser();

private:
	void switchPictures(bool fwd, string_view picname);
	void offerMoveBooks(fs::path&& oldLib);
	static uint finishComboBox(Button* but);
	template <Derived<ProgState> T, class... A> void setState(A&&... args);
	void reposizeWindow(ivec2 dres, ivec2 wsiz);
};

inline ProgState* Program::getState() {
	return state;
}

inline Browser* Program::getBrowser() {
	return browser.get();
}
