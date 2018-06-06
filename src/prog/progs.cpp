#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(const string& str, int height, int margin) :
	text(str),
	length(World::drawSys()->textLength(str, height) + margin * 2)
{}

int ProgState::findMaxLength(const vector<string>& strs, int height, int margin) {
	int width = 0;
	for (const string& it : strs) {
		int len = World::drawSys()->textLength(it, height) + margin * 2;
		if (len > width)
			width = len;
	}
	return width;
}

// PROGRAM STATE

const int ProgState::lineHeight = 30;
const int ProgState::topHeight = 40;
const int ProgState::topSpacing = 10;
const int ProgState::picSize = 40;
const int ProgState::picMargin = 4;
const vec2s ProgState::messageSize(300, 100);

void ProgState::eventEnter() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else if (World::scene()->select)
		World::scene()->select->onClick(World::scene()->select->position(), SDL_BUTTON_LEFT);
}

void ProgState::eventUp() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onNavSelectUp();
	else
		World::scene()->selectFirst();
}

void ProgState::eventDown() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onNavSelectDown();
	else
		World::scene()->selectFirst();
}

void ProgState::eventLeft() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onNavSelectLeft();
	else
		World::scene()->selectFirst();
}

void ProgState::eventRight() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onNavSelectRight();
	else
		World::scene()->selectFirst();
}

void ProgState::eventCursorUp(float amt) {
	World::winSys()->moveCursor(vec2i(-amt * Default::cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorDown(float amt) {
	World::winSys()->moveCursor(vec2i(amt * Default::cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorLeft(float amt) {
	World::winSys()->moveCursor(vec2i(0, -amt * Default::cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventCursorRight(float amt) {
	World::winSys()->moveCursor(vec2i(0, amt * Default::cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventFullscreen() {
	World::winSys()->setFullscreen(!World::winSys()->sets.fullscreen);
}

Popup* ProgState::createPopupMessage(const string& msg, const vec2<Size>& size) {
	vector<Widget*> bot = {
		new Widget(1.f),
		new Label(1.f, World::drawSys()->translation("ok"), &Program::eventClosePopup, nullptr, nullptr, Label::Alignment::center),
		new Widget(1.f)
	};
	vector<Widget*> con = {
		new Label(1.f, msg),
		new Layout(1.f, bot, false, Layout::Select::none, 0)
	};
	return new Popup(size, con);
}

// PROG BOOKS

void ProgBooks::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventExit();
}

void ProgBooks::eventFileDrop(char* file) {
	World::program()->openFile(file);
}

Layout* ProgBooks::createLayout() {
	Text settings(World::drawSys()->translation("settings"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(settings.length, settings.text, &Program::eventOpenSettings),
		new Label(exit.length, exit.text, &Program::eventExit)
	};

	vector<string> books = Filer::listDir(World::winSys()->sets.getDirLib(), FTYPE_DIR);
	vector<Widget*> tiles(books.size()+1);
	for (sizt i=0; i<books.size(); i++) {
		Text txt(filename(books[i]), Default::itemHeight);
		tiles[i] = new Label(txt.length, txt.text, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage);
	}
	tiles.back() = new Button(Default::itemHeight, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage, nullptr, World::drawSys()->texture("search"));

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, Layout::Select::none, topSpacing),
		new TileBox(1.f, tiles)
	};
	return new Layout(1.f, cont, true, Layout::Select::none, topSpacing);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventFileDrop(char* file) {
	World::program()->openFile(file);
}

Layout* ProgPageBrowser::createLayout() {
	vector<string> txs = {
		World::drawSys()->translation("back"),
		World::drawSys()->translation("up")
	};
	vector<Widget*> bar = {
		new Label(lineHeight, txs[0], &Program::eventExitBrowser),
		new Label(lineHeight, txs[1], &Program::eventBrowserGoUp)
	};

	vector<string> dirs = World::program()->getBrowser()->listDirs();
	vector<string> files = World::program()->getBrowser()->listFiles();
	vector<Widget*> items(dirs.size() + files.size());
	for (sizt i=0; i<dirs.size(); i++)
		items[i] = new Label(lineHeight, dirs[i], &Program::eventBrowserGoIn, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("folder"));
	for (sizt i=0; i<files.size(); i++)
		items[dirs.size()+i] = new Label(lineHeight, files[i], &Program::eventOpenReader, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("file"));
	
	vector<Widget*> cont = {
		new Layout(findMaxLength(txs, lineHeight), bar),
		new ScrollArea(1.f, items)
	};
	return new Layout(1.f, cont, false, Layout::Select::none, topSpacing);
}

// PROG READER

void ProgReader::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventExitReader();
}

void ProgReader::eventUp() {
	eventScrollUp(1.f);
}

void ProgReader::eventDown() {
	eventScrollDown(1.f);
}

void ProgReader::eventLeft() {
	eventScrollLeft(1.f);
}

void ProgReader::eventRight() {
	eventScrollRight(1.f);
}

void ProgReader::eventScrollUp(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(0, -modifySpeed(amt * World::winSys()->sets.scrollSpeed.y)));
}

void ProgReader::eventScrollDown(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(0, modifySpeed(amt * World::winSys()->sets.scrollSpeed.y)));
}

void ProgReader::eventScrollRight(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(modifySpeed(amt * World::winSys()->sets.scrollSpeed.x), 0));
}

void ProgReader::eventScrollLeft(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(-modifySpeed(amt * World::winSys()->sets.scrollSpeed.x), 0));
}

void ProgReader::eventPageUp() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	sizt i = reader->visibleWidgets().l;
	reader->scrollToWidgetPos((i == 0) ? i : i-1);
}

void ProgReader::eventPageDown() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	sizt i = reader->visibleWidgets().u;
	reader->scrollToWidgetPos((i == reader->getWidgets().size()) ? i-1 : i);
}

void ProgReader::eventZoomIn() {
	World::program()->eventZoomIn();
}

void ProgReader::eventZoomOut() {
	World::program()->eventZoomOut();
}

void ProgReader::eventZoomReset() {
	World::program()->eventZoomReset();
}

void ProgReader::eventCenterView() {
	World::program()->eventCenterView();
}

void ProgReader::eventNextDir() {
	World::program()->eventNextDir();
}

void ProgReader::eventPrevDir() {
	World::program()->eventPrevDir();
}

void ProgReader::eventClosing() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	vec2t vis = reader->visibleWidgets();
	if (vis.l < vis.u)
		Filer::saveLastPage(appendDsep(World::program()->getBrowser()->getCurDir()) + reader->getFile(vis.l));
}

Layout* ProgReader::createLayout() {
	return new ReaderBox(1.f, World::program()->getBrowser()->getCurDir());
}

Overlay* ProgReader::createOverlay() {
	vector<Widget*> menu = {
		new Button(picSize, &Program::eventExitReader, nullptr, nullptr, World::drawSys()->texture("cross"), false, picMargin),
		new Button(picSize, &Program::eventNextDir, nullptr, nullptr, World::drawSys()->texture("right"), false, picMargin),
		new Button(picSize, &Program::eventPrevDir, nullptr, nullptr, World::drawSys()->texture("left"), false, picMargin),
		new Button(picSize, &Program::eventZoomIn, nullptr, nullptr, World::drawSys()->texture("plus"), false, picMargin),
		new Button(picSize, &Program::eventZoomOut, nullptr, nullptr, World::drawSys()->texture("minus"), false, picMargin),
		new Button(picSize, &Program::eventZoomReset, nullptr, nullptr, World::drawSys()->texture("circle"), false, picMargin),
		new Button(picSize, &Program::eventCenterView, nullptr, nullptr, World::drawSys()->texture("square"), false, picMargin)
	};
	return new Overlay(vec2s(0), vec2s(picSize, picSize*int(menu.size())), vec2s(0), vec2s(picSize/2, 1.f), menu, true, 0);
}

float ProgReader::modifySpeed(float value) {
	float factor = 1.f;
	if (World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value *= Default::scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value /= Default::scrollFactor * factor;
	return value * World::winSys()->getDSec();
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventOpenBookList();
}

void ProgSettings::eventFullscreen() {
	ProgState::eventFullscreen();
	static_cast<CheckBox*>(static_cast<Layout*>(static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getWidget(0))->getWidget(1))->on = World::winSys()->sets.fullscreen;
}

void ProgSettings::eventFileDrop(char* file) {
	string path = getAbsolute(file);
	if (Filer::isFont(path)) {
		World::drawSys()->setFont(path);
		World::scene()->resetLayouts();
	} else if (Filer::fileType(path) == FTYPE_DIR)
		World::winSys()->sets.setDirLib(path);
}

Layout* ProgSettings::createLayout() {
	Text back(World::drawSys()->translation("back"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(back.length, back.text, &Program::eventOpenBookList),
		new Label(exit.length, exit.text, &Program::eventExit)
	};

	vector<string> txs = {
		World::drawSys()->translation("fullscreen"),
		World::drawSys()->translation("theme"),
		World::drawSys()->translation("language"),
		World::drawSys()->translation("font"),
		World::drawSys()->translation("library"),
		World::drawSys()->translation("renderer"),
		World::drawSys()->translation("scroll speed"),
		World::drawSys()->translation("deadzone")
	};
	sizt lcnt = txs.size();
	const vector<Binding>& bindings = World::inputSys()->getBindings();
	txs.resize(txs.size() + bindings.size());
	for (sizt i=0; i<bindings.size(); i++)
		txs[lcnt+i] = World::drawSys()->translation(enumToStr(Default::bindingNames, i));
	
	int descLength = findMaxLength(txs, lineHeight);
	Text dots("...", lineHeight);
	Text dznum(ntos(Default::axisLimit), lineHeight);
	vector<Widget*> lx[] = { {
		new Label(descLength, txs[0]),
		new CheckBox(lineHeight, World::winSys()->sets.fullscreen, &Program::eventSwitchFullscreen)
	}, {
		new Label(descLength, txs[1]),
		new SwitchBox(1.f, Filer::getAvailibleThemes(), World::winSys()->sets.getTheme(), &Program::eventSetTheme)
	}, {
		new Label(descLength, txs[2]),
		new SwitchBox(1.f, Filer::getAvailibleLanguages(), World::winSys()->sets.getLang(), &Program::eventSwitchLanguage)
	}, {
		new Label(descLength, txs[3]),
		new LineEdit(1.f, World::winSys()->sets.getFont(), &Program::eventSetFont)
	}, {
		new Label(descLength, txs[4]),
		new LineEdit(1.f, World::winSys()->sets.getDirLib(), &Program::eventSetLibraryDirLE),
		new Label(dots.length, dots.text, &Program::eventOpenLibDirBrowser, nullptr, nullptr, Label::Alignment::center)
	}, {
		new Label(descLength, txs[5]),
		new SwitchBox(1.f, Settings::getAvailibleRenderers(), World::winSys()->sets.renderer, &Program::eventSetRenderer)
	}, {
		new Label(descLength, txs[6]),
		new LineEdit(1.f, World::winSys()->sets.getScrollSpeedString(), &Program::eventSetScrollSpeed, nullptr, nullptr, LineEdit::TextType::sFloatingSpaced)
	}, {
		new Label(descLength, txs[7]),
		new Slider(1.f, World::winSys()->sets.getDeadzone(), 0, Default::axisLimit, &Program::eventSetDeadzoneSL),
		new LineEdit(dznum.length, ntos(World::winSys()->sets.getDeadzone()), &Program::eventSetDeadzoneLE, nullptr, nullptr, LineEdit::TextType::uInteger)
	} };
	vector<Widget*> lns(lcnt + 1 + bindings.size());
	for (sizt i=0; i<lcnt; i++)
		lns[i] = new Layout(lineHeight, lx[i], false);
	lns[lcnt] = new Widget(0);
	
	for (sizt i=0; i<bindings.size(); i++) {
		Binding::Type type = static_cast<Binding::Type>(i);
		vector<Widget*> lin {
			new Label(descLength, txs[lcnt+i]),
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, type),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, type),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, type),
		};
		lns[lcnt+1+i] = new Layout(lineHeight, lin, false);
	}

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, Layout::Select::none, topSpacing),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, true, Layout::Select::none, topSpacing);
}

// PROG SEARCH DIR

void ProgSearchDir::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventBrowserGoUp();
}

Layout* ProgSearchDir::createLayout() {
	vector<string> txs = {
		World::drawSys()->translation("back"),
		World::drawSys()->translation("up"),
		World::drawSys()->translation("set")
	};
	vector<Widget*> bar = {
		new Label(lineHeight, txs[0], &Program::eventExitBrowser),
		new Label(lineHeight, txs[1], &Program::eventBrowserGoUp),
		new Label(lineHeight, txs[2], &Program::eventSetLibraryDirBW)
	};

	vector<string> strs = World::program()->getBrowser()->listDirs();
	vector<Widget*> items(strs.size());
	for (sizt i=0; i<strs.size(); i++)
		items[i] = new Label(lineHeight, strs[i], nullptr, nullptr, &Program::eventBrowserGoIn, Label::Alignment::left, World::drawSys()->texture("folder"));

	vector<Widget*> cont = {
		new Layout(findMaxLength(txs, lineHeight), bar),
		new ScrollArea(1.f, items, Layout::Select::one)
	};
	return new Layout(1.f, cont, false, Layout::Select::none, topSpacing);
}
