#include "program.h"
#include "fileOps.h"
#include "progs.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include "utils/compare.h"
#include "utils/layouts.h"

Program::~Program() {
	delete state;
#ifdef CAN_SECRET
	if (credential)
		delete *credential;
#endif
}

void Program::start(const vector<string>& cmdVals) {
	eventOpenBookList();
	if (!cmdVals.empty())
		openFile(cmdVals[0].data());
}

void Program::tick() {
	if (auto pe = dynamic_cast<ProgFileExplorer*>(state))
		pe->processFileChanges(&browser);
}

void Program::handleGeneralEvent(const SDL_UserEvent& event) {
	switch (GeneralEvent(event.code)) {
	using enum GeneralEvent;
	case closePopup:
		World::scene()->setPopup(nullptr);
		break;
	case closeContext:
		World::scene()->setContext(nullptr);
		break;
	case confirmComboBox:
		eventConfirmComboBox(static_cast<PushButton*>(event.data1));
		break;
	case resizeComboContext:
		eventResizeComboContext(static_cast<Context*>(event.data1));
		break;
	case startRequestPassphrase:
		eventStartRequestPassphrase(static_cast<ArchiveData*>(event.data1), static_cast<std::binary_semaphore*>(event.data2));
		break;
	case requestPassphrase:
		eventRequestPassphrase();
		break;
	case confirmPassphrase:
		eventSetPassphrase(true);
		break;
	case cancelPassphrase:
		eventSetPassphrase(false);
		break;
	case exit:
		eventExit();
	}
}

void Program::handleProgBooksEvent(const SDL_UserEvent& event) {
	Actions action = Actions(uintptr_t(event.data2));
	switch (ProgBooksEvent(event.code)) {
	using enum ProgBooksEvent;
	case openBookList:
		eventOpenBookList();
		break;
	case openBookListLogin:
		eventOpenBookListLogin();
		break;
	case openPageBrowser:
		if (action == ACT_LEFT)
			eventOpenPageBrowser(static_cast<PushButton*>(event.data1));
		else
			eventOpenBookContext(static_cast<Widget*>(event.data1));
		break;
	case openPageBrowserGeneral:
		if (action == ACT_LEFT)
			eventOpenPageBrowserGeneral();
		else
			eventOpenBookContextGeneral(static_cast<Widget*>(event.data1));
		break;
	case openLastPage:
		eventOpenLastPage();
		break;
	case openLastPageGeneral:
		eventOpenLastPageGeneral();
		break;
	case askDeleteBook:
		eventAskDeleteBook();
		break;
	case deleteBook:
		eventDeleteBook();
		break;
	case queryRenameBook:
		eventQueryRenameBook();
		break;
	case renameBook:
		eventRenameBook();
		break;
	case openFileLogin:
		eventOpenFileLogin();
		break;
	case openSettings:
		eventOpenSettings();
	}
}

void Program::handleProgFileExplorerEvent(const SDL_UserEvent& event) {
	switch (ProgFileExplorerEvent(event.code)) {
	using enum ProgFileExplorerEvent;
	case goUp:
		eventBrowserGoUp();
		break;
	case goIn:
		eventBrowserGoIn(static_cast<PushButton*>(event.data1));
		break;
	case goTo:
		eventBrowserGoTo(static_cast<LabelEdit*>(event.data1));
		break;
	case goToLogin:
		eventBrowserGoToLogin();
		break;
	case exit:
		eventExitBrowser();
	}
}

void Program::handleProgPageBrowserEvent(const SDL_UserEvent& event) {
	switch (ProgPageBrowserEvent(event.code)) {
	using enum ProgPageBrowserEvent;
	case fileLoadingCancelled:
		browser.requestStop();
		break;
	case goFile:
		eventBrowserGoFile(static_cast<PushButton*>(event.data1));
	}
}

void Program::handleThreadPreviewEvent(const SDL_UserEvent& event) {
	switch (ThreadEvent(event.code)) {
	using enum ThreadEvent;
	case progress:
		eventPreviewProgress(static_cast<char*>(event.data1), static_cast<SDL_Surface*>(event.data2));
		break;
	case finished:
		browser.stopThread();
	}
}

void Program::handleProgReaderEvent(const SDL_UserEvent& event) {
	switch (ProgReaderEvent(event.code)) {
	using enum ProgReaderEvent;
	case zoomIn:
		eventZoomIn();
		break;
	case zoomOut:
		eventZoomOut();
		break;
	case zoomReset:
		eventZoomReset();
		break;
	case zoomFit:
		eventZoomFit();
		break;
	case centerView:
		eventCenterView();
		break;
	case nextDir:
		eventNextDir();
		break;
	case prevDir:
		eventPrevDir();
		break;
	case exit:
		eventExitReader();
	}
}

void Program::handleThreadReaderEvent(const SDL_UserEvent& event) {
	switch (ThreadEvent(event.code)) {
	using enum ThreadEvent;
	case progress:
		eventReaderProgress(static_cast<BrowserPictureProgress*>(event.data1), static_cast<char*>(event.data2));
		break;
	case finished:
		eventReaderFinished(ResultCode(uintptr_t(event.data1)), static_cast<BrowserResultPicture*>(event.data2));
	}
}

void Program::handleProgSettingsEvent(const SDL_UserEvent& event) {
	switch (ProgSettingsEvent(event.code)) {
	using enum ProgSettingsEvent;
	case setDirection:
		World::sets()->direction = Direction::Dir(finishComboBox(static_cast<PushButton*>(event.data1)));
		break;
	case setZoomType:
		eventSetZoomType(static_cast<PushButton*>(event.data1));
		break;
	case setZoom:
		eventSetZoom(static_cast<Slider*>(event.data1));
		break;
	case setSpacing:
		World::sets()->spacing = toNum<ushort>(static_cast<LabelEdit*>(event.data1)->getText());
		break;
	case setLibraryDirLe:
		setLibraryDir(static_cast<LabelEdit*>(event.data1)->getText(), true);
		break;
	case openLibDirBrowser:
		eventOpenLibDirBrowser();
		break;
	case moveBooks:
		eventMoveBooks();
		break;
	case moveCancelled:
		eventMoveCancelled();
		break;
	case setScreenMode:
		eventSetScreenMode(static_cast<PushButton*>(event.data1));
		break;
	case setRenderer:
		eventSetRenderer(static_cast<PushButton*>(event.data1));
		break;
	case setDevice:
		eventSetDevice(static_cast<PushButton*>(event.data1));
		break;
	case setCompression:
		eventSetCompression(static_cast<PushButton*>(event.data1));
		break;
	case setVsync:
		eventSetVsync(static_cast<CheckBox*>(event.data1));
		break;
	case setGpuSelecting:
		World::sets()->gpuSelecting = static_cast<CheckBox*>(event.data1)->on;
		break;
	case setMultiFullscreen:
		eventSetMultiFullscreen(static_cast<WindowArranger*>(event.data1));
		break;
	case setPreview:
		World::sets()->preview = static_cast<CheckBox*>(event.data1)->on;
		break;
	case setHide:
		World::sets()->showHidden = static_cast<CheckBox*>(event.data1)->on;
		break;
	case setTooltips:
		World::sets()->tooltips = static_cast<CheckBox*>(event.data1)->on;
		break;
	case setTheme:
		eventSetTheme(static_cast<PushButton*>(event.data1));
		break;
	case setFontCmb:
		eventSetFont(static_cast<PushButton*>(event.data1));
		break;
	case setFontLe:
		setFont(toPath(static_cast<LabelEdit*>(event.data1)->getText()));
		break;
	case setFontHinting:
		eventSetFontHinting(static_cast<PushButton*>(event.data1));
		break;
	case setScrollSpeed:
		World::sets()->scrollSpeed = toVec<vec2>(static_cast<LabelEdit*>(event.data1)->getText());
		break;
	case setDeadzoneSl:
		eventSetDeadzone(static_cast<Slider*>(event.data1));
		break;
	case setDeadzoneLe:
		eventSetDeadzone(static_cast<LabelEdit*>(event.data1));
		break;
	case setPicLimitType:
		eventSetPicLimType(static_cast<PushButton*>(event.data1));
		break;
	case setPicLimitCount:
		eventSetPicLimCount(static_cast<LabelEdit*>(event.data1));
		break;
	case setPicLimitSize:
		eventSetPicLimSize(static_cast<LabelEdit*>(event.data1));
		break;
	case setMaxPicResSl:
		eventSetMaxPicRes(static_cast<Slider*>(event.data1));
		break;
	case setMaxPicResLe:
		eventSetMaxPicRes(static_cast<LabelEdit*>(event.data1));
		break;
	case reset:
		eventResetSettings();
	}
}

void Program::handleThreadMoveEvent(const SDL_UserEvent& event) {
	switch (ThreadEvent(event.code)) {
	using enum ThreadEvent;
	case progress:
		state->updatePopupMessage(std::format("Moving {}/{}", uintptr_t(event.data1), uintptr_t(event.data2)));
		break;
	case finished:
		eventMoveFinished(static_cast<string*>(event.data1));
	}
}

void Program::handleProgSearchDirEvent(const SDL_UserEvent& event) {
	Actions action = Actions(uintptr_t(event.data2));
	switch (ProgSearchDirEvent(event.code)) {
	using enum ProgSearchDirEvent;
	case goIn:
		if (action == ACT_LEFT)
			static_cast<ProgSearchDir*>(state)->selected = static_cast<Button*>(event.data1)->toggleHighlighted() ? static_cast<PushButton*>(event.data1) : nullptr;
		else
			eventBrowserGoIn(static_cast<PushButton*>(event.data1));
		break;
	case setLibraryDirBw:
		eventSetLibraryDirBw();
	}
}

// BOOKS

void Program::eventOpenBookList() {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(World::sets()->dirLib))
			browserLoginAuto(std::move(*rl), ProgBooksEvent::openBookListLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { openBookListHandle(prl, std::move(pwds)); });
		else
			openBookListHandle(valcp(World::sets()->dirLib));
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
		eventOpenSettings();
	}
}

void Program::eventOpenBookListLogin() {
	browserLoginManual([this](const RemoteLocation& rl) { openBookListHandle(rl); });
}

template <class... A>
void Program::openBookListHandle(A&&... args) {
	browser.beginFs(string(), std::forward<A>(args)...);
	browser.exCall = &Program::eventOpenBookList;
	setState<ProgBooks>();
}

void Program::eventOpenPageBrowser(PushButton* lbl) {
	try {
		string root = World::sets()->dirLib / lbl->getText().data();
		browser.beginFs(valcp(root), valcp(root));	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventOpenPageBrowserGeneral() {
	try {
		browser.beginFs(string(), string());	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventListFinished(const SDL_UserEvent& event) {
	browser.stopThread();
	auto rl = static_cast<BrowserResultList*>(event.data2);
	auto pe = static_cast<ProgFileExplorer*>(state);
	if (ResultCode(uintptr_t(event.data1)) == ResultCode::ok) {
		pe->fillFileList(std::move(rl->files), std::move(rl->dirs));
		browser.setDirectoryWatch();
	} else
		pe->fillFileList(vector<Cstring>(), vector<Cstring>(), false);
	delete rl;
}

void Program::eventOpenBookContext(Widget* wgt) {
	vector<pair<Cstring, EventId>> items = {
		pair("Continue", ProgBooksEvent::openLastPage),
		pair("Rename", ProgBooksEvent::queryRenameBook),
		pair("Delete", ProgBooksEvent::askDeleteBook)
	};
	state->showContext(std::move(items), wgt);
}

void Program::eventOpenBookContextGeneral(Widget* wgt) {
	state->showContext({ pair("Continue", ProgBooksEvent::openLastPageGeneral) }, wgt);
}

void Program::eventOpenLastPage() {
	string_view bname = static_cast<PushButton*>(World::scene()->getContext()->owner())->getText().data();
	if (vector<string> paths = World::fileSys()->getLastPage(bname); !paths.empty()) {
		string root = World::sets()->dirLib / bname;
		paths[0] = !paths[0].empty() ? root / paths[0] : root;
		if (browser.openPicture(std::move(root), std::move(paths)))
			setPopupProgress();
		else
			state->showPopupMessage("Invalid last page entry");
	} else
		state->showPopupMessage("No last page entry");
}

void Program::eventOpenLastPageGeneral() {
	if (vector<string> paths = World::fileSys()->getLastPage(Browser::dotStr); !paths.empty()) {
		if (browser.openPicture(string(), std::move(paths)))
			setPopupProgress();
		else
			state->showPopupMessage("Invalid last page entry");
	} else
		state->showPopupMessage("No last page entry");
}

void Program::eventAskDeleteBook() {
	auto pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<PushButton>();
	state->showPopupChoice(std::format("Are you sure you want to delete '{}'?", pb->contextBook->getText().data()), ProgBooksEvent::deleteBook);
}

void Program::eventDeleteBook() {
	if (browser.startDeleteEntry(static_cast<ProgBooks*>(state)->contextBook->getText().data()))
		setPopupProgress("Deleting...");
}

void Program::eventDeleteFinished(const SDL_UserEvent& event) {
	browser.stopThread();
	auto pb = static_cast<ProgBooks*>(state);
	switch (ResultCode(uintptr_t(event.data1))) {
	using enum ResultCode;
	case ok:
		pb->contextBook->getParent()->deleteWidget(pb->contextBook->getIndex());
	case stop:
		World::scene()->setPopup(nullptr);
		break;
	case error:
		state->showPopupMessage("Failed to delete all files");
	}
}

void Program::eventQueryRenameBook() {
	auto pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<PushButton>();
	state->showPopupInput("Rename book", pb->contextBook->getText().data(), ProgBooksEvent::renameBook, GeneralEvent::closePopup, true, "Rename");
}

void Program::eventRenameBook() {
	auto pb = static_cast<ProgBooks*>(state);
	const string& newName = ProgState::inputFromPopup();
	if (browser.renameEntry(pb->contextBook->getText().data(), newName)) {
		auto box = static_cast<TileBox*>(pb->contextBook->getParent());
		box->deleteWidget(pb->contextBook->getIndex());
		std::span<Widget*> wgts = box->getWidgets();
		uint id = std::lower_bound(wgts.begin(), wgts.end(), newName, [](const Widget* a, const string& b) -> bool { return Strcomp::less(static_cast<const PushButton*>(a)->getText().data(), b.data()); }) - wgts.begin();
		box->insertWidget(id, pb->makeBookTile(newName));
		World::scene()->setPopup(nullptr);
	} else
		state->showPopupMessage("Failed to rename");
}

void Program::openFile(const char* file) {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(file))
			browserLoginAuto(std::move(*rl), ProgBooksEvent::openFileLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { openFileHandle(prl, std::move(pwds)); });
		else
			openFileHandle(file);
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventOpenFileLogin() {
	browserLoginManual([this](const RemoteLocation& rl) { openFileHandle(rl); });
}

template <class... A>
void Program::openFileHandle(A&&... args) {
	if (browser.goTo(std::forward<A>(args)...)) {
		if (auto pb = dynamic_cast<ProgPageBrowser*>(state))
			pb->resetFileIcons();
		setPopupProgress();
	} else
		setState<ProgPageBrowser>();
}

void Program::eventOpenSettings() {
	setState<ProgSettings>();
}

// BROWSER

void Program::eventArchiveFinished(const SDL_UserEvent& event) {
	browser.stopThread();
	auto ra = static_cast<BrowserResultArchive*>(event.data1);
	switch (ra->rc) {
	using enum ResultCode;
	case ok:
		if (browser.finishArchive(std::move(*ra)))
			setState<ProgPageBrowser>();
		else
			setPopupProgress();
		break;
	case stop:
		startBrowserPreview();
		World::scene()->setPopup(nullptr);
		break;
	case error:
		startBrowserPreview();
		state->showPopupMessage(!ra->error.empty() ? ra->error : "Failed to load archive");
	}
	delete ra;
}

void Program::startBrowserPreview() {
	if (auto pe = dynamic_cast<ProgFileExplorer*>(state))
		if (World::sets()->preview)
			browser.startPreview(pe->getLineHeight());
}

void Program::eventBrowserGoUp() {
	if (browser.goUp())
		World::scene()->resetLayouts();
	else
		eventExitBrowser();
}

void Program::eventBrowserGoIn(PushButton* lbl) {
	if (browser.goIn(lbl->getText().data()))
		World::scene()->resetLayouts();
}

void Program::eventBrowserGoFile(PushButton* lbl) {
	if (browser.goFile(lbl->getText().data())) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupProgress();
	}
}

void Program::eventBrowserGoTo(LabelEdit* le) {
	try {
		string path = browser.prepareNavigationPath(le->getText());
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(path))
			browserLoginAuto(std::move(*rl), ProgFileExplorerEvent::goToLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { browserGoToHandle(prl, std::move(pwds)); });
		else
			browserGoToHandle(path);
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventBrowserGoToLogin() {
	browserLoginManual([this](const RemoteLocation& rl) { browserGoToHandle(rl); });
}

template <class... A>
void Program::browserGoToHandle(A&&... args) {
	if (browser.goTo(std::forward<A>(args)...)) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupProgress();
	} else
		World::scene()->resetLayouts();
}

template <Invocable<const RemoteLocation&, vector<string>&&> F>
void Program::browserLoginAuto(RemoteLocation&& rl, EventId kcal, F func) {
#ifdef CAN_SECRET
	try {
		if (!credential || *credential) {
			if (!credential) {
				credential = nullptr;
				credential = new CredentialManager;
			}
			if (vector<string> passwords = (*credential)->loadPasswords(rl); !passwords.empty())
				func(rl, std::move(passwords));
		}
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
#endif
	state->showPopupRemoteLogin(std::move(rl), kcal);
}

template <Invocable<const RemoteLocation&> F>
void Program::browserLoginManual(F func) {
	try {
		auto [rl, save] = ProgState::remoteLocationFromPopup();
		func(rl);
#ifdef CAN_SECRET
		if (save)
			(*credential)->saveCredentials(rl);
#endif
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventPreviewProgress(char* ndata, SDL_Surface* icon) {
	Renderer* renderer = World::drawSys()->getRenderer();
	if (Texture* tex = renderer->texFromIcon(icon)) {
		auto pe = static_cast<ProgFileExplorer*>(state);
		std::span<Widget*> wgts = pe->fileList->getWidgets();
		auto [pos, end] = ndata[0] ? pair(wgts.begin() + pe->dirEnd, wgts.begin() + pe->fileEnd) : pair(wgts.begin(), wgts.begin() + pe->dirEnd);
		char* name = ndata + 1;
		if (std::span<Widget*>::iterator it = std::lower_bound(pos, end, name, [](const Widget* a, const char* b) -> bool { return Strcomp::less(static_cast<const PushButton*>(a)->getText().data(), b); }); it != end && static_cast<PushButton*>(*it)->getText() == name) {
			static_cast<IconPushButton*>(*it)->setIcon(tex);
			renderer->synchTransfer();
		} else {
			renderer->synchTransfer();
			renderer->freeTexture(tex);
		}
	}
	delete[] ndata;
}

void Program::eventExitBrowser() {
	(this->*browser.exCall)();
}

// READER

void Program::eventReaderProgress(BrowserPictureProgress* pp, char* text) {
	pp->pnt->mpic.lock();
	pp->pnt->pics[pp->id].second = World::drawSys()->getRenderer()->texFromRpic(pp->img);
	pp->pnt->mpic.unlock();
	delete pp;

	state->updatePopupMessage(std::format("Loading {}", text));
	delete[] text;
}

void Program::eventReaderFinished(ResultCode rc, BrowserResultPicture* rp) {
	browser.stopThread();
	switch (rc) {
	using enum ResultCode;
	case ok:
		if (rp->arch)	// when loading from an archive then the pictures will likely be out of order
			rng::sort(rp->pics, [](const pair<Cstring, Texture*>& a, const pair<Cstring, Texture*>& b) -> bool { return Strcomp::less(a.first.data(), b.first.data()); });
		else if (!rp->fwd)
			rng::reverse(rp->pics);	// when loading from a directory or pdf backwards then the pictures are gonna be sorted but in reverse order

		browser.finishLoadPictures(*rp);
		setState<ProgReader>();
		static_cast<ProgReader*>(state)->reader->setPictures(rp->pics, rp->picname, rp->fwd);
		break;
	case stop:
		startBrowserPreview();
		World::scene()->setPopup(nullptr);
		break;
	case error:
		startBrowserPreview();
		state->showPopupMessage(!rp->error.empty() ? rp->error : "Failed to load pictures");
	}
	delete rp;
}

void Program::eventZoomIn() {
	static_cast<ProgReader*>(state)->reader->addZoom(1);
}

void Program::eventZoomOut() {
	static_cast<ProgReader*>(state)->reader->addZoom(-1);
}

void Program::eventZoomReset() {
	static_cast<ProgReader*>(state)->reader->setZoom(0);
}

void Program::eventZoomFit() {
	auto rb = static_cast<ProgReader*>(state)->reader;
	int di = rb->direction.horizontal();
	uint maxRes = 0;
	for (const Widget* it : rb->getWidgets())
		if (uint res = static_cast<const Picture*>(it)->getTex()->getRes()[di]; res > maxRes)
			maxRes = res;
	if (maxRes)
		rb->setZoom(rb->zoomStepToFit(maxRes));
}

void Program::eventCenterView() {
	static_cast<ProgReader*>(state)->reader->centerList();
}

void Program::eventNextDir() {
	browser.startGoNext(static_cast<ProgReader*>(state)->reader->lastPage(), true);
	setPopupProgress();
}

void Program::eventPrevDir() {
	browser.startGoNext(static_cast<ProgReader*>(state)->reader->firstPage(), false);
	setPopupProgress();
}

void Program::eventGoNextFinished(const SDL_UserEvent& event) {
	browser.stopThread();
	auto rp = static_cast<BrowserResultPicture*>(event.data2);
	switch (ResultCode(uintptr_t(event.data1))) {
	using enum ResultCode;
	case ok:
		browser.startLoadPictures(rp);
		return;
	case stop:
		World::scene()->setPopup(nullptr);
		break;
	case error:
		state->showPopupMessage("No more images");
	}
	delete rp;
}

void Program::eventExitReader() {
	state->eventClosing();
	browser.exitFile();
	setState<ProgPageBrowser>();
}

// SETTINGS

void Program::eventSetLibraryDirBw() {
	auto sel = static_cast<ProgSearchDir*>(state)->selected;
	setLibraryDir(sel ? browser.getCurDir() / sel->getText().data() : browser.getCurDir());
}

void Program::setLibraryDir(string_view path, bool byText) {
	auto ps = dynamic_cast<ProgSettings*>(state);
	bool dstLocal = RemoteLocation::getProtocol(path) == Protocol::none;
	if (std::error_code ec; dstLocal && (!fs::is_directory(toPath(path), ec) || ec)) {	// TODO: establish a new browser connection if remote and check if it's a valid directory
		if (byText)
			ps->libraryDir->setText(World::sets()->dirLib);
		state->showPopupMessage("Invalid directory");
		return;
	}

	string oldLib = std::move(World::sets()->dirLib);
	World::sets()->dirLib = path;
	if (ps) {
		if (!byText)
			ps->libraryDir->setText(World::sets()->dirLib);
	} else
		eventOpenSettings();

	if (oldLib != World::sets()->dirLib && RemoteLocation::getProtocol(oldLib) == Protocol::none && dstLocal) {
		ps->oldPathBuffer = std::move(oldLib);
		state->showPopupChoice("Move books to new location?", ProgSettingsEvent::moveBooks);
	}
}

void Program::eventOpenLibDirBrowser() {
#ifdef _WIN32
	const char* home = "UserProfile";
#else
	const char* home = "HOME";
#endif
	try {
		browser.prepareFileOps(string_view());
		browser.beginFs(string(), SDL_getenv(home));
		browser.exCall = &Program::eventOpenSettings;
		setState<ProgSearchDir>();
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventMoveBooks() {
	state->showPopupMessage("Moving...", ProgSettingsEvent::moveCancelled, "Cancel", Alignment::center);
	static_cast<ProgSettings*>(state)->startMove();
}

void Program::eventMoveCancelled() {
	static_cast<ProgSettings*>(state)->stopMove();
	World::scene()->setPopup(nullptr);
}

void Program::eventMoveFinished(string* errors) {
	static_cast<ProgSettings*>(state)->stopMove();
	if (errors->empty())
		World::scene()->setPopup(nullptr);
	else {
		ProgSettings::logMoveErrors(errors);
		state->showPopupMultiline(*errors);
	}
	delete errors;
}

void Program::eventFontsFinished(const SDL_UserEvent& event) {
	auto ps = static_cast<ProgSettings*>(state);
	ps->stopFonts();
	auto flr = static_cast<FontListResult*>(event.data1);
	if (flr->error.empty())
		ps->setFontField(std::move(flr->families), std::move(flr->files), flr->select);
	else
		logError(flr->error);
	delete flr;
}

void Program::eventSetZoomType(PushButton* but) {
	if (Settings::Zoom type = Settings::Zoom(finishComboBox(but)); type != World::sets()->zoomType) {
		auto ps = static_cast<ProgSettings*>(state);
		World::sets()->zoomType = type;
		ps->zoomLine->replaceWidget(ps->zoomLine->getWidgets().size() - 1, ps->createZoomEdit());
	}
}

void Program::eventSetZoom(Slider* sl) {
	if (World::sets()->zoom != sl->getVal()) {
		World::sets()->zoom = sl->getVal();
		sl->getParent()->getWidget<Label>(sl->getIndex() + 1)->setText(ProgSettings::makeZoomText());
	}
}

void Program::eventSetScreenMode(PushButton* but) {
	if (Settings::Screen screen = Settings::Screen(finishComboBox(but)); World::sets()->screen != screen)
		World::winSys()->setScreenMode(screen);
}

void Program::eventSetRenderer(PushButton* but) {
	if (Settings::Renderer renderer = Settings::Renderer(finishComboBox(but)); World::sets()->renderer != renderer) {
		World::sets()->renderer = renderer;
		World::winSys()->recreateWindows();
	}
}

void Program::eventSetDevice(PushButton* but) {
	if (u32vec2 device = static_cast<ProgSettings*>(state)->getDevice(finishComboBox(but)); World::sets()->device != device) {
		World::sets()->device = device;
		World::winSys()->recreateWindows();
	}
}

void Program::eventSetCompression(PushButton* but) {
	ComboBox* cmb = World::scene()->getContext()->owner<ComboBox>();
	Settings::Compression compression = strToEnum(Settings::compressionNames, cmb->getOptions()[but->getIndex()].data(), World::sets()->compression);
	cmb->setCurOpt(but->getIndex());
	World::scene()->setContext(nullptr);

	if (World::sets()->compression != compression) {
		World::sets()->compression = compression;
		World::drawSys()->getRenderer()->setCompression(compression);
	}
}

void Program::eventSetVsync(CheckBox* cb) {
	World::sets()->vsync = cb->on;
	World::drawSys()->getRenderer()->setVsync(World::sets()->vsync);
}

void Program::eventSetMultiFullscreen(WindowArranger* wa) {
	if (umap<int, Recti> dsps = wa->getActiveDisps(); dsps != World::sets()->displays) {
		World::sets()->displays = std::move(dsps);
		if (World::sets()->screen == Settings::Screen::multiFullscreen)
			World::winSys()->recreateWindows();
	}
}

void Program::eventSetTheme(PushButton* lbl) {
	ComboBox* cmb = World::scene()->getContext()->owner<ComboBox>();
	World::drawSys()->setTheme(lbl->getText().data());
	if (World::sets()->getTheme() != cmb->getText())
		cmb->setText(World::sets()->getTheme());
	World::scene()->setContext(nullptr);
}

void Program::eventSetFont(PushButton* but) {
	if (fs::path file = toPath(World::scene()->getContext()->owner<ComboBox>()->getTooltips()[finishComboBox(but)].data()); !file.empty())
		setFont(file);
}

void Program::setFont(const fs::path& font) {
	try {
		World::drawSys()->setFont(font);
		state->eventRefresh();
	} catch (const std::runtime_error& err) {
		state->showPopupMessage(err.what());
	}
}

void Program::eventSetFontHinting(PushButton* but) {
	World::sets()->hinting = Settings::Hinting(finishComboBox(but));
	World::drawSys()->setFontHinting(World::sets()->hinting);
	state->eventRefresh();
}

void Program::eventSetDeadzone(Slider* sl) {
	World::sets()->setDeadzone(sl->getVal());
	sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1)->setText(toStr(World::sets()->getDeadzone()));
}

void Program::eventSetDeadzone(LabelEdit* le) {
	World::sets()->setDeadzone(toNum<uint>(le->getText()));
	le->setText(toStr(World::sets()->getDeadzone()));	// set text again in case the value was out of range
	le->getParent()->getWidget<Slider>(le->getIndex() - 1)->setVal(World::sets()->getDeadzone());
}

void Program::eventSetPicLimType(PushButton* but) {
	if (PicLim::Type plim = PicLim::Type(finishComboBox(but)); plim != World::sets()->picLim.type) {
		auto ps = static_cast<ProgSettings*>(state);
		World::sets()->picLim.type = plim;
		ps->limitLine->replaceWidget(ps->limitLine->getWidgets().size() - 1, ps->createLimitEdit());
	}
}

void Program::eventSetPicLimCount(LabelEdit* le) {
	World::sets()->picLim.count = PicLim::toCount(le->getText());
	le->setText(toStr(World::sets()->picLim.count));
}

void Program::eventSetPicLimSize(LabelEdit* le) {
	World::sets()->picLim.size = PicLim::toSize(le->getText());
	le->setText(PicLim::memoryString(World::sets()->picLim.size));
}

void Program::eventSetMaxPicRes(Slider* sl) {
	World::sets()->maxPicRes = sl->getVal();
	World::drawSys()->getRenderer()->setMaxPicRes(World::sets()->maxPicRes);
	sl->getParent()->getWidget<LabelEdit>(sl->getIndex() + 1)->setText(toStr(World::sets()->maxPicRes));
}

void Program::eventSetMaxPicRes(LabelEdit* le) {
	World::sets()->maxPicRes = toNum<uint>(le->getText());
	World::drawSys()->getRenderer()->setMaxPicRes(World::sets()->maxPicRes);
	le->setText(toStr(World::sets()->maxPicRes));
	le->getParent()->getWidget<Slider>(le->getIndex() - 1)->setVal(World::sets()->getDeadzone());
}

void Program::eventResetSettings() {
	World::inputSys()->resetBindings();
	World::winSys()->resetSettings();
}

// OTHER

void Program::eventConfirmComboBox(PushButton* cbut) {
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(cbut->getIndex());
	World::scene()->setContext(nullptr);
}

uint Program::finishComboBox(PushButton* but) {
	uint val = but->getIndex();
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(val);
	World::scene()->setContext(nullptr);
	return val;
}

void Program::eventResizeComboContext(Context* ctx) {
	ctx->setRect(ProgState::calcTextContextRect(ctx->getWidget<ScrollArea>(0)->getWidgets(), ctx->owner()->position(), ctx->owner()->size(), ctx->getSpacing()));
}

void Program::eventStartRequestPassphrase(ArchiveData* ad, std::binary_semaphore* done) {
	archiveRequest = ad;
	archiveRequestDone = done;
	prevPopup = World::scene()->releasePopup();
	if (ad->pc != ArchiveData::PassCode::set)
		eventRequestPassphrase();
	else
		state->showPopupMessage("Incorrect password", GeneralEvent::requestPassphrase);
}

void Program::eventRequestPassphrase() {
	state->showPopupInput("Archive password", string(), GeneralEvent::confirmPassphrase, GeneralEvent::cancelPassphrase, false);
}

void Program::eventSetPassphrase(bool ok) {
	if (ok) {
		archiveRequest->passphrase = ProgState::inputFromPopup();
		archiveRequest->pc = ArchiveData::PassCode::set;
	} else
		archiveRequest->pc = ArchiveData::PassCode::ignore;
	archiveRequestDone->release();
	World::scene()->setPopup(prevPopup);
}

void Program::eventExit() {
	state->eventClosing();
	World::winSys()->close();
}

void Program::setPopupProgress(Cstring&& msg) {
	state->showPopupMessage(std::move(msg), ProgPageBrowserEvent::fileLoadingCancelled, "Cancel", Alignment::center);
}

template <Derived<ProgState> T, class... A>
void Program::setState(A&&... args) {
	SDL_FlushEvents(SDL_USEREVENT_GENERAL, SDL_USEREVENT_PROG_MAX);
	delete state;
	state = new T(std::forward<A>(args)...);
	World::scene()->resetLayouts();
}
