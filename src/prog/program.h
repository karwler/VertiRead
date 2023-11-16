#pragma once

#include "browser.h"

// handles the front-end
class Program {
private:
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
	ProgState* getState() { return state; }
	Browser* getBrowser() { return &browser; }
	bool canStoreCredentials() const;

	void handleGeneralEvent(const SDL_UserEvent& event);
	void handleProgBooksEvent(const SDL_UserEvent& event);
	void handleProgFileExplorerEvent(const SDL_UserEvent& event);
	void handleProgPageBrowserEvent(const SDL_UserEvent& event);
	void handleThreadArchiveEvent(const SDL_UserEvent& event);
	void handleThreadPreviewEvent(const SDL_UserEvent& event);
	void handleProgReaderEvent(const SDL_UserEvent& event);
	void handleThreadReaderEvent(const SDL_UserEvent& event);
	void handleProgSettingsEvent(const SDL_UserEvent& event);
	void handleThreadMoveEvent(const SDL_UserEvent& event);
	void eventFontsFinished(const SDL_UserEvent& event);
	void handleProgSearchDirEvent(const SDL_UserEvent& event);

	void eventOpenBookList();
	void openFile(string_view file);
	void eventBrowserGoUp();
	void eventZoomIn();
	void eventZoomOut();
	void eventZoomReset();
	void eventZoomFit();
	void eventCenterView();
	void eventNextDir();
	void eventPrevDir();
	void eventExitReader();
	void setLibraryDir(string_view path, bool byText = false);
	void setFont(const fs::path& font);
	void setPopupLoading();
	void eventExit();

private:
	// books
	void eventOpenBookListLogin();
	void eventOpenPageBrowser(PushButton* lbl);
	void eventOpenPageBrowserGeneral();
	void eventOpenBookContext(Widget* wgt);
	void eventOpenBookContextGeneral(Widget* wgt);
	void eventOpenLastPage();
	void eventOpenLastPageGeneral();
	void eventAskDeleteBook();
	void eventDeleteBook();
	void eventQueryRenameBook();
	void eventRenameBook();
	void eventOpenFileLogin();
	void eventOpenSettings();

	// browser
	void eventArchiveFinished(BrowserResultAsync* ra);
	void eventFileLoadingCancelled();
	void eventBrowserGoIn(PushButton* lbl);
	void eventBrowserGoFile(PushButton* lbl);
	void eventBrowserGoTo(LabelEdit* le);
	void eventBrowserGoToLogin();
	void eventPreviewProgress(char* ndata, SDL_Surface* icon);
	void eventExitBrowser();

	// reader
	void eventReaderProgress(BrowserPictureProgress* pp, char* text);
	void eventReaderFinished(BrowserResultPicture* rp, bool fwd);

	// settings
	void eventSetLibraryDirBw();
	void eventOpenLibDirBrowser();
	void eventMoveBooks();
	void eventMoveCancelled();
	void eventMoveFinished(string* errors);
	void eventSetZoom(Slider* sl);
	void eventSetScreenMode(PushButton* but);
	void eventSetRenderer(PushButton* but);
	void eventSetDevice(PushButton* but);
	void eventSetCompression(PushButton* but);
	void eventSetVsync(CheckBox* cb);
	void eventSetMultiFullscreen(WindowArranger* wa);
	void eventSetTheme(PushButton* lbl);
	void eventSetFont(PushButton* but);
	void eventSetFontHinting(PushButton* but);
	void eventSetDeadzone(Slider* sl);
	void eventSetDeadzone(LabelEdit* le);
	void eventSetPortrait();
	void eventSetLandscape();
	void eventSetSquare();
	void eventSetFill();
	void eventSetPicLimitType(PushButton* but);
	void eventSetPicLimCount(LabelEdit* le);
	void eventSetPicLimSize(LabelEdit* le);
	void eventSetMaxPicRes(Slider* sl);
	void eventSetMaxPicRes(LabelEdit* le);
	void eventResetSettings();

	// other
	void eventConfirmComboBox(PushButton* cbut);
	void eventResizeComboContext(Context* ctx);

private:
	template <class... A> void openBookListHandle(A&&... args);
	template <class... A> void openFileHandle(A&&... args);
	template <class... A> void browserGoToHandle(A&&... args);
	template <Invocable<const RemoteLocation&, vector<string>&&> F> void browserLoginAuto(RemoteLocation&& rl, EventId kcal, F func);
	template <Invocable<const RemoteLocation&> F> void browserLoginManual(F func);
	void switchPictures(bool fwd, string_view picname);
	static uint finishComboBox(PushButton* but);
	template <Derived<ProgState> T, class... A> void setState(A&&... args);
};

inline bool Program::canStoreCredentials() const {
#ifdef CAN_SECRET
	return credential && *credential;
#else
	return false;
#endif
}
