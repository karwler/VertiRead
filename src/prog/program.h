#pragma once

#include "browser.h"
#include <semaphore>

// handles the front-end
class Program {
private:
	enum class InitState : uint8 {
		none,
		done,
		error
	};

#ifdef WITH_FTP
	static constexpr Protocol defaultProtocolSelect = Protocol::ftp;
#elif defined(CAN_SFTP)
	static constexpr Protocol defaultProtocolSelect = Protocol::sftp;
#elif defined(CAN_SMB)
	static constexpr Protocol defaultProtocolSelect = Protocol::smb;
#else
	static constexpr Protocol defaultProtocolSelect = Protocol::none;
#endif

	ProgState* state = nullptr;
	Browser browser;
#ifdef WITH_ARCHIVE
	ArchiveData* archiveRequest;
	std::binary_semaphore* archiveRequestDone;
#endif
	Popup* prevPopup;	// might wanna implement a popup stack instead
#ifdef CAN_SECRET
	CredentialManager* credential;	// loaded lazily
	InitState credentialState = InitState::none;
#endif

public:
	~Program();

	void start();
	void tick();
	ProgState* getState() { return state; }
	Browser* getBrowser() { return &browser; }
	bool canStoreCredentials();

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
	void eventBrowserOpenLogin();
	void eventPreviewProgress(uptr<char[]> ndata, SDL_Surface* icon);
	void eventExitBrowser();

	// reader
	void eventReaderProgress(uptr<BrowserPictureProgress> pp);
	void eventReaderFinished(ResultCode rc, uptr<BrowserResultPicture> rp);

	// settings
	void eventSetLibraryDirBw();
	void eventOpenLibDirBrowser();
	void eventMoveBooks();
	void eventMoveCancelled();
	void eventMoveFinished(uptr<string> errors);
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
	void eventSetMonoFont(CheckBox* cb);
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
	void eventSetLoginPopupProtocol(PushButton* but);

	template <class... A> void openBookListHandle(A&&... args);
	template <class... A> void openFileHandle(A&&... args);
	template <class... A> void browserGoToHandle(A&&... args);
	template <Invocable<const RemoteLocation&, vector<string>&&> F> void browserLoginAuto(RemoteLocation&& rl, EventId kcal, F func);
	template <Invocable<const RemoteLocation&> F> void browserLoginManual(F func);
	void restartBrowserList();
	void startBrowserPreview();
	static uint finishComboBox(PushButton* but);
	template <Derived<ProgState> T, class... A> void setState(A&&... args);
#ifdef CAN_SECRET
	bool lazyInitCredentials();
#endif
};

inline bool Program::canStoreCredentials() {
#ifdef CAN_SECRET
	return lazyInitCredentials();
#else
	return false;
#endif
}
