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
		openFile(cmdVals[0].c_str());
}

void Program::tick() {
	if (ProgFileExplorer* pe = dynamic_cast<ProgFileExplorer*>(state))
		pe->processFileChanges(&browser);
}

// BOOKS

void Program::eventOpenBookList(Button*) {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(World::sets()->dirLib))
			browserLoginAuto(std::move(*rl), &Program::eventOpenBookListLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { openBookListHandle(prl, std::move(pwds)); });
		else
			openBookListHandle(valcp(World::sets()->dirLib));
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
		eventOpenSettings();
	}
}

void Program::eventOpenBookListLogin(Button*) {
	browserLoginManual([this](const RemoteLocation& rl) { openBookListHandle(rl); });
}

template <class... A>
void Program::openBookListHandle(A&&... args) {
	browser.start(string(), std::forward<A>(args)...);
	browser.exCall = &Program::eventOpenBookList;
	setState<ProgBooks>();
}

void Program::eventOpenPageBrowser(Button* but) {
	try {
		browser.start(valcp(World::sets()->dirLib), World::sets()->dirLib / static_cast<Label*>(but)->getText());	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventOpenPageBrowserGeneral(Button*) {
	try {
		browser.start(string(), string());	// browser should already be in the right state
		setState<ProgPageBrowser>();
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventOpenBookContext(Button* but) {
	FileOpCapabilities caps = browser.fileOpCapabilities();
	vector<pair<string, PCall>> items = { pair("Continue", &Program::eventOpenLastPage) };
	if (bool(caps & FileOpCapabilities::rename))
		items.emplace_back("Rename", &Program::eventQueryRenameBook);
	if (bool(caps & FileOpCapabilities::remove))
		items.emplace_back("Delete", &Program::eventAskDeleteBook);
	World::scene()->setContext(state->createContext(std::move(items), but));
}

void Program::eventOpenBookContextGeneral(Button* but) {
	World::scene()->setContext(state->createContext({ pair("Continue", &Program::eventOpenLastPageGeneral) }, but));
}

void Program::eventOpenLastPage(Button*) {
	Label* lbl = static_cast<Label*>(World::scene()->getContext()->owner());
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(lbl->getText()); ok) {
		if (browser.openPicture(World::sets()->dirLib / lbl->getText(), World::sets()->dirLib / lbl->getText() / drc, std::move(fname)))
			setPopupLoading();
		else
			World::scene()->setPopup(state->createPopupMessage("Invalid last page entry", &Program::eventClosePopup));
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry", &Program::eventClosePopup));
}

void Program::eventOpenLastPageGeneral(Button*) {
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(ProgState::dotStr); ok) {
		if (browser.openPicture(string(), std::move(drc), std::move(fname)))
			setPopupLoading();
		else
			World::scene()->setPopup(state->createPopupMessage("Invalid last page entry", &Program::eventClosePopup));
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry", &Program::eventClosePopup));
}

void Program::eventAskDeleteBook(Button*) {
	ProgBooks* pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<Label>();
	eventCloseContext();
	World::scene()->setPopup(state->createPopupChoice(std::format("Are you sure you want to delete '{}'?", pb->contextBook->getText()), &Program::eventDeleteBook, &Program::eventClosePopup));
}

void Program::eventDeleteBook(Button*) {
	ProgBooks* pb = static_cast<ProgBooks*>(state);
	if (browser.deleteEntry(pb->contextBook->getText())) {
		pb->contextBook->getParent()->deleteWidget(pb->contextBook->getIndex());
		eventClosePopup();
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to delete all files", &Program::eventClosePopup));
}

void Program::eventQueryRenameBook(Button*) {
	ProgBooks* pb = static_cast<ProgBooks*>(state);
	pb->contextBook = World::scene()->getContext()->owner<Label>();
	eventCloseContext();
	World::scene()->setPopup(state->createPopupInput("Rename book", valcp(pb->contextBook->getText()), &Program::eventRenameBook, &Program::eventClosePopup, "Rename"));
}

void Program::eventRenameBook(Button*) {
	ProgBooks* pb = static_cast<ProgBooks*>(state);
	const string& newName = ProgState::inputFromPopup();
	if (browser.renameEntry(pb->contextBook->getText(), newName)) {
		TileBox* box = static_cast<TileBox*>(pb->contextBook->getParent());
		uint id = std::lower_bound(box->getWidgets().begin(), box->getWidgets().end(), newName, [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); }) - box->getWidgets().begin();
		box->deleteWidget(pb->contextBook->getIndex());
		box->insertWidget(id, pb->makeBookTile(valcp(newName)));
		eventClosePopup();
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to rename", &Program::eventClosePopup));
}

void Program::openFile(const char* file) {
	try {
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(file))
			browserLoginAuto(std::move(*rl), &Program::eventOpenFileLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { openFileHandle(prl, std::move(pwds)); });
		else
			openFileHandle(file);
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventOpenFileLogin(Button*) {
	browserLoginManual([this](const RemoteLocation& rl) { openFileHandle(rl); });
}

template <class... A>
void Program::openFileHandle(A&&... args) {
	if (browser.goTo(std::forward<A>(args)...)) {
		if (ProgPageBrowser* pb = dynamic_cast<ProgPageBrowser*>(state))
			pb->resetFileIcons();
		setPopupLoading();
	} else
		setState<ProgPageBrowser>();
}

// BROWSER

void Program::eventArchiveProgress(const SDL_UserEvent& user) {
	state->updatePopupMessage(std::format("Loading {}", uintptr_t(user.data1)));
}

void Program::eventArchiveFinished(const SDL_UserEvent& user) {
	browser.stopThread();
	if (BrowserResultAsync* ra = static_cast<BrowserResultAsync*>(user.data1)) {
		if (browser.finishArchive(std::move(*ra)))
			setState<ProgPageBrowser>();
		else
			setPopupLoading();
		delete ra;
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to load archive", &Program::eventClosePopup));
}

void Program::eventFileLoadingCancelled(Button*) {
	browser.stopThread();
	if (ProgFileExplorer* pe = dynamic_cast<ProgFileExplorer*>(state))
		if (World::sets()->preview)
			if (auto [files, dirs] = browser.listCurDir(); pe->fileList->getWidgets().size() == files.size() + dirs.size())
				browser.startPreview(files, dirs, pe->getLineHeight());
	eventClosePopup();
}

void Program::eventBrowserGoUp(Button*) {
	if (browser.goUp())
		World::scene()->resetLayouts();
	else
		eventExitBrowser();
}

void Program::eventBrowserGoIn(Button* but) {
	if (browser.goIn(static_cast<Label*>(but)->getText()))
		World::scene()->resetLayouts();
}

void Program::eventBrowserGoFile(Button* but) {
	if (browser.goFile(static_cast<Label*>(but)->getText())) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupLoading();
	}
}

void Program::eventBrowserGoTo(Button* but) {
	try {
		LabelEdit* le = static_cast<LabelEdit*>(but);
		if (uptr<RemoteLocation> rl = browser.prepareFileOps(le->getText()))
			browserLoginAuto(std::move(*rl), &Program::eventBrowserGoToLogin, [this](const RemoteLocation& prl, vector<string>&& pwds) { browserGoToHandle(prl, std::move(pwds)); });
		else
			browserGoToHandle(le->getText());
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventBrowserGoToLogin(Button*) {
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
void Program::browserLoginAuto(RemoteLocation&& rl, PCall kcal, F func) {
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
	World::scene()->setPopup(state->createPopupRemoteLogin(std::move(rl), kcal, &Program::eventClosePopup));
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
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventPreviewProgress(const SDL_UserEvent& user) {
	char* ndata = static_cast<char*>(user.data1);
	Renderer* renderer = World::drawSys()->getRenderer();
	if (Texture* tex = renderer->texFromIcon(static_cast<SDL_Surface*>(user.data2))) {
		ProgFileExplorer* pe = static_cast<ProgFileExplorer*>(state);
		const vector<Widget*>& wgts = pe->fileList->getWidgets();
		auto [pos, end] = ndata[0] ? pair(wgts.begin() + pe->dirEnd, wgts.begin() + pe->fileEnd) : pair(wgts.begin(), wgts.begin() + pe->dirEnd);
		if (vector<Widget*>::const_iterator it = std::lower_bound(pos, end, string_view(ndata + 1), [](const Widget* a, string_view b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); }); it != end && static_cast<Label*>(*it)->getText() == ndata + 1) {
			static_cast<Label*>(*it)->setTex(tex, true);
			renderer->synchTransfer();
		} else {
			renderer->synchTransfer();
			renderer->freeTexture(tex);
		}
	}
	delete[] ndata;
}

void Program::eventPreviewFinished() {
	browser.stopThread();
}

void Program::eventExitBrowser(Button*) {
	(this->*browser.exCall)(nullptr);
}

// READER

void Program::eventReaderProgress(const SDL_UserEvent& user) {
	BrowserPictureProgress* pp = static_cast<BrowserPictureProgress*>(user.data1);
	pp->pnt->mpic.lock();
	pp->pnt->pics[pp->id].second = World::drawSys()->getRenderer()->texFromRpic(pp->img);
	pp->pnt->mpic.unlock();
	delete pp;

	char* text = static_cast<char*>(user.data2);
	state->updatePopupMessage(std::format("Loading {}", text));
	delete[] text;
}

void Program::eventReaderFinished(const SDL_UserEvent& user) {
	browser.stopThread();
	if (BrowserResultPicture* rp = static_cast<BrowserResultPicture*>(user.data1)) {
		if (rp->archive)
			rng::sort(rp->pics, [](const pair<string, Texture*>& a, const pair<string, Texture*>& b) -> bool { return StrNatCmp::less(a.first, b.first); });
		else if (!user.data2)
			rng::reverse(rp->pics);

		browser.finishLoadPictures(*rp);
		setState<ProgReader>();
		static_cast<ProgReader*>(state)->reader->setPictures(rp->pics, filename(rp->file));
		delete rp;
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to load pictures", &Program::eventClosePopup));
}

void Program::eventZoomIn(Button*) {
	static_cast<ProgReader*>(state)->reader->setZoom(zoomFactor);
}

void Program::eventZoomOut(Button*) {
	static_cast<ProgReader*>(state)->reader->setZoom(1.f / zoomFactor);
}

void Program::eventZoomReset(Button*) {
	ProgReader* pr = static_cast<ProgReader*>(state);
	pr->reader->setZoom(1.f / pr->reader->getZoom());
}

void Program::eventCenterView(Button*) {
	static_cast<ProgReader*>(state)->reader->centerList();
}

void Program::eventNextDir(Button*) {
	switchPictures(true, static_cast<ProgReader*>(state)->reader->lastPage());
}

void Program::eventPrevDir(Button*) {
	switchPictures(false, static_cast<ProgReader*>(state)->reader->firstPage());
}

void Program::switchPictures(bool fwd, string_view picname) {
	if (browser.goNext(fwd, picname))
		setPopupLoading();
}

void Program::eventExitReader(Button*) {
	state->eventClosing();
	setState<ProgPageBrowser>();
}

// SETTINGS

void Program::eventOpenSettings(Button*) {
	setState<ProgSettings>();
}

void Program::eventSwitchDirection(Button* but) {
	World::sets()->direction = Direction::Dir(finishComboBox(but));
}

void Program::eventSetZoom(Button* but) {
	World::sets()->zoom = toNum<float>(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetSpacing(Button* but) {
	World::sets()->spacing = toNum<ushort>(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetLibraryDirLE(Button* but) {
	setLibraryDir(static_cast<LabelEdit*>(but)->getText(), true);
}

void Program::eventSetLibraryDirBW(Button*) {
	const uset<Widget*>& select = static_cast<ProgFileExplorer*>(state)->fileList->getSelected();
	setLibraryDir(!select.empty() ? browser.getCurDir() / static_cast<Label*>(*select.begin())->getText() : browser.getCurDir());
}

void Program::setLibraryDir(string_view path, bool byText) {
	ProgSettings* ps = dynamic_cast<ProgSettings*>(state);
	bool dstLocal = RemoteLocation::getProtocol(path) == Protocol::none;
	if (std::error_code ec; dstLocal && (!fs::is_directory(toPath(path), ec) || ec)) {	// TODO: establish a new browser connection if remote and check if it's a valid directory
		if (byText)
			ps->libraryDir->setText(World::sets()->dirLib);
		World::scene()->setPopup(state->createPopupMessage("Invalid directory", &Program::eventClosePopup));
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
		World::scene()->setPopup(state->createPopupChoice("Move books to new location?", &Program::eventMoveComics, &Program::eventClosePopup));
	}
}

void Program::eventOpenLibDirBrowser(Button*) {
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
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::eventMoveComics(Button*) {
	World::scene()->setPopup(state->createPopupMessage("Moving...", &Program::eventMoveCancelled, "Cancel", Alignment::center));
	static_cast<ProgSettings*>(state)->startMove();
}

void Program::eventMoveCancelled(Button*) {
	static_cast<ProgSettings*>(state)->stopMove();
	eventClosePopup();
}

void Program::eventMoveProgress(const SDL_UserEvent& user) {
	state->updatePopupMessage(std::format("Moving {}/{}", uintptr_t(user.data1), uintptr_t(user.data2)));
}

void Program::eventMoveFinished(const SDL_UserEvent& user) {
	static_cast<ProgSettings*>(state)->stopMove();
	string* errors = static_cast<string*>(user.data1);
	if (errors->empty())
		eventClosePopup();
	else {
		ProgSettings::logMoveErrors(errors);
		World::scene()->setPopup(state->createPopupMultiline(std::move(*errors), &Program::eventClosePopup));
	}
	delete errors;
}

void Program::eventFontsFinished(const SDL_UserEvent& user) {
	ProgSettings* ps = static_cast<ProgSettings*>(state);
	ps->stopFonts();
	FontListResult* flr = static_cast<FontListResult*>(user.data1);
	if (flr->error.empty())
		ps->setFontField(std::move(flr->families), std::move(flr->files), flr->select);
	else
		logError(flr->error);
	delete flr;
}

void Program::eventSetScreenMode(Button* but) {
	if (Settings::Screen screen = Settings::Screen(finishComboBox(but)); World::sets()->screen != screen)
		World::winSys()->setScreenMode(screen);
}

void Program::eventSetRenderer(Button* but) {
	if (Settings::Renderer renderer = Settings::Renderer(finishComboBox(but)); World::sets()->renderer != renderer) {
		World::sets()->renderer = renderer;
		World::winSys()->recreateWindows();
	}
}

void Program::eventSetDevice(Button* but) {
	if (u32vec2 device = static_cast<ProgSettings*>(state)->getDevice(finishComboBox(but)); World::sets()->device != device) {
		World::sets()->device = device;
		World::winSys()->recreateWindows();
	}
}

void Program::eventSetCompression(Button* but) {
	if (Settings::Compression compression = Settings::Compression(finishComboBox(but)); World::sets()->compression != compression) {
		World::sets()->compression = compression;
		World::drawSys()->getRenderer()->setCompression(compression);
	}
}

void Program::eventSetVsync(Button* but) {
	World::sets()->vsync = static_cast<CheckBox*>(but)->on;
	World::drawSys()->getRenderer()->setVsync(World::sets()->vsync);
}

void Program::eventSetGpuSelecting(Button* but) {
	World::sets()->gpuSelecting = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetMultiFullscreen(Button* but) {
	if (umap<int, Recti> dsps = static_cast<WindowArranger*>(but)->getActiveDisps(); dsps != World::sets()->displays) {
		World::sets()->displays = std::move(dsps);
		if (World::sets()->screen == Settings::Screen::multiFullscreen)
			World::winSys()->recreateWindows();
	}
}

void Program::eventSetPreview(Button* but) {
	World::sets()->preview = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetHide(Button* but) {
	World::sets()->showHidden = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetTooltips(Button* but) {
	World::sets()->tooltips = static_cast<CheckBox*>(but)->on;
	state->eventRefresh();
}

void Program::eventSetTheme(Button* but) {
	ComboBox* cmb = World::scene()->getContext()->owner<ComboBox>();
	Label* lbl = static_cast<Label*>(but);
	World::drawSys()->setTheme(lbl->getText());
	if (World::sets()->getTheme() != cmb->getText())
		cmb->setText(World::sets()->getTheme());
	World::scene()->setContext(nullptr);
}

void Program::eventSetFontCMB(Button* but) {
	if (fs::path file = toPath(static_cast<ComboBox*>(but)->getTooltips()[finishComboBox(but)]); !file.empty()) {
		World::drawSys()->setFont(file.native().starts_with(World::fileSys()->getDirConfs().native()) ? file.stem() : file);
		state->eventRefresh();
	}
}

void Program::eventSetFontLE(Button* but) {
	string otxt = static_cast<LabelEdit*>(but)->getText();
	World::drawSys()->setFont(toPath(otxt));
	state->eventRefresh();
	if (World::sets()->font != otxt)
		World::scene()->setPopup(state->createPopupMessage("Invalid font", &Program::eventClosePopup));
}

void Program::eventSetFontHinting(Button* but) {
	World::sets()->hinting = Settings::Hinting(finishComboBox(but));
	World::drawSys()->setFontHinting(World::sets()->hinting);
	state->eventRefresh();
}

void Program::eventSetScrollSpeed(Button* but) {
	World::sets()->scrollSpeed = toVec<vec2>(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::sets()->setDeadzone(static_cast<Slider*>(but)->getVal());
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->getDeadzone()));
}

void Program::eventSetDeadzoneLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->setDeadzone(toNum<uint>(le->getText()));
	le->setText(toStr(World::sets()->getDeadzone()));	// set text again in case the value was out of range
	but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(World::sets()->getDeadzone());
}

void Program::eventSetPortrait(Button*) {
	ivec2 res = World::winSys()->displayResolution();
	float width, height;
	if (res.x > res.y) {
		height = float(res.y) * resModeBorder;
		width = height * resModeRatio;
	} else {
		width = float(res.x) * resModeBorder;
		height = width / resModeRatio;
	}
	reposizeWindow(res, ivec2(width, height));
}

void Program::eventSetLandscape(Button*) {
	ivec2 res = World::winSys()->displayResolution();
	float width, height;
	if (res.x < res.y) {
		width = float(res.x) * resModeBorder;
		height = width * resModeRatio;
	} else {
		height = float(res.y) * resModeBorder;
		width = height / resModeRatio;
	}
	reposizeWindow(res, ivec2(width, height));
}

void Program::eventSetSquare(Button*) {
	ivec2 res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2(res.x < res.y ? res.x : res.y) * resModeBorder);
}

void Program::eventSetFill(Button*) {
	ivec2 res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2(res) * resModeBorder);
}

void Program::eventSetPicLimitType(Button* but) {
	if (PicLim::Type plim = PicLim::Type(finishComboBox(but)); plim != World::sets()->picLim.type) {
		ProgSettings* ps = static_cast<ProgSettings*>(state);
		World::sets()->picLim.type = plim;
		ps->limitLine->replaceWidget(ps->limitLine->getWidgets().size() - 1, static_cast<ProgSettings*>(state)->createLimitEdit());
	}
}

void Program::eventSetPicLimCount(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->picLim.setCount(le->getText());
	le->setText(toStr(World::sets()->picLim.getCount()));
}

void Program::eventSetPicLimSize(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->picLim.setSize(le->getText());
	le->setText(PicLim::memoryString(World::sets()->picLim.getSize()));
}

void Program::eventSetMaxPicResSL(Button* but) {
	World::sets()->maxPicRes = static_cast<Slider*>(but)->getVal();
	World::drawSys()->getRenderer()->setMaxPicRes(World::sets()->maxPicRes);
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->maxPicRes));
}

void Program::eventSetMaxPicResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->maxPicRes = toNum<uint>(le->getText());
	World::drawSys()->getRenderer()->setMaxPicRes(World::sets()->maxPicRes);
	le->setText(toStr(World::sets()->maxPicRes));
	but->getParent()->getWidget<Slider>(but->getIndex() - 1)->setVal(World::sets()->getDeadzone());
}

void Program::eventResetSettings(Button*) {
	World::inputSys()->resetBindings();
	World::winSys()->resetSettings();
}

void Program::reposizeWindow(ivec2 dres, ivec2 wsiz) {
	World::winSys()->setWindowPos((dres - wsiz) / 2);
	World::winSys()->setResolution(wsiz);
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventCloseContext(Button*) {
	World::scene()->setContext(nullptr);
}

void Program::eventConfirmComboBox(Button* but) {
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(but->getIndex());
	World::scene()->setContext(nullptr);
}

uint Program::finishComboBox(Button* but) {
	uint val = but->getIndex();
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(val);
	World::scene()->setContext(nullptr);
	return val;
}

void Program::eventResizeComboContext(Layout* lay) {
	Context* ctx = static_cast<Context*>(lay);
	ctx->setRect(ProgState::calcTextContextRect(ctx->getWidget<ScrollArea>(0)->getWidgets(), ctx->owner()->position(), ctx->owner()->size(), ctx->getSpacing()));
}

void Program::eventExit(Button*) {
	state->eventClosing();
	World::winSys()->close();
}

void Program::setPopupLoading() {
	World::scene()->setPopup(state->createPopupMessage("Loading...", &Program::eventFileLoadingCancelled, "Cancel", Alignment::center));
}

template <Derived<ProgState> T, class... A>
void Program::setState(A&&... args) {
	delete state;
	state = new T(std::forward<A>(args)...);
	World::scene()->resetLayouts();
}
