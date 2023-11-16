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
		openFile(cmdVals[0]);
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
		eventFileLoadingCancelled();
		break;
	case goFile:
		eventBrowserGoFile(static_cast<PushButton*>(event.data1));
	}
}

void Program::handleThreadArchiveEvent(const SDL_UserEvent& event) {
	switch (ThreadEvent(event.code)) {
	using enum ThreadEvent;
	case progress:
		state->updatePopupMessage(std::format("Loading {}", uintptr_t(event.data1)));
		break;
	case finished:
		eventArchiveFinished(static_cast<BrowserResultAsync*>(event.data1));
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
		eventReaderFinished(static_cast<BrowserResultPicture*>(event.data1), event.data2);
	}
}

void Program::handleProgSettingsEvent(const SDL_UserEvent& event) {
	switch (ProgSettingsEvent(event.code)) {
	using enum ProgSettingsEvent;
	case setDirection:
		World::sets()->direction = Direction::Dir(finishComboBox(static_cast<PushButton*>(event.data1)));
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
	case setPortrait:
		eventSetPortrait();
		break;
	case setLandscape:
		eventSetLandscape();
		break;
	case setSquare:
		eventSetSquare();
		break;
	case setFill:
		eventSetFill();
		break;
	case setPicLimitType:
		eventSetPicLimitType(static_cast<PushButton*>(event.data1));
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
		World::scene()->setPopup(state->createPopupMessage(err.what()));
		eventOpenSettings();
	}
}

void Program::eventOpenBookListLogin() {
	browserLoginManual([this](const RemoteLocation& rl) { openBookListHandle(rl); });
}

template <class... A>
void Program::openBookListHandle(A&&... args) {
	browser.start(string(), std::forward<A>(args)...);
	browser.exCall = &Program::eventOpenBookList;
	setState<ProgBooks>();
}

void Program::eventOpenPageBrowser(PushButton* lbl) {
	try {
		browser.start(World::sets()->dirLib / lbl->getText().data(), World::sets()->dirLib / lbl->getText().data());	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
	}
}

void Program::eventOpenPageBrowserGeneral() {
	try {
		browser.start(string(), string());	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
	}
}

void Program::eventOpenBookContext(Widget* wgt) {
	FileOpCapabilities caps = browser.fileOpCapabilities();
	vector<pair<Cstring, EventId>> items = { pair("Continue", ProgBooksEvent::openLastPage) };
	if (bool(caps & FileOpCapabilities::rename))
		items.emplace_back("Rename", ProgBooksEvent::queryRenameBook);
	if (bool(caps & FileOpCapabilities::remove))
		items.emplace_back("Delete", ProgBooksEvent::askDeleteBook);
	World::scene()->setContext(state->createContext(std::move(items), wgt));
}

void Program::eventOpenBookContextGeneral(Widget* wgt) {
	World::scene()->setContext(state->createContext({ pair("Continue", ProgBooksEvent::openLastPageGeneral) }, wgt));
}

void Program::eventOpenLastPage() {
	string_view bname = static_cast<PushButton*>(World::scene()->getContext()->owner())->getText();
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(bname); ok) {
		if (browser.openPicture(World::sets()->dirLib / bname, World::sets()->dirLib / bname / drc, std::move(fname)))
			setPopupLoading();
		else
			World::scene()->setPopup(state->createPopupMessage("Invalid last page entry"));
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry"));
}

void Program::eventOpenLastPageGeneral() {
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(ProgState::dotStr); ok) {
		if (browser.openPicture(string(), std::move(drc), std::move(fname)))
			setPopupLoading();
		else
			World::scene()->setPopup(state->createPopupMessage("Invalid last page entry"));
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry"));
}

void Program::eventAskDeleteBook() {
	auto pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<PushButton>();
	World::scene()->setContext(nullptr);
	World::scene()->setPopup(state->createPopupChoice(std::format("Are you sure you want to delete '{}'?", pb->contextBook->getText().data()), ProgBooksEvent::deleteBook));
}

void Program::eventDeleteBook() {
	auto pb = static_cast<ProgBooks*>(state);
	if (browser.deleteEntry(pb->contextBook->getText())) {
		pb->contextBook->getParent()->deleteWidget(pb->contextBook->getIndex());
		World::scene()->setPopup(nullptr);
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to delete all files"));
}

void Program::eventQueryRenameBook() {
	auto pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<PushButton>();
	World::scene()->setContext(nullptr);
	World::scene()->setPopup(state->createPopupInput("Rename book", pb->contextBook->getText().data(), ProgBooksEvent::renameBook, GeneralEvent::closePopup, "Rename"));
}

void Program::eventRenameBook() {
	auto pb = static_cast<ProgBooks*>(state);
	const string& newName = ProgState::inputFromPopup();
	if (browser.renameEntry(pb->contextBook->getText(), newName)) {
		auto box = static_cast<TileBox*>(pb->contextBook->getParent());
		box->deleteWidget(pb->contextBook->getIndex());
		std::span<Widget*> wgts = box->getWidgets();
		uint id = std::lower_bound(wgts.begin(), wgts.end(), newName, [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const PushButton*>(a)->getText(), b); }) - wgts.begin();
		box->insertWidget(id, pb->makeBookTile(newName));
		World::scene()->setPopup(nullptr);
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to rename"));
}

void Program::openFile(string_view file) {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(file))
			browserLoginAuto(std::move(*rl), ProgBooksEvent::openFileLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { openFileHandle(prl, std::move(pwds)); });
		else
			openFileHandle(file);
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
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
		setPopupLoading();
	} else
		setState<ProgPageBrowser>();
}

void Program::eventOpenSettings() {
	setState<ProgSettings>();
}

// BROWSER

void Program::eventArchiveFinished(BrowserResultAsync* ra) {
	browser.stopThread();
	if (ra) {
		if (browser.finishArchive(std::move(*ra)))
			setState<ProgPageBrowser>();
		else
			setPopupLoading();
		delete ra;
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to load archive"));
}

void Program::eventFileLoadingCancelled() {
	browser.stopThread();
	if (auto pe = dynamic_cast<ProgFileExplorer*>(state))
		if (World::sets()->preview)
			if (auto [files, dirs] = browser.listCurDir(); pe->fileList->getWidgets().size() == files.size() + dirs.size())
				browser.startPreview(files, dirs, pe->getLineHeight());
	World::scene()->setPopup(nullptr);
}

void Program::eventBrowserGoUp() {
	if (browser.goUp())
		World::scene()->resetLayouts();
	else
		eventExitBrowser();
}

void Program::eventBrowserGoIn(PushButton* lbl) {
	if (browser.goIn(lbl->getText()))
		World::scene()->resetLayouts();
}

void Program::eventBrowserGoFile(PushButton* lbl) {
	if (browser.goFile(lbl->getText())) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupLoading();
	}
}

void Program::eventBrowserGoTo(LabelEdit* le) {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(le->getText()))
			browserLoginAuto(std::move(*rl), ProgFileExplorerEvent::goToLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { browserGoToHandle(prl, std::move(pwds)); });
		else
			browserGoToHandle(le->getText());
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
	}
}

void Program::eventBrowserGoToLogin() {
	browserLoginManual([this](const RemoteLocation& rl) { browserGoToHandle(rl); });
}

template <class... A>
void Program::browserGoToHandle(A&&... args) {
	if (browser.goTo(std::forward<A>(args)...)) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupLoading();
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
	World::scene()->setPopup(state->createPopupRemoteLogin(std::move(rl), kcal));
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
		World::scene()->setPopup(state->createPopupMessage(err.what()));
	}
}

void Program::eventPreviewProgress(char* ndata, SDL_Surface* icon) {
	Renderer* renderer = World::drawSys()->getRenderer();
	if (Texture* tex = renderer->texFromIcon(icon)) {
		auto pe = static_cast<ProgFileExplorer*>(state);
		std::span<Widget*> wgts = pe->fileList->getWidgets();
		auto [pos, end] = ndata[0] ? pair(wgts.begin() + pe->dirEnd, wgts.begin() + pe->fileEnd) : pair(wgts.begin(), wgts.begin() + pe->dirEnd);
		if (std::span<Widget*>::iterator it = std::lower_bound(pos, end, string_view(ndata + 1), [](const Widget* a, string_view b) -> bool { return StrNatCmp::less(static_cast<const PushButton*>(a)->getText(), b); }); it != end && static_cast<PushButton*>(*it)->getText() == ndata + 1) {
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

void Program::eventReaderFinished(BrowserResultPicture* rp, bool fwd) {
	browser.stopThread();
	if (rp) {
		if (rp->archive)
			rng::sort(rp->pics, [](const pair<string, Texture*>& a, const pair<string, Texture*>& b) -> bool { return StrNatCmp::less(a.first, b.first); });
		else if (!fwd)
			rng::reverse(rp->pics);

		browser.finishLoadPictures(*rp);
		setState<ProgReader>();
		static_cast<ProgReader*>(state)->reader->setPictures(rp->pics, filename(rp->file));
		delete rp;
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to load pictures"));
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
	int maxRes = 0;
	for (const Widget* it : rb->getWidgets())
		if (int res = static_cast<const Picture*>(it)->getTex()->getRes()[di]; res > maxRes)
			maxRes = res;

	if (maxRes > 0) {
		double target = double(World::drawSys()->getViewRes()[di]) / double(maxRes);
		double zoom = int(std::log(target) / std::log(Settings::zoomBase));
		rb->setZoom(std::clamp(int(std::floor(zoom)), -Settings::zoomLimit, int(Settings::zoomLimit)));
	}
}

void Program::eventCenterView() {
	static_cast<ProgReader*>(state)->reader->centerList();
}

void Program::eventNextDir() {
	switchPictures(true, static_cast<ProgReader*>(state)->reader->lastPage());
}

void Program::eventPrevDir() {
	switchPictures(false, static_cast<ProgReader*>(state)->reader->firstPage());
}

void Program::switchPictures(bool fwd, string_view picname) {
	if (browser.goNext(fwd, picname))
		setPopupLoading();
}

void Program::eventExitReader() {
	state->eventClosing();
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
		World::scene()->setPopup(state->createPopupMessage("Invalid directory"));
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
		World::scene()->setPopup(state->createPopupChoice("Move books to new location?", ProgSettingsEvent::moveBooks));
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
		browser.start(string(), SDL_getenv(home));
		browser.exCall = &Program::eventOpenSettings;
		setState<ProgSearchDir>();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
	}
}

void Program::eventMoveBooks() {
	World::scene()->setPopup(state->createPopupMessage("Moving...", ProgSettingsEvent::moveCancelled, "Cancel", Alignment::center));
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
		World::scene()->setPopup(state->createPopupMultiline(std::move(*errors)));
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
	if (Settings::Compression compression = Settings::Compression(finishComboBox(but)); World::sets()->compression != compression) {
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
	World::drawSys()->setTheme(lbl->getText());
	if (World::sets()->getTheme() != cmb->getText())
		cmb->setText(World::sets()->getTheme());
	World::scene()->setContext(nullptr);
}

void Program::eventSetFont(PushButton* but) {
	if (fs::path file = toPath(World::scene()->getContext()->owner<ComboBox>()->getTooltips()[finishComboBox(but)]); !file.empty())
		setFont(file);
}

void Program::setFont(const fs::path& font) {
	try {
		World::drawSys()->setFont(font);
		state->eventRefresh();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what()));
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

void Program::eventSetPortrait() {
	ivec2 res = World::winSys()->displayResolution();
	float width, height;
	if (res.x > res.y) {
		height = float(res.y) * resModeBorder;
		width = height * resModeRatio;
	} else {
		width = float(res.x) * resModeBorder;
		height = width / resModeRatio;
	}
	World::winSys()->reposizeWindow(res, ivec2(width, height));
}

void Program::eventSetLandscape() {
	ivec2 res = World::winSys()->displayResolution();
	float width, height;
	if (res.x < res.y) {
		width = float(res.x) * resModeBorder;
		height = width * resModeRatio;
	} else {
		height = float(res.y) * resModeBorder;
		width = height / resModeRatio;
	}
	World::winSys()->reposizeWindow(res, ivec2(width, height));
}

void Program::eventSetSquare() {
	ivec2 res = World::winSys()->displayResolution();
	World::winSys()->reposizeWindow(res, vec2(res.x < res.y ? res.x : res.y) * resModeBorder);
}

void Program::eventSetFill() {
	ivec2 res = World::winSys()->displayResolution();
	World::winSys()->reposizeWindow(res, vec2(res) * resModeBorder);
}

void Program::eventSetPicLimitType(PushButton* but) {
	if (PicLim::Type plim = PicLim::Type(finishComboBox(but)); plim != World::sets()->picLim.type) {
		auto ps = static_cast<ProgSettings*>(state);
		World::sets()->picLim.type = plim;
		ps->limitLine->replaceWidget(ps->limitLine->getWidgets().size() - 1, static_cast<ProgSettings*>(state)->createLimitEdit());
	}
}

void Program::eventSetPicLimCount(LabelEdit* le) {
	World::sets()->picLim.setCount(le->getText());
	le->setText(toStr(World::sets()->picLim.getCount()));
}

void Program::eventSetPicLimSize(LabelEdit* le) {
	World::sets()->picLim.setSize(le->getText());
	le->setText(PicLim::memoryString(World::sets()->picLim.getSize()));
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

void Program::eventExit() {
	state->eventClosing();
	World::winSys()->close();
}

void Program::setPopupLoading() {
	World::scene()->setPopup(state->createPopupMessage("Loading...", ProgPageBrowserEvent::fileLoadingCancelled, "Cancel", Alignment::center));
}

template <Derived<ProgState> T, class... A>
void Program::setState(A&&... args) {
	SDL_FlushEvents(SDL_USEREVENT_GENERAL, SDL_USEREVENT_PROG_MAX);
	delete state;
	state = new T(std::forward<A>(args)...);
	World::scene()->resetLayouts();
}
