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

bool ProgState::tryClosePopup() {
	if (World::scene()->getPopup()) {
		World::program()->eventClosePopup();
		return true;
	}
	return false;
}

void ProgState::eventSelect(const Direction& dir) {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onNavSelect(dir);
	else
		World::scene()->selectFirst();
}

void ProgState::eventUp() {
	eventSelect(Direction::up);
}

void ProgState::eventDown() {
	eventSelect(Direction::down);
}

void ProgState::eventLeft() {
	eventSelect(Direction::left);
}

void ProgState::eventRight() {
	eventSelect(Direction::right);
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
	World::winSys()->setFullscreen(!World::sets()->fullscreen);
}

void ProgState::eventRefresh() {
	World::scene()->resetLayouts();
}

Layout* ProgState::createLayout() {
	return nullptr;
}

Overlay* ProgState::createOverlay() {
	return nullptr;
}

Popup* ProgState::createPopupMessage(const string& msg, const vec2<Size>& size) {
	vector<Widget*> bot = {
		new Widget(1.f),
		new Label(1.f, World::drawSys()->translation("ok"), &Program::eventClosePopup, nullptr, nullptr, Label::Alignment::center),
		new Widget(1.f)
	};
	vector<Widget*> con = {
		new Label(1.f, msg),
		new Layout(1.f, bot, Direction::right, Layout::Select::none, 0)
	};
	return new Popup(size, con);
}

// PROG BOOKS

void ProgBooks::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventExit();
}

void ProgBooks::eventFileDrop(const string& file) {
	World::program()->openFile(file);
}

Layout* ProgBooks::createLayout() {
	// top bar
	Text settings(World::drawSys()->translation("settings"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(settings.length, settings.text, &Program::eventOpenSettings),
		new Label(exit.length, exit.text, &Program::eventExit)
	};

	// book list
	vector<string> books = FileSys::listDir(World::sets()->getDirLib(), FTYPE_DIR);
	vector<Widget*> tiles(books.size()+1);
	for (sizt i = 0; i < books.size(); i++) {
		Text txt(filename(books[i]), Default::itemHeight);
		tiles[i] = new Label(txt.length, txt.text, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage);
	}
	tiles.back() = new Button(Default::itemHeight, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage, nullptr, World::drawSys()->texture("search"));

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, top, Direction::right, Layout::Select::none, topSpacing),
		new TileBox(1.f, tiles)
	};
	return new Layout(1.f, cont, Direction::down, Layout::Select::none, topSpacing);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventFileDrop(const string& file) {
	World::program()->openFile(file);
}

Layout* ProgPageBrowser::createLayout() {
	// sidebar
	vector<string> txs = {
		World::drawSys()->translation("back"),
		World::drawSys()->translation("up")
	};
	vector<Widget*> bar = {
		new Label(lineHeight, txs[0], &Program::eventExitBrowser),
		new Label(lineHeight, txs[1], &Program::eventBrowserGoUp)
	};

	// list of files and directories
	pair<vector<string>, vector<string>> entries = FileSys::listDirSeparate(World::browser()->getCurDir());
	vector<Widget*> items(entries.first.size() + entries.second.size());
	for (sizt i = 0; i < entries.second.size(); i++)
		items[i] = new Label(lineHeight, entries.second[i], &Program::eventBrowserGoIn, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("folder"));
	for (sizt i = 0; i < entries.first.size(); i++)
		items[entries.second.size()+i] = new Label(lineHeight, entries.first[i], &Program::eventOpenReader, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("file"));
	
	// main content
	vector<Widget*> mid = {
		new Layout(findMaxLength(txs, lineHeight), bar),
		new ScrollArea(1.f, items)
	};

	// root layout
	vector<Widget*> cont = {
		new LineEdit(lineHeight, World::browser()->getRootDir() == dseps ? World::browser()->getCurDir() : World::browser()->getCurDir().substr(appendDsep(World::sets()->getDirLib()).length()), &Program::eventBrowserGoTo),
		new Layout(1.f, mid, Direction::right, Layout::Select::none, topSpacing)
	};
	return new Layout(1.f, cont, Direction::down, Layout::Select::none, topSpacing);
}

// PROG READER

void ProgReader::eventEscape() {
	if (!tryClosePopup())
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
	World::scene()->getLayout()->onScroll(vec2i(0, -modifySpeed(amt * World::sets()->scrollSpeed.y)));
}

void ProgReader::eventScrollDown(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(0, modifySpeed(amt * World::sets()->scrollSpeed.y)));
}

void ProgReader::eventScrollRight(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(modifySpeed(amt * World::sets()->scrollSpeed.x), 0));
}

void ProgReader::eventScrollLeft(float amt) {
	World::scene()->getLayout()->onScroll(vec2i(-modifySpeed(amt * World::sets()->scrollSpeed.x), 0));
}

void ProgReader::eventNextPage() {
	if (!static_cast<ReaderBox*>(World::scene()->getLayout())->scrollToNext())
		World::program()->eventNextDir();
}

void ProgReader::eventPrevPage() {
	if (!static_cast<ReaderBox*>(World::scene()->getLayout())->scrollToPrevious())
		World::program()->eventPrevDir();
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

void ProgReader::eventToStart() {
	static_cast<ReaderBox*>(World::scene()->getLayout())->scrollToLimit(true);
}

void ProgReader::eventToEnd() {
	static_cast<ReaderBox*>(World::scene()->getLayout())->scrollToLimit(false);
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
	if (string rpath = getChild(World::browser()->getCurDir(), World::sets()->getDirLib()); rpath.empty())
		World::fileSys()->saveLastPage(".", World::browser()->getCurDir(), static_cast<ReaderBox*>(World::scene()->getLayout())->curPage());
	else {
		sizt mid = rpath.find_first_of(dsep);
		sizt nxt = rpath.find_first_not_of(dsep, mid);
		World::fileSys()->saveLastPage(rpath.substr(0, mid), nxt < rpath.length() ? rpath.substr(nxt) : "", static_cast<ReaderBox*>(World::scene()->getLayout())->curPage());
	}
	World::browser()->clearCurFile();
}

Layout* ProgReader::createLayout() {
	return new ReaderBox(1.f, World::sets()->direction, World::sets()->zoom, World::sets()->spacing);
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
	return new Overlay(vec2s(0), vec2s(picSize, picSize*int(menu.size())), vec2s(0), vec2s(picSize/2, picSize*int(menu.size())), menu, Direction::down, 0);
}

int ProgReader::modifySpeed(float value) {
	if (float factor = 1.f; World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value *= Default::scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::scrollSlow, factor))
		value /= Default::scrollFactor * factor;
	return int(value * World::winSys()->getDSec());
}

// PROG SETTINGS

void ProgSettings::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventOpenBookList();
}

void ProgSettings::eventFullscreen() {
	ProgState::eventFullscreen();
	static_cast<CheckBox*>(static_cast<Layout*>(static_cast<Layout*>(World::scene()->getLayout()->getWidget(1))->getWidget(0))->getWidget(1))->on = World::sets()->fullscreen;
}

void ProgSettings::eventFileDrop(const string& file) {
	if (FileSys::isFont(file)) {
		World::drawSys()->setFont(file);
		World::scene()->resetLayouts();
	} else if (FileSys::fileType(file) == FTYPE_DIR)
		World::sets()->setDirLib(file);
}

Layout* ProgSettings::createLayout() {
	// top bar
	Text back(World::drawSys()->translation("back"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(back.length, back.text, &Program::eventOpenBookList),
		new Label(exit.length, exit.text, &Program::eventExit)
	};

	// setting buttons and labels
	vector<Text> butts = {
		Text(World::drawSys()->translation("portrait"), lineHeight),
		Text(World::drawSys()->translation("landscape"), lineHeight),
		Text(World::drawSys()->translation("square"), lineHeight),
		Text(World::drawSys()->translation("fill"), lineHeight)
	};
	vector<string> txs = {
		World::drawSys()->translation("direction"),
		World::drawSys()->translation("zoom"),
		World::drawSys()->translation("spacing"),
		World::drawSys()->translation("fullscreen"),
		World::drawSys()->translation("size"),
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
	for (sizt i = 0; i < bindings.size(); i++)
		txs[lcnt+i] = World::drawSys()->translation(enumToStr(Default::bindingNames, i));
	
	// action fields for labels
	int descLength = findMaxLength(txs, lineHeight);
	Text dots("...", lineHeight);
	Text dznum(to_string(Default::axisLimit), lineHeight);
	vector<Widget*> lx[] = { {
		new Label(descLength, txs[0]),
		new SwitchBox(1.f, Default::directionNames, World::sets()->direction.toString(), &Program::eventSwitchDirection)
	}, {
		new Label(descLength, txs[1]),
		new LineEdit(1.f, trimZero(to_string(World::sets()->zoom)), &Program::eventSetZoom, nullptr, nullptr, LineEdit::TextType::uFloating)
	}, {
		new Label(descLength, txs[2]),
		new LineEdit(1.f, to_string(World::sets()->spacing), &Program::eventSetSpacing, nullptr, nullptr, LineEdit::TextType::uInteger)
	}, {
		new Label(descLength, txs[3]),
		new CheckBox(lineHeight, World::sets()->fullscreen, &Program::eventSwitchFullscreen)
	}, {
		new Label(descLength, txs[4]),
		new Label(butts[0].length, butts[0].text, &Program::eventSetPortrait),
		new Label(butts[1].length, butts[1].text, &Program::eventSetLandscape),
		new Label(butts[2].length, butts[2].text, &Program::eventSetSquare),
		new Label(butts[3].length, butts[3].text, &Program::eventSetFill)
	}, {
		new Label(descLength, txs[5]),
		new SwitchBox(1.f, World::fileSys()->getAvailibleThemes(), World::sets()->getTheme(), &Program::eventSetTheme)
	}, {
		new Label(descLength, txs[6]),
		new SwitchBox(1.f, World::fileSys()->getAvailibleLanguages(), World::sets()->getLang(), &Program::eventSwitchLanguage)
	}, {
		new Label(descLength, txs[7]),
		new LineEdit(1.f, World::sets()->getFont(), &Program::eventSetFont)
	}, {
		new Label(descLength, txs[8]),
		new LineEdit(1.f, World::sets()->getDirLib(), &Program::eventSetLibraryDirLE),
		new Label(dots.length, dots.text, &Program::eventOpenLibDirBrowser, nullptr, nullptr, Label::Alignment::center)
	}, {
		new Label(descLength, txs[9]),
		new SwitchBox(1.f, getAvailibleRenderers(), World::sets()->renderer, &Program::eventSetRenderer)
	}, {
		new Label(descLength, txs[10]),
		new LineEdit(1.f, World::sets()->getScrollSpeedString(), &Program::eventSetScrollSpeed, nullptr, nullptr, LineEdit::TextType::sFloatingSpaced)
	}, {
		new Label(descLength, txs[11]),
		new Slider(1.f, World::sets()->getDeadzone(), 0, Default::axisLimit, &Program::eventSetDeadzoneSL),
		new LineEdit(dznum.length, to_string(World::sets()->getDeadzone()), &Program::eventSetDeadzoneLE, nullptr, nullptr, LineEdit::TextType::uInteger)
	} };
	vector<Widget*> lns(lcnt + 1 + bindings.size() + 2);
	for (sizt i = 0; i < lcnt; i++)
		lns[i] = new Layout(lineHeight, lx[i], Direction::right);
	lns[lcnt] = new Widget(0);
	
	// shortcut entries
	for (sizt i = 0; i < bindings.size(); i++) {
		vector<Widget*> lin {
			new Label(descLength, txs[lcnt+i]),
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, Binding::Type(i)),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, Binding::Type(i)),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, Binding::Type(i))
		};
		lns[lcnt+1+i] = new Layout(lineHeight, lin, Direction::right);
	}
	lcnt += 1 + bindings.size();

	// reset button
	Text resbut(World::drawSys()->translation("reset"), lineHeight);
	vector<Widget*> resln = {
		new Label(resbut.length, resbut.text, &Program::eventResetSettings)
	};
	lns[lcnt] = new Widget(0);
	lns[lcnt+1] = new Layout(lineHeight, resln, Direction::right);

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, top, Direction::right, Layout::Select::none, topSpacing),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, Direction::down, Layout::Select::none, topSpacing);
}

// PROG SEARCH DIR

void ProgSearchDir::eventEscape() {
	if (!tryClosePopup())
		World::program()->eventBrowserGoUp();
}

Layout* ProgSearchDir::createLayout() {
	// sidebar
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

	// directory list
	vector<string> strs = FileSys::listDir(World::browser()->getCurDir(), FTYPE_DIR);
	vector<Widget*> items(strs.size());
	for (sizt i = 0; i < strs.size(); i++)
		items[i] = new Label(lineHeight, strs[i], nullptr, nullptr, &Program::eventBrowserGoIn, Label::Alignment::left, World::drawSys()->texture("folder"));

	// main content
	vector<Widget*> mid = {
		new Layout(findMaxLength(txs, lineHeight), bar),
		new ScrollArea(1.f, items, Direction::down, Layout::Select::one)
	};

	// root layout
	vector<Widget*> cont = {
		new LineEdit(lineHeight, World::browser()->getCurDir(), &Program::eventBrowserGoTo),
		new Layout(1.f, mid, Direction::right, Layout::Select::none, topSpacing)
	};
	return new Layout(1.f, cont, Direction::down, Layout::Select::none, topSpacing);
}
