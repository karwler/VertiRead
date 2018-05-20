#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(const string& TXT, int H) :
	text(TXT),
	length(World::drawSys()->textLength(TXT, H) + Default::textOffset * 2)
{}

vector<ProgState::Text> ProgState::createTextList(const vector<string>& strs, int height) {
	vector<Text> txts(strs.size());
	for (sizt i=0; i<txts.size(); i++)
		txts[i] = Text(strs[i], height);
	return txts;
}

int ProgState::findMaxLength(const vector<string>& strs, int height) {
	int width = 0;
	for (const string& it : strs) {
		int len = World::drawSys()->textLength(it, height) + Default::textOffset * 2;
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
const vec2s ProgState::messageSize(300, 100);

void ProgState::eventEnter() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else if (World::scene()->select)
		World::scene()->select->onClick(World::scene()->select->position(), SDL_BUTTON_LEFT);
}

void ProgState::eventUp() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onSelectUp();
	else
		World::scene()->selectFirst();
}

void ProgState::eventDown() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onSelectDown();
	else
		World::scene()->selectFirst();
}

void ProgState::eventLeft() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onSelectLeft();
	else
		World::scene()->selectFirst();
}

void ProgState::eventRight() {
	if (dynamic_cast<Button*>(World::scene()->select))
		World::scene()->select->onSelectRight();
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
		new Label(Button(1.f, &Program::eventClosePopup), World::drawSys()->translation("ok"), Label::Alignment::center),
		new Widget(1.f)
	};
	vector<Widget*> con = {
		new Label(Button(), msg),
		new Layout(1.f, bot, false, 0)
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

Layout* ProgBooks::createLayout() {
	Text settings(World::drawSys()->translation("settings"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(Button(settings.length, &Program::eventOpenSettings), settings.text),
		new Label(Button(exit.length, &Program::eventExit), exit.text)
	};

	vector<Text> txs = createTextList(Filer::listDir(World::winSys()->sets.getDirLib(), FTYPE_DIR), Default::itemHeight);
	vector<Widget*> tiles(txs.size());
	for (sizt i=0; i<txs.size(); i++)
		tiles[i] = new Label(Button(txs[i].length, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage), txs[i].text);

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, topSpacing),
		new TileBox(1.f, tiles)
	};
	return new Layout(1.f, cont, true, topSpacing);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventEscape() {
	if (World::scene()->getPopup())
		World::program()->eventClosePopup();
	else
		World::program()->eventBrowserGoUp();
}

Layout* ProgPageBrowser::createLayout() {
	Text back(World::drawSys()->translation("back"), lineHeight);
	vector<Widget*> bar = {
		new Label(Button(lineHeight, &Program::eventBrowserGoUp), back.text)
	};

	vector<string> dirs = World::program()->getBrowser()->listDirs();
	vector<string> files = World::program()->getBrowser()->listFiles();
	vector<Widget*> items(dirs.size() + files.size());
	for (sizt i=0; i<dirs.size(); i++)
		items[i] = new Label(Button(lineHeight, &Program::eventBrowserGoIn), dirs[i]);
	for (sizt i=0; i<files.size(); i++)
		items[dirs.size()+i] = new Label(Button(lineHeight, &Program::eventOpenReader), files[i]);
	
	vector<Widget*> cont = {
		new Layout(back.length, bar),
		new ScrollArea(1.f, items)
	};
	return new Layout(1.f, cont, false, topSpacing);
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
		Filer::saveLastPage(static_cast<Picture*>(reader->getWidget(vis.l))->getFile());
}

Layout* ProgReader::createLayout() {
	vector<Widget*> pics;
	for (string& it : Filer::listDir(World::program()->getBrowser()->getCurDir(), FTYPE_FILE)) {
		string file = appendDsep(World::program()->getBrowser()->getCurDir()) + it;
		SDL_Surface* pic = IMG_Load(file.c_str());
		if (pic) {
			pics.push_back(new Picture(Button(pic->h), file));
			SDL_FreeSurface(pic);
		}
	}
	return new ReaderBox(1.f, pics);
}

Overlay* ProgReader::createOverlay() {
	vector<Widget*> menu = {
		new Picture(Button(picSize, &Program::eventExitReader), Filer::dirTexs + "back.png"),
		new Picture(Button(picSize, &Program::eventNextDir), Filer::dirTexs + "next_dir.png"),
		new Picture(Button(picSize, &Program::eventPrevDir), Filer::dirTexs + "prev_dir.png"),
		new Picture(Button(picSize, &Program::eventZoomIn), Filer::dirTexs + "zoom_in.png"),
		new Picture(Button(picSize, &Program::eventZoomOut), Filer::dirTexs + "zoom_out.png"),
		new Picture(Button(picSize, &Program::eventZoomReset), Filer::dirTexs + "zoom_reset.png"),
		new Picture(Button(picSize, &Program::eventCenterView), Filer::dirTexs + "center.png")
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

Layout* ProgSettings::createLayout() {
	Text back(World::drawSys()->translation("back"), topHeight);
	Text exit(World::drawSys()->translation("exit"), topHeight);
	vector<Widget*> top = {
		new Label(Button(back.length, &Program::eventOpenBookList), back.text),
		new Label(Button(exit.length, &Program::eventExit), exit.text)
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
		new Label(Button(descLength), txs[0]),
		new CheckBox(Button(lineHeight, &Program::eventSwitchFullscreen), World::winSys()->sets.fullscreen)
	}, {
		new Label(Button(descLength), txs[1]),
		new SwitchBox(Button(1.f, &Program::eventSetTheme, &Program::eventSetTheme), Filer::getAvailibleThemes(), World::winSys()->sets.getTheme())
	}, {
		new Label(Button(descLength), txs[2]),
		new SwitchBox(Button(1.f, &Program::eventSwitchLanguage, &Program::eventSwitchLanguage), Filer::getAvailibleLanguages(), World::winSys()->sets.getLang())
	}, {
		new Label(Button(descLength), txs[3]),
		new LineEdit(Button(1.f, &Program::eventSetFont), World::winSys()->sets.getFont())
	}, {
		new Label(Button(descLength), txs[4]),
		new LineEdit(Button(1.f, &Program::eventSetLibraryDirLE), World::winSys()->sets.getDirLib()),
		new Label(Button(dots.length, &Program::eventOpenLibDirBrowser), dots.text, Label::Alignment::center)
	}, {
		new Label(Button(descLength), txs[5]),
		new SwitchBox(Button(1.f, &Program::eventSetRenderer, &Program::eventSetRenderer), Settings::getAvailibleRenderers(), World::winSys()->sets.renderer)
	}, {
		new Label(Button(descLength), txs[6]),
		new LineEdit(Button(1.f, &Program::eventSetScrollSpeed), World::winSys()->sets.getScrollSpeedString(), LineEdit::TextType::sFloatingSpaced)
	}, {
		new Label(Button(descLength), txs[7]),
		new Slider(Button(1.f, &Program::eventSetDeadzoneSL), World::winSys()->sets.getDeadzone(), 0, Default::axisLimit),
		new LineEdit(Button(dznum.length, &Program::eventSetDeadzoneLE), ntos(World::winSys()->sets.getDeadzone()), LineEdit::TextType::uInteger)
	} };
	vector<Widget*> lns(lcnt + 1 + bindings.size());
	for (sizt i=0; i<lcnt; i++)
		lns[i] = new Layout(lineHeight, lx[i], false);
	lns[lcnt] = new Widget(0);
	
	for (sizt i=0; i<bindings.size(); i++) {
		Binding::Type type = static_cast<Binding::Type>(i);
		vector<Widget*> lin {
			new Label(Button(descLength), txs[lcnt+i]),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::keyboard, type),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::joystick, type),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::gamepad, type),
		};
		lns[lcnt+1+i] = new Layout(lineHeight, lin, false);
	}

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, topSpacing),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, true, topSpacing);
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
		World::drawSys()->translation("set"),
		World::drawSys()->translation("up"),
		World::drawSys()->translation("back")
	};
	int barWidth = findMaxLength(txs, lineHeight);
	vector<Widget*> bar = {
		new Label(Button(lineHeight, &Program::eventSetLibraryDirBW), txs[0]),
		new Label(Button(lineHeight, &Program::eventBrowserGoUp), txs[1]),
		new Label(Button(lineHeight, &Program::eventExitBrowser), txs[2])
	};

	vector<string> strs = World::program()->getBrowser()->listDirs();
	vector<Widget*> items(strs.size());
	for (sizt i=0; i<strs.size(); i++)
		items[i] = new Label(Button(lineHeight, nullptr, nullptr, &Program::eventBrowserGoIn), strs[i]);

	vector<Widget*> cont = {
		new Layout(barWidth, bar),
		new ScrollArea(1.f, items)
	};
	return new Layout(1.f, cont, false, topSpacing);
}
