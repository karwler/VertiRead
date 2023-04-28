#include "engine/world.h"
#include "progs.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "utils/compare.h"
#include "utils/layouts.h"

// PROGRAM

Program::~Program() {
	delete state;
}

void Program::start() {
	eventOpenBookList();
	if (!World::getVals().empty())
		openFile(fs::u8path(World::getVals()[0]));
}

void Program::tick() {
	if (browser)
		if (ProgFileExplorer* pe = dynamic_cast<ProgFileExplorer*>(state)) {
			auto [files, gone] = browser->directoryUpdate();
			pe->processFileChanges(browser.get(), files, gone);
		}
}

// BOOKS

void Program::eventOpenBookList(Button*) {
	browser = std::make_unique<Browser>(World::sets()->getDirLib(), World::sets()->getDirLib());
	setState<ProgBooks>();
}

void Program::eventOpenPageBrowser(Button* but) {
	if (browser = Browser::openExplorer(World::sets()->getDirLib() / fs::u8path(static_cast<Label*>(but)->getText()), fs::path(), &Program::eventOpenBookList); browser)
		setState<ProgPageBrowser>();
}

void Program::eventOpenPageBrowserGeneral(Button*) {
	if (browser = Browser::openExplorer(Browser::topDir, fs::path(), &Program::eventOpenBookList); browser)
		setState<ProgPageBrowser>();
}

void Program::eventOpenBookContext(Button* but) {
	vector<pair<string, PCall>> items = {
		pair("Continue", &Program::eventOpenLastPage),
		pair("Delete", &Program::eventDeleteBook)
	};
	World::scene()->setContext(state->createContext(std::move(items), but));
}

void Program::eventOpenBookContextGeneral(Button* but) {
	World::scene()->setContext(state->createContext({ pair("Continue", &Program::eventOpenLastPageGeneral) }, but));
}

void Program::eventOpenLastPage(Button*) {
	Label* lbl = static_cast<Label*>(World::scene()->getContext()->owner());
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(lbl->getText()); ok) {
		if (browser->openPicture(World::sets()->getDirLib() / fs::u8path(lbl->getText()), World::sets()->getDirLib() / fs::u8path(lbl->getText()) / fs::u8path(drc), fs::u8path(fname)))
			setPopupLoading();
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry", &Program::eventClosePopup));
}

void Program::eventOpenLastPageGeneral(Button*) {
	if (auto [ok, drc, fname] = World::fileSys()->getLastPage(ProgState::dotStr); ok) {
		if (browser->openPicture(Browser::topDir, fs::u8path(drc), fs::u8path(fname)))
			setPopupLoading();
	} else
		World::scene()->setPopup(state->createPopupMessage("No last page entry", &Program::eventClosePopup));
}

void Program::eventDeleteBook(Button*) {
	try {
		Label* lbl = World::scene()->getContext()->owner<Label>();
		fs::remove_all(World::sets()->getDirLib() / fs::u8path(lbl->getText()));
		World::scene()->setContext(nullptr);
		lbl->getParent()->deleteWidget(lbl->getIndex());
	} catch (const std::runtime_error& err) {
		World::scene()->setPopup(state->createPopupMessage(err.what(), &Program::eventClosePopup));
	}
}

void Program::openFile(const fs::path& file) {
	switch (browser->openFile(file)) {
	case Browser::RWAIT:
		if (ProgPageBrowser* pb = dynamic_cast<ProgPageBrowser*>(state))
			pb->resetFileIcons();
		setPopupLoading();
		break;
	case Browser::REXPLORER:
		setState<ProgPageBrowser>();
	}
}

// BROWSER

void Program::eventArchiveProgress(const SDL_UserEvent& user) {
	state->updatePopupMessage("Loading " + toStr(uintptr_t(user.data1)));
}

void Program::eventArchiveFinished(const SDL_UserEvent& user) {
	browser->stopThread();
	if (BrowserResultAsync* ra = static_cast<BrowserResultAsync*>(user.data1)) {
		if (browser->finishArchive(std::move(*ra)))
			setState<ProgPageBrowser>();
		else
			setPopupLoading();
		delete ra;
	} else
		World::scene()->setPopup(state->createPopupMessage("Failed to load archive", &Program::eventClosePopup));
}

void Program::eventFileLoadingCancelled(Button*) {
	browser->stopThread();
	if (ProgFileExplorer* pe = dynamic_cast<ProgFileExplorer*>(state))
		if (World::sets()->preview)
			if (auto [files, dirs] = browser->listCurDir(); pe->fileList->getWidgets().size() == files.size() + dirs.size())
				browser->startPreview(files, dirs, pe->getLineHeight());
	eventClosePopup();
}

void Program::eventBrowserGoUp(Button*) {
	if (browser->goUp())
		World::scene()->resetLayouts();
	else
		eventExitBrowser();
}

void Program::eventBrowserGoIn(Button* but) {
	if (browser->goIn(fs::u8path(static_cast<Label*>(but)->getText())))
		World::scene()->resetLayouts();
}

void Program::eventBrowserGoFile(Button* but) {
	if (browser->goFile(fs::u8path(static_cast<Label*>(but)->getText()))) {
		static_cast<ProgPageBrowser*>(state)->resetFileIcons();
		setPopupLoading();
	}
}

void Program::eventBrowserGoTo(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (Browser::Response rs = browser->goTo(browser->getRootDir() == Browser::topDir ? fs::u8path(le->getText()) : World::sets()->getDirLib() / fs::u8path(le->getText())); rs & Browser::RERROR) {
		le->setText(le->getOldText());
		World::scene()->setPopup(state->createPopupMessage("Invalid path", &Program::eventClosePopup));
	} else {
		if (rs & Browser::REXPLORER)
			World::scene()->resetLayouts();
		if (rs & Browser::RWAIT) {
			static_cast<ProgPageBrowser*>(state)->resetFileIcons();
			setPopupLoading();
		}
	}
}

void Program::eventPreviewProgress(const SDL_UserEvent& user) {
	char* ndata = static_cast<char*>(user.data1);
	if (Texture* tex = World::renderer()->texFromIcon(static_cast<SDL_Surface*>(user.data2))) {
		ProgFileExplorer* pe = static_cast<ProgFileExplorer*>(state);
		const vector<Widget*>& wgts = pe->fileList->getWidgets();
		auto [pos, end] = ndata[0] ? pair(wgts.begin() + pe->dirEnd, wgts.begin() + pe->fileEnd) : pair(wgts.begin(), wgts.begin() + pe->dirEnd);
		if (vector<Widget*>::const_iterator it = std::lower_bound(pos, end, ndata + 1, [](const Widget* a, const char* b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); }); it != end && static_cast<Label*>(*it)->getText() == ndata + 1) {
			static_cast<Label*>(*it)->setTex(tex, true);
			World::renderer()->synchTransfer();
		} else {
			World::renderer()->synchTransfer();
			World::renderer()->freeTexture(tex);
		}
	}
	delete[] ndata;
}

void Program::eventPreviewFinished() {
	browser->stopThread();
}

void Program::eventExitBrowser(Button*) {
	PCall call = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

void Program::eventReaderProgress(const SDL_UserEvent& user) {
	BrowserPictureProgress* pp = static_cast<BrowserPictureProgress*>(user.data1);
	pp->pnt->mpic.lock();
	pp->pnt->pics[pp->id].second = World::renderer()->texFromRpic(pp->img);
	pp->pnt->mpic.unlock();
	delete pp;

	char* text = static_cast<char*>(user.data2);
	state->updatePopupMessage("Loading "s + text);
	delete[] text;
}

void Program::eventReaderFinished(const SDL_UserEvent& user) {
	browser->stopThread();
	if (BrowserResultPicture* rp = static_cast<BrowserResultPicture*>(user.data1)) {
		if (rp->archive)
			std::sort(rp->pics.begin(), rp->pics.end(), [](const pair<string, Texture*>& a, const pair<string, Texture*>& b) -> bool { return StrNatCmp::less(a.first, b.first); });
		else if (!user.data2)
			std::reverse(rp->pics.begin(), rp->pics.end());

		browser->finishLoadPictures(*rp);
		setState<ProgReader>();
		static_cast<ProgReader*>(state)->reader->setWidgets(rp->pics, rp->file.filename().u8string());
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
	if (browser->goNext(fwd, picname))
		setPopupLoading();
}

void Program::eventExitReader(Button*) {
	state->eventClosing();
	setState<ProgPageBrowser>();
}

// DOWNLOADER

#ifdef DOWNLOADER
void Program::eventOpenDownloader(Button*) {
	setState<ProgDownloader>();
}

void Program::eventSwitchSource(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);
	downloader.setSource(WebSource::Type(but->getIndex()));
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(downloader.getSource()->source());
	World::scene()->setContext(nullptr);
	pd->printResults({});
	pd->printInfo(vector<pair<string, string>>());
}

void Program::eventQuery(Button*) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);
	pd->printResults(downloader.getSource()->query(pd->query->getText()));
}

void Program::eventShowComicInfo(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);
	WebSource* wsrc = downloader.getSource();
	string url = pd->resultUrls[but->getIndex()];
	pd->printInfo(wsrc->getChapters(url));
}

void Program::eventSelectAllChapters(Button* but) {
	for (Widget* it : static_cast<ProgDownloader*>(state)->chapters->getWidgets())
		static_cast<Layout*>(it)->getWidget<CheckBox>(0)->on = static_cast<CheckBox*>(but)->on;
}

void Program::eventSelectChapter(Button* but) {
	static_cast<ProgDownloader*>(state)->chapters->getWidget<Layout>(but->getParent()->getIndex())->getWidget<CheckBox>(0)->toggle();
}

void Program::eventDownloadAllChapters(Button*) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);
	downloader.downloadComic(pd->curInfo());

	pd->chaptersTick->on = false;
	for (Widget* it : pd->chapters->getWidgets())
		static_cast<Layout*>(it)->getWidget<CheckBox>(0)->on = false;
}

void Program::eventDownloadChapter(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);

	downloader.downloadComic(Comic(static_cast<LabelEdit*>(*pd->results->getSelected().begin())->getText(), { pair(static_cast<Label*>(but)->getText(), pd->resultUrls[but->getParent()->getIndex()]) }));
	but->getParent()->getWidget<CheckBox>(0)->on = false;
}

void Program::eventDownloadComic(Button* but) {
	downloader.downloadComic(Comic(static_cast<LabelEdit*>(but)->getText(), downloader.getSource()->getChapters(static_cast<ProgDownloader*>(state)->resultUrls[but->getIndex()])));
}

// DOWNLOADS

void Program::eventOpenDownloadList(Button*) {
	setState<ProgDownloads>();
}

void Program::eventDownloadProgress() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state)) {
		Label* lb = pd->list->getWidget<Layout>(0))->getWidget<Label>(0);
		lb->setText(lb->getText() + " - " + toStr(downloader.getDlProg().x) + '/' + toStr(downloader.getDlProg().y));
	}
}

void Program::eventDownloadNext() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state)) {
		pd->list->deleteWidget(0);
		if (!pd->list->getWidgets().empty()) {
			Label* lb = pd->list->getWidget<Layout>(0)->getWidget<Label>(0);
			lb->setText(lb->getText() + " - preparing");
		}
	}
}

void Program::eventDownloadFinish() {
	downloader.finishProc();
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state))
		pd->list->deleteWidget(0);
}

void Program::eventDownloadDelete(Button* but) {
	size_t id = but->getParent()->getIndex();
	static_cast<ProgDownloads*>(state)->list->deleteWidget(id);
	downloader.deleteEntry(id);
}

void Program::eventResumeDownloads(Button*) {
	downloader.startProc();
}

void Program::eventStopDownloads(Button*) {
	downloader.interruptProc();
}

void Program::eventClearDownloads(Button*) {
	downloader.clearQueue();
	static_cast<ProgDownloads*>(state)->list->setWidgets({});
}
#endif

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
	fs::path oldLib = World::sets()->getDirLib();
#ifdef DOWNLOADER
	if (downloader.getDlState() != DownloadState::stop) {
		World::scene()->setPopup(state->createPopupMessage("Can't change while downloading.", &Program::eventClosePopup));
		return;
	}
#endif
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (fs::path path = fs::u8path(le->getText()); World::sets()->setDirLib(path, World::fileSys()->getDirSets()) != path) {
		le->setText(World::sets()->getDirLib().u8string());
		World::scene()->setPopup(state->createPopupMessage("Invalid directory", &Program::eventClosePopup));
	} else
		offerMoveBooks(std::move(oldLib));
}

void Program::eventSetLibraryDirBW(Button*) {
	fs::path oldLib = World::sets()->getDirLib();
	const uset<Widget*>& select = static_cast<ProgFileExplorer*>(state)->fileList->getSelected();

	World::sets()->setDirLib(!select.empty() ? browser->getCurDir() / fs::u8path(static_cast<Label*>(*select.begin())->getText()) : browser->getCurDir(), World::fileSys()->getDirSets());
	browser.reset();
	eventOpenSettings();
	offerMoveBooks(std::move(oldLib));
}

void Program::offerMoveBooks(fs::path&& oldLib) {
	if (World::sets()->getDirLib() != oldLib) {
		static_cast<ProgSettings*>(state)->oldPathBuffer = std::move(oldLib);
		World::scene()->setPopup(state->createPopupChoice("Move books to new location?", &Program::eventMoveComics, &Program::eventClosePopup));
	}
}

void Program::eventOpenLibDirBrowser(Button*) {
#ifdef _WIN32
	const char* home = "UserProfile";
#else
	const char* home = "HOME";
#endif
	if (browser = Browser::openExplorer(Browser::topDir, SDL_getenv(home), &Program::eventOpenSettings); browser)
		setState<ProgSearchDir>();
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
	state->updatePopupMessage("Moving " + toStr(uintptr_t(user.data1)) + '/' + toStr(uintptr_t(user.data2)));
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
	if (u32vec2 device = static_cast<ProgSettings*>(state)->getDvice(finishComboBox(but)); World::sets()->device != device) {
		World::sets()->device = device;
		World::winSys()->recreateWindows();
	}
}

void Program::eventSetCompression(Button* but) {
	if (Settings::Compression compression = Settings::Compression(finishComboBox(but)); World::sets()->compression != compression) {
		World::sets()->compression = compression;
		World::renderer()->setCompression(compression);
	}
}

void Program::eventSetVsync(Button* but) {
	World::sets()->vsync = static_cast<CheckBox*>(but)->on;
	World::renderer()->setVsync(World::sets()->vsync);
}

void Program::eventSetGpuSelecting(Button* but) {
	World::sets()->gpuSelecting = static_cast<CheckBox*>(but)->on;
}

size_t Program::finishComboBox(Button* but) {
	size_t val = but->getIndex();
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(val);
	World::scene()->setContext(nullptr);
	return val;
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
	ComboBox* cmb = static_cast<ComboBox*>(but);
	World::drawSys()->setTheme(cmb->getText(), World::sets(), World::fileSys());
	if (World::sets()->getTheme() != cmb->getText())
		cmb->setText(World::sets()->getTheme());
}

void Program::eventSetFont(Button* but) {
	string otxt = static_cast<Label*>(but)->getText();
	World::drawSys()->setFont(otxt, World::sets(), World::fileSys());
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
	World::renderer()->setMaxPicRes(World::sets()->maxPicRes);
	but->getParent()->getWidget<LabelEdit>(but->getIndex() + 1)->setText(toStr(World::sets()->maxPicRes));
}

void Program::eventSetMaxPicResLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->maxPicRes = toNum<uint>(le->getText());
	World::renderer()->setMaxPicRes(World::sets()->maxPicRes);
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

void Program::eventResizeComboContext(Layout* lay) {
	Context* ctx = static_cast<Context*>(lay);
	ctx->setRect(ProgState::calcTextContextRect(ctx->getWidget<ScrollArea>(0)->getWidgets(), ctx->owner()->position(), ctx->owner()->size(), ctx->getSpacing()));
}

void Program::eventTryExit(Button*) {
#ifdef DOWNLOADER
	if (downloader.getDlState() == DownloadState::stop)
		eventForceExit();
	else
		World::scene()->setPopup(state->createPopupChoice("Cancel downloads?", &Program::eventForceExit, &Program::eventClosePopup));
#else
	eventForceExit();
#endif
}

void Program::eventForceExit(Button*) {
#ifdef DOWNLOADER
	downloader.interruptProc();
#endif
	state->eventClosing();
	World::winSys()->close();
}

void Program::setPopupLoading() {
	World::scene()->setPopup(state->createPopupMessage("Loading...", &Program::eventFileLoadingCancelled, "Cancel", Alignment::center));
}

template <class T, class... A>
void Program::setState(A&&... args) {
	delete state;
	state = new T(std::forward<A>(args)...);
	World::scene()->resetLayouts();
}
