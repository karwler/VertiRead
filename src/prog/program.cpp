#include "engine/world.h"

// PROGRAM

void Program::start() {
	if (!(!World::getVals().empty() && openFile(fs::u8path(World::getVals()[0]))))
		eventOpenBookList();
}

void Program::eventUser(const SDL_UserEvent& user) {
	switch (UserCode(user.code)) {
	case UserCode::readerProgress:
		World::scene()->setPopup(ProgState::createPopupMessage("Loading " + static_cast<PictureLoader*>(user.data1)->progVal + '/' + static_cast<PictureLoader*>(user.data1)->progLim, &Program::eventReaderLoadingCancelled, "Cancel", Label::Alignment::center));
		break;
	case UserCode::readerFinished:
		eventReaderLoadingFinished(static_cast<PictureLoader*>(user.data1));
		break;
	case UserCode::downloadProgress:
		eventDownloadListProgress();
		break;
	case UserCode::downloadNext:
		eventDownloadListNext();
		break;
	case UserCode::downlaodFinished:
		eventDownloadListFinish();
		break;
	case UserCode::moveProgress:
		eventMoveProgress(uptrt(user.data1), uptrt(user.data2));
		break;
	case UserCode::moveFinished:
		thread.reset();
		eventClosePopup();
		break;
	default:
		std::cerr << "unknown user event code: " << user.code << std::endl;
	}
}

// BOOKS

void Program::eventOpenPageBrowser(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	browser = std::make_unique<Browser>(lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) : topDir, fs::path(), &Program::eventOpenBookList);
	setState(new ProgPageBrowser);
}

void Program::eventOpenLastPage(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	if (string drc, fname; World::fileSys()->getLastPage(lbl ? lbl->getText() : ProgState::dotStr, drc, fname)) {
		try {
			browser = std::make_unique<Browser>(lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) : topDir, lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) / fs::u8path(drc) : fs::u8path(drc), fs::u8path(fname), &Program::eventOpenBookList, true);
			eventStartLoadingReader(fname);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			eventOpenPageBrowser(but);
		}
	} else
		eventOpenPageBrowser(but);
}

bool Program::openFile(const fs::path& file) {
	switch (fs::status(file).type()) {
	case fs::file_type::regular:
		try {
			bool isPic = FileSys::isPicture(file);
			browser = std::make_unique<Browser>(topDir, isPic ? parentPath(file) : file, isPic ? file.filename() : fs::path(), &Program::eventOpenBookList, false);
			eventStartLoadingReader(file.u8string());
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			browser.reset();
			return false;
		}
		return true;
	case fs::file_type::directory:
		browser = std::make_unique<Browser>(topDir, file, &Program::eventOpenBookList);
		setState(new ProgPageBrowser);
		return true;
	}
	return false;
}

// BROWSER

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
	switch (browser->goFile(fs::u8path(static_cast<Label*>(but)->getText()))) {
	case fs::file_type::regular:
		eventStartLoadingReader(browser->getCurFile().u8string());
		break;
	case fs::file_type::directory:
		World::scene()->resetLayouts();
	}
}

void Program::eventBrowserGoTo(Button* but) {
	switch (LabelEdit* le = static_cast<LabelEdit*>(but); browser->goTo(browser->getRootDir() == topDir ? fs::u8path(le->getText()) : World::sets()->getDirLib() / fs::u8path(le->getText()))) {
	case fs::file_type::directory:
		World::scene()->resetLayouts();
		break;
	case fs::file_type::regular:
		eventStartLoadingReader(browser->getCurFile().u8string());
		break;
	default:
		le->setText(le->getOldText());
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid path", &Program::eventClosePopup));
	}
}

void Program::eventExitBrowser(Button*) {
	PCall call = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

void Program::eventStartLoadingReader(const string& first, bool fwd) {
	World::scene()->setPopup(ProgState::createPopupMessage("Loading...", &Program::eventReaderLoadingCancelled, "Cancel", Label::Alignment::center));
	thread = std::make_unique<Thread>(World::browser()->getInArchive() ? &DrawSys::loadTexturesArchiveThreaded : &DrawSys::loadTexturesDirectoryThreaded, new PictureLoader(World::drawSys(), browser->getCurDir(), first, World::sets()->picLim, fwd, World::sets()->showHidden));
}

void Program::eventReaderLoadingCancelled(Button*) {
	thread.reset();
	eventClosePopup();
}

void Program::eventReaderLoadingFinished(PictureLoader* pl) {
	thread.reset();
	setState(new ProgReader);

	static_cast<ProgReader*>(state.get())->reader->setWidgets(std::move(pl->pics));
	delete pl;
}

void Program::eventZoomIn(Button*) {
	static_cast<ProgReader*>(state.get())->reader->setZoom(zoomFactor);
}

void Program::eventZoomOut(Button*) {
	static_cast<ProgReader*>(state.get())->reader->setZoom(1.f / zoomFactor);
}

void Program::eventZoomReset(Button*) {
	ProgReader* pr = static_cast<ProgReader*>(state.get());
	pr->reader->setZoom(1.f / pr->reader->getZoom());
}

void Program::eventCenterView(Button*) {
	static_cast<ProgReader*>(state.get())->reader->centerList();
}

void Program::eventNextDir(Button*) {
	switchPictures(true, static_cast<ProgReader*>(state.get())->reader->lastPage());
}

void Program::eventPrevDir(Button*) {
	switchPictures(false, static_cast<ProgReader*>(state.get())->reader->firstPage());
}

void Program::switchPictures(bool fwd, const string& picname) {
	if (!picname.empty())
		if (string file = World::browser()->nextFile(picname, fwd, World::sets()->showHidden); !file.empty()) {
			World::program()->eventStartLoadingReader(file, fwd);
			return;
		}
	browser->goNext(fwd, World::sets()->showHidden);
	eventStartLoadingReader(string(), fwd);
}

void Program::eventExitReader(Button*) {
	state->eventClosing();
	setState(new ProgPageBrowser);
}

// DOWNLOADER
#ifdef DOWNLOADER
void Program::eventOpenDownloader(Button*) {
	setState(new ProgDownloader);
}

void Program::eventSwitchSource(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state.get());

	downloader.setSource(WebSource::Type(static_cast<SwitchBox*>(but)->getCurOpt()));
	pd->printResults({});
	pd->printInfo(vector<pairStr>());
}

void Program::eventQuery(Button*) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state.get());
	pd->printResults(downloader.getSource()->query(pd->query->getText()));
}

void Program::eventShowComicInfo(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state.get());
	WebSource* wsrc = downloader.getSource();
	string url = pd->resultUrls[but->getIndex()];
	pd->printInfo(wsrc->getChapters(url));
}

void Program::eventSelectAllChapters(Button* but) {
	for (Widget* it : static_cast<ProgDownloader*>(state.get())->chapters->getWidgets())
		static_cast<CheckBox*>(static_cast<Layout*>(it)->getWidget(0))->on = static_cast<CheckBox*>(but)->on;
}

void Program::eventSelectChapter(Button* but) {
	static_cast<CheckBox*>(static_cast<Layout*>(static_cast<ProgDownloader*>(state.get())->chapters->getWidget(but->getParent()->getIndex()))->getWidget(0))->toggle();
}

void Program::eventDownloadAllChapters(Button*) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state.get());
	downloader.downloadComic(pd->curInfo());

	pd->chaptersTick->on = false;
	for (Widget* it : pd->chapters->getWidgets())
		static_cast<CheckBox*>(static_cast<Layout*>(it)->getWidget(0))->on = false;
}

void Program::eventDownloadChapter(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state.get());

	downloader.downloadComic(Comic(static_cast<LabelEdit*>(*pd->results->getSelected().begin())->getText(), { pair(static_cast<Label*>(but)->getText(), pd->resultUrls[but->getParent()->getIndex()]) }));
	static_cast<CheckBox*>(but->getParent()->getWidget(0))->on = false;
}

void Program::eventDownloadComic(Button* but) {
	downloader.downloadComic(Comic(static_cast<LabelEdit*>(but)->getText(), downloader.getSource()->getChapters(static_cast<ProgDownloader*>(state.get())->resultUrls[but->getIndex()])));
}

// DOWNLOADS

void Program::eventOpenDownloadList(Button*) {
	setState(new ProgDownloads);
}

void Program::eventDownloadListProgress() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state.get())) {
		Label* lb = static_cast<Label*>(static_cast<Layout*>(pd->list->getWidget(0))->getWidget(0));
		lb->setText(lb->getText() + " - " + to_string(downloader.getDlProg().x) + '/' + to_string(downloader.getDlProg().y));
	}
}

void Program::eventDownloadListNext() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state.get())) {
		pd->list->deleteWidget(0);
		if (!pd->list->getWidgets().empty()) {
			Label* lb = static_cast<Label*>(static_cast<Layout*>(pd->list->getWidget(0))->getWidget(0));
			lb->setText(lb->getText() + " - preparing");
		}
	}
}

void Program::eventDownloadListFinish() {
	downloader.finishProc();
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state.get()))
		pd->list->deleteWidget(0);
}

void Program::eventDownloadDelete(Button* but) {
	sizet id = but->getParent()->getIndex();
	static_cast<ProgDownloads*>(state.get())->list->deleteWidget(id);
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
	static_cast<ProgDownloads*>(state.get())->list->setWidgets({});
}
#endif
// SETTINGS

void Program::eventSwitchDirection(Button* but) {
	World::sets()->direction = strToEnum<Direction::Dir>(Direction::names, static_cast<SwitchBox*>(but)->getText());
}

void Program::eventSetZoom(Button* but) {
	World::sets()->zoom = sstof(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetSpacing(Button* but) {
	World::sets()->spacing = int(sstoul(static_cast<LabelEdit*>(but)->getText()));
}

void Program::eventSetLibraryDirLE(Button* but) {
	fs::path oldLib = World::sets()->getDirLib();
#ifdef DOWNLOADER
	if (downloader.getDlState() != DownloadState::stop) {
		World::scene()->setPopup(ProgState::createPopupMessage("Can't change while downloading.", &Program::eventClosePopup));
		return;
	}
#endif
	LabelEdit* le = static_cast<LabelEdit*>(but);
	if (fs::path path = fs::u8path(le->getText()); World::sets()->setDirLib(path, World::fileSys()->getDirSets()) != path) {
		le->setText(World::sets()->getDirLib().u8string());
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory", &Program::eventClosePopup));
	} else
		offerMoveBooks(std::move(oldLib));
}

void Program::eventSetLibraryDirBW(Button*) {
	fs::path oldLib = World::sets()->getDirLib();
	const uset<Widget*>& select = static_cast<ProgSearchDir*>(state.get())->list->getSelected();

	World::sets()->setDirLib(!select.empty() ? browser->getCurDir() / fs::u8path(static_cast<Label*>(*select.begin())->getText()) : browser->getCurDir(), World::fileSys()->getDirSets());
	browser.reset();
	eventOpenSettings();
	offerMoveBooks(std::move(oldLib));
}

void Program::offerMoveBooks(fs::path&& oldLib) {
	if (World::sets()->getDirLib() != oldLib) {
		static_cast<ProgSettings*>(state.get())->oldPathBuffer = std::move(oldLib);
		World::scene()->setPopup(ProgState::createPopupChoice("Move comics to new location?", &Program::eventMoveComics, &Program::eventClosePopup));
	}
}

void Program::eventOpenLibDirBrowser(Button*) {
#ifdef _WIN32
	browser = std::make_unique<Browser>(topDir, SDL_getenv("UserProfile"), &Program::eventOpenSettings);
#else
	browser = std::make_unique<Browser>(topDir, SDL_getenv("HOME"), &Program::eventOpenSettings);
#endif
	setState(new ProgSearchDir);
}

void Program::eventMoveComics(Button*) {
	World::scene()->setPopup(ProgState::createPopupMessage("Moving...", &Program::eventReaderLoadingCancelled, "Cancel", Label::Alignment::center));
	ProgSettings* ps = static_cast<ProgSettings*>(state.get());

	thread = std::make_unique<Thread>(&FileSys::moveContentThreaded, new pairPath(ps->oldPathBuffer, World::sets()->getDirLib()));
	ps->oldPathBuffer.clear();
}

void Program::eventDontMoveComics(Button*) {
	static_cast<ProgSettings*>(state.get())->oldPathBuffer.clear();
	eventClosePopup();
}

void Program::eventMoveProgress(uptrt prg, uptrt lim) {
	World::scene()->setPopup(ProgState::createPopupMessage("Moving " + to_string(prg) + '/' + to_string(lim), &Program::eventReaderLoadingCancelled, "Cancel", Label::Alignment::center));
}

void Program::eventMoveFinished() {
	thread.reset();
	eventClosePopup();
}

void Program::eventSetFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
}

void Program::eventSetHide(Button* but) {
	World::sets()->showHidden = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetTheme(Button* but) {
	World::drawSys()->setTheme(static_cast<SwitchBox*>(but)->getText(), World::sets(), World::fileSys());
	World::scene()->resetLayouts();
}

void Program::eventSetFont(Button* but) {
	string otxt = static_cast<LabelEdit*>(but)->getText();
	World::drawSys()->setFont(otxt, World::sets(), World::fileSys());
	World::scene()->resetLayouts();
	if (World::sets()->font != otxt)
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid font", &Program::eventClosePopup));
}

void Program::eventSetRenderer(Button* but) {
	World::winSys()->setRenderer(static_cast<SwitchBox*>(but)->getText());
}

void Program::eventSetScrollSpeed(Button* but) {
	World::sets()->scrollSpeed = stofv<vec2>(static_cast<LabelEdit*>(but)->getText().c_str());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::sets()->setDeadzone(static_cast<Slider*>(but)->getVal());
	static_cast<ProgSettings*>(state.get())->deadzoneLE->setText(to_string(World::sets()->getDeadzone()));
}

void Program::eventSetDeadzoneLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->setDeadzone(int(sstoul(le->getText())));
	le->setText(to_string(World::sets()->getDeadzone()));	// set text again in case the value was out of range
	static_cast<ProgSettings*>(state.get())->deadzoneSL->setVal(World::sets()->getDeadzone());
}

void Program::eventSetPortrait(Button*) {
	ivec2 res = World::winSys()->displayResolution();
	float width, height;
	if (res.x < res.y) {
		width = float(res.x) * resModeBorder;
		height = width / resModeRatio;
	} else {
		height = float(res.y) * resModeBorder;
		width = height * resModeRatio;
	}
	reposizeWindow(res, ivec2(height * resModeRatio, height));
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
	ProgSettings* ps = static_cast<ProgSettings*>(state.get());
	World::sets()->picLim.type = strToEnum<PicLim::Type>(PicLim::names, static_cast<SwitchBox*>(but)->getText());
	ps->limitLine->replaceWidget(ps->limitLine->getWidgets().size() - 1, static_cast<ProgSettings*>(state.get())->createLimitEdit());
}

void Program::eventSetPicLimCount(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->picLim.setCount(le->getText());
	le->setText(to_string(World::sets()->picLim.getCount()));
}

void Program::eventSetPicLimSize(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->picLim.setSize(le->getText());
	le->setText(memoryString(World::sets()->picLim.getSize()));
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

bool Program::tryClosePopupThread() {
	bool tp = thread.get();
	if (tp)
		thread.reset();
	bool pp = World::scene()->getPopup();
	if (pp)
		World::scene()->setPopup(nullptr);
	return tp || pp;
}

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventTryExit(Button*) {
#ifdef DOWNLOADER
	if (downloader.getDlState() == DownloadState::stop)
		eventForceExit();
	else
		World::scene()->setPopup(ProgState::createPopupChoice("Cancel downloads?", &Program::eventForceExit, &Program::eventClosePopup));
#else
	eventForceExit();
#endif
}

void Program::eventForceExit(Button*) {
	downloader.interruptProc();
	state->eventClosing();
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state.reset(newState);
	state->eventRefresh();
}
