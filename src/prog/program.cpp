#include "engine/world.h"

// BOOKS

Program::Program() :
	state(new ProgBooks)
{}

void Program::eventOpenBookList(Button* but) {
	setState(new ProgBooks);
}

void Program::eventOpenPageBrowser(Button* but) {
	browser.reset(new Browser(appendDsep(World::winSys()->sets.getDirLib()) + static_cast<Label*>(but)->getText(), "", &Program::eventOpenBookList));
	setState(new ProgPageBrowser);
}

void Program::eventOpenReader(Button* but) {
	startReader(static_cast<Label*>(but)->getText());
}

void Program::eventOpenLastPage(Button* but) {
	const string& book = static_cast<Label*>(but)->getText();
	string file = Filer::getLastPage(book);
	if (file.empty())
		eventOpenPageBrowser(but);
	else {
		browser.reset(new Browser(appendDsep(World::winSys()->sets.getDirLib()) + book, "", &Program::eventOpenBookList));
		if (!startReader(filename(file)))
			eventOpenPageBrowser(but);
	}
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
	void (Program::*call)(Button*) = browser->exCall;
	browser.reset();
	(this->*call)(nullptr);
}

// READER

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
	static_cast<ReaderBox*>(World::scene()->getLayout())->centerListX();
}

void Program::eventNextDir(Button* but) {
	browser->goNext();
	browser->selectFirstPicture();
	World::scene()->resetLayouts();
}

void Program::eventPrevDir(Button* but) {
	browser->goPrev();
	browser->selectFirstPicture();
	World::scene()->resetLayouts();
}

void Program::eventExitReader(Button* but) {
	SDL_ShowCursor(SDL_ENABLE);
	setState(new ProgPageBrowser);
}

bool Program::startReader(const string& picname) {
	if (!browser->selectPicture(picname))
		return false;

	setState(new ProgReader);
	return true;
}

// SETTINGS

void Program::eventOpenSettings(Button* but) {
	setState(new ProgSettings);
}

void Program::eventSwitchLanguage(Button* but) {
	World::drawSys()->setLanguage(static_cast<SwitchBox*>(but)->getText());
	World::scene()->resetLayouts();
}

void Program::eventSetLibraryDirLE(Button* but) {
	LineEdit* le = static_cast<LineEdit*>(but);
	if (World::winSys()->sets.setDirLib(le->getText())) {
		World::scene()->setPopup(ProgState::createPopupMessage("Invalid directory."));
		le->setText(World::winSys()->sets.getDirLib());
	}
}

void Program::eventSetLibraryDirBW(Button* but) {
	string path = browser->getCurDir();
	if (Label* lbl = dynamic_cast<Label*>(World::scene()->select))
		path = appendDsep(path) + lbl->getText();

	World::winSys()->sets.setDirLib(path);
	browser.reset();
	eventOpenSettings();
}

void Program::eventOpenLibDirBrowser(Button* but) {
#ifdef _WIN32
	browser.reset(new Browser("\\", std::getenv("UserProfile"), &Program::eventOpenSettings));
#else
	browser.reset(new Browser("/", std::getenv("HOME"), &Program::eventOpenSettings));
#endif
	setState(new ProgSearchDir);
}

void Program::eventSwitchFullscreen(Button* but) {
	World::winSys()->setFullscreen(static_cast<CheckBox*>(but)->on);
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
