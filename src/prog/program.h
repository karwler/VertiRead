#pragma once

#include "browser.h"
#include <semaphore>

// handles the front-end
class Program {
private:
	ProgState* state = nullptr;
	Browser browser;
#ifdef CAN_SECRET
	optional<CredentialManager*> credential;	// loaded lazily
#endif
	ArchiveData* archiveRequest;
	std::binary_semaphore* archiveRequestDone;
	Popup* prevPopup;	// might wanna implement a popup stack instead

public:
	~Program();

	void start(const vector<string>& cmdVals);
	void tick();
	ProgState* getState() { return state; }
	Browser* getBrowser() { return &browser; }
	bool canStoreCredentials() const;

	void handleGeneralEvent(const SDL_UserEvent& event);
	void handleProgBooksEvent(const SDL_UserEvent& event);
	void eventListFinished(const SDL_UserEvent& event);
	void eventDeleteFinished(const SDL_UserEvent& event);
	void handleProgFileExplorerEvent(const SDL_UserEvent& event);
	void handleProgPageBrowserEvent(const SDL_UserEvent& event);
	void eventArchiveFinished(const SDL_UserEvent& event);
	void handleThreadPreviewEvent(const SDL_UserEvent& event);
	void handleProgReaderEvent(const SDL_UserEvent& event);
	void eventGoNextFinished(const SDL_UserEvent& event);
	void handleThreadReaderEvent(const SDL_UserEvent& event);
	void handleProgSettingsEvent(const SDL_UserEvent& event);
	void handleThreadMoveEvent(const SDL_UserEvent& event);
	void eventFontsFinished(const SDL_UserEvent& event);
	void handleProgSearchDirEvent(const SDL_UserEvent& event);

	void eventOpenBookList();
	void openFile(const char* file);
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
	void setPopupProgress(Cstring&& msg = "Loading...");
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
	void eventBrowserGoIn(PushButton* lbl);
	void eventBrowserGoFile(PushButton* lbl);
	void eventBrowserGoTo(LabelEdit* le);
	void eventBrowserGoToLogin();
	void eventPreviewProgress(char* ndata, SDL_Surface* icon);
	void eventExitBrowser();

	// reader
	void eventReaderProgress(BrowserPictureProgress* pp, char* text);
	void eventReaderFinished(ResultCode rc, BrowserResultPicture* rp);

	// settings
	void eventSetLibraryDirBw();
	void eventOpenLibDirBrowser();
	void eventMoveBooks();
	void eventMoveCancelled();
	void eventMoveFinished(string* errors);
	void eventSetZoomType(PushButton* but);
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
	void eventSetPicLimType(PushButton* but);
	void eventSetPicLimCount(LabelEdit* le);
	void eventSetPicLimSize(LabelEdit* le);
	void eventSetMaxPicRes(Slider* sl);
	void eventSetMaxPicRes(LabelEdit* le);
	void eventResetSettings();

	// other
	void eventConfirmComboBox(PushButton* cbut);
	void eventResizeComboContext(Context* ctx);
	void eventStartRequestPassphrase(ArchiveData* ad, std::binary_semaphore* done);
	void eventRequestPassphrase();
	void eventSetPassphrase(bool ok);

	template <class... A> void openBookListHandle(A&&... args);
	template <class... A> void openFileHandle(A&&... args);
	template <class... A> void browserGoToHandle(A&&... args);
	template <Invocable<const RemoteLocation&, vector<string>&&> F> void browserLoginAuto(RemoteLocation&& rl, EventId kcal, F func);
	template <Invocable<const RemoteLocation&> F> void browserLoginManual(F func);
	void startBrowserPreview();
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
