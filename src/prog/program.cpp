#include "engine/world.h"

// BOOKS

Program::Program() :
	state(new ProgState)	// necessary as a placeholder to prevent nullptr exceptions
{}

void Program::start() {
	if (!(World::getArgs().size() && openFile(World::getArg(0))))
		eventOpenBookList();
}

void Program::eventOpenBookList(Button* but) {
	setState(new ProgBooks);
}

void Program::eventOpenPageBrowser(Button* but) {
	browser.reset(new Browser(dynamic_cast<Label*>(but) ? childPath(World::winSys()->sets.getDirLib(), static_cast<Label*>(but)->getText()) : dseps, "", &Program::eventOpenBookList));
	setState(new ProgPageBrowser);
}

void Program::eventOpenReader(Button* but) {
	startReader(static_cast<Label*>(but)->getText());
}

void Program::eventOpenLastPage(Button* but) {
	Label* lbl = dynamic_cast<Label*>(but);
	string file = Filer::getLastPage(lbl ? lbl->getText() : ".");
	if (file.empty())
		eventOpenPageBrowser(but);
	else {
		browser.reset(new Browser(lbl ? childPath(World::winSys()->sets.getDirLib(), lbl->getText()) : dseps, parentPath(file), &Program::eventOpenBookList));
		if (!startReader(filename(file)))
			eventOpenPageBrowser(but);
	}
}

bool Program::openFile(string file) {
	file = absolutePath(file);
	if (Filer::isPicture(file) || Filer::isArchive(file)) {
		browser.reset(new Browser(dseps, parentPath(file), &Program::eventOpenBookList));
		if (startReader(filename(file)))
			return true;
		browser.reset();
	} else if (Filer::fileType(file) == FTYPE_DIR) {
		browser.reset(new Browser(dseps, file, &Program::eventOpenBookList));
		setState(new ProgPageBrowser);
		return true;
	}
	return false;
}

// BROWSER

void Program::eventBrowserGoUp(Button* but) {
	if (browser->goUp())
		World::scene()->resetLayouts();
	else
		eventExitBrowser();
}

void Program::eventBrowserGoIn(Button* but) {
	if (browser->goIn(static_cast<Label*>(but)->getText()))
		World::scene()->resetLayouts();
}

void Program::eventExitBrowser(Button* but) {
	PCall call = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

bool Program::startReader(const string& filename) {
	if (!browser->selectFile(filename))
		return false;

	setState(new ProgReader);
	return true;
}

void Program::eventZoomIn(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->setZoom(Default::zoomFactor);
}

void Program::eventZoomOut(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->setZoom(1.f / Default::zoomFactor);
}

void Program::eventZoomReset(Button* but) {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	reader->setZoom(1.f / reader->getZoom());
}

void Program::eventCenterView(Button* but) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->centerList();
}

void Program::eventNextDir(Button* but) {
	browser->goNext(true);
	World::scene()->resetLayouts();
}

void Program::eventPrevDir(Button* but) {
	browser->goNext(false);
	World::scene()->resetLayouts();
}

void Program::eventExitReader(Button* but) {
	SDL_ShowCursor(SDL_ENABLE);
	setState(new ProgPageBrowser);
}

// SETTINGS

void Program::eventOpenSettings(Button* but) {
	setState(new ProgSettings);
}

void Program::eventSwitchDirection(Button* but) {
	World::winSys()->sets.direction.set(static_cast<SwitchBox*>(but)->getText());
}

void Program::eventSetZoom(Button * but) {
	World::winSys()->sets.zoom = stof(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSetSpacing(Button* but) {
	World::winSys()->sets.spacing = stoi(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSwitchLanguage(Button* but) {
	World::drawSys()->setLanguage(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetLibraryDirLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	if (World::winSys()->sets.setDirLib(le->getText()) != le->getText()) {
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
		le->setText(World::winSys()->sets.getDirLib());
	}
}

void Program::eventSetLibraryDirBW(Button* but) {
	const uset<Widget*>& select = static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getSelected();
	string path = select.size() ? childPath(browser->getCurDir(), static_cast<Label*>(*select.begin())->getText()) : browser->getCurDir();

	World::winSys()->sets.setDirLib(path);
	browser.reset();
	eventOpenSettings();
}

void Program::eventOpenLibDirBrowser(Button* but) {
#ifdef _WIN32
	browser.reset(new Browser("\\", Filer::wgetenv("UserProfile"), &Program::eventOpenSettings));
#else
	browser.reset(new Browser("/", std::getenv("HOME"), &Program::eventOpenSettings));
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
	LineEdit* le = static_cast<LineEdit*>(but);
	World::drawSys()->setFont(le->getText());
	World::scene()->resetLayouts();
	if (World::winSys()->sets.getFont() != le->getText())
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid font."));
}

void Program::eventSetRenderer(Button* but) {
	World::winSys()->setRenderer(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetScrollSpeed(Button* but) {
	World::winSys()->sets.setScrollSpeed(static_cast<LineEdit*>(but)->getText());
}

void Program::eventSetDeadzoneSL(Button* but) {
	World::winSys()->sets.setDeadzone(static_cast<Slider*>(but)->getVal());
	static_cast<LineEdit*>(but->getParent()->getWidget(2))->setText(ntos(World::winSys()->sets.getDeadzone()));	// update line edit
}

void Program::eventSetDeadzoneLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	World::winSys()->sets.setDeadzone(stoi(le->getText()));
	le->setText(ntos(World::winSys()->sets.getDeadzone()));	// set text again in case the volume was out of range
	static_cast<Slider*>(but->getParent()->getWidget(1))->setVal(World::winSys()->sets.getDeadzone());	// update slider
}

void Program::eventSetPortrait(Button* but) {
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

void Program::eventSetLandscape(Button* but) {
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

void Program::eventSetSquare(Button* but) {
	vec2i res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2f(res.x < res.y ? res.x : res.y) * Default::resModeBorder);
}

void Program::eventSetFill(Button* but) {
	vec2i res = World::winSys()->displayResolution();
	reposizeWindow(res, vec2f(res) * Default::resModeBorder);
}

void Program::eventResetSettings(Button* but) {
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

void Program::eventClosePopup(Button* but) {
	World::scene()->setPopup(nullptr);
}

void Program::eventExit(Button* but) {
	World::winSys()->close();
}

void Program::setState(ProgState* newState) {
	state->eventClosing();
	state.reset(newState);
	World::scene()->resetLayouts();
}
