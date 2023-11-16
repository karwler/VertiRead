#include "progs.h"
#include "program.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include "utils/compare.h"
#include "utils/layouts.h"

// PROGRAM STATE

void ProgState::eventEnter() {
	if (World::scene()->getSelect())
		World::scene()->getSelect()->onClick(World::scene()->getSelect()->position(), SDL_BUTTON_LEFT);
}

void ProgState::eventEscape() {
	World::scene()->onCancel();
}

void ProgState::eventSelect(Direction dir) {
	World::inputSys()->mouseWin = std::nullopt;
	if (World::scene()->getSelect() && !dynamic_cast<Layout*>(World::scene()->getSelect()))
		World::scene()->getSelect()->onNavSelect(dir);
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
	popupLineHeight = int(40.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	tooltipHeight = int(16.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	lineHeight = int(30.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	topHeight = int(40.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	topSpacing = int(10.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	picSize = int(40.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	contextMargin = int(3.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi());
	maxTooltipLength = World::drawSys()->getViewRes().x * 2 / 3;
	cursorMoveFactor = 10.f / DrawSys::fallbackDpi * World::drawSys()->getWinDpi();
}

Overlay* ProgState::createOverlay() {
	return nullptr;
}

Popup* ProgState::createPopupMessage(Cstring&& msg, EventId ccal, Cstring&& ctxt, Alignment malign) {
	uint oklen = measureText(ctxt, popupLineHeight);
	uint mglen = measureText(msg, popupLineHeight);
	Widget* first;
	Children bot = {
		new Widget(),
		first = new PushButton(oklen, std::move(ctxt), ccal, ACT_LEFT, Cstring(), Alignment::center),
		new Widget()
	};
	Children con = {
		new Label(1.f, std::move(msg), malign),
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(std::max(mglen, oklen) + Layout::defaultItemSpacing * 2, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

void ProgState::updatePopupMessage(Cstring&& msg) {
	if (Popup* popup = World::scene()->getPopup()) {
		uint mglen = measureText(msg, popupLineHeight);
		popup->getWidget<Label>(0)->setText(std::move(msg));
		popup->setSize(std::max(mglen, measureText(popup->getWidget<Layout>(1)->getWidget<PushButton>(1)->getText(), popupLineHeight)) + Layout::defaultItemSpacing * 2);
	}
}

Popup* ProgState::createPopupMultiline(Cstring&& msg, EventId ccal, Cstring&& ctxt) {
	uint oklen = measureText(ctxt, popupLineHeight);
	uvec2 viewRes = World::drawSys()->getViewRes();
	uint width = oklen, lines = 0;
	for (char* pos = msg.data();;) {
		++lines;
		char* end = strchr(pos, '\n');
		if (uint siz = World::drawSys()->textLength(string_view(pos, end ? end : pos + strlen(pos)), lineHeight) + (TextDsp<Cstring>::textMargin + Layout::defaultItemSpacing) * 2; siz > width)
			if (width = std::min(siz, viewRes.x); width == viewRes.x)
				break;
		if (!end)
			break;
		pos = end + 1;
	}

	Widget* first;
	Children bot = {
		new Widget(),
		first = new PushButton(oklen, std::move(ctxt), ccal, ACT_LEFT, Cstring(), Alignment::center),
		new Widget()
	};
	Children con = {
		new TextBox(1.f, lineHeight, std::move(msg)),
		new Layout(popupLineHeight, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(width, std::min(lineHeight * lines + popupLineHeight + Layout::defaultItemSpacing * 3, viewRes.y)), std::move(con), ccal, first);
}

Popup* ProgState::createPopupChoice(Cstring&& msg, EventId kcal, EventId ccal, Alignment malign) {
	uint mglen = measureText(msg, popupLineHeight);
	uint yeslen = measureText("Yes", popupLineHeight);
	uint nolen = measureText("No", popupLineHeight);
	Widget* first;
	Children bot = {
		first = new PushButton(1.f, "Yes", kcal, ACT_LEFT, Cstring(), Alignment::center),
		new PushButton(1.f, "No", ccal, ACT_LEFT, Cstring(), Alignment::center)
	};
	Children con = {
		new Label(1.f, std::move(msg), malign),
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(std::max(mglen, yeslen + nolen) + Layout::defaultItemSpacing * 3, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

Popup* ProgState::createPopupInput(Cstring&& msg, string&& text, EventId kcal, EventId ccal, Cstring&& ktxt, Alignment malign) {
	Widget* first;
	Children bot = {
		new PushButton(1.f, std::move(ktxt), kcal, ACT_LEFT, Cstring(), Alignment::center),
		new PushButton(1.f, "Cancel", ccal, ACT_LEFT, Cstring(), Alignment::center)
	};
	Children con = {
		new Label(1.f, std::move(msg), malign),
		first = new LabelEdit(1.f, std::move(text), kcal, ACT_LEFT),
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(0.75f, popupLineHeight * 3 + Layout::defaultItemSpacing * 4), std::move(con), ccal, first);
}

const string& ProgState::inputFromPopup() {
	return World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
}

Popup* ProgState::createPopupRemoteLogin(RemoteLocation&& rl, EventId kcal, EventId ccal) {
	static constexpr std::initializer_list<const char*> txs = {
		"User",
		"Password",
		"Server",
		"Path",
		"Port",
		"Workgroup",
		"Family",
		"Save"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	uint descLength = findMaxLength(txs.begin(), txs.end(), popupLineHeight);
	uint protocolLength = findMaxLength(protocolNames.begin(), protocolNames.end(), popupLineHeight);
	uint portLength = measureText("00000", popupLineHeight) + LabelEdit::caretWidth;
	uint familyLblLength = measureText(txs.end()[-1], popupLineHeight);
	uint familyValLength = findMaxLength(familyNames.begin(), familyNames.end(), popupLineHeight);
	uint valLength = std::max(protocolLength + Layout::defaultItemSpacing + descLength * 2, portLength + familyLblLength + familyValLength + Layout::defaultItemSpacing * 2);

	string msg = std::format("{} Login", rl.protocol == Protocol::smb ? "SMB" : "SFTP");
	uint mglen = measureText(msg, popupLineHeight);
	uint yeslen = measureText("Log In", popupLineHeight);
	uint nolen = measureText("Cancel", popupLineHeight);
	Children user = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.user), nullEvent, ACT_NONE, "Username")
	};
	Children password = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.password), nullEvent, ACT_NONE, "Password", LabelEdit::TextType::password)
	};
	Children server = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.server), nullEvent, ACT_NONE, "Server address")
	};
	Children path = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.path), nullEvent, ACT_NONE, "Directory to open"s + (rl.protocol == Protocol::smb ? " (must start with the share name)" : ""))
	};
	vector<Widget*> port = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, toStr(rl.port), nullEvent, ACT_NONE, "Port", rl.protocol == Protocol::smb ? LabelEdit::TextType::uInt : LabelEdit::TextType::any)
	};
	if (rl.protocol == Protocol::sftp)
		port.insert(port.end(), {
			new Label(familyLblLength, *++itxs),
			new ComboBox(familyValLength, eint(Family::any), vector<Cstring>(familyNames.begin(), familyNames.end()), GeneralEvent::confirmComboBox, "Family for resolving the server address")
		});
	Children bot = {
		new PushButton(1.f, "Log In", kcal, ACT_LEFT, Cstring(), Alignment::center),
		new PushButton(1.f, "Cancel", ccal, ACT_LEFT, Cstring(), Alignment::center)
	};
	Widget* first = rl.user.empty() ? user[1] : password[1];

	vector<Widget*> con {
		new Label(popupLineHeight, std::move(msg), Alignment::center),
		new Layout(popupLineHeight, std::move(user), Direction::right),
		new Layout(popupLineHeight, std::move(password), Direction::right),
		new Layout(popupLineHeight, std::move(server), Direction::right),
		new Layout(popupLineHeight, std::move(path), Direction::right),
		new Layout(popupLineHeight, std::move(port), Direction::right),
		new Layout(popupLineHeight, std::move(bot), Direction::right),
	};
	if (rl.protocol == Protocol::smb) {
		Children workgroup = {
			new Label(descLength, *itxs),
			new LabelEdit(1.f, std::move(rl.workgroup), nullEvent, ACT_NONE, "Workgroup")
		};
		con.insert(con.begin() + 2, new Layout(popupLineHeight, std::move(workgroup), Direction::right));
	}
	++itxs;
	if (World::program()->canStoreCredentials()) {
		Children saveCredentials {
			new Label(descLength, *itxs),
			new CheckBox(popupLineHeight, false, nullEvent, "Remember credentials")
		};
		con.insert(con.end() - 1, new Layout(popupLineHeight, std::move(saveCredentials), Direction::right));
	}
	uint numLines = con.size();
	return new Popup(svec2(std::max(std::max(mglen, yeslen + nolen), descLength + valLength) + Layout::defaultItemSpacing * 3, popupLineHeight * numLines + Layout::defaultItemSpacing * (numLines + 1)), std::move(con), ccal, first);
}

pair<RemoteLocation, bool> ProgState::remoteLocationFromPopup() {
	std::span<Widget*> con = World::scene()->getPopup()->getWidgets();
	uint offs = con.size() == uint(8 + World::program()->canStoreCredentials());
	std::span<Widget*> portfam = static_cast<Layout*>(con[5 + offs])->getWidgets();
	return pair(RemoteLocation{
		.server = static_cast<Layout*>(con[2 + offs])->getWidget<LabelEdit>(2)->getText(),
		.path = static_cast<Layout*>(con[4 + offs])->getWidget<LabelEdit>(1)->getText(),
		.user = static_cast<Layout*>(con[1])->getWidget<LabelEdit>(1)->getText(),
		.workgroup = offs ? static_cast<Layout*>(con[2])->getWidget<LabelEdit>(1)->getText() : string(),
		.password = static_cast<Layout*>(con[2 + offs])->getWidget<LabelEdit>(1)->getText(),
		.port = toNum<uint16>(static_cast<LabelEdit*>(portfam[1])->getText()),
		.protocol = offs ? Protocol::smb : Protocol::sftp,
		.family = !offs ? Family(static_cast<ComboBox*>(portfam[3])->getCurOpt()) : Family::any
	}, World::program()->canStoreCredentials() ? static_cast<Layout*>(con[con.size() - 2])->getWidget<CheckBox>(1)->on : false);
}

Context* ProgState::createContext(vector<pair<Cstring, EventId>>&& items, Widget* parent) {
	Children wgts(items.size());
	for (uint i = 0; i < wgts.num; ++i)
		wgts[i] = new PushButton(lineHeight, std::move(items[i].first), items[i].second, ACT_LEFT);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, World::winSys()->mousePos(), ivec2(0, lineHeight), contextMargin);
	return new Context(rect.pos(), rect.size(), Children{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, 0) }, first, parent, Color::dark, nullEvent, Layout::defaultDirection, contextMargin);
}

Context* ProgState::createComboContext(ComboBox* parent, EventId kcal) {
	ivec2 size = parent->size();
	Children wgts(parent->getOptions().size());
	if (parent->getTooltips()) {
		for (uint i = 0; i < wgts.num; ++i)
			wgts[i] = new PushButton(size.y, valcp(parent->getOptions()[i]), kcal, ACT_LEFT, valcp(parent->getTooltips()[i]));
	} else
		for (uint i = 0; i < wgts.num; ++i)
			wgts[i] = new PushButton(size.y, valcp(parent->getOptions()[i]), kcal, ACT_LEFT);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, parent->position(), size, contextMargin);
	return new Context(rect.pos(), rect.size(), Children{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, 0) }, first, parent, Color::dark, GeneralEvent::resizeComboContext, Layout::defaultDirection, contextMargin);
}

Recti ProgState::calcTextContextRect(const Children& items, ivec2 pos, ivec2 size, int margin) {
	for (uint i = 0; i < items.num; ++i)
		if (auto lbl = dynamic_cast<PushButton*>(items[i]))
			if (int w = World::drawSys()->textLength(lbl->getText(), size.y) + TextDsp<Cstring>::textMargin * 2 + Scrollable::barSizeVal + margin * 2; w > size.x)
				size.x = w;
	size.y = size.y * uint(items.num) + margin * 2;

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

template <Iterator T>
uint ProgState::findMaxLength(T pos, T end, uint height) {
	uint width = 0;
	for (; pos != end; ++pos)
		if (uint len = measureText(*pos, height); len > width)
			width = len;
	return width;
}

uint ProgState::measureText(string_view str, uint height) {
	return World::drawSys()->textLength(str, height) + TextDsp<Cstring>::textMargin * 2;
}

// PROG FILE EXPLORER

void ProgFileExplorer::eventHide() {
	ProgState::eventHide();
	eventRefresh();
}

void ProgFileExplorer::eventRefresh() {
	float loc = fileList->getScrollLocation();
	World::scene()->resetLayouts();
	fileList->setScrollLocation(loc);
	World::scene()->updateSelect();
}

void ProgFileExplorer::processFileChanges(Browser* browser) {
	if (browser->directoryUpdate(fileChanges)) {
		fileList->setWidgets(Children());
		if (locationBar)
			locationBar->setText(browser->currentLocation());
		return fileChanges.clear();
	}

	auto compare = [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const PushButton*>(a)->getText(), b); };
	std::span<Widget*> wgts = fileList->getWidgets();
	for (FileChange& fc : fileChanges) {
		if (fc.type == FileChange::deleteEntry) {
			std::span<Widget*>::iterator sp = wgts.begin() + dirEnd;
			std::span<Widget*>::iterator it = std::lower_bound(wgts.begin(), sp, fc.name, compare);
			bool directory = it != sp && static_cast<const PushButton*>(*it)->getText() == fc.name;
			if (!directory) {
				std::span<Widget*>::iterator ef = wgts.begin() + fileEnd;
				if (it = std::lower_bound(sp, ef, fc.name, compare); it == ef || static_cast<const PushButton*>(*it)->getText() != fc.name)
					continue;
			}
			fileList->deleteWidget((*it)->getIndex());
			if (directory)
				--dirEnd;
			--fileEnd;
		} else {
			auto [pos, end] = fc.type == FileChange::addDirectory ? pair(wgts.begin(), wgts.begin() + dirEnd) : pair(wgts.begin() + dirEnd, wgts.begin() + fileEnd);
			uint id = std::lower_bound(pos, end, fc.name, compare) - wgts.begin();
			Size size = fileEntrySize(fc.name);
			if (fc.type == FileChange::addDirectory) {
				fileList->insertWidget(id, makeDirectoryEntry(size, std::move(fc.name)));
				++dirEnd;
				++fileEnd;
			} else if (PushButton* pb = makeFileEntry(size, std::move(fc.name))) {
				fileList->insertWidget(id, pb);
				++fileEnd;
			}
		}
	}
	fileChanges.clear();
}

PushButton* ProgFileExplorer::makeFileEntry(const Size&, Cstring&&) {
	return nullptr;
}

Size ProgFileExplorer::fileEntrySize(string_view) {
	return lineHeight;
}

// PROG BOOKS

void ProgBooks::eventSpecEscape() {
	World::program()->eventExit();
}

void ProgBooks::eventFileDrop(const char* file) {
	World::program()->openFile(file);
}

RootLayout* ProgBooks::createLayout() {
	// top bar
	Children top = {
		new PushButton(measureText("Settings", topHeight), "Settings", ProgBooksEvent::openSettings),
		new PushButton(measureText("Exit", topHeight), "Exit", GeneralEvent::exit)
	};

	// book list
	vector<string> books = World::program()->getBrowser()->listDirDirs(World::sets()->dirLib);
	Children tiles(books.size() + 1);
	for (size_t i = 0; i < books.size(); ++i)
		tiles[i] = makeBookTile(std::move(books[i]));
	tiles[books.size()] = new IconButton(TileBox::defaultItemHeight, World::drawSys()->texture("search"), ProgBooksEvent::openPageBrowserGeneral, ACT_LEFT | ACT_RIGHT, "Browse other directories");
	dirEnd = books.size();
	fileEnd = dirEnd;

	// root layout
	Children cont = {
		new Layout(topHeight, std::move(top), Direction::right, topSpacing),
		fileList = new TileBox(1.f, std::move(tiles))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

PushButton* ProgBooks::makeBookTile(Cstring&& name) {
	uint len = measureText(name, TileBox::defaultItemHeight);
	return makeDirectoryEntry(len, std::move(name));
}

PushButton* ProgBooks::makeDirectoryEntry(const Size& size, Cstring&& name) {
	return new PushButton(size, std::move(name), ProgBooksEvent::openPageBrowser, ACT_LEFT | ACT_RIGHT);
}

Size ProgBooks::fileEntrySize(string_view name) {
	return measureText(name, TileBox::defaultItemHeight);
}

// PROG PAGE BROWSER

ProgPageBrowser::~ProgPageBrowser() {
	World::program()->getBrowser()->stopThread();
}

void ProgPageBrowser::eventSpecEscape() {
	World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventFileDrop(const char* file) {
	World::program()->openFile(file);
}

void ProgPageBrowser::resetFileIcons() {
	const Texture* dtex = World::drawSys()->texture("folder");
	const Texture* ftex = World::drawSys()->texture("file");
	std::span<Widget*> wgts = fileList->getWidgets();
	size_t i = 0;
	for (; i < dirEnd; ++i)
		static_cast<IconPushButton*>(wgts[i])->setIcon(dtex);
	for (; i < fileEnd; ++i)
		static_cast<IconPushButton*>(wgts[i])->setIcon(ftex);
}

RootLayout* ProgPageBrowser::createLayout() {
	// sidebar
	static constexpr std::initializer_list<const char*> txs = {
		"Exit",
		"Up"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	uint txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	Children bar = {
		new PushButton(lineHeight, *itxs++, ProgFileExplorerEvent::exit),
		new PushButton(lineHeight, *itxs++, ProgFileExplorerEvent::goUp)
	};

	// list of files and directories
	Browser* browser = World::program()->getBrowser();
	auto [files, dirs] = browser->listCurDir();
	if (World::sets()->preview)
		browser->startPreview(files, dirs, lineHeight);
	Children items(files.size() + dirs.size());
	for (size_t i = 0; i < dirs.size(); ++i)
		items[i] = makeDirectoryEntry(lineHeight, std::move(dirs[i]));
	for (size_t i = 0; i < files.size(); ++i)
		items[dirs.size() + i] = makeFileEntry(lineHeight, std::move(files[i]));
	dirEnd = dirs.size();
	fileEnd = dirEnd + files.size();

	// main content
	Children mid = {
		new Layout(txsWidth, std::move(bar)),
		fileList = new ScrollArea(1.f, std::move(items))
	};

	// root layout
	string location = browser->currentLocation();
	string_view rpath = relativePath(location, World::sets()->dirLib);
	Children cont = {
		locationBar = new LabelEdit(lineHeight, rpath.empty() ? std::move(location) : string(rpath), ProgFileExplorerEvent::goTo),
		new Layout(1.f, std::move(mid), Direction::right, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

PushButton* ProgPageBrowser::makeDirectoryEntry(const Size& size, Cstring&& name) {
	return new IconPushButton(size, std::move(name), World::drawSys()->texture("folder"), ProgFileExplorerEvent::goIn);
}

PushButton* ProgPageBrowser::makeFileEntry(const Size&, Cstring&& name) {
	return new IconPushButton(lineHeight, std::move(name), World::drawSys()->texture("file"), ProgPageBrowserEvent::goFile);
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

void ProgReader::eventZoomFit() {
	World::program()->eventZoomFit();
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
	World::program()->getBrowser()->startLoadPictures(string(reader->firstPage()));
	World::program()->setPopupLoading();
}

void ProgReader::eventRefresh() {
	float loc = reader->getScrollLocation();
	World::scene()->resetLayouts();
	reader->setScrollLocation(loc);
	World::scene()->updateSelect();
}

void ProgReader::eventClosing() {
	Browser* browser = World::program()->getBrowser();
	if (string_view rpath = relativePath(browser->getCurDir(), World::sets()->dirLib); rpath.empty())
		World::fileSys()->saveLastPage(dotStr, browser->getCurDir(), browser->curDirSuffix() + reader->curPage());
	else {
		string_view::iterator mid = rng::find_if(rpath, isDsep);
		string_view::iterator nxt = std::find_if(mid, rpath.end(), notDsep);
		World::fileSys()->saveLastPage(string_view(rpath.begin(), mid), string_view(nxt, rpath.end()), browser->curDirSuffix() + reader->curPage());
	}
	SDL_ShowCursor(SDL_ENABLE);
}

RootLayout* ProgReader::createLayout() {
	Children cont = { reader = new ReaderBox(1.f, World::sets()->direction, World::sets()->zoom, World::sets()->spacing) };
	return new RootLayout(1.f, std::move(cont), Direction::right, 0);
}

Overlay* ProgReader::createOverlay() {
	Children menu = {
		new IconButton(picSize, World::drawSys()->texture("cross"), ProgReaderEvent::exit, ACT_LEFT, makeTooltipWithKey("Exit", Binding::Type::escape)),
		new IconButton(picSize, World::drawSys()->texture("right"), ProgReaderEvent::nextDir, ACT_LEFT, makeTooltipWithKey("Next", Binding::Type::nextDir)),
		new IconButton(picSize, World::drawSys()->texture("left"), ProgReaderEvent::prevDir, ACT_LEFT, makeTooltipWithKey("Previous", Binding::Type::prevDir)),
		new IconButton(picSize, World::drawSys()->texture("plus"), ProgReaderEvent::zoomIn, ACT_LEFT, makeTooltipWithKey("Zoom in", Binding::Type::zoomIn)),
		new IconButton(picSize, World::drawSys()->texture("minus"), ProgReaderEvent::zoomOut, ACT_LEFT, makeTooltipWithKey("Zoom out", Binding::Type::zoomOut)),
		new IconButton(picSize, World::drawSys()->texture("reset"), ProgReaderEvent::zoomReset, ACT_LEFT, makeTooltipWithKey("Zoom reset", Binding::Type::zoomReset)),
		new IconButton(picSize, World::drawSys()->texture("fit"), ProgReaderEvent::zoomFit, ACT_LEFT, makeTooltipWithKey("Zoom fit", Binding::Type::zoomFit)),
		new IconButton(picSize, World::drawSys()->texture("center"), ProgReaderEvent::centerView, ACT_LEFT, makeTooltipWithKey("Center", Binding::Type::centerView))
	};
	int ysiz = picSize * menu.num;
	return new Overlay(svec2(0), svec2(picSize, ysiz), svec2(0), svec2(picSize/2, ysiz), std::move(menu), Color::normal, Direction::down, 0);
}

Cstring ProgReader::makeTooltipWithKey(const char* text, Binding::Type type) {
	string btext = World::inputSys()->getBoundName(type);
	return !btext.empty() ? std::format("{} ({})", text, btext) : text;
}

int ProgReader::modifySpeed(float value) {
	if (float factor = 1.f; World::inputSys()->isPressed(Binding::Type::scrollFast, factor))
		value *= scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::scrollSlow, factor))
		value /= scrollFactor * factor;
	return int(value * World::winSys()->getDSec());
}

// PROG SETTINGS

ProgSettings::~ProgSettings() {
	stopFonts();
	stopMove();
}

void ProgSettings::eventSpecEscape() {
	World::program()->eventOpenBookList();
}

void ProgSettings::eventFullscreen() {
	ProgState::eventFullscreen();
	screen->setCurOpt(uint(World::sets()->screen));
}

void ProgSettings::eventMultiFullscreen() {
	ProgState::eventMultiFullscreen();
	screen->setCurOpt(uint(World::sets()->screen));
}

void ProgSettings::eventHide() {
	ProgState::eventHide();
	showHidden->on = World::sets()->showHidden;
}

void ProgSettings::eventRefresh() {
	float loc = static_cast<ScrollArea*>(limitLine->getParent())->getScrollLocation();
	World::scene()->resetLayouts();
	static_cast<ScrollArea*>(limitLine->getParent())->setScrollLocation(loc);
	World::scene()->updateSelect();
}

void ProgSettings::eventFileDrop(const char* file) {
	if (fs::path path = toPath(file);  FileSys::isFont(path))
		World::program()->setFont(path);
	else if (fs::is_directory(toPath(file)))
		World::program()->setLibraryDir(file);
}

RootLayout* ProgSettings::createLayout() {
	// top bar
	Children top = {
		new PushButton(measureText("Library", topHeight), "Library", ProgBooksEvent::openBookList),
		new PushButton(measureText("Exit", topHeight), "Exit", GeneralEvent::exit)
	};

	// setting buttons and labels
	static constexpr std::initializer_list<const char*> txs = {
		"Direction",
		"Zoom",
		"Spacing",
		"Picture limit",
		"Max picture size",
		"Screen",
		"Renderer",
		"Device",
		"Image compression",
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

	auto [maxRes, maxCompress] = World::drawSys()->getRenderer()->getSettings(devices);
	vector<Cstring> dnames(devices.size());
	rng::transform(devices, dnames.begin(), [](pair<u32vec2, string>& it) -> Cstring { return it.second; });
	vector<pair<u32vec2, string>>::iterator curDev = rng::find_if(devices, [](const pair<u32vec2, string>& it) -> bool { return World::sets()->device == it.first; });

	array<string, Binding::names.size()> bnames;
	for (uint8 i = 0; i < Binding::names.size(); ++i) {
		bnames[i] = Binding::names[i];
		bnames[i][0] = toupper(bnames[i][0]);
		std::replace(bnames[i].begin() + 1, bnames[i].end(), '_', ' ');
	}
	uint plimLength = findMaxLength(PicLim::names.begin(), PicLim::names.end(), lineHeight);
	uint descLength = std::max(findMaxLength(txs.begin(), txs.end(), lineHeight), findMaxLength(Binding::names.begin(), Binding::names.end(), lineHeight));
	static constexpr char tipPicLim[] = "Picture limit per batch:\n"
		"- none: all pictures in directory/archive\n"
		"- count: number of pictures\n"
		"- size: total size of pictures";
	static constexpr char tipDeadzone[] = "Controller axis deadzone";
	static constexpr char tipMaxPicRes[] = "Maximum picture resolution";
	static constexpr char tipCompression[] = "Texture compression of pictures:\n"
		"- none: load textures uncompressed\n"
		"- 16 b: squash texels to 16 bits\n"
		"- compress: use compressed textures";

	Size monitorSize = Size([](const Widget* wgt) -> int {
		auto box = static_cast<const Layout*>(wgt);
		return box->getWidget<WindowArranger>(1)->precalcSizeExpand(box->getParent()->size().x - box->getWidget(0)->getRelSize().pix - box->getSpacing() - Scrollable::barSizeVal);
	});

	// action fields for labels
	vector<string> themes = World::fileSys()->getAvailableThemes();
	uint unumLen = measureText("0000000000", lineHeight) + LabelEdit::caretWidth;
	startFonts();

	vector<pair<Size, Children>> lx;
	lx.reserve(txs.size());

	array<pair<Size, Children>, 7> sec0 = {
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new ComboBox(1.f, uint(World::sets()->direction), vector<Cstring>(Direction::names.begin(), Direction::names.end()), ProgSettingsEvent::setDirection, "Reading direction")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->zoom, -Settings::zoomLimit, Settings::zoomLimit, ProgSettingsEvent::setZoom, ACT_LEFT, "Default reader zoom"),
			new Label(unumLen, makeZoomText())
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, toStr(World::sets()->spacing), ProgSettingsEvent::setSpacing, ACT_LEFT, "Picture spacing in reader", LabelEdit::TextType::uInt)
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new ComboBox(plimLength, eint(World::sets()->picLim.type), vector<Cstring>(PicLim::names.begin(), PicLim::names.end()), ProgSettingsEvent::setPicLimitType, tipPicLim),
			createLimitEdit()
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->maxPicRes, Settings::minPicRes, maxRes, ProgSettingsEvent::setMaxPicResSl, ACT_LEFT, tipMaxPicRes),
			new LabelEdit(unumLen, toStr(World::sets()->maxPicRes), ProgSettingsEvent::setMaxPicResLe, ACT_LEFT, tipMaxPicRes)
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			screen = new ComboBox(1.f, eint(World::sets()->screen), vector<Cstring>(Settings::screenModeNames.begin(), Settings::screenModeNames.end()), ProgSettingsEvent::setScreenMode, "Window screen mode")
		}),
		pair(monitorSize, Children{
			new Widget(descLength),
			new WindowArranger(1.f, float(lineHeight * 2) / 1080.f, true, ProgSettingsEvent::setMultiFullscreen, ACT_LEFT, "Monitor arrangement for multi fullscreen")
		})
	};
	lx.insert(lx.end(), std::make_move_iterator(sec0.begin()), std::make_move_iterator(sec0.end()));

	if constexpr (Settings::rendererNames.size() > 1) {
		lx.emplace_back(lineHeight, Children{
			new Label(descLength, *itxs),
			new ComboBox(1.f, eint(World::sets()->renderer), vector<Cstring>(Settings::rendererNames.begin(), Settings::rendererNames.end()), ProgSettingsEvent::setRenderer, "Rendering backend")
		});
	}
	++itxs;
	if (!devices.empty()) {
		lx.emplace_back(lineHeight, Children{
			new Label(descLength, *itxs),
			new ComboBox(1.f, curDev != devices.end() ? curDev - devices.begin() : 0, std::move(dnames), ProgSettingsEvent::setDevice, "Rendering devices")
		});
	}
	++itxs;
	if (maxCompress > Settings::Compression::none) {
		lx.emplace_back(lineHeight, Children{
			new Label(descLength, *itxs),
			new ComboBox(1.f, eint(World::sets()->compression), vector<Cstring>(Settings::compressionNames.begin(), Settings::compressionNames.begin() + eint(maxCompress) + 1), ProgSettingsEvent::setCompression, tipCompression)
		});
	}
	++itxs;

	array<pair<Size, Children>, 11> sec1 = {
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->vsync, ProgSettingsEvent::setVsync, "Vertical synchronization")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->gpuSelecting, ProgSettingsEvent::setGpuSelecting, "Use the graphics process to determine which widget is being selected")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new PushButton(measureText("Portrait", lineHeight), "Portrait", ProgSettingsEvent::setPortrait, ACT_LEFT, "Portrait window size"),
			new PushButton(measureText("Landscape", lineHeight), "Landscape", ProgSettingsEvent::setLandscape, ACT_LEFT, "Landscape window size"),
			new PushButton(measureText("Square", lineHeight), "Square", ProgSettingsEvent::setSquare, ACT_LEFT, "Square window size"),
			new PushButton(measureText("Fill", lineHeight), "Fill", ProgSettingsEvent::setFill, ACT_LEFT, "Fill screen with window")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new ComboBox(1.f, valcp(World::sets()->getTheme()), vector<Cstring>(themes.begin(), themes.end()), ProgSettingsEvent::setTheme, "Color scheme")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->preview, ProgSettingsEvent::setPreview, "Show preview icons of pictures in browser")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			showHidden = new CheckBox(lineHeight, World::sets()->showHidden, ProgSettingsEvent::setHide, "Show hidden files")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new CheckBox(lineHeight, World::sets()->tooltips, ProgSettingsEvent::setTooltips, "Display tooltips on hover")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			fontList = new ComboBox(1.f, 0, { "loading..." }, nullEvent, "Font family"),
			new ComboBox(findMaxLength(Settings::hintingNames.begin(), Settings::hintingNames.end(), lineHeight), eint(World::sets()->hinting), vector<Cstring>(Settings::hintingNames.begin(), Settings::hintingNames.end()), ProgSettingsEvent::setFontHinting, "Font hinting")
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			libraryDir = new LabelEdit(1.f, valcp(World::sets()->dirLib), ProgSettingsEvent::setLibraryDirLe, ACT_LEFT, "Library path"),
			new PushButton(measureText(KeyGetter::ellipsisStr, lineHeight), KeyGetter::ellipsisStr, ProgSettingsEvent::openLibDirBrowser, ACT_LEFT, "Browse for library", Alignment::center)
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, World::sets()->scrollSpeedString(), ProgSettingsEvent::setScrollSpeed, ACT_LEFT, "Scroll speed for button presses or axes", LabelEdit::TextType::sFloatSpaced)
		}),
		pair(lineHeight, Children{
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->getDeadzone(), 0, Settings::axisLimit, ProgSettingsEvent::setDeadzoneSl, ACT_LEFT, tipDeadzone),
			new LabelEdit(unumLen, toStr(World::sets()->getDeadzone()), ProgSettingsEvent::setDeadzoneLe, ACT_LEFT, tipDeadzone, LabelEdit::TextType::uInt)
		})
	};
	lx.insert(lx.end(), std::make_move_iterator(sec1.begin()), std::make_move_iterator(sec1.end()));

	uint lcnt = lx.size();
	Children lns(lcnt + 2 + bnames.size() + 2);
	for (uint i = 0; i < lcnt; ++i)
		lns[i] = new Layout(lx[i].first, std::move(lx[i].second), Direction::right);
	lns[lcnt] = new Widget(0);
	lns[lcnt + 1] = new Layout(lineHeight, { new Widget(descLength), new Label(1.f, "Keyboard", Alignment::center, false), new Label(1.f, "DirectInput", Alignment::center, false), new Label(1.f, "XInput", Alignment::center, false) }, Direction::right);
	limitLine = static_cast<Layout*>(lns[3]);

	// shortcut entries
	for (size_t i = 0; i < bnames.size(); ++i) {
		auto lbl = new Label(descLength, std::move(bnames[i]));
		Children lin {
			lbl,
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, Binding::Type(i), std::format("{} keyboard binding", lbl->getText().data())),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, Binding::Type(i), std::format("{} joystick binding", lbl->getText().data())),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, Binding::Type(i), std::format("{} gamepad binding", lbl->getText().data()))
		};
		lns[lcnt + 2 + i] = new Layout(lineHeight, std::move(lin), Direction::right);
	}
	lcnt += 2 + Binding::names.size();

	// reset button
	lns[lcnt] = new Widget(0);
	lns[lcnt + 1] = new Layout(lineHeight, { new PushButton(measureText("Reset", lineHeight), "Reset", ProgSettingsEvent::reset, ACT_LEFT, "Reset all settings") }, Direction::right);

	// root layout
	Children cont = {
		new Layout(topHeight, std::move(top), Direction::right, topSpacing),
		new ScrollArea(1.f, std::move(lns))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

Widget* ProgSettings::createLimitEdit() {
	switch (World::sets()->picLim.type) {
	using enum PicLim::Type;
	case count:
		return new LabelEdit(1.f, toStr(World::sets()->picLim.getCount()), ProgSettingsEvent::setPicLimitCount, ACT_LEFT, "Number of pictures per batch", LabelEdit::TextType::uInt);
	case size:
		return new LabelEdit(1.f, PicLim::memoryString(World::sets()->picLim.getSize()), ProgSettingsEvent::setPicLimitSize, ACT_LEFT, "Total size of pictures per batch");
	}
	return new Widget();
}

Cstring ProgSettings::makeZoomText() {
	double zoom = Settings::zoomValue(World::sets()->zoom);
	Cstring str = toStr<Cstring>(zoom);
	size_t len = str.length();
	if (zoom >= 1.0) {
		if (size_t i = std::find(str.data(), str.data() + len, '.') - str.data(); i + 4 < len)
			return Cstring(str.data(), i + 4);
	} else if (size_t i = std::find(str.data(), str.data() + len, '.') - str.data(); i < len)
		if (i = std::find_if(str.data() + i + 1, str.data() + len, [](char ch) -> bool { return ch != '0'; }) - str.data(); i < len)
			return Cstring(str.data(), std::min(i + 3, len));
	return str;
}

void ProgSettings::startFonts() {
	stopFonts();
	fontThread = std::jthread(&FileSys::listFontFamiliesThread, World::fileSys()->getDirConfs(), World::sets()->font, ' ', '~');
}

void ProgSettings::stopFonts() {
	if (fontThread.joinable()) {
		fontThread = std::jthread();
		cleanupEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, [](SDL_UserEvent& user) { delete static_cast<FontListResult*>(user.data1); });
	}
}

void ProgSettings::setFontField(vector<Cstring>&& families, uptr<Cstring[]>&& files, size_t select) {
	if (!families.empty()) {
		fontList->setOptions(select, std::move(families), std::move(files));
		fontList->setEvent(ProgSettingsEvent::setFontCmb, ACT_LEFT | ACT_RIGHT);
	} else
		fontList->getParent()->replaceWidget(fontList->getIndex(), new LabelEdit(1.f, valcp(World::sets()->font), ProgSettingsEvent::setFontLe, ACT_LEFT, "Font name or path"));
}

void ProgSettings::startMove() {
	stopMove();
	moveThread = std::jthread(&FileSys::moveContentThread, toPath(oldPathBuffer), toPath(World::sets()->dirLib));
}

void ProgSettings::stopMove() {
	if (moveThread.joinable()) {
		moveThread = std::jthread();
		cleanupEvent(SDL_USEREVENT_THREAD_MOVE, [](SDL_UserEvent& event) {
			if (ThreadEvent(event.code) == ThreadEvent::finished) {
				auto errors = static_cast<string*>(event.data1);
				logMoveErrors(errors);
				delete errors;
			}
		});
	}
}

void ProgSettings::logMoveErrors(const string* errors) {
	for (string::const_iterator pos = errors->begin(); pos != errors->end();) {
		string::const_iterator next = std::find(pos, errors->end(), '\n');
		logError(string_view(pos, next));
		pos = std::find_if(next, errors->end(), [](char c) -> bool { return c != '\n'; });
	}
}

// PROG SEARCH DIR

void ProgSearchDir::eventSpecEscape() {
	World::program()->eventBrowserGoUp();
}

RootLayout* ProgSearchDir::createLayout() {
	// sidebar
	static constexpr std::initializer_list<const char*> txs = {
		"Exit",
		"Up",
		"Set"
	};
	std::initializer_list<const char*>::iterator itxs = txs.begin();
	uint txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	Children bar = {
		new PushButton(lineHeight, *itxs++, ProgFileExplorerEvent::exit),
		new PushButton(lineHeight, *itxs++, ProgFileExplorerEvent::goUp),
		new PushButton(lineHeight, *itxs++, ProgSearchDirEvent::setLibraryDirBw)
	};

	// directory list
	Browser* browser = World::program()->getBrowser();
	vector<string> strs = browser->listDirDirs(browser->getCurDir());
	Children items(strs.size());
	for (size_t i = 0; i < strs.size(); ++i)
		items[i] = makeDirectoryEntry(lineHeight, std::move(strs[i]));
	dirEnd = strs.size();
	fileEnd = dirEnd;

	// main content
	Children mid = {
		new Layout(txsWidth, std::move(bar)),
		fileList = new ScrollArea(1.f, std::move(items))
	};

	// root layout
	Children cont = {
		locationBar = new LabelEdit(lineHeight, valcp(browser->getCurDir()), ProgFileExplorerEvent::goTo),
		new Layout(1.f, std::move(mid), Direction::right, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

PushButton* ProgSearchDir::makeDirectoryEntry(const Size& size, Cstring&& name) {
	return new IconPushButton(size, std::move(name), World::drawSys()->texture("folder"), ProgSearchDirEvent::goIn, ACT_LEFT | ACT_DOUBLE);
}
