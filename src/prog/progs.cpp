#include "engine/world.h"

// PROGRAM TEXT

ProgState::Text::Text(string str, int height, int margin) :
	text(std::move(str)),
	length(World::drawSys()->textLength(text, height) + margin * 2)
{}

int ProgState::findMaxLength(const string* strs, sizet scnt, int height, int margin) {
	int width = 0;
	for (sizet i = 0; i < scnt; i++)
		if (int len = World::drawSys()->textLength(strs[i], height) + margin * 2; len > width)
			width = len;
	return width;
}

// PROGRAM STATE

void ProgState::eventEnter() {
	if (!World::program()->tryClosePopupThread() && World::scene()->select)
		World::scene()->select->onClick(World::scene()->select->position(), SDL_BUTTON_LEFT);
}

void ProgState::eventSelect(Direction dir) {
	if (World::scene()->select && !dynamic_cast<Layout*>(World::scene()->select))
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
	World::winSys()->moveCursor(vec2i(-amt * cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorDown(float amt) {
	World::winSys()->moveCursor(vec2i(amt * cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorLeft(float amt) {
	World::winSys()->moveCursor(vec2i(0, -amt * cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventCursorRight(float amt) {
	World::winSys()->moveCursor(vec2i(0, amt * cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventFullscreen() {
	World::winSys()->setFullscreen(!World::sets()->fullscreen);
}

void ProgState::eventHide() {
	World::sets()->showHidden = !World::sets()->showHidden;
}

void ProgState::eventBoss() {
	World::winSys()->toggleOpacity();
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

Popup* ProgState::createPopupMessage(string msg, PCall ccal, string ctxt, Label::Alignment malign) {
	Text ok(std::move(ctxt), popupLineHeight);
	Text mg(std::move(msg), popupLineHeight);
	vector<Widget*> bot = {
		new Widget(),
		new Label(ok.length, std::move(ok.text), ccal, nullptr, nullptr, Label::Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(mg.text), nullptr, nullptr, nullptr, malign),
		new Layout(1.f, std::move(bot), Direction::right, Layout::Select::none, 0)
	};
	return new Popup(vec2s(mg.length, popupLineHeight * 2 + Layout::defaultItemSpacing), std::move(con));
}

Popup* ProgState::createPopupChoice(string msg, PCall kcal, PCall ccal, Label::Alignment malign) {
	Text mg(std::move(msg), popupLineHeight);
	vector<Widget*> bot = {
		new Label(1.f, "Yes", kcal, nullptr, nullptr, Label::Alignment::center),
		new Label(1.f, "No", ccal, nullptr, nullptr, Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(mg.text), nullptr, nullptr, nullptr, malign),
		new Layout(1.f, std::move(bot), Direction::right, Layout::Select::none, 0)
	};
	return new Popup(vec2s(mg.length, popupLineHeight * 2 + Layout::defaultItemSpacing), std::move(con));
}

// PROG BOOKS

void ProgBooks::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventTryExit();
}

void ProgBooks::eventHide() {
	ProgState::eventHide();
	eventRefresh();
}

void ProgBooks::eventFileDrop(const string& file) {
	World::program()->openFile(file);
}

Layout* ProgBooks::createLayout() {
	// top bar
	Text download("Download", topHeight);
	Text settings("Settings", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
#ifdef BUILD_DOWNLOADER
		new Label(download.length, download.text, &Program::eventOpenDownloader),
#endif
		new Label(settings.length, std::move(settings.text), &Program::eventOpenSettings),
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit)
	};

	// book list
	vector<string> books = FileSys::listDir(World::sets()->getDirLib(), FTYPE_DIR, World::sets()->showHidden);
	vector<Widget*> tiles(books.size()+1);
	for (sizet i = 0; i < books.size(); i++) {
		Text txt(filename(books[i]), TileBox::defaultItemHeight);
		tiles[i] = new Label(txt.length, std::move(txt.text), &Program::eventOpenPageBrowser, &Program::eventOpenLastPage);
	}
	tiles.back() = new Button(TileBox::defaultItemHeight, &Program::eventOpenPageBrowser, &Program::eventOpenLastPage, nullptr, true, World::drawSys()->texture("search"));

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		new TileBox(1.f, tiles)
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventHide() {
	ProgState::eventHide();
	eventRefresh();
}

void ProgPageBrowser::eventFileDrop(const string& file) {
	World::program()->openFile(file);
}

Layout* ProgPageBrowser::createLayout() {
	// sidebar
	vector<string> txs = {
		"Back",
		"Up"
	};
	int txsWidth = findMaxLength(txs.data(), txs.size(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, std::move(txs[0]), &Program::eventExitBrowser),
		new Label(lineHeight, std::move(txs[1]), &Program::eventBrowserGoUp)
	};

	// list of files and directories
	auto [files, dirs] = World::browser()->listCurDir();
	vector<Widget*> items(files.size() + dirs.size());
	for (sizet i = 0; i < dirs.size(); i++)
		items[i] = new Label(lineHeight, std::move(dirs[i]), &Program::eventBrowserGoIn, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("folder"));
	for (sizet i = 0; i < files.size(); i++)
		items[dirs.size()+i] = new Label(lineHeight, std::move(files[i]), &Program::eventBrowserGoFile, nullptr, nullptr, Label::Alignment::left, World::drawSys()->texture("file"));
	
	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		new ScrollArea(1.f, std::move(items))
	};

	// root layout
	vector<Widget*> cont = {
		new LabelEdit(lineHeight, std::all_of(World::browser()->getRootDir().begin(), World::browser()->getRootDir().end(), isDsep) ? World::browser()->getCurDir() : World::browser()->getCurDir().substr(appDsep(World::sets()->getDirLib()).length()), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, Layout::Select::none, topSpacing)
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

// PROG READER

void ProgReader::eventEscape() {
	if (!World::program()->tryClosePopupThread())
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
	if (!reader->scrollToNext())
		World::program()->eventNextDir();
}

void ProgReader::eventPrevPage() {
	if (!reader->scrollToPrevious())
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
	reader->scrollToLimit(true);
}

void ProgReader::eventToEnd() {
	reader->scrollToLimit(false);
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

void ProgReader::eventHide() {
	ProgState::eventHide();
	World::program()->eventStartLoadingReader(reader->firstPage());
}

void ProgReader::eventClosing() {
	if (string rpath = getChild(World::browser()->getCurDir(), World::sets()->getDirLib()); rpath.empty())
		World::fileSys()->saveLastPage(dotStr, World::browser()->getCurDir(), reader->curPage());
	else {
		string::iterator mid = std::find_if(rpath.begin(), rpath.end(), isDsep);
		string::iterator nxt = std::find_if(mid, rpath.end(), notDsep);
		World::fileSys()->saveLastPage(string(rpath.begin(), mid), nxt != rpath.end() ? string(nxt, rpath.end()) : string(), reader->curPage());
	}
	SDL_ShowCursor(SDL_ENABLE);
}

Layout* ProgReader::createLayout() {
	return reader = new ReaderBox(1.f, {}, World::sets()->direction, World::sets()->zoom, World::sets()->spacing);
}

Overlay* ProgReader::createOverlay() {
	vector<Widget*> menu = {
		new Button(picSize, &Program::eventExitReader, nullptr, nullptr, false, World::drawSys()->texture("cross"), picMargin),
		new Button(picSize, &Program::eventNextDir, nullptr, nullptr, false, World::drawSys()->texture("right"), picMargin),
		new Button(picSize, &Program::eventPrevDir, nullptr, nullptr, false, World::drawSys()->texture("left"), picMargin),
		new Button(picSize, &Program::eventZoomIn, nullptr, nullptr, false, World::drawSys()->texture("plus"), picMargin),
		new Button(picSize, &Program::eventZoomOut, nullptr, nullptr, false, World::drawSys()->texture("minus"), picMargin),
		new Button(picSize, &Program::eventZoomReset, nullptr, nullptr, false, World::drawSys()->texture("circle"), picMargin),
		new Button(picSize, &Program::eventCenterView, nullptr, nullptr, false, World::drawSys()->texture("square"), picMargin)
	};
	int ysiz = picSize * int(menu.size());
	return new Overlay(vec2s(0), vec2s(picSize, ysiz), vec2s(0), vec2s(picSize/2, ysiz), std::move(menu), Direction::down, 0);
}

int ProgReader::modifySpeed(float value) {
	if (float factor = 1.f; World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value *= scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::scrollSlow, factor))
		value /= scrollFactor * factor;
	return int(value * World::winSys()->getDSec());
}

// PROG DOWNLOADER
#ifdef BUILD_DOWNLOADER
void ProgDownloader::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventOpenBookList();
}

Layout* ProgDownloader::createLayout() {
	// top bar
	Text books("Library", topHeight);
	Text settings("Settings", topHeight);
	Text exit("Exit", topHeight);
	Text downloads("Downloads", topHeight);
	vector<Widget*> top = {
		new Label(books.length, std::move(books.text), &Program::eventOpenBookList),
		new Label(settings.length, std::move(settings.text), &Program::eventOpenSettings),
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit),
		new Widget(),
		new Label(downloads.length, std::move(downloads.text), &Program::eventOpenDownloadList),
		new SwitchBox(findMaxLength(WebSource::sourceNames.data(), WebSource::sourceNames.size(), topHeight), WebSource::sourceNames.data(), WebSource::sourceNames.size(), World::downloader()->getSource()->name(), &Program::eventSwitchSource, Label::Alignment::center)
	};

	// downlaod button and bar
	Text download("Download all", lineHeight);
	vector<Widget*> dow = {
		chaptersTick = new CheckBox(lineHeight, false, &Program::eventSelectAllChapters, &Program::eventSelectAllChapters),
		new Label(download.length, std::move(download.text), &Program::eventDownloadAllChapters)
	};

	// info and chapter section (left side children)
	vector<Widget*> inf = {
		new Layout(lineHeight, std::move(dow), Direction::right),
		chapters = new ScrollArea()
	};

	// search bar children
	Text search("Search", lineHeight);
	vector<Widget*> nav = {
		query = new LabelEdit(0.8f, string(), &Program::eventQuery, nullptr, nullptr, LabelEdit::TextType::text, false),
		new Label(search.length, std::move(search.text), &Program::eventQuery)
	};

	// search bar + query results (right side)
	vector<Widget*> qrs = {
		new Layout(lineHeight, std::move(nav), Direction::right, Layout::Select::none, topSpacing),
		results = new ScrollArea(1.f, {}, Direction::down, Layout::Select::one)
	};

	// left + right
	vector<Widget*> mid = {
		new Layout(1.f, std::move(inf), Direction::down, Layout::Select::none),
		new Layout(2.f, std::move(qrs), Direction::down)
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		new Layout(1.f, std::move(mid), Direction::right, Layout::Select::none, topSpacing)
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Comic ProgDownloader::curInfo() const {
	Comic info(static_cast<LabelEdit*>(*results->getSelected().begin())->getText(), vector<pairStr>(chapterUrls.size()));
	for (sizet i = 0; i < chapterUrls.size(); i++)
		info.chapters[i] = pair(static_cast<Label*>(static_cast<Layout*>(chapters->getWidget(i))->getWidget(1))->getText(), chapterUrls[i]);
	return info;
}

void ProgDownloader::printResults(vector<pairStr>&& comics) {
	resultUrls.resize(comics.size());
	vector<Widget*> wgts(comics.size());
	for (sizet i = 0; i < wgts.size(); i++) {
		wgts[i] = new Label(TileBox::defaultItemHeight, std::move(comics[i].first), &Program::eventShowComicInfo, nullptr, &Program::eventDownloadComic);
		resultUrls[i] = std::move(comics[i].second);
	}
	results->setWidgets(std::move(wgts));
}

void ProgDownloader::printInfo(vector<pairStr>&& chaps) {
	chapterUrls.resize(chaps.size());
	vector<Widget*> wgts(chaps.size());
	for (sizet i = 0; i < wgts.size(); i++) {
		vector<Widget*> line = {
			new CheckBox(TileBox::defaultItemHeight, true),
			new Label(1.f, std::move(chaps[i].first), &Program::eventSelectChapter, nullptr, &Program::eventDownloadChapter)
		};
		wgts[i] = new Layout(TileBox::defaultItemHeight, std::move(line), Direction::right);
		chapterUrls[i] = std::move(chaps[i].second);
	}
	chapters->setWidgets(std::move(wgts));
	chaptersTick->on = !chaps.empty();
}

// PROG DOWNLOADS

void ProgDownloads::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventOpenDownloader();
}

Layout* ProgDownloads::createLayout() {
	// top bar
	Text books("Library", topHeight);
	Text download("Download", topHeight);
	Text settings("Settings", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
		new Label(books.length, std::move(books.text), &Program::eventOpenBookList),
		new Label(download.length, std::move(download.text), &Program::eventOpenDownloader),
		new Label(settings.length, std::move(settings.text), &Program::eventOpenSettings),
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit)
	};

	// sidebar
	vector<string> txs = {
		"Resume",
		"Stop",
		"Clear"
	};
	int txsWidth = findMaxLength(txs.data(), txs.size(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, std::move(txs[0]), &Program::eventResumeDownloads),
		new Label(lineHeight, std::move(txs[1]), &Program::eventStopDownloads),
		new Label(lineHeight, std::move(txs[2]), &Program::eventClearDownloads)
	};

	// queue list
	while (SDL_TryLockMutex(World::downloader()->queueLock));
	vector<Widget*> lse(World::downloader()->getDlQueue().size());
	for (sizet i = 0; i < lse.size(); i++) {
		vector<Widget*> ln = {
			new Label(1.f, World::downloader()->getDlQueue()[i].title + " - waiting"),
			new Label(TileBox::defaultItemHeight, "X", &Program::eventDownloadDelete)
		};
		lse[i] = new Layout(TileBox::defaultItemHeight, std::move(ln), Direction::right);
	}
	if (!lse.empty())
		static_cast<Label*>(static_cast<Layout*>(lse[0])->getWidget(0))->setText(World::downloader()->getDlQueue()[0].title + " - " + to_string(World::downloader()->getDlProg().b) + '/' + to_string(World::downloader()->getDlProg().t));
	SDL_UnlockMutex(World::downloader()->queueLock);

	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		list = new ScrollArea(1.f, std::move(lse))
	};

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		new Layout(1.f, std::move(mid), Direction::right)
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}
#endif
// PROG SETTINGS

void ProgSettings::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventOpenBookList();
}

void ProgSettings::eventFullscreen() {
	ProgState::eventFullscreen();
	fullscreen->on = World::sets()->fullscreen;
}

void ProgSettings::eventHide() {
	ProgState::eventHide();
	showHidden->on = World::sets()->showHidden;
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
	Text books("Library", topHeight);
	Text download("Download", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
		new Label(books.length, std::move(books.text), &Program::eventOpenBookList),
#ifdef BUILD_DOWNLOADER
		new Label(download.length, std::move(download.text), &Program::eventOpenDownloader),
#endif
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit)
	};

	// setting buttons and labels
	Text tcnt("Count:", lineHeight);
	Text tsiz("Size:", lineHeight);
	vector<Text> butts = {
		Text("Portrait", lineHeight),
		Text("Landscape", lineHeight),
		Text("Square", lineHeight),
		Text("Fill", lineHeight)
	};
	vector<string> txs = {
		"Direction",
		"Zoom",
		"Spacing",
		"Picture limit",
		"Fullscreen",
		"Size",
		"Theme",
		"Show hidden",
		"Font",
		"Library",
		"Renderer",
		"Scroll speed",
		"Deadzone"
	};
	sizet lcnt = txs.size();
	txs.resize(txs.size() + Binding::names.size());
	for (sizet i = 0; i < Binding::names.size(); i++)
		txs[lcnt+i] = firstUpper(Binding::names[i]);
	std::reverse(txs.begin(), txs.end());
	int descLength = findMaxLength(txs.data(), txs.size(), lineHeight);
	
	// action fields for labels
	vector<string> themes = World::fileSys()->getAvailibleThemes();
	vector<string> renderers = getAvailibleRenderers();
	Text dots(KeyGetter::ellipsisStr, lineHeight);
	Text dznum(to_string(Settings::axisLimit), lineHeight);
	vector<Widget*> lx[] = { {
		new Label(descLength, popBack(txs)),
		new SwitchBox(1.f, Direction::names.data(), Direction::names.size(), Direction::names[uint8(World::sets()->direction)], &Program::eventSwitchDirection)
	}, {
		new Label(descLength, popBack(txs)),
		new LabelEdit(1.f, trimZero(to_string(World::sets()->zoom)), &Program::eventSetZoom, nullptr, nullptr, LabelEdit::TextType::uFloat)
	}, {
		new Label(descLength, popBack(txs)),
		new LabelEdit(1.f, to_string(World::sets()->spacing), &Program::eventSetSpacing, nullptr, nullptr, LabelEdit::TextType::uInt)
	}, {
		new Label(descLength, popBack(txs)),
		new SwitchBox(findMaxLength(PicLim::names.data(), PicLim::names.size(), lineHeight), PicLim::names.data(), PicLim::names.size(), PicLim::names[uint8(World::sets()->picLim.type)], &Program::eventSetPicLimitType),
		createLimitEdit()
	}, {
		new Label(descLength, popBack(txs)),
		fullscreen = new CheckBox(lineHeight, World::sets()->fullscreen, &Program::eventSetFullscreen)
	}, {
		new Label(descLength, popBack(txs)),
		new Label(butts[0].length, std::move(butts[0].text), &Program::eventSetPortrait),
		new Label(butts[1].length, std::move(butts[1].text), &Program::eventSetLandscape),
		new Label(butts[2].length, std::move(butts[2].text), &Program::eventSetSquare),
		new Label(butts[3].length, std::move(butts[3].text), &Program::eventSetFill)
	}, {
		new Label(descLength, popBack(txs)),
		new SwitchBox(1.f, themes.data(), themes.size(), World::sets()->getTheme(), &Program::eventSetTheme)
	}, {
		new Label(descLength, popBack(txs)),
		showHidden = new CheckBox(lineHeight, World::sets()->showHidden, &Program::eventSetHide)
	}, {
		new Label(descLength, popBack(txs)),
		new LabelEdit(1.f, World::sets()->getFont(), &Program::eventSetFont)
	}, {
		new Label(descLength, popBack(txs)),
		new LabelEdit(1.f, World::sets()->getDirLib(), &Program::eventSetLibraryDirLE),
		new Label(dots.length, std::move(dots.text), &Program::eventOpenLibDirBrowser, nullptr, nullptr, Label::Alignment::center)
	}, {
		new Label(descLength, popBack(txs)),
		new SwitchBox(1.f, renderers.data(), renderers.size(), World::sets()->renderer, &Program::eventSetRenderer)
	}, {
		new Label(descLength, popBack(txs)),
		new LabelEdit(1.f, World::sets()->scrollSpeedString(), &Program::eventSetScrollSpeed, nullptr, nullptr, LabelEdit::TextType::sFloatSpaced)
	}, {
		new Label(descLength, popBack(txs)),
		deadzoneSL = new Slider(1.f, World::sets()->getDeadzone(), 0, Settings::axisLimit, &Program::eventSetDeadzoneSL),
		deadzoneLE = new LabelEdit(dznum.length + LabelEdit::caretWidth, to_string(World::sets()->getDeadzone()), &Program::eventSetDeadzoneLE, nullptr, nullptr, LabelEdit::TextType::uInt)
	} };
	vector<Widget*> lns(lcnt + 1 + Binding::names.size() + 2);
	for (sizet i = 0; i < lcnt; i++)
		lns[i] = new Layout(lineHeight, std::move(lx[i]), Direction::right);
	lns[lcnt] = new Widget(0);
	limitLine = static_cast<Layout*>(lns[3]);

	// shortcut entries
	for (sizet i = 0; i < Binding::names.size(); i++) {
		vector<Widget*> lin {
			new Label(descLength, popBack(txs)),
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, Binding::Type(i)),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, Binding::Type(i)),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, Binding::Type(i))
		};
		lns[lcnt+1+i] = new Layout(lineHeight, std::move(lin), Direction::right);
	}
	lcnt += 1 + Binding::names.size();

	// reset button
	Text resbut("Reset", lineHeight);
	lns[lcnt] = new Widget(0);
	lns[lcnt+1] = new Layout(lineHeight, { new Label(resbut.length, std::move(resbut.text), &Program::eventResetSettings) }, Direction::right);

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		new ScrollArea(1.f, std::move(lns))
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Widget* ProgSettings::createLimitEdit() {
	switch (World::sets()->picLim.type) {
	case PicLim::Type::count:
		return new LabelEdit(1.f, to_string(World::sets()->picLim.getCount()), &Program::eventSetPicLimCount, nullptr, nullptr, LabelEdit::TextType::uInt);
	case PicLim::Type::size:
		return new LabelEdit(1.f, memoryString(World::sets()->picLim.getSize()), &Program::eventSetPicLimSize);
	}
	return new Widget();
}

// PROG SEARCH DIR

void ProgSearchDir::eventEscape() {
	if (!World::program()->tryClosePopupThread())
		World::program()->eventBrowserGoUp();
}

void ProgSearchDir::eventHide() {
	ProgState::eventHide();
	eventRefresh();
}

Layout* ProgSearchDir::createLayout() {
	// sidebar
	vector<string> txs = {
		"Back",
		"Up",
		"Set"
	};
	int txsWidth = findMaxLength(txs.data(), txs.size(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, std::move(txs[0]), &Program::eventExitBrowser),
		new Label(lineHeight, std::move(txs[1]), &Program::eventBrowserGoUp),
		new Label(lineHeight, std::move(txs[2]), &Program::eventSetLibraryDirBW)
	};

	// directory list
	vector<string> strs = FileSys::listDir(World::browser()->getCurDir(), FTYPE_DIR, World::sets()->showHidden);
	vector<Widget*> items(strs.size());
	for (sizet i = 0; i < strs.size(); i++)
		items[i] = new Label(lineHeight, std::move(strs[i]), nullptr, nullptr, &Program::eventBrowserGoIn, Label::Alignment::left, World::drawSys()->texture("folder"));

	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		list = new ScrollArea(1.f, std::move(items), Direction::down, Layout::Select::one)
	};

	// root layout
	vector<Widget*> cont = {
		new LabelEdit(lineHeight, World::browser()->getCurDir(), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, Layout::Select::none, topSpacing)
	};
	return new Layout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}
