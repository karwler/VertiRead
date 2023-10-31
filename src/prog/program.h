#pragma once

#include "browser.h"

// handles the front-end
class Program {
private:
	static constexpr float zoomFactor = 1.2f;
	static constexpr float resModeBorder = 0.85f;
	static constexpr float resModeRatio = 0.75f;

	ProgState* state = nullptr;
	Browser browser;
#ifdef CAN_SECRET
	optional<CredentialManager*> credential;	// loaded lazily
#endif

public:
	~Program();

	void start(const vector<string>& cmdVals);
	void tick();
	template <MemberFunction F, class... A> void exec(F func, A&&... args);

	// books
	void eventOpenBookList(Button* but = nullptr);
	void eventOpenBookListLogin(Button* but = nullptr);
	void eventOpenPageBrowser(Button* but);
	void eventOpenPageBrowserGeneral(Button* but = nullptr);
	void eventOpenBookContext(Button* but);
	void eventOpenBookContextGeneral(Button* but);
	void eventOpenLastPage(Button* but = nullptr);
	void eventOpenLastPageGeneral(Button* but = nullptr);
	void eventAskDeleteBook(Button* but = nullptr);
	void eventDeleteBook(Button* but = nullptr);
	void eventQueryRenameBook(Button* but = nullptr);
	void eventRenameBook(Button* but = nullptr);
	void openFile(const char* file);
	void eventOpenFileLogin(Button* but = nullptr);

	// browser
	void eventArchiveProgress(const SDL_UserEvent& user);
	void eventArchiveFinished(const SDL_UserEvent& user);
	void eventFileLoadingCancelled(Button* but = nullptr);
	void eventBrowserGoUp(Button* but = nullptr);
	void eventBrowserGoIn(Button* but);
	void eventBrowserGoFile(Button* but);
	void eventBrowserGoTo(Button* but);
	void eventBrowserGoToLogin(Button* but = nullptr);
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
	void setLibraryDir(string_view path, bool byText = false);
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
	void eventConfirmComboBox(Button* but);
	void eventResizeComboContext(Layout* lay = nullptr);
	void eventExit(Button* but = nullptr);
	void setPopupLoading();

	ProgState* getState();
	Browser* getBrowser();
	bool canStoreCredentials() const;

private:
	template <class... A> void openBookListHandle(A&&... args);
	template <class... A> void openFileHandle(A&&... args);
	template <class... A> void browserGoToHandle(A&&... args);
	template <Invocable<const RemoteLocation&, vector<string>&&> F> void browserLoginAuto(RemoteLocation&& rl, PCall kcal, F func);
	template <Invocable<const RemoteLocation&> F> void browserLoginManual(F func);
	void switchPictures(bool fwd, string_view picname);
	static uint finishComboBox(Button* but);
	template <Derived<ProgState> T, class... A> void setState(A&&... args);
	void reposizeWindow(ivec2 dres, ivec2 wsiz);
};

template <MemberFunction F, class... A>
void Program::exec(F func, A&&... args) {
	if (func)
		(this->*func)(std::forward<A>(args)...);
}

inline ProgState* Program::getState() {
	return state;
}

inline Browser* Program::getBrowser() {
	return &browser;
}

inline bool Program::canStoreCredentials() const {
#ifdef CAN_SECRET
	return credential && *credential;
#else
	return false;
#endif
}
