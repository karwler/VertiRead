#include "engine/world.h"
#include "progs.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "utils/layouts.h"

// PROGRAM

Program::~Program() {
	delete state;
}

void Program::start() {
	if (!(!World::getVals().empty() && openFile(fs::u8path(World::getVals()[0]))))
		eventOpenBookList();
}

void Program::eventUser(const SDL_UserEvent& user) {
	switch (UserCode(user.code)) {
	case UserCode::readerProgress:
		World::scene()->setPopup(state->createPopupMessage("Loading " + static_cast<PictureLoader*>(user.data1)->progVal + '/' + static_cast<PictureLoader*>(user.data1)->progLim, &Program::eventReaderLoadingCancelled, "Cancel", Alignment::center));
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
		eventMoveFinished();
		break;
	default:
		throw std::runtime_error("Invalid user event code: " + toStr(user.code));
	}
}

// BOOKS

void Program::eventOpenBookList(Button*) {
	setState<ProgBooks>();
}

void Program::eventOpenPageBrowser(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	browser = std::make_unique<Browser>(lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) : Browser::topDir, fs::path(), &Program::eventOpenBookList);
	setState<ProgPageBrowser>();
}

void Program::eventOpenBookContext(Button* but) {
	vector<pair<string, PCall>> items = dynamic_cast<Label*>(but) ? vector<pair<string, PCall>>{
		pair("Continue", &Program::eventOpenLastPage),
		pair("Delete", &Program::eventDeleteBook)
	} : vector<pair<string, PCall>>{
		pair("Continue", &Program::eventOpenLastPage)
	};
	World::scene()->setContext(state->createContext(std::move(items), but));
}

void Program::eventOpenLastPage(Button*) {
	Label* lbl = dynamic_cast<Label*>(World::scene()->getContext()->owner());
	if (string drc, fname; World::fileSys()->getLastPage(lbl ? lbl->getText() : ProgState::dotStr, drc, fname)) {
		try {
			browser = std::make_unique<Browser>(lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) : Browser::topDir, lbl ? World::sets()->getDirLib() / fs::u8path(lbl->getText()) / fs::u8path(drc) : fs::u8path(drc), fs::u8path(fname), &Program::eventOpenBookList, true);
			eventStartLoadingReader(fname);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			eventOpenPageBrowser(lbl);
		}
	} else
		eventOpenPageBrowser(World::scene()->getContext()->owner<Button>());
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

bool Program::openFile(const fs::path& file) {
	switch (fs::status(file).type()) {
	case fs::file_type::regular:
		try {
			bool isPic = FileSys::isPicture(file);
			browser = std::make_unique<Browser>(Browser::topDir, isPic ? parentPath(file) : file, isPic ? file.filename() : fs::path(), &Program::eventOpenBookList, false);
			eventStartLoadingReader(file.u8string());
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			return false;
		}
		return true;
	case fs::file_type::directory:
		browser = std::make_unique<Browser>(Browser::topDir, file, &Program::eventOpenBookList);
		setState<ProgPageBrowser>();
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
	switch (LabelEdit* le = static_cast<LabelEdit*>(but); browser->goTo(browser->getRootDir() == Browser::topDir ? fs::u8path(le->getText()) : World::sets()->getDirLib() / fs::u8path(le->getText()))) {
	case fs::file_type::directory:
		World::scene()->resetLayouts();
		break;
	case fs::file_type::regular:
		eventStartLoadingReader(browser->getCurFile().u8string());
		break;
	default:
		le->setText(le->getOldText());
		World::scene()->setPopup(state->createPopupMessage("Invalid path", &Program::eventClosePopup));
	}
}

void Program::eventExitBrowser(Button*) {
	PCall call = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

void Program::eventStartLoadingReader(const string& first, bool fwd) {
	World::scene()->setPopup(state->createPopupMessage("Loading...", &Program::eventReaderLoadingCancelled, "Cancel", Alignment::center));
	threadRunning = true;
	thread = std::thread(browser->getInArchive() ? &DrawSys::loadTexturesArchiveThreaded : &DrawSys::loadTexturesDirectoryThreaded, &threadRunning, std::make_unique<PictureLoader>(browser->getCurDir(), first, World::sets()->picLim, fwd, World::sets()->showHidden));
}

void Program::eventReaderLoadingCancelled(Button*) {
	threadRunning = false;
	thread.join();
	eventClosePopup();
}

void Program::eventReaderLoadingFinished(PictureLoader* pl) {
	thread.join();
	setState<ProgReader>();

	static_cast<ProgReader*>(state)->reader->setWidgets(World::drawSys()->transferPictures(pl->pics));
	delete pl;
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
	if (!picname.empty())
		if (string file = browser->nextFile(picname, fwd, World::sets()->showHidden); !file.empty()) {
			eventStartLoadingReader(file, fwd);
			return;
		}
	browser->goNext(fwd, World::sets()->showHidden);
	eventStartLoadingReader(string(), fwd);
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
	pd->printInfo(vector<pairStr>());
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
		static_cast<CheckBox*>(static_cast<Layout*>(it)->getWidget(0))->on = static_cast<CheckBox*>(but)->on;
}

void Program::eventSelectChapter(Button* but) {
	static_cast<CheckBox*>(static_cast<Layout*>(static_cast<ProgDownloader*>(state)->chapters->getWidget(but->getParent()->getIndex()))->getWidget(0))->toggle();
}

void Program::eventDownloadAllChapters(Button*) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);
	downloader.downloadComic(pd->curInfo());

	pd->chaptersTick->on = false;
	for (Widget* it : pd->chapters->getWidgets())
		static_cast<CheckBox*>(static_cast<Layout*>(it)->getWidget(0))->on = false;
}

void Program::eventDownloadChapter(Button* but) {
	ProgDownloader* pd = static_cast<ProgDownloader*>(state);

	downloader.downloadComic(Comic(static_cast<LabelEdit*>(*pd->results->getSelected().begin())->getText(), { pair(static_cast<Label*>(but)->getText(), pd->resultUrls[but->getParent()->getIndex()]) }));
	static_cast<CheckBox*>(but->getParent()->getWidget(0))->on = false;
}

void Program::eventDownloadComic(Button* but) {
	downloader.downloadComic(Comic(static_cast<LabelEdit*>(but)->getText(), downloader.getSource()->getChapters(static_cast<ProgDownloader*>(state)->resultUrls[but->getIndex()])));
}

// DOWNLOADS

void Program::eventOpenDownloadList(Button*) {
	setState<ProgDownloads>();
}

void Program::eventDownloadListProgress() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state)) {
		Label* lb = static_cast<Label*>(static_cast<Layout*>(pd->list->getWidget(0))->getWidget(0));
		lb->setText(lb->getText() + " - " + toStr(downloader.getDlProg().x) + '/' + toStr(downloader.getDlProg().y));
	}
}

void Program::eventDownloadListNext() {
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state)) {
		pd->list->deleteWidget(0);
		if (!pd->list->getWidgets().empty()) {
			Label* lb = static_cast<Label*>(static_cast<Layout*>(pd->list->getWidget(0))->getWidget(0));
			lb->setText(lb->getText() + " - preparing");
		}
	}
}

void Program::eventDownloadListFinish() {
	downloader.finishProc();
	if (ProgDownloads* pd = dynamic_cast<ProgDownloads*>(state))
		pd->list->deleteWidget(0);
}

void Program::eventDownloadDelete(Button* but) {
	sizet id = but->getParent()->getIndex();
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
	World::sets()->direction = strToEnum<Direction::Dir>(Direction::names, static_cast<Label*>(but)->getText(), Direction::down);
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(World::sets()->direction);
	World::scene()->setContext(nullptr);
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
		World::scene()->setPopup(ProgState::createPopupMessage("Can't change while downloading.", &Program::eventClosePopup));
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
	const uset<Widget*>& select = static_cast<ProgSearchDir*>(state)->list->getSelected();

	World::sets()->setDirLib(!select.empty() ? browser->getCurDir() / fs::u8path(static_cast<Label*>(*select.begin())->getText()) : browser->getCurDir(), World::fileSys()->getDirSets());
	browser.reset();
	eventOpenSettings();
	offerMoveBooks(std::move(oldLib));
}

void Program::offerMoveBooks(fs::path&& oldLib) {
	if (World::sets()->getDirLib() != oldLib) {
		static_cast<ProgSettings*>(state)->oldPathBuffer = std::move(oldLib);
		World::scene()->setPopup(state->createPopupChoice("Move comics to new location?", &Program::eventMoveComics, &Program::eventClosePopup));
	}
}

void Program::eventOpenLibDirBrowser(Button*) {
#ifdef _WIN32
	browser = std::make_unique<Browser>(Browser::topDir, SDL_getenv("UserProfile"), &Program::eventOpenSettings);
#else
	browser = std::make_unique<Browser>(Browser::topDir, SDL_getenv("HOME"), &Program::eventOpenSettings);
#endif
	setState<ProgSearchDir>();
}

void Program::eventMoveComics(Button*) {
	World::scene()->setPopup(state->createPopupMessage("Moving...", &Program::eventReaderLoadingCancelled, "Cancel", Alignment::center));
	ProgSettings* ps = static_cast<ProgSettings*>(state);
	threadRunning = true;
	thread = std::thread(&FileSys::moveContentThreaded, &threadRunning, ps->oldPathBuffer, World::sets()->getDirLib());
	ps->oldPathBuffer.clear();
}

void Program::eventDontMoveComics(Button*) {
	static_cast<ProgSettings*>(state)->oldPathBuffer.clear();
	eventClosePopup();
}

void Program::eventMoveProgress(uptrt prg, uptrt lim) {
	World::scene()->setPopup(state->createPopupMessage("Moving " + toStr(prg) + '/' + toStr(lim), &Program::eventReaderLoadingCancelled, "Cancel", Alignment::center));
}

void Program::eventMoveFinished() {
	thread.join();
	eventClosePopup();
}

void Program::eventSetFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
}

void Program::eventSetHide(Button* but) {
	World::sets()->showHidden = static_cast<CheckBox*>(but)->on;
}

void Program::eventSetTheme(Button* but) {
	World::drawSys()->setTheme(static_cast<ComboBox*>(but)->getText(), World::sets(), World::fileSys());
	World::scene()->resetLayouts();
}

void Program::eventSetFont(Button* but) {
	string otxt = static_cast<LabelEdit*>(but)->getText();
	World::drawSys()->setFont(otxt, World::sets(), World::fileSys());
	World::scene()->resetLayouts();
	if (World::sets()->font != otxt)
		World::scene()->setPopup(state->createPopupMessage("Invalid font", &Program::eventClosePopup));
}

void Program::eventSetRenderer(Button* but) {
	World::winSys()->setRenderer(static_cast<ComboBox*>(but)->getText());
}

void Program::eventSetScrollSpeed(Button* but) {
	World::sets()->scrollSpeed = toVec<vec2>(static_cast<LabelEdit*>(but)->getText());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::sets()->setDeadzone(static_cast<Slider*>(but)->getVal());
	static_cast<ProgSettings*>(state)->deadzoneLE->setText(toStr(World::sets()->getDeadzone()));
}

void Program::eventSetDeadzoneLE(Button* but) {
	LabelEdit* le = static_cast<LabelEdit*>(but);
	World::sets()->setDeadzone(toNum<uint>(le->getText()));
	le->setText(toStr(World::sets()->getDeadzone()));	// set text again in case the value was out of range
	static_cast<ProgSettings*>(state)->deadzoneSL->setVal(World::sets()->getDeadzone());
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
	ProgSettings* ps = static_cast<ProgSettings*>(state);
	World::sets()->picLim.type = strToEnum<PicLim::Type>(PicLim::names, static_cast<Label*>(but)->getText(), PicLim::Type::none);
	World::scene()->getContext()->owner<ComboBox>()->setCurOpt(uint8(World::sets()->picLim.type));
	World::scene()->setContext(nullptr);
	ps->limitLine->replaceWidget(ps->limitLine->getWidgets().size() - 1, static_cast<ProgSettings*>(state)->createLimitEdit());
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
	bool tp = threadRunning;
	if (tp) {
		threadRunning = false;
		thread.join();
	}
	bool pp = World::scene()->getPopup();
	if (pp)
		World::scene()->setPopup(nullptr);
	return tp || pp;
}

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventCloseContext(Button*) {
	World::scene()->setContext(nullptr);
}

void Program::eventResizeComboContext(Layout* lay) {
	Context* ctx = static_cast<Context*>(lay);
	ctx->setRect(ProgState::calcTextContextRect(static_cast<ScrollArea*>(ctx->getWidget(0))->getWidgets(), ctx->owner()->position(), ctx->owner()->size(), ctx->getSpacing()));
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

template <class T, class... A>
void Program::setState(A&&... args) {
	delete state;
	state = new T(std::forward<A>(args)...);
	state->eventRefresh();
}
