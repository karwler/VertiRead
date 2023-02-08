#include "progs.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include "utils/compare.h"
#include "utils/layouts.h"

// PROGRAM TEXT

ProgState::Text::Text(string str, int height) :
	text(std::move(str)),
	length(measure(text.c_str(), height))
{}

int ProgState::Text::measure(const char* str, int height) {
	return World::drawSys()->textLength(str, height) + Label::defaultTextMargin * 2;
}

// PROGRAM STATE

void ProgState::eventEnter() {
	if (World::scene()->select)
		World::scene()->select->onClick(World::scene()->select->position(), SDL_BUTTON_LEFT);
}

void ProgState::eventEscape() {
	World::scene()->onCancel();
}

void ProgState::eventSelect(Direction dir) {
	World::inputSys()->mouseWin = std::nullopt;
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
	World::winSys()->moveCursor(ivec2(-amt * cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorDown(float amt) {
	World::winSys()->moveCursor(ivec2(amt * cursorMoveFactor * World::winSys()->getDSec(), 0));
}

void ProgState::eventCursorLeft(float amt) {
	World::winSys()->moveCursor(ivec2(0, -amt * cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventCursorRight(float amt) {
	World::winSys()->moveCursor(ivec2(0, amt * cursorMoveFactor * World::winSys()->getDSec()));
}

void ProgState::eventFullscreen() {
	World::winSys()->setScreenMode(World::sets()->screen != Settings::Screen::fullscreen ? Settings::Screen::fullscreen : Settings::Screen::windowed);
}

void ProgState::eventMultiFullscreen() {
	World::winSys()->setScreenMode(World::sets()->screen != Settings::Screen::multiFullscreen ? Settings::Screen::multiFullscreen : Settings::Screen::windowed);
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

void ProgState::onResize() {
	popupLineHeight = int(40.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	tooltipHeight = int(16.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	lineHeight = int(30.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	topHeight = int(40.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	topSpacing = int(10.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	picSize = int(40.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	picMargin = int(4.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	contextMargin = int(3.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi());
	maxTooltipLength = World::drawSys()->getViewRes().x * 2 / 3;
	cursorMoveFactor = 10.f / WindowSys::fallbackDpi * World::winSys()->getWinDpi();
}

Overlay* ProgState::createOverlay() {
	return nullptr;
}

Popup* ProgState::createPopupMessage(string msg, PCall ccal, string ctxt, Alignment malign) {
	Text ok(std::move(ctxt), popupLineHeight);
	Text mg(std::move(msg), popupLineHeight);
	Widget* first;
	vector<Widget*> bot = {
		new Widget(),
		first = new Label(ok.length, std::move(ok.text), ccal, nullptr, nullptr, nullptr, Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(mg.text), nullptr, nullptr, nullptr, nullptr, malign),
		new Layout(1.f, std::move(bot), Direction::right, Layout::Select::none, 0)
	};
	return new Popup(svec2(std::max(mg.length, ok.length) + Layout::defaultItemSpacing * 2, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

void ProgState::updatePopupMessage(string msg) {
	if (Popup* popup = World::scene()->getPopup()) {
		Text mg(std::move(msg), popupLineHeight);
		popup->getWidget<Label>(0)->setText(std::move(mg.text));
		popup->setSize(std::max(mg.length, Text::measure(popup->getWidget<Layout>(1)->getWidget<Label>(1)->getText().c_str(), popupLineHeight)) + Layout::defaultItemSpacing * 2);
	}
}

Popup* ProgState::createPopupMultiline(string msg, PCall ccal, string ctxt, Alignment malign) {
	Text ok(std::move(ctxt), popupLineHeight);
	ivec2 viewRes = World::drawSys()->getViewRes();
	int width = ok.length, lines = 0;
	for (size_t i = 0; i < msg.length();) {
		++lines;
		size_t end = std::min(msg.find('\n', i), msg.length());
		if (int siz = World::drawSys()->textLength(msg.data() + i, end - i, lineHeight) + (Label::defaultTextMargin + Layout::defaultItemSpacing) * 2; siz > width)
			if (width = std::min(siz, viewRes.x); width == viewRes.x)
				break;
		i = msg.find_first_not_of('\n', end);
	}

	Widget* first;
	vector<Widget*> bot = {
		new Widget(),
		first = new Label(ok.length, std::move(ok.text), ccal, nullptr, nullptr, nullptr, Alignment::center),
		new Widget()
	};
	vector<Widget*> con = {
		new TextBox(1.f, lineHeight, std::move(msg), nullptr, nullptr, nullptr, nullptr, malign),
		new Layout(popupLineHeight, std::move(bot), Direction::right, Layout::Select::none, 0)
	};
	return new Popup(svec2(width, std::min(lineHeight * lines + popupLineHeight + Layout::defaultItemSpacing * 3, viewRes.y)), std::move(con), ccal, first);
}

Popup* ProgState::createPopupChoice(string msg, PCall kcal, PCall ccal, Alignment malign) {
	Text mg(std::move(msg), popupLineHeight);
	Text yes("Yes", popupLineHeight);
	Text no("No", popupLineHeight);
	Widget* first;
	vector<Widget*> bot = {
		first = new Label(1.f, std::move(yes.text), kcal, nullptr, nullptr, nullptr, Alignment::center),
		new Label(1.f, std::move(no.text), ccal, nullptr, nullptr, nullptr, Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(mg.text), nullptr, nullptr, nullptr, nullptr, malign),
		new Layout(1.f, std::move(bot), Direction::right, Layout::Select::none, 0)
	};
	return new Popup(svec2(std::max(mg.length, yes.length + no.length) + Layout::defaultItemSpacing * 3, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

Context* ProgState::createContext(vector<pair<string, PCall>>&& items, Widget* parent) {
	vector<Widget*> wgts(items.size());
	for (size_t i = 0; i < items.size(); ++i)
		wgts[i] = new Label(lineHeight, std::move(items[i].first), items[i].second, &Program::eventCloseContext);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, World::winSys()->mousePos(), ivec2(0, lineHeight), contextMargin);
	return new Context(rect.pos(), rect.size(), vector<Widget*>{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, Layout::Select::none, 0) }, first, parent, Color::dark, nullptr, Layout::defaultDirection, contextMargin);
}

Context* ProgState::createComboContext(ComboBox* parent, PCall kcal) {
	ivec2 size = parent->size();
	vector<Widget*> wgts(parent->getOptions().size());
	for (size_t i = 0; i < parent->getOptions().size(); ++i)
		wgts[i] = new Label(size.y, string(parent->getOptions()[i]), kcal, &Program::eventCloseContext);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, parent->position(), size, contextMargin);
	return new Context(rect.pos(), rect.size(), vector<Widget*>{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, Layout::Select::none, 0) }, first, parent, Color::dark, &Program::eventResizeComboContext, Layout::defaultDirection, contextMargin);
}

Recti ProgState::calcTextContextRect(const vector<Widget*>& items, ivec2 pos, ivec2 size, int margin) {
	for (Widget* it : items)
		if (Label* lbl = dynamic_cast<Label*>(it))
			if (int w = World::drawSys()->textLength(lbl->getText(), size.y) + lbl->getTextMargin() * 2 + ScrollBar::barSizeVal + margin * 2; w > size.x)
				size.x = w;
	size.y = size.y * items.size() + margin * 2;

	ivec2 res = World::drawSys()->getViewRes();
	calcContextPos(pos.x, size.x, res.x);
	calcContextPos(pos.y, size.y, res.y);
	return Recti(pos, size);
}

void ProgState::calcContextPos(int& pos, int& siz, int limit) {
	if (siz < limit)
		pos = pos + siz <= limit ? pos : limit - siz;
	else {
		siz = limit;
		pos = 0;
	}
}

template <class T>
int ProgState::findMaxLength(T pos, T end, int height) {
	int width = 0;
	for (; pos != end; ++pos)
		if (int len = World::drawSys()->textLength(*pos, height) + Label::defaultTextMargin * 2; len > width)
			width = len;
	return width;
}

Texture* ProgState::makeTooltip(const char* str) {
	return World::sets()->tooltips ? World::drawSys()->renderText(str, tooltipHeight, maxTooltipLength) : nullptr;
}

Texture* ProgState::makeTooltipL(const char* str) {
	if (!World::sets()->tooltips)
		return nullptr;

	uint width = 0;
	for (const char* pos = str; *pos;) {
		size_t len = strcspn(pos, "\n");
		if (uint siz = World::drawSys()->textLength(string(pos, len), tooltipHeight) + Label::tooltipMargin.x * 2; siz > width)
			if (width = std::min(siz, maxTooltipLength); width == maxTooltipLength)
				break;
		pos += pos[len] ? len + 1 : len;
	}
	return World::drawSys()->renderText(str, tooltipHeight, width);
}

// PROG FILE EXPLORER

void ProgFileExplorer::eventHide() {
	ProgState::eventHide();
	World::scene()->resetLayouts();
}

void ProgFileExplorer::processFileChanges(const Browser* browser, vector<pair<bool, string>>& files, bool gone) {
	if (gone) {
		fileList->setWidgets(vector<Widget*>());
		if (locationBar)
			locationBar->setText(browser->currentLocation().u8string());
		return;
	}

	const vector<Widget*>& wgts = fileList->getWidgets();
	for (auto& [add, name] : files) {
		bool directory = fs::is_directory(browser->getCurDir() / name);
		auto [pos, end] = directory ? pair(wgts.begin(), wgts.begin() + dirEnd) : pair(wgts.begin() + dirEnd, wgts.begin() + fileEnd);
		if (add) {
			size_t id = std::lower_bound(pos, end, name, [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); }) - wgts.begin();
			Size size = fileEntrySize(name);
			if (directory) {
				fileList->insertWidget(id, makeDirectoryEntry(size, std::move(name)));
				++dirEnd;
				++fileEnd;
			} else if (Label* flbl = makeFileEntry(size, std::move(name))) {
				fileList->insertWidget(id, flbl);
				++fileEnd;
			}
		} else if (vector<Widget*>::const_iterator it = std::lower_bound(pos, end, name, [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); }); it != end && static_cast<const Label*>(*it)->getText() == name) {
			fileList->deleteWidget((*it)->getIndex());
			if (directory)
				--dirEnd;
			--fileEnd;
		}
	}
}

Label* ProgFileExplorer::makeFileEntry(const Size&, string&&) {
	return nullptr;
}

Size ProgFileExplorer::fileEntrySize(const string& name) {
	return lineHeight;
}

// PROG BOOKS

void ProgBooks::eventSpecEscape() {
	World::program()->eventTryExit();
}

void ProgBooks::eventFileDrop(const fs::path& file) {
	World::program()->openFile(file);
}

RootLayout* ProgBooks::createLayout() {
	// top bar
	Text download("Download", topHeight);
	Text settings("Settings", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
#ifdef DOWNLOADER
		new Label(download.length, download.text, &Program::eventOpenDownloader),
#endif
		new Label(settings.length, std::move(settings.text), &Program::eventOpenSettings),
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit)
	};

	// book list
	vector<fs::path> books = FileSys::listDir(World::sets()->getDirLib(), false, true, World::sets()->showHidden);
	vector<Widget*> tiles(books.size()+1);
	for (size_t i = 0; i < books.size(); ++i) {
		Text txt(books[i].u8string(), TileBox::defaultItemHeight);
		tiles[i] = makeDirectoryEntry(txt.length, std::move(txt.text));
	}
	tiles.back() = new Button(TileBox::defaultItemHeight, &Program::eventOpenPageBrowser, &Program::eventOpenBookContext, nullptr, makeTooltip("Browse other directories"), true, World::drawSys()->texture("search"));
	dirEnd = books.size();
	fileEnd = dirEnd;

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		fileList = new TileBox(1.f, std::move(tiles))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Label* ProgBooks::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), &Program::eventOpenPageBrowser, &Program::eventOpenBookContext);
}

Size ProgBooks::fileEntrySize(const string& name) {
	return Text::measure(name.c_str(), TileBox::defaultItemHeight);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventSpecEscape() {
	World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventFileDrop(const fs::path& file) {
	World::program()->openFile(file);
}

void ProgPageBrowser::resetFileIcons() {
	Texture* dtex = World::drawSys()->texture("folder").first;
	Texture* ftex = World::drawSys()->texture("file").first;
	const vector<Widget*>& wgts = fileList->getWidgets();
	size_t i = 0;
	for (; i < dirEnd; ++i)
		static_cast<Label*>(wgts[i])->setTex(dtex, false);
	for (; i < fileEnd; ++i)
		static_cast<Label*>(wgts[i])->setTex(ftex, false);
}

RootLayout* ProgPageBrowser::createLayout() {
	// sidebar
	std::initializer_list<const char*> txs = {
		"Exit",
		"Up"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	int txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, *itxs++, &Program::eventExitBrowser),
		new Label(lineHeight, *itxs++, &Program::eventBrowserGoUp)
	};

	// list of files and directories
	auto [files, dirs] = World::browser()->listCurDir();
	if (World::sets()->preview)
		World::browser()->startPreview(files, dirs, lineHeight);
	vector<Widget*> items(files.size() + dirs.size());
	for (size_t i = 0; i < dirs.size(); ++i)
		items[i] = makeDirectoryEntry(lineHeight, std::move(dirs[i]));
	for (size_t i = 0; i < files.size(); ++i)
		items[dirs.size() + i] = makeFileEntry(lineHeight, std::move(files[i]));
	dirEnd = dirs.size();
	fileEnd = dirEnd + files.size();

	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		fileList = new ScrollArea(1.f, std::move(items))
	};

	// root layout
	fs::path location = World::browser()->currentLocation();
	vector<Widget*> cont = {
		locationBar = new LabelEdit(lineHeight, World::browser()->getRootDir() != Browser::topDir ? relativePath(location, World::sets()->getDirLib()).u8string() : location.u8string(), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, Layout::Select::none, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Label* ProgPageBrowser::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), &Program::eventBrowserGoIn, nullptr, nullptr, nullptr, Alignment::left, World::drawSys()->texture("folder"));
}

Label* ProgPageBrowser::makeFileEntry(const Size& size, string&& name) {
	return new Label(lineHeight, std::move(name), &Program::eventBrowserGoFile, nullptr, nullptr, nullptr, Alignment::left, World::drawSys()->texture("file"));
}

// PROG READER

void ProgReader::eventSpecEscape() {
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
	reader->onScroll(ivec2(0, -modifySpeed(amt * World::sets()->scrollSpeed.y)));
}

void ProgReader::eventScrollDown(float amt) {
	reader->onScroll(ivec2(0, modifySpeed(amt * World::sets()->scrollSpeed.y)));
}

void ProgReader::eventScrollRight(float amt) {
	reader->onScroll(ivec2(modifySpeed(amt * World::sets()->scrollSpeed.x), 0));
}

void ProgReader::eventScrollLeft(float amt) {
	reader->onScroll(ivec2(-modifySpeed(amt * World::sets()->scrollSpeed.x), 0));
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
	World::browser()->startLoadPictures(fs::u8path(reader->firstPage()));
	World::program()->setPopupLoading();
}

void ProgReader::eventClosing() {
	if (fs::path rpath = relativePath(World::browser()->getCurDir(), World::sets()->getDirLib()); rpath.empty())
		World::fileSys()->saveLastPage(dotStr, World::browser()->getCurDir().u8string(), World::browser()->curDirSuffix() + reader->curPage());
	else {
		string path = rpath.u8string();
		string::iterator mid = std::find_if(path.begin(), path.end(), isDsep);
		string::iterator nxt = std::find_if(mid, path.end(), notDsep);
		World::fileSys()->saveLastPage(string(path.begin(), mid), string(nxt, path.end()), World::browser()->curDirSuffix() + reader->curPage());
	}
	SDL_ShowCursor(SDL_ENABLE);
}

RootLayout* ProgReader::createLayout() {
	vector<Widget*> cont = { reader = new ReaderBox(1.f, World::sets()->direction, World::sets()->zoom, World::sets()->spacing) };
	return new RootLayout(1.f, std::move(cont), Direction::right, Layout::Select::none, 0);
}

Overlay* ProgReader::createOverlay() {
	vector<Widget*> menu = {
		new Button(picSize, &Program::eventExitReader, nullptr, nullptr, makeTooltip("Exit"), false, World::drawSys()->texture("cross"), picMargin),
		new Button(picSize, &Program::eventNextDir, nullptr, nullptr, makeTooltip("Next"), false, World::drawSys()->texture("right"), picMargin),
		new Button(picSize, &Program::eventPrevDir, nullptr, nullptr, makeTooltip("Previous"), false, World::drawSys()->texture("left"), picMargin),
		new Button(picSize, &Program::eventZoomIn, nullptr, nullptr, makeTooltip("Zoom in"), false, World::drawSys()->texture("plus"), picMargin),
		new Button(picSize, &Program::eventZoomOut, nullptr, nullptr, makeTooltip("Zoom out"), false, World::drawSys()->texture("minus"), picMargin),
		new Button(picSize, &Program::eventZoomReset, nullptr, nullptr, makeTooltip("Zoom reset"), false, World::drawSys()->texture("reset"), picMargin),
		new Button(picSize, &Program::eventCenterView, nullptr, nullptr, makeTooltip("Center"), false, World::drawSys()->texture("center"), picMargin)
	};
	int ysiz = picSize * menu.size();
	return new Overlay(svec2(0), svec2(picSize, ysiz), svec2(0), svec2(picSize/2, ysiz), std::move(menu), Color::normal, Direction::down, 0);
}

int ProgReader::modifySpeed(float value) {
	if (float factor = 1.f; World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value *= scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::scrollSlow, factor))
		value /= scrollFactor * factor;
	return int(value * World::winSys()->getDSec());
}

// PROG DOWNLOADER

#ifdef DOWNLOADER
void ProgDownloader::eventEscape() {
	if (!eventCommonEscape())
		World::program()->eventOpenBookList();
}

RootLayout* ProgDownloader::createLayout() {
	// top bar
	vector<string> wsrcs(WebSource::sourceNames.begin(), WebSource::sourceNames.end());
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
		new ComboBox(findMaxLength(wsrcs.begin(), wsrcs.end(), topHeight), World::downloader()->getSource()->name(), std::move(wsrcs), &Program::eventSwitchSource, nullptr, Alignment::center)
	};

	// download button and bar
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
		query = new LabelEdit(0.8f, string(), &Program::eventQuery, nullptr, nullptr, nullptr, LabelEdit::TextType::text, false),
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
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Comic ProgDownloader::curInfo() const {
	Comic info(static_cast<LabelEdit*>(*results->getSelected().begin())->getText(), vector<pair<string, string>>(chapterUrls.size()));
	for (size_t i = 0; i < chapterUrls.size(); ++i)
		info.chapters[i] = pair(chapters->getWidget<Layout>(i)->getWidget<Label>(1)->getText(), chapterUrls[i]);
	return info;
}

void ProgDownloader::printResults(vector<pair<string, string>>&& comics) {
	resultUrls.resize(comics.size());
	vector<Widget*> wgts(comics.size());
	for (size_t i = 0; i < wgts.size(); ++i) {
		wgts[i] = new Label(TileBox::defaultItemHeight, std::move(comics[i].first), &Program::eventShowComicInfo, nullptr, &Program::eventDownloadComic);
		resultUrls[i] = std::move(comics[i].second);
	}
	results->setWidgets(std::move(wgts));
}

void ProgDownloader::printInfo(vector<pair<string, string>>&& chaps) {
	chapterUrls.resize(chaps.size());
	vector<Widget*> wgts(chaps.size());
	for (size_t i = 0; i < wgts.size(); ++i) {
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
	if (!eventCommonEscape())
		World::program()->eventOpenDownloader();
}

RootLayout* ProgDownloads::createLayout() {
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
	std::initializer_list<const char*> txs = {
		"Resume",
		"Stop",
		"Clear"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	int txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, *itxs++, &Program::eventResumeDownloads),
		new Label(lineHeight, *itxs++, &Program::eventStopDownloads),
		new Label(lineHeight, *itxs++, &Program::eventClearDownloads)
	};

	// queue list
	World::downloader()->queueLock.lock();
	vector<Widget*> lse(World::downloader()->getDlQueue().size());
	for (size_t i = 0; i < lse.size(); ++i) {
		vector<Widget*> ln = {
			new Label(1.f, World::downloader()->getDlQueue()[i].title + " - waiting"),
			new Label(TileBox::defaultItemHeight, "X", &Program::eventDownloadDelete)
		};
		lse[i] = new Layout(TileBox::defaultItemHeight, std::move(ln), Direction::right);
	}
	if (!lse.empty())
		static_cast<Layout*>(lse[0])->getWidget<Label>(0)->setText(World::downloader()->getDlQueue()[0].title + " - " + toStr(World::downloader()->getDlProg().x) + '/' + toStr(World::downloader()->getDlProg().y));
	World::downloader()->queueLock.unlock();

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
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}
#endif

// PROG SETTINGS

ProgSettings::~ProgSettings() {
	stopMove();
}

void ProgSettings::eventSpecEscape() {
	World::program()->eventOpenBookList();
}

void ProgSettings::eventFullscreen() {
	ProgState::eventFullscreen();
	screen->setCurOpt(size_t(World::sets()->screen));
}

void ProgSettings::eventMultiFullscreen() {
	ProgState::eventMultiFullscreen();
	screen->setCurOpt(size_t(World::sets()->screen));
}

void ProgSettings::eventHide() {
	ProgState::eventHide();
	showHidden->on = World::sets()->showHidden;
}

void ProgSettings::eventFileDrop(const fs::path& file) {
	if (FileSys::isFont(file)) {
		World::drawSys()->setFont(file.u8string(), World::sets(), World::fileSys());
		World::scene()->resetLayouts();
	} else {
		try {
			if (fs::is_directory(file))
				World::sets()->setDirLib(file, World::fileSys()->getDirSets());
		} catch (const fs::filesystem_error& err) {
			logError(err.what());
		}
	}
}

RootLayout* ProgSettings::createLayout() {
	// top bar
	Text books("Library", topHeight);
	Text download("Download", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
		new Label(books.length, std::move(books.text), &Program::eventOpenBookList),
#ifdef DOWNLOADER
		new Label(download.length, std::move(download.text), &Program::eventOpenDownloader),
#endif
		new Label(exit.length, std::move(exit.text), &Program::eventTryExit)
	};

	// setting buttons and labels
	Text tcnt("Count:", lineHeight);
	Text tsiz("Size:", lineHeight);
	Text tsizp("Portrait", lineHeight);
	Text tsizl("Landscape", lineHeight);
	Text tsizs("Square", lineHeight);
	Text tsizf("Fill", lineHeight);
	static constexpr std::initializer_list<const char*> txs = {
		"Direction",
		"Zoom",
		"Spacing",
		"Picture limit",
		"Max picture size",
		"Screen",
		"Renderer",
		"Device",
		"Compress images",
		"VSync",
		"GPU selecting",
		"Size",
		"Theme",
		"Preview",
		"Show hidden",
		"Show tooltips",
		"Font",
		"Library",
		"Scroll speed",
		"Deadzone"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();

	uint maxRes;
	bool compression;
	World::renderer()->getSettings(maxRes, compression, devices);
	vector<string> dnames(devices.size());
	std::transform(devices.begin(), devices.end(), dnames.begin(), [](pair<u32vec2, string>& it) -> string { return std::move(it.second); });
	vector<pair<u32vec2, string>>::iterator curDev = std::find_if(devices.begin(), devices.end(), [](const pair<u32vec2, string>& it) -> bool { return World::sets()->device == it.first; });

	array<string, Binding::names.size()> bnames;
	for (uint8 i = 0; i < Binding::names.size(); ++i) {
		bnames[i] = Binding::names[i];
		bnames[i][0] = toupper(bnames[i][0]);
		std::replace(bnames[i].begin() + 1, bnames[i].end(), '_', ' ');
	}
	int plimLength = findMaxLength(PicLim::names.begin(), PicLim::names.end(), lineHeight);
	int descLength = std::max(findMaxLength(txs.begin(), txs.end(), lineHeight), findMaxLength(Binding::names.begin(), Binding::names.end(), lineHeight));
	constexpr char tipPicLim[] = "Picture limit per batch:\n"
		"- none: all pictures in directory/archive\n"
		"- count: number of pictures\n"
		"- size: total size of pictures";
	constexpr char tipDeadzon[] = "Controller axis deadzone";
	constexpr char tipMaxPicRes[] = "Maximum picture resolution";

	Size monitorSize = Size([](const Widget* wgt) -> int {
		const Layout* box = static_cast<const Layout*>(wgt);
		return box->getWidget<WindowArranger>(1)->precalcSizeExpand(box->size());
	});

	// action fields for labels
	vector<string> themes = World::fileSys()->getAvailableThemes();
	Text dots(KeyGetter::ellipsisStr, lineHeight);
	int unumLen = Text::measure("0000000000", lineHeight) + LabelEdit::caretWidth;
	vector<pair<Size, vector<Widget*>>> lx;
	lx.reserve(txs.size());
	lx.insert(lx.end(), {
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new ComboBox(1.f, size_t(World::sets()->direction), vector<string>(Direction::names.begin(), Direction::names.end()), &Program::eventSwitchDirection, makeTooltip("Reading direction"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, toStr(World::sets()->zoom), &Program::eventSetZoom, nullptr, nullptr, makeTooltip("Default reader zoom"), LabelEdit::TextType::uFloat)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, toStr(World::sets()->spacing), &Program::eventSetSpacing, nullptr, nullptr, makeTooltip("Picture spacing in reader"), LabelEdit::TextType::uInt)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new ComboBox(plimLength, size_t(World::sets()->picLim.type), vector<string>(PicLim::names.begin(), PicLim::names.end()), &Program::eventSetPicLimitType, makeTooltipL(tipPicLim)),
			createLimitEdit()
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->maxPicRes, Settings::minPicRes, maxRes, &Program::eventSetMaxPicResSL, nullptr, nullptr, makeTooltip(tipMaxPicRes)),
			new LabelEdit(unumLen, toStr(World::sets()->maxPicRes), &Program::eventSetMaxPicResLE, nullptr, nullptr, makeTooltip(tipMaxPicRes))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			screen = new ComboBox(1.f, size_t(World::sets()->screen), vector<string>(Settings::screenModeNames.begin(), Settings::screenModeNames.end()), &Program::eventSetScreenMode, makeTooltip("Window screen mode"))
		} },
		{ monitorSize, {
			new Widget(descLength),
			new WindowArranger(1.f, float(lineHeight * 2) / 1080.f, true, &Program::eventSetMultiFullscreen, &Program::eventSetMultiFullscreen, makeTooltip("Monitor arrangement for multi fullscreen"))
		} }
	});

	if constexpr (Settings::rendererNames.size() > 1) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new ComboBox(1.f, size_t(World::sets()->renderer), vector<string>(Settings::rendererNames.begin(), Settings::rendererNames.end()), &Program::eventSetRenderer, makeTooltip("Rendering backend"))
		} });
	}
	++itxs;
	if (!devices.empty()) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new ComboBox(1.f, curDev != devices.end() ? curDev - devices.begin() : 0, std::move(dnames),& Program::eventSetDevice, makeTooltip("Rendering devices"))
		} });
	}
	++itxs;
	if (compression) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new CheckBox(lineHeight, World::sets()->compression, &Program::eventSetCompression, nullptr, nullptr, makeTooltip("Compress pictures to reduce memory usage"))
		} });
	}
	++itxs;

	lx.insert(lx.end(), {
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->vsync, &Program::eventSetVsync, nullptr, nullptr, makeTooltip("Vertical synchronization"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->gpuSelecting, &Program::eventSetGpuSelecting, nullptr, nullptr, makeTooltip("Use the graphics process to determine which widget is being selected"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new Label(tsizp.length, std::move(tsizp.text), &Program::eventSetPortrait, nullptr, nullptr, makeTooltip("Portrait window size")),
			new Label(tsizl.length, std::move(tsizl.text), &Program::eventSetLandscape, nullptr, nullptr, makeTooltip("Landscape window size")),
			new Label(tsizs.length, std::move(tsizs.text), &Program::eventSetSquare, nullptr, nullptr, makeTooltip("Square window size")),
			new Label(tsizf.length, std::move(tsizf.text), &Program::eventSetFill, nullptr, nullptr, makeTooltip("Fill screen with window"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new ComboBox(1.f, string(World::sets()->getTheme()), std::move(themes), &Program::eventSetTheme, makeTooltip("Color scheme"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->preview, &Program::eventSetPreview, nullptr, nullptr, makeTooltip("Show preview icons of pictures in browser"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			showHidden = new CheckBox(lineHeight, World::sets()->showHidden, &Program::eventSetHide, nullptr, nullptr, makeTooltip("Show hidden files"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->tooltips, &Program::eventSetTooltips, nullptr, nullptr, makeTooltip("Display tooltips on hover"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, string(World::sets()->font), &Program::eventSetFont, nullptr, nullptr, makeTooltip("Font name or path"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, World::sets()->getDirLib().u8string(), &Program::eventSetLibraryDirLE, nullptr, nullptr, makeTooltip("Library path")),
			new Label(dots.length, std::move(dots.text), &Program::eventOpenLibDirBrowser, nullptr, nullptr, makeTooltip("Browse for library"), Alignment::center)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, World::sets()->scrollSpeedString(), &Program::eventSetScrollSpeed, nullptr, nullptr, makeTooltip("Scroll speed for button presses or axes"), LabelEdit::TextType::sFloatSpaced)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->getDeadzone(), 0, Settings::axisLimit, &Program::eventSetDeadzoneSL, nullptr, nullptr, makeTooltip(tipDeadzon)),
			new LabelEdit(unumLen, toStr(World::sets()->getDeadzone()), &Program::eventSetDeadzoneLE, nullptr, nullptr, makeTooltip(tipDeadzon), LabelEdit::TextType::uInt)
		} }
	});
	size_t lcnt = lx.size();
	vector<Widget*> lns(lcnt + 2 + bnames.size() + 2);
	for (size_t i = 0; i < lcnt; ++i)
		lns[i] = new Layout(lx[i].first, std::move(lx[i].second), Direction::right);
	lns[lcnt] = new Widget(0);
	lns[lcnt + 1] = new Layout(lineHeight, { new Widget(descLength), new Label(1.f, "Keyboard", nullptr, nullptr, nullptr, nullptr, Alignment::center, Picture::nullTex, false), new Label(1.f, "DirectInput", nullptr, nullptr, nullptr, nullptr, Alignment::center, Picture::nullTex, false), new Label(1.f, "XInput", nullptr, nullptr, nullptr, nullptr, Alignment::center, Picture::nullTex, false) }, Direction::right);
	limitLine = static_cast<Layout*>(lns[3]);

	// shortcut entries
	for (size_t i = 0; i < bnames.size(); ++i) {
		Label* lbl = new Label(descLength, std::move(bnames[i]));
		vector<Widget*> lin {
			lbl,
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, Binding::Type(i), makeTooltip((lbl->getText() + " keyboard binding").c_str())),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, Binding::Type(i), makeTooltip((lbl->getText() + " joystick binding").c_str())),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, Binding::Type(i), makeTooltip((lbl->getText() + " gamepad binding").c_str()))
		};
		lns[lcnt + 2 + i] = new Layout(lineHeight, std::move(lin), Direction::right);
	}
	lcnt += 2 + Binding::names.size();

	// reset button
	Text resbut("Reset", lineHeight);
	lns[lcnt] = new Widget(0);
	lns[lcnt + 1] = new Layout(lineHeight, { new Label(resbut.length, std::move(resbut.text), &Program::eventResetSettings, nullptr, nullptr, makeTooltip("Reset all settings")) }, Direction::right);

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, Layout::Select::none, topSpacing),
		new ScrollArea(1.f, std::move(lns))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Widget* ProgSettings::createLimitEdit() {
	switch (World::sets()->picLim.type) {
	case PicLim::Type::count:
		return new LabelEdit(1.f, toStr(World::sets()->picLim.getCount()), &Program::eventSetPicLimCount, nullptr, nullptr, makeTooltip("Number of pictures per batch"), LabelEdit::TextType::uInt);
	case PicLim::Type::size:
		return new LabelEdit(1.f, PicLim::memoryString(World::sets()->picLim.getSize()), &Program::eventSetPicLimSize, nullptr, nullptr, makeTooltip("Total size of pictures per batch"));
	}
	return new Widget();
}

void ProgSettings::startMove() {
	stopMove();
	threadType = ThreadType::move;
	thread = std::thread(&FileSys::moveContentThread, std::ref(threadType), std::move(oldPathBuffer), World::sets()->getDirLib());
}

void ProgSettings::stopMove() {
	if (thread.joinable()) {
		threadType = ThreadType::none;
		thread.join();

		SDL_FlushEvent(SDL_USEREVENT_MOVE_PROGRESS);
		cleanupEvent(SDL_USEREVENT_MOVE_FINISHED, [](SDL_UserEvent& user) {
			string* errors = static_cast<string*>(user.data1);
			logMoveErrors(errors);
			delete errors;
		});
	}
}

void ProgSettings::logMoveErrors(const string* errors) {
	for (string::const_iterator pos = errors->begin(); pos != errors->end();) {
		string::const_iterator next = std::find(pos, errors->end(), '\n');
		logError(string_view(&*pos, next - pos));
		pos = std::find_if(next, errors->end(), [](char c) -> bool { return c != '\n'; });
	}
}

// PROG SEARCH DIR

void ProgSearchDir::eventSpecEscape() {
	World::program()->eventBrowserGoUp();
}

RootLayout* ProgSearchDir::createLayout() {
	// sidebar
	std::initializer_list<const char*> txs = {
		"Exit",
		"Up",
		"Set"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	int txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, *itxs++, &Program::eventExitBrowser),
		new Label(lineHeight, *itxs++, &Program::eventBrowserGoUp),
		new Label(lineHeight, *itxs++, &Program::eventSetLibraryDirBW)
	};

	// directory list
	vector<fs::path> strs = FileSys::listDir(World::browser()->getCurDir(), false, true, World::sets()->showHidden);
	vector<Widget*> items(strs.size());
	for (size_t i = 0; i < strs.size(); ++i)
		items[i] = makeDirectoryEntry(lineHeight, strs[i].u8string());
	dirEnd = strs.size();
	fileEnd = dirEnd;

	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		fileList = new ScrollArea(1.f, std::move(items), Direction::down, Layout::Select::one)
	};

	// root layout
	vector<Widget*> cont = {
		locationBar = new LabelEdit(lineHeight, World::browser()->getCurDir().u8string(), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, Layout::Select::none, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, Layout::Select::none, topSpacing);
}

Label* ProgSearchDir::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), nullptr, nullptr, &Program::eventBrowserGoIn, nullptr, Alignment::left, World::drawSys()->texture("folder"));
}
