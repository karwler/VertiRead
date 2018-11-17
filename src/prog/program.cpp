#include "engine/world.h"

// BOOKS

Program::Program() :
	state(new ProgState)	// necessary as a placeholder to prevent nullptr exceptions
{}

void Program::start() {
	if (!(World::getArgs().size() && openFile(World::getArg(0))))
		eventOpenBookList();
}

void Program::eventOpenPageBrowser(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	browser.reset(new Browser(lbl ? childPath(World::sets()->getDirLib(), lbl->getText()) : dseps, "", &Program::eventOpenBookList));
	setState(new ProgPageBrowser);
}

void Program::eventOpenReader(Button* but) {
	if (browser->selectFile(static_cast<Label*>(but)->getText()))
		setState(new ProgReader);
}

void Program::eventOpenLastPage(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	if (string drc, fname; World::fileSys()->getLastPage(lbl ? lbl->getText() : ".", drc, fname)) {
		try {
			browser.reset(new Browser(lbl ? childPath(World::sets()->getDirLib(), lbl->getText()) : dseps, lbl ? childPath(World::sets()->getDirLib(), childPath(lbl->getText(), drc)) : drc, fname, &Program::eventOpenBookList, true));
			setState(new ProgReader);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			eventOpenPageBrowser(but);
		}
	} else
		eventOpenPageBrowser(but);
}

bool Program::openFile(const string& file) {
	switch (FileSys::fileType(file)) {
	case FTYPE_FILE:
		try {
			bool isPic = FileSys::isPicture(file);
			browser.reset(new Browser(dseps, isPic ? parentPath(file) : file, isPic ? filename(file) : "", &Program::eventOpenBookList, false));
			setState(new ProgReader);
		} catch (const std::runtime_error& e) {
			std::cerr << e.what() << std::endl;
			browser.reset();
			return false;
		}
		return true;
	case FTYPE_DIR:
		browser.reset(new Browser(dseps, file, &Program::eventOpenBookList));
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
	if (browser->goIn(static_cast<Label*>(but)->getText()))
		World::scene()->resetLayouts();
}

void Program::eventBrowserGoTo(Button* but) {
	if (LineEdit* le = static_cast<LineEdit*>(but); browser->goTo(browser->getRootDir() == dseps ? le->getText() : childPath(World::sets()->getDirLib(), le->getText())))
		World::scene()->resetLayouts();
	else {
		le->setText(le->getOldText());
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid path."));
	}
}

void Program::eventExitBrowser(Button*) {
	PCall call = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

void Program::eventZoomIn(Button*) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->setZoom(Default::zoomFactor);
}

void Program::eventZoomOut(Button*) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->setZoom(1.f / Default::zoomFactor);
}

void Program::eventZoomReset(Button*) {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	reader->setZoom(1.f / reader->getZoom());
}

void Program::eventCenterView(Button*) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->centerList();
}

void Program::eventNextDir(Button*) {
	browser->goNext(true);
	World::scene()->resetLayouts();
}

void Program::eventPrevDir(Button*) {
	browser->goNext(false);
	World::scene()->resetLayouts();
}

void Program::eventExitReader(Button*) {
	SDL_ShowCursor(SDL_ENABLE);
	setState(new ProgPageBrowser);
}

// SETTINGS

void Program::eventSwitchDirection(Button* but) {
	World::sets()->direction.set(static_cast<SwitchBox*>(but)->getText());
}

void Program::eventSetZoom(Button * but) {
	World::sets()->zoom = sstof(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSetSpacing(Button* but) {
	World::sets()->spacing = sstoul(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSwitchLanguage(Button* but) {
	World::drawSys()->setLanguage(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetLibraryDirLE(Button* but) {
	if (LineEdit* le = static_cast<LineEdit*>(but); World::sets()->setDirLib(le->getText()) != le->getText()) {
		le->setText(World::sets()->getDirLib());
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
	}
}

void Program::eventSetLibraryDirBW(Button*) {
	const uset<Widget*>& select = static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getSelected();
	string path = select.size() ? childPath(browser->getCurDir(), static_cast<Label*>(*select.begin())->getText()) : browser->getCurDir();

	World::sets()->setDirLib(path);
	browser.reset();
	eventOpenSettings();
}

void Program::eventOpenLibDirBrowser(Button*) {
#ifdef _WIN32
	browser.reset(new Browser(dseps, FileSys::wgetenv("UserProfile"), &Program::eventOpenSettings));
#else
	browser.reset(new Browser(dseps, getenv("HOME"), &Program::eventOpenSettings));
#endif
	setState(new ProgSearchDir);
}

void Program::eventSwitchFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
	World::scene()->onResize();
}

void Program::eventSetTheme(Button* but) {
	World::drawSys()->setTheme(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetFont(Button* but) {
	string otxt = static_cast<LineEdit*>(but)->getText();
	World::drawSys()->setFont(otxt);
	World::scene()->resetLayouts();
	if (World::sets()->getFont() != otxt)
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid font."));
}

void Program::eventSetRenderer(Button* but) {
	World::winSys()->setRenderer(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetScrollSpeed(Button* but) {
	World::sets()->setScrollSpeed(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::sets()->setDeadzone(static_cast<Slider*>(but)->getVal());
	static_cast<LineEdit*>(but->getParent()->getWidget(2))->setText(to_string(World::sets()->getDeadzone()));	// update line edit
}

void Program::eventSetDeadzoneLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	World::sets()->setDeadzone(sstoul(le->getText()));
	le->setText(to_string(World::sets()->getDeadzone()));	// set text again in case the volume was out of range
	static_cast<Slider*>(but->getParent()->getWidget(1))->setVal(World::sets()->getDeadzone());	// update slider
}

void Program::eventSetPortrait(Button*) {
	vec2i res = World::winSys()->displayResolution();
	float width, height;
	if (res.x < res.y) {
		width = float(res.x) * Default::resModeBorder;
		height = width / Default::resModeRatio;
	} else {
		height = float(res.y) * Default::resModeBorder;
		width = height * Default::resModeRatio;
	}
	reposizeWindow(res, vec2i(height * Default::resModeRatio, height));
}

void Program::eventSetLandscape(Button*) {
	vec2i res = World::winSys()->displayResolution();
	float width, height;
	if (res.x < res.y) {
		width = float(res.x) * Default::resModeBorder;
		height = width * Default::resModeRatio;
	} else {
		height = float(res.y) * Default::resModeBorder;
		width = height / Default::resModeRatio;
	}
	reposizeWindow(res, vec2i(width, height));
}

void Program::eventSetSquare(Button*) {
	vec2i res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2f(res.x < res.y ? res.x : res.y) * Default::resModeBorder);
}

void Program::eventSetFill(Button*) {
	vec2i res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2f(res) * Default::resModeBorder);
}

void Program::eventResetSettings(Button*) {
	World::winSys()->resetSettings();
	World::inputSys()->resetBindings();
	World::scene()->resetLayouts();
}

void Program::reposizeWindow(const vec2i& dres, const vec2i& wsiz) {
	World::winSys()->setWindowPos((dres - wsiz) / 2);
	World::winSys()->setResolution(wsiz);
	World::scene()->onResize();
}

// OTHER

void Program::eventClosePopup(Button*) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button*) {
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state->eventClosing();
	state.reset(newState);
	World::scene()->resetLayouts();
}
