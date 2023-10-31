#include "progs.h"
#include "program.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/inputSys.h"
#include "engine/scene.h"
#include "engine/world.h"
#include "utils/compare.h"
#include "utils/layouts.h"

// PROGRAM TEXT

ProgState::Text::Text(string&& str, uint height) :
	text(std::move(str)),
	length(measure(text, height))
{}

uint ProgState::Text::measure(string_view str, uint height) {
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

Popup* ProgState::createPopupMessage(string&& msg, PCall ccal, string ctxt, Alignment malign) {
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
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(std::max(mg.length, ok.length) + Layout::defaultItemSpacing * 2, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

void ProgState::updatePopupMessage(string&& msg) {
	if (Popup* popup = World::scene()->getPopup()) {
		Text mg(std::move(msg), popupLineHeight);
		popup->getWidget<Label>(0)->setText(std::move(mg.text));
		popup->setSize(std::max(mg.length, Text::measure(popup->getWidget<Layout>(1)->getWidget<Label>(1)->getText(), popupLineHeight)) + Layout::defaultItemSpacing * 2);
	}
}

Popup* ProgState::createPopupMultiline(string&& msg, PCall ccal, string ctxt, Alignment malign) {
	Text ok(std::move(ctxt), popupLineHeight);
	ivec2 viewRes = World::drawSys()->getViewRes();
	int width = ok.length, lines = 0;
	for (size_t i = 0; i < msg.length();) {
		++lines;
		size_t end = std::min(msg.find('\n', i), msg.length());
		if (int siz = World::drawSys()->textLength(string_view(msg.data() + i, end - i), lineHeight) + (Label::defaultTextMargin + Layout::defaultItemSpacing) * 2; siz > width)
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
		new Layout(popupLineHeight, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(width, std::min(lineHeight * lines + popupLineHeight + Layout::defaultItemSpacing * 3, viewRes.y)), std::move(con), ccal, first);
}

Popup* ProgState::createPopupChoice(string&& msg, PCall kcal, PCall ccal, Alignment malign) {
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
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(std::max(mg.length, yes.length + no.length) + Layout::defaultItemSpacing * 3, popupLineHeight * 2 + Layout::defaultItemSpacing * 3), std::move(con), ccal, first);
}

Popup* ProgState::createPopupInput(string&& msg, string&& text, PCall kcal, PCall ccal, string&& ktxt, Alignment malign) {
	Text mg(std::move(msg), popupLineHeight);
	Text yes(std::move(ktxt), popupLineHeight);
	Text no("Cancel", popupLineHeight);
	Widget* first;
	vector<Widget*> bot = {
		new Label(1.f, std::move(yes.text), kcal, nullptr, nullptr, nullptr, Alignment::center),
		new Label(1.f, std::move(no.text), ccal, nullptr, nullptr, nullptr, Alignment::center)
	};
	vector<Widget*> con = {
		new Label(1.f, std::move(mg.text), nullptr, nullptr, nullptr, nullptr, malign),
		first = new LabelEdit(1.f, std::move(text), kcal, ccal),
		new Layout(1.f, std::move(bot), Direction::right, 0)
	};
	return new Popup(svec2(0.75f, popupLineHeight * 3 + Layout::defaultItemSpacing * 4), std::move(con), ccal, first);
}

const string& ProgState::inputFromPopup() {
	return World::scene()->getPopup()->getWidget<LabelEdit>(1)->getText();
}

Popup* ProgState::createPopupRemoteLogin(RemoteLocation&& rl, PCall kcal, PCall ccal) {
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
	int descLength = findMaxLength(txs.begin(), txs.end(), popupLineHeight);
	int protocolLength = findMaxLength(protocolNames.begin(), protocolNames.end(), popupLineHeight);
	int portLength = Text::measure("00000", popupLineHeight) + LabelEdit::caretWidth;
	int familyLblLength = Text::measure(txs.end()[-1], popupLineHeight);
	int familyValLength = findMaxLength(familyNames.begin(), familyNames.end(), popupLineHeight);
	int valLength = std::max(protocolLength + Layout::defaultItemSpacing + descLength * 2, portLength + familyLblLength + familyValLength + Layout::defaultItemSpacing * 2);

	Text mg(std::format("{} Login", rl.protocol == Protocol::smb ? "SMB" : "SFTP"), popupLineHeight);
	Text yes("Log In", popupLineHeight);
	Text no("Cancel", popupLineHeight);
	vector<Widget*> user = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.user), nullptr, nullptr, nullptr, makeTooltip("Username"))
	};
	vector<Widget*> password = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.password), nullptr, nullptr, nullptr, makeTooltip("Password"), LabelEdit::TextType::password)
	};
	vector<Widget*> server = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.server), nullptr, nullptr, nullptr, makeTooltip("Server address"))
	};
	vector<Widget*> path = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, std::move(rl.path), nullptr, nullptr, nullptr, makeTooltip("Directory to open0"s + (rl.protocol == Protocol::smb ? " (must start with the share name)" : "")))
	};
	vector<Widget*> port = {
		new Label(descLength, *itxs++),
		new LabelEdit(1.f, toStr(rl.port), nullptr, nullptr, nullptr, makeTooltip("Port"), rl.protocol == Protocol::smb ? LabelEdit::TextType::uInt : LabelEdit::TextType::any)
	};
	if (rl.protocol == Protocol::sftp)
		port.insert(port.end(), {
			new Label(familyLblLength, *++itxs),
			new ComboBox(familyValLength, eint(Family::any), vector<string>(familyNames.begin(), familyNames.end()), &Program::eventConfirmComboBox, makeTooltip("Family for resolving the server address"))
		});
	vector<Widget*> bot = {
		new Label(1.f, std::move(yes.text), kcal, nullptr, nullptr, nullptr, Alignment::center),
		new Label(1.f, std::move(no.text), ccal, nullptr, nullptr, nullptr, Alignment::center)
	};
	Widget* first = rl.user.empty() ? user[1] : password[1];

	vector<Widget*> con {
		new Label(popupLineHeight, std::move(mg.text), nullptr, nullptr, nullptr, nullptr, Alignment::center),
		new Layout(popupLineHeight, std::move(user), Direction::right),
		new Layout(popupLineHeight, std::move(password), Direction::right),
		new Layout(popupLineHeight, std::move(server), Direction::right),
		new Layout(popupLineHeight, std::move(path), Direction::right),
		new Layout(popupLineHeight, std::move(port), Direction::right),
		new Layout(popupLineHeight, std::move(bot), Direction::right),
	};
	if (rl.protocol == Protocol::smb) {
		vector<Widget*> workgroup = {
			new Label(descLength, *itxs),
			new LabelEdit(1.f, std::move(rl.workgroup), nullptr, nullptr, nullptr, makeTooltip("Workgroup"))
		};
		con.insert(con.begin() + 2, new Layout(popupLineHeight, std::move(workgroup), Direction::right));
	}
	++itxs;
	if (World::program()->canStoreCredentials()) {
		vector<Widget*> saveCredentials {
			new Label(descLength, *itxs),
			new CheckBox(popupLineHeight, false, nullptr, nullptr, nullptr, makeTooltip("Remember credentials"))
		};
		con.insert(con.end() - 1, new Layout(popupLineHeight, std::move(saveCredentials), Direction::right));
	}
	uint numLines = con.size();
	return new Popup(svec2(std::max(std::max(mg.length, yes.length + no.length), uint(descLength + valLength)) + Layout::defaultItemSpacing * 3, popupLineHeight * numLines + Layout::defaultItemSpacing * (numLines + 1)), std::move(con), ccal, first);
}

pair<RemoteLocation, bool> ProgState::remoteLocationFromPopup() {
	const vector<Widget*>& con = World::scene()->getPopup()->getWidgets();
	uint offs = con.size() == uint(8 + World::program()->canStoreCredentials());
	const vector<Widget*>& portfam = static_cast<Layout*>(con[5 + offs])->getWidgets();
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

Context* ProgState::createContext(vector<pair<string, PCall>>&& items, Widget* parent) {
	vector<Widget*> wgts(items.size());
	for (size_t i = 0; i < items.size(); ++i)
		wgts[i] = new Label(lineHeight, std::move(items[i].first), items[i].second, &Program::eventCloseContext);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, World::winSys()->mousePos(), ivec2(0, lineHeight), contextMargin);
	return new Context(rect.pos(), rect.size(), vector<Widget*>{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, ScrollArea::Select::none, 0) }, first, parent, Color::dark, nullptr, Layout::defaultDirection, contextMargin);
}

Context* ProgState::createComboContext(ComboBox* parent, PCall kcal) {
	ivec2 size = parent->size();
	vector<Widget*> wgts(parent->getOptions().size());
	if (parent->getTooltips()) {
		for (size_t i = 0; i < wgts.size(); ++i)
			wgts[i] = new Label(size.y, valcp(parent->getOptions()[i]), kcal, &Program::eventCloseContext, nullptr, makeTooltip(parent->getTooltips()[i]));
	} else
		for (size_t i = 0; i < wgts.size(); ++i)
			wgts[i] = new Label(size.y, valcp(parent->getOptions()[i]), kcal, &Program::eventCloseContext);

	Widget* first = wgts[0];
	Recti rect = calcTextContextRect(wgts, parent->position(), size, contextMargin);
	return new Context(rect.pos(), rect.size(), vector<Widget*>{ new ScrollArea(1.f, std::move(wgts), Layout::defaultDirection, ScrollArea::Select::none, 0) }, first, parent, Color::dark, &Program::eventResizeComboContext, Layout::defaultDirection, contextMargin);
}

Recti ProgState::calcTextContextRect(const vector<Widget*>& items, ivec2 pos, ivec2 size, int margin) {
	for (Widget* it : items)
		if (Label* lbl = dynamic_cast<Label*>(it))
			if (int w = World::drawSys()->textLength(lbl->getText(), size.y) + lbl->getTextMargin() * 2 + Scrollable::barSizeVal + margin * 2; w > size.x)
				size.x = w;
	size.y = size.y * uint(items.size()) + margin * 2;

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
int ProgState::findMaxLength(T pos, T end, int height) {
	int width = 0;
	for (; pos != end; ++pos)
		if (int len = World::drawSys()->textLength(*pos, height) + Label::defaultTextMargin * 2; len > width)
			width = len;
	return width;
}

Texture* ProgState::makeTooltip(string_view str) {
	return World::sets()->tooltips ? World::drawSys()->renderText(str, tooltipHeight, maxTooltipLength) : nullptr;
}

Texture* ProgState::makeTooltipL(string_view str) {
	if (!World::sets()->tooltips)
		return nullptr;

	uint width = 0;
	for (string_view::iterator pos = str.begin(); pos != str.end();) {
		string_view::iterator brk = std::find(pos, str.end(), '\n');
		if (uint siz = World::drawSys()->textLength(string_view(pos, brk), tooltipHeight) + Label::tooltipMargin.x * 2; siz > width)
			if (width = std::min(siz, maxTooltipLength); width == maxTooltipLength)
				break;
		pos = brk + (brk != str.end());
	}
	return World::drawSys()->renderText(str, tooltipHeight, width);
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
	optional<vector<FileChange>> files = browser->directoryUpdate();
	if (!files) {
		fileList->setWidgets(vector<Widget*>());
		if (locationBar)
			locationBar->setText(browser->currentLocation());
		return;
	}

	auto compare = [](const Widget* a, const string& b) -> bool { return StrNatCmp::less(static_cast<const Label*>(a)->getText(), b); };
	const vector<Widget*>& wgts = fileList->getWidgets();
	for (FileChange& fc : *files) {
		if (fc.type == FileChange::deleteEntry) {
			vector<Widget*>::const_iterator sp = wgts.begin() + dirEnd;
			vector<Widget*>::const_iterator it = std::lower_bound(wgts.begin(), sp, fc.name, compare);
			bool directory = it != sp && static_cast<const Label*>(*it)->getText() == fc.name;
			if (!directory) {
				vector<Widget*>::const_iterator ef = wgts.begin() + fileEnd;
				if (it = std::lower_bound(sp, ef, fc.name, compare); it == ef || static_cast<const Label*>(*it)->getText() != fc.name)
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
			} else if (Label* flbl = makeFileEntry(size, std::move(fc.name))) {
				fileList->insertWidget(id, flbl);
				++fileEnd;
			}
		}
	}
}

Label* ProgFileExplorer::makeFileEntry(const Size&, string&&) {
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
	Text settings("Settings", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
		new Label(settings.length, std::move(settings.text), &Program::eventOpenSettings),
		new Label(exit.length, std::move(exit.text), &Program::eventExit)
	};

	// book list
	Browser* browser = World::program()->getBrowser();
	vector<string> books = browser->listDirDirs(World::sets()->dirLib);
	vector<Widget*> tiles(books.size() + 1);
	for (size_t i = 0; i < books.size(); ++i)
		tiles[i] = makeBookTile(std::move(books[i]));
	tiles.back() = new Button(TileBox::defaultItemHeight, &Program::eventOpenPageBrowserGeneral, &Program::eventOpenBookContextGeneral, nullptr, makeTooltip("Browse other directories"), true, World::drawSys()->texture("search"));
	dirEnd = books.size();
	fileEnd = dirEnd;

	// root layout
	vector<Widget*> cont = {
		new Layout(topHeight, std::move(top), Direction::right, topSpacing),
		fileList = new TileBox(1.f, std::move(tiles))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

Label* ProgBooks::makeBookTile(string&& name) {
	Text txt(std::move(name), TileBox::defaultItemHeight);
	return makeDirectoryEntry(txt.length, std::move(txt.text));
}

Label* ProgBooks::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), &Program::eventOpenPageBrowser, &Program::eventOpenBookContext);
}

Size ProgBooks::fileEntrySize(string_view name) {
	return Text::measure(name, TileBox::defaultItemHeight);
}

// PROG PAGE BROWSER

void ProgPageBrowser::eventSpecEscape() {
	World::program()->eventBrowserGoUp();
}

void ProgPageBrowser::eventFileDrop(const char* file) {
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
	static constexpr std::initializer_list<const char*> txs = {
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
	Browser* browser = World::program()->getBrowser();
	auto [files, dirs] = browser->listCurDir();
	if (World::sets()->preview)
		browser->startPreview(files, dirs, lineHeight);
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
	string location = browser->currentLocation();
	string_view rpath = relativePath(location, World::sets()->dirLib);
	vector<Widget*> cont = {
		locationBar = new LabelEdit(lineHeight, rpath.empty() ? std::move(location) : string(rpath), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

Label* ProgPageBrowser::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), &Program::eventBrowserGoIn, nullptr, nullptr, nullptr, Alignment::left, World::drawSys()->texture("folder"));
}

Label* ProgPageBrowser::makeFileEntry(const Size&, string&& name) {
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
	vector<Widget*> cont = { reader = new ReaderBox(1.f, World::sets()->direction, World::sets()->zoom, World::sets()->spacing) };
	return new RootLayout(1.f, std::move(cont), Direction::right, 0);
}

Overlay* ProgReader::createOverlay() {
	vector<Widget*> menu = {
		new Button(picSize, &Program::eventExitReader, nullptr, nullptr, makeTooltipWithKey("Exit", Binding::Type::escape), false, World::drawSys()->texture("cross"), picMargin),
		new Button(picSize, &Program::eventNextDir, nullptr, nullptr, makeTooltipWithKey("Next", Binding::Type::nextDir), false, World::drawSys()->texture("right"), picMargin),
		new Button(picSize, &Program::eventPrevDir, nullptr, nullptr, makeTooltipWithKey("Previous", Binding::Type::prevDir), false, World::drawSys()->texture("left"), picMargin),
		new Button(picSize, &Program::eventZoomIn, nullptr, nullptr, makeTooltipWithKey("Zoom in", Binding::Type::zoomIn), false, World::drawSys()->texture("plus"), picMargin),
		new Button(picSize, &Program::eventZoomOut, nullptr, nullptr, makeTooltipWithKey("Zoom out", Binding::Type::zoomOut), false, World::drawSys()->texture("minus"), picMargin),
		new Button(picSize, &Program::eventZoomReset, nullptr, nullptr, makeTooltipWithKey("Zoom reset", Binding::Type::zoomReset), false, World::drawSys()->texture("reset"), picMargin),
		new Button(picSize, &Program::eventCenterView, nullptr, nullptr, makeTooltipWithKey("Center", Binding::Type::centerView), false, World::drawSys()->texture("center"), picMargin)
	};
	int ysiz = picSize * menu.size();
	return new Overlay(svec2(0), svec2(picSize, ysiz), svec2(0), svec2(picSize/2, ysiz), std::move(menu), Color::normal, Direction::down, 0);
}

Texture* ProgReader::makeTooltipWithKey(const char* text, Binding::Type type) {
	string btext = World::inputSys()->getBoundName(type);
	return makeTooltip(!btext.empty() ? std::format("{} ({})", text, btext) : text);
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

void ProgSettings::eventRefresh() {
	float loc = static_cast<ScrollArea*>(limitLine->getParent())->getScrollLocation();
	World::scene()->resetLayouts();
	static_cast<ScrollArea*>(limitLine->getParent())->setScrollLocation(loc);
	World::scene()->updateSelect();
}

void ProgSettings::eventFileDrop(const char* file) {
	if (fs::path path = toPath(file);  FileSys::isFont(path)) {
		World::drawSys()->setFont(file);
		eventRefresh();
	} else
		World::program()->setLibraryDir(file);
}

RootLayout* ProgSettings::createLayout() {
	// top bar
	Text books("Library", topHeight);
	Text exit("Exit", topHeight);
	vector<Widget*> top = {
		new Label(books.length, std::move(books.text), &Program::eventOpenBookList),
		new Label(exit.length, std::move(exit.text), &Program::eventExit)
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
	vector<string> dnames(devices.size());
	rng::transform(devices, dnames.begin(), [](pair<u32vec2, string>& it) -> string { return std::move(it.second); });
	vector<pair<u32vec2, string>>::iterator curDev = rng::find_if(devices, [](const pair<u32vec2, string>& it) -> bool { return World::sets()->device == it.first; });

	array<string, Binding::names.size()> bnames;
	for (uint8 i = 0; i < Binding::names.size(); ++i) {
		bnames[i] = Binding::names[i];
		bnames[i][0] = toupper(bnames[i][0]);
		std::replace(bnames[i].begin() + 1, bnames[i].end(), '_', ' ');
	}
	int plimLength = findMaxLength(PicLim::names.begin(), PicLim::names.end(), lineHeight);
	int descLength = std::max(findMaxLength(txs.begin(), txs.end(), lineHeight), findMaxLength(Binding::names.begin(), Binding::names.end(), lineHeight));
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
		const Layout* box = static_cast<const Layout*>(wgt);
		return box->getWidget<WindowArranger>(1)->precalcSizeExpand(box->getParent()->size().x - box->getWidget(0)->getRelSize().pix - box->getSpacing() - Scrollable::barSizeVal);
	});

	// action fields for labels
	vector<string> themes = World::fileSys()->getAvailableThemes();
	Text dots(KeyGetter::ellipsisStr, lineHeight);
	int unumLen = Text::measure("0000000000", lineHeight) + LabelEdit::caretWidth;
	startFonts();

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
			new ComboBox(plimLength, eint(World::sets()->picLim.type), vector<string>(PicLim::names.begin(), PicLim::names.end()), &Program::eventSetPicLimitType, makeTooltipL(tipPicLim)),
			createLimitEdit()
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->maxPicRes, Settings::minPicRes, maxRes, &Program::eventSetMaxPicResSL, nullptr, nullptr, makeTooltip(tipMaxPicRes)),
			new LabelEdit(unumLen, toStr(World::sets()->maxPicRes), &Program::eventSetMaxPicResLE, nullptr, nullptr, makeTooltip(tipMaxPicRes))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			screen = new ComboBox(1.f, eint(World::sets()->screen), vector<string>(Settings::screenModeNames.begin(), Settings::screenModeNames.end()), &Program::eventSetScreenMode, makeTooltip("Window screen mode"))
		} },
		{ monitorSize, {
			new Widget(descLength),
			new WindowArranger(1.f, float(lineHeight * 2) / 1080.f, true, &Program::eventSetMultiFullscreen, &Program::eventSetMultiFullscreen, makeTooltip("Monitor arrangement for multi fullscreen"))
		} }
	});

	if constexpr (Settings::rendererNames.size() > 1) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new ComboBox(1.f, eint(World::sets()->renderer), vector<string>(Settings::rendererNames.begin(), Settings::rendererNames.end()), &Program::eventSetRenderer, makeTooltip("Rendering backend"))
		} });
	}
	++itxs;
	if (!devices.empty()) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new ComboBox(1.f, curDev != devices.end() ? curDev - devices.begin() : 0, std::move(dnames), &Program::eventSetDevice, makeTooltip("Rendering devices"))
		} });
	}
	++itxs;
	if (maxCompress > Settings::Compression::none) {
		lx.push_back({ lineHeight, {
			new Label(descLength, *itxs),
			new ComboBox(1.f, eint(World::sets()->compression), vector<string>(Settings::compressionNames.begin(), Settings::compressionNames.begin() + eint(maxCompress) + 1), &Program::eventSetCompression, makeTooltipL(tipCompression))
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
			new ComboBox(1.f, valcp(World::sets()->getTheme()), std::move(themes), &Program::eventSetTheme, makeTooltip("Color scheme"))
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
			fontList = new ComboBox(1.f, 0, { "loading..." }, nullptr, makeTooltip("Font family")),
			new ComboBox(findMaxLength(Settings::hintingNames.begin(), Settings::hintingNames.end(), lineHeight), eint(World::sets()->hinting), vector<string>(Settings::hintingNames.begin(), Settings::hintingNames.end()), &Program::eventSetFontHinting, makeTooltip("Font hinting"))
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			libraryDir = new LabelEdit(1.f, valcp(World::sets()->dirLib), &Program::eventSetLibraryDirLE, nullptr, nullptr, makeTooltip("Library path")),
			new Label(dots.length, std::move(dots.text), &Program::eventOpenLibDirBrowser, nullptr, nullptr, makeTooltip("Browse for library"), Alignment::center)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new LabelEdit(1.f, World::sets()->scrollSpeedString(), &Program::eventSetScrollSpeed, nullptr, nullptr, makeTooltip("Scroll speed for button presses or axes"), LabelEdit::TextType::sFloatSpaced)
		} },
		{ lineHeight, {
			new Label(descLength, *itxs++),
			new Slider(1.f, World::sets()->getDeadzone(), 0, Settings::axisLimit, &Program::eventSetDeadzoneSL, nullptr, nullptr, makeTooltip(tipDeadzone)),
			new LabelEdit(unumLen, toStr(World::sets()->getDeadzone()), &Program::eventSetDeadzoneLE, nullptr, nullptr, makeTooltip(tipDeadzone), LabelEdit::TextType::uInt)
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
			new KeyGetter(1.f, KeyGetter::AcceptType::keyboard, Binding::Type(i), makeTooltip(std::format("{} keyboard binding", lbl->getText()))),
			new KeyGetter(1.f, KeyGetter::AcceptType::joystick, Binding::Type(i), makeTooltip(std::format("{} joystick binding", lbl->getText()))),
			new KeyGetter(1.f, KeyGetter::AcceptType::gamepad, Binding::Type(i), makeTooltip(std::format("{} gamepad binding", lbl->getText())))
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
		new Layout(topHeight, std::move(top), Direction::right, topSpacing),
		new ScrollArea(1.f, std::move(lns))
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

Widget* ProgSettings::createLimitEdit() {
	switch (World::sets()->picLim.type) {
	using enum PicLim::Type;
	case count:
		return new LabelEdit(1.f, toStr(World::sets()->picLim.getCount()), &Program::eventSetPicLimCount, nullptr, nullptr, makeTooltip("Number of pictures per batch"), LabelEdit::TextType::uInt);
	case size:
		return new LabelEdit(1.f, PicLim::memoryString(World::sets()->picLim.getSize()), &Program::eventSetPicLimSize, nullptr, nullptr, makeTooltip("Total size of pictures per batch"));
	}
	return new Widget();
}

void ProgSettings::startFonts() {
	stopFonts();
	fontThread = std::jthread(&FileSys::listFontFamiliesThread, World::fileSys()->getDirConfs(), World::sets()->font, ' ', '~');
}

void ProgSettings::stopFonts() {
	if (fontThread.joinable()) {
		fontThread = std::jthread();
		cleanupEvent(SDL_USEREVENT_FONTS_FINISHED, [](SDL_UserEvent& user) { delete static_cast<FontListResult*>(user.data1); });
	}
}

void ProgSettings::setFontField(vector<string>&& families, uptr<string[]>&& files, size_t select) {
	if (!families.empty()) {
		fontList->setOptions(select, std::move(families), std::move(files));
		fontList->setCalls(&Program::eventSetFontCMB, &Program::eventSetFontCMB, std::nullopt);
	} else
		fontList->getParent()->replaceWidget(fontList->getIndex(), new LabelEdit(1.f, valcp(World::sets()->font), &Program::eventSetFontLE, nullptr, nullptr, makeTooltip("Font name or path")));
}

void ProgSettings::startMove() {
	stopMove();
	moveThread = std::jthread(&FileSys::moveContentThread, toPath(oldPathBuffer), toPath(World::sets()->dirLib));
}

void ProgSettings::stopMove() {
	if (moveThread.joinable()) {
		moveThread = std::jthread();
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
	int txsWidth = findMaxLength(txs.begin(), txs.end(), lineHeight);
	vector<Widget*> bar = {
		new Label(lineHeight, *itxs++, &Program::eventExitBrowser),
		new Label(lineHeight, *itxs++, &Program::eventBrowserGoUp),
		new Label(lineHeight, *itxs++, &Program::eventSetLibraryDirBW)
	};

	// directory list
	Browser* browser = World::program()->getBrowser();
	vector<string> strs = browser->listDirDirs(browser->getCurDir());
	vector<Widget*> items(strs.size());
	for (size_t i = 0; i < strs.size(); ++i)
		items[i] = makeDirectoryEntry(lineHeight, std::move(strs[i]));
	dirEnd = strs.size();
	fileEnd = dirEnd;

	// main content
	vector<Widget*> mid = {
		new Layout(txsWidth, std::move(bar)),
		fileList = new ScrollArea(1.f, std::move(items), Direction::down, ScrollArea::Select::one)
	};

	// root layout
	vector<Widget*> cont = {
		locationBar = new LabelEdit(lineHeight, valcp(browser->getCurDir()), &Program::eventBrowserGoTo),
		new Layout(1.f, std::move(mid), Direction::right, topSpacing)
	};
	return new RootLayout(1.f, std::move(cont), Direction::down, topSpacing);
}

Label* ProgSearchDir::makeDirectoryEntry(const Size& size, string&& name) {
	return new Label(size, std::move(name), nullptr, nullptr, &Program::eventBrowserGoIn, nullptr, Alignment::left, World::drawSys()->texture("folder"));
}
