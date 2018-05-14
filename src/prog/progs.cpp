#include "engine/world.h"

// PROGRAM

const int ProgState::topHeight = 40;
const int ProgState::topSpacing = 10;
const vec2i ProgState::optSize(120, 30);
const int ProgState::itemHeight = 30;
const int ProgState::sideWidth = 120;
const int ProgState::picSize = 40;
const int ProgState::picSpaer = 10;
const int ProgState::setsDescLength = 200;
const vec2s ProgState::messageSize(300, 100);
const vec2s ProgState::inputSize(300, 150);

void ProgState::eventScreenMode() {
	World::winSys()->setFullscreen(!World::winSys()->sets.fullscreen);
}

Popup* ProgState::createPopupMessage(const string& msg, const vec2<Size>& size) {
	vector<Widget*> bot = {
		new Widget(1.f),
		new Label(Button(1.f, &Program::eventClosePopup), "Ok", Label::Alignment::center),
		new Widget(1.f)
	};
	vector<Widget*> con = {
		new Label(Button(), msg),
		new Layout(1.f, bot, false, Layout::Selection::none, 0)
	};
	return new Popup(size, con);
}

Popup* ProgState::createPopupChoice(const string& msg, void (Program::*call)(Button*), const vec2<Size>& size) {
	vector<Widget*> bot = {
		new Label(Button(1.f, &Program::eventClosePopup), "No", Label::Alignment::center),
		new Label(Button(1.f, call), "Yes", Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(Button(), msg),
		new Layout(1.f, bot, false)
	};
	return new Popup(size, con);
}

pair<Popup*, LineEdit*> ProgState::createPopupTextInput(const string& msg, const string& txt, void (Program::*call)(Button*), LineEdit::TextType type, const vec2<Size>& size) {
	LineEdit* field = new LineEdit(Button(1.f, call), txt, type);
	vector<Widget*> bot = {
		new Label(Button(1.f, &Program::eventClosePopup), "Cancel", Label::Alignment::center),
		new Label(Button(1.f, call), "Ok", Label::Alignment::center)
	};
	vector<Widget*> con = {
		new Label(Button(), msg),
		field,
		new Layout(1.f, bot, false)
	};
	return make_pair(new Popup(size, con), field);
}

// PROG BOOKS

void ProgBooks::eventBack() {
	World::program()->eventOpenPlaylistList();
}

Layout* ProgBooks::createLayout() {
	vector<Widget*> top = {
		new Label(Button(1.f, &Program::eventOpenPlaylistList), World::drawSys()->translation("playlists")),
		new Label(Button(1.f, &Program::eventOpenSettings), World::drawSys()->translation("settings")),
		new Label(Button(1.f, &Program::eventExit), World::drawSys()->translation("exit"))
	};

	vector<Widget*> tiles;
	for (string& it : Filer::listDir(World::winSys()->sets.getDirLib(), FTYPE_DIR))
		tiles.push_back(new Label(Button(World::drawSys()->textLength(it, Default::itemHeight) + Default::textOffset*2, &Program::eventOpenBrowser, &Program::eventOpenLastPage), it));

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, Layout::Selection::none, topSpacing),
		new TileBox(1.f, tiles)
	};
	return new Layout(1.f, cont, true, Layout::Selection::none, topSpacing);
}

// PROG BROWSER

void ProgBrowser::eventBack() {
	World::program()->eventBrowserGoUp();
}

Layout* ProgBrowser::createLayout() {
	vector<Widget*> bar = {
		new Label(Button(itemHeight, &Program::eventBrowserGoUp), World::drawSys()->translation("back"))
	};

	vector<Widget*> items;
	for (string& it : World::program()->getBrowser()->listDirs())
		items.push_back(new Label(Button(itemHeight, &Program::eventBrowserGoIn), it));
	for (string& it : World::program()->getBrowser()->listFiles())
		items.push_back(new Label(Button(itemHeight, &Program::eventOpenReader), it));

	vector<Widget*> cont = {
		new Layout(sideWidth, bar),
		new ScrollArea(1.f, items)
	};
	return new Layout(1.f, cont, false, Layout::Selection::none, topSpacing);
}

// PROG READER

void ProgReader::eventBack() {
	World::program()->eventExitReader();
}

void ProgReader::eventUp(float amt) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->onScroll(vec2i(0, -modifySpeed(amt * World::winSys()->sets.scrollSpeed.y)));
}

void ProgReader::eventDown(float amt) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->onScroll(vec2i(0, modifySpeed(amt * World::winSys()->sets.scrollSpeed.y)));
}

void ProgReader::eventRight(float amt) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->onScroll(vec2i(-modifySpeed(amt * World::winSys()->sets.scrollSpeed.x), 0));
}

void ProgReader::eventLeft(float amt) {
	static_cast<ReaderBox*>(World::scene()->getLayout())->onScroll(vec2i(modifySpeed(amt * World::winSys()->sets.scrollSpeed.x), 0));
}

void ProgReader::eventPageUp() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	sizt i = reader->visibleWidgets().l;
	reader->onScroll(vec2i(0, reader->wgtPosition((i == 0) ? i : i-1).y));
}

void ProgReader::eventPageDown() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	sizt i = reader->visibleWidgets().u;
	reader->onScroll(vec2i(reader->wgtPosition((i == reader->getWidgets().size()) ? i-1 : i).y));
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

void ProgReader::eventPlayPause() {
	World::program()->eventPlayPause();
}

void ProgReader::eventNextSong() {
	World::program()->eventNextSong();
}

void ProgReader::eventPrevSong() {
	World::program()->eventPrevSong();
}

void ProgReader::eventVolumeUp() {
	World::program()->eventVolumeUp();
}

void ProgReader::eventVolumeDown() {
	World::program()->eventVolumeDown();
}

void ProgReader::eventMute() {
	World::program()->eventMute();
}

void ProgReader::eventClosing() {
	ReaderBox* reader = static_cast<ReaderBox*>(World::scene()->getLayout());
	vec2t vis = reader->visibleWidgets();
	if (vis.l < vis.u)
		Filer::saveLastPage(static_cast<Picture*>(reader->getWidget(vis.l))->getTex().getFile());
}

Layout* ProgReader::createLayout() {
	vector<Widget*> pics;
	for (string& it : Filer::listDir(World::program()->getBrowser()->getCurDir(), FTYPE_FILE)) {
		Texture tex(World::program()->getBrowser()->getCurDir() + it);
		if (tex.tex)
			pics.push_back(new Picture(Button(tex.getRes().y), {tex.getFile()}));
	}
	return new ReaderBox(1.f, pics);
}

vector<Overlay*> ProgReader::createOverlays() {
	vector<Widget*> menu = {
		new Picture(Button(picSize, &Program::eventExitReader), {Filer::dirTexs + "back.png"}),
		new Picture(Button(picSize, &Program::eventNextDir), {Filer::dirTexs + "next_dir.png"}),
		new Picture(Button(picSize, &Program::eventPrevDir), {Filer::dirTexs + "prev_dir.png"}),
		new Picture(Button(picSize, &Program::eventZoomIn), {Filer::dirTexs + "zoom_in.png"}),
		new Picture(Button(picSize, &Program::eventZoomOut), {Filer::dirTexs + "zoom_out.png"}),
		new Picture(Button(picSize, &Program::eventZoomReset), {Filer::dirTexs + "zoom_reset.png"}),
		new Picture(Button(picSize, &Program::eventCenterView), {Filer::dirTexs + "center.png"})
	};
	if (World::program()->getPlayer()) {
		vector<Widget*> player = {
			new Widget(picSpaer),
			new Picture(Button(picSize, &Program::eventPlayPause), {Filer::dirTexs + "play.png", Filer::dirTexs + "pause.png"}),
			new Picture(Button(picSize, &Program::eventNextSong), {Filer::dirTexs + "next_song.png"}),
			new Picture(Button(picSize, &Program::eventPrevSong), {Filer::dirTexs + "prev_song.png"}),
			new Picture(Button(picSize, &Program::eventVolumeUp), {Filer::dirTexs + "vol_up.png"}),
			new Picture(Button(picSize, &Program::eventVolumeDown), {Filer::dirTexs + "vol_down.png"}),
			new Picture(Button(picSize, &Program::eventMute), {Filer::dirTexs + "mute.png"})
		};
		menu.insert(menu.end(), player.begin(), player.end());
	}
	return {new Overlay(vec2s(0), vec2s(picSize, 1.f), vec2s(0), vec2s(picSize/2, 1.f), menu, true, Layout::Selection::none, 0)};
}

float ProgReader::modifySpeed(float value) {
	float factor = 1.f;
	if (World::inputSys()->isPressed(Binding::Type::fast, factor))
		value *= Default::scrollFactor * factor;
	else if (World::inputSys()->isPressed(Binding::Type::slow, factor))
		value /= Default::scrollFactor * factor;
	return value / static_cast<ReaderBox*>(World::scene()->getLayout())->zoom * World::winSys()->getDSec();
}

// PROG PLAYLISTS

void ProgPlaylists::eventBack() {
	World::program()->eventOpenBookList();
}

Layout* ProgPlaylists::createLayout() {
	vector<Widget*> top = {
		new Label(Button(1.f, &Program::eventOpenBookList), World::drawSys()->translation("library")),
		new Label(Button(1.f, &Program::eventOpenSettings), World::drawSys()->translation("settings")),
		new Label(Button(1.f, &Program::eventExit), World::drawSys()->translation("exit"))
	};
	vector<Widget*> ops = {
		new Label(Button(optSize.x, &Program::eventAddPlaylist), World::drawSys()->translation("new")),
		new Label(Button(optSize.x, &Program::eventEditPlaylist), World::drawSys()->translation("edit")),
		new Label(Button(optSize.x, &Program::eventRenamePlaylist), World::drawSys()->translation("rename")),
		new Label(Button(optSize.x, &Program::eventDeletePlaylist), World::drawSys()->translation("delete"))
	};

	vector<Widget*> tiles;
	for (string& it : Filer::listDir(World::winSys()->sets.getDirPlist(), FTYPE_FILE))
		tiles.push_back(new Label(Button(World::drawSys()->textLength(it, Default::itemHeight), nullptr, nullptr, &Program::eventEditPlaylist), delExt(it)));

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, Layout::Selection::none, topSpacing),
		new Layout(optSize.y, ops, false),
		new TileBox(1.f, tiles, Layout::Selection::one)
	};
	return new Layout(1.f, cont, true, Layout::Selection::none, topSpacing);
}

// PROG EDITOR

void ProgEditor::eventBack() {
	World::program()->eventExitPlaylistEditor();
}

void ProgEditor::eventFileDrop(char* file) {
	if (World::program()->getEditor()->showSongs) {
		World::program()->getEditor()->addSong(file);
		World::scene()->resetLayouts();
	} else if (Filer::fileType(file) == FTYPE_DIR && parentPath(file) == appendDsep(World::winSys()->sets.getDirLib())) {
		World::program()->getEditor()->addBook(filename(file));
		World::scene()->resetLayouts();
	}
}

Layout* ProgEditor::createLayout() {
	vector<Widget*> bar = {
		new Label(Button(itemHeight, &Program::eventSwitchSB), World::drawSys()->translation(World::program()->getEditor()->showSongs ? "books" : "songs")),
		new Label(Button(itemHeight, &Program::eventBrowseSB), World::drawSys()->translation("browse")),
		new Label(Button(itemHeight, &Program::eventAddSB), World::drawSys()->translation("add")),
		new Label(Button(itemHeight, &Program::eventEditSB), World::drawSys()->translation("edit")),
		new Label(Button(itemHeight, &Program::eventDeleteSB), World::drawSys()->translation("delete")),
		new Label(Button(itemHeight, &Program::eventSavePlaylist), World::drawSys()->translation("save")),
		new Label(Button(itemHeight, &Program::eventExitPlaylistEditor), World::drawSys()->translation("close"))
	};

	vector<Widget*> items;
	if (World::program()->getEditor()->showSongs)
		for (const string& it : World::program()->getEditor()->getPlaylist().songs)
			items.push_back(new Label(Button(itemHeight, nullptr, nullptr, &Program::eventEditSB), it));
	else
		for (const string& it : World::program()->getEditor()->getPlaylist().books)
			items.push_back(new Label(Button(itemHeight, nullptr, nullptr, &Program::eventEditSB), it));

	vector<Widget*> cont = {
		new Layout(sideWidth, bar),
		new ScrollArea(1.f, items, Layout::Selection::any)
	};
	return new Layout(1.f, cont, false, Layout::Selection::none, topSpacing);
}

// PROG SEARCH SONGS

void ProgSearchSongs::eventBack() {
	World::program()->eventSongBrowserGoUp();
}

Layout* ProgSearchSongs::createLayout() {
	vector<Widget*> bar = {
		new Label(Button(itemHeight, &Program::eventAddSongFD), World::drawSys()->translation("add")),
		new Label(Button(itemHeight, &Program::eventSongBrowserGoUp), World::drawSys()->translation("up")),
		new Label(Button(itemHeight, &Program::eventExitSongBrowser), World::drawSys()->translation("back"))
	};

	vector<Widget*> items;
	for (string& it : World::program()->getBrowser()->listDirs())
		items.push_back(new Label(Button(itemHeight, nullptr, nullptr, &Program::eventBrowserGoIn), it));
	for (string& it : World::program()->getBrowser()->listFiles())
		items.push_back(new Label(Button(itemHeight, nullptr, nullptr, &Program::eventAddSongFD), it));

	vector<Widget*> cont = {
		new Layout(sideWidth, bar),
		new ScrollArea(1.f, items, Layout::Selection::any)
	};
	return new Layout(1.f, cont, false, Layout::Selection::none, topSpacing);
}

// PROG SEARCH BOOKS

void ProgSearchBooks::eventBack() {
	World::program()->eventExitBookBrowser();
}

Layout* ProgSearchBooks::createLayout() {
	vector<Widget*> bar = {
		new Label(Button(itemHeight, &Program::eventExitBookBrowser), World::drawSys()->translation("back"))
	};

	vector<Widget*> items;
	for (string& it : Filer::listDir(World::winSys()->sets.getDirLib(), FTYPE_DIR))
		items.push_back(new Label(Button(itemHeight, &Program::eventAddBook), it));

	vector<Widget*> cont {
		new Layout(sideWidth, bar),
		new ScrollArea(1.f, items)
	};
	return new Layout(1.f, cont, false, Layout::Selection::none, topSpacing);
}

// PROG SETTINGS

void ProgSettings::eventBack() {
	World::program()->eventOpenBookList();
}

Layout* ProgSettings::createLayout() {
	vector<Widget*> top = {
		new Label(Button(1.f, &Program::eventOpenBookList), World::drawSys()->translation("library")),
		new Label(Button(1.f, &Program::eventOpenPlaylistList), World::drawSys()->translation("playlists")),
		new Label(Button(1.f, &Program::eventExit), World::drawSys()->translation("exit"))
	};

	vector<Widget*> lx[] = { {
		new Label(Button(setsDescLength), World::drawSys()->translation("fullscreen")),
		new CheckBox(Button(itemHeight, &Program::eventSwitchFullscreen), World::winSys()->sets.fullscreen)
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("theme")),
		new SwitchBox(Button(1.f, &Program::eventSetTheme, &Program::eventSetTheme), Filer::getAvailibleThemes(), World::winSys()->sets.getTheme())
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("language")),
		new SwitchBox(Button(1.f, &Program::eventSwitchLanguage, &Program::eventSwitchLanguage), Filer::getAvailibleLanguages(), World::winSys()->sets.getLang())
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("font")),
		new LineEdit(Button(1.f, &Program::eventSetFont), World::winSys()->sets.getFont())
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("library")),
		new LineEdit(Button(1.f, &Program::eventSetLibraryPath), World::winSys()->sets.getDirLib())
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("playlist")),
		new LineEdit(Button(1.f, &Program::eventSetPlaylistsPath), World::winSys()->sets.getDirPlist())
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("volume")),
		new Slider(Button(1.f, &Program::eventSetVolumeSL), World::winSys()->sets.getVolume(), 0, MIX_MAX_VOLUME),
		new LineEdit(Button(100, &Program::eventSetVolumeLE), ntos(World::winSys()->sets.getVolume()))
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("renderer")),
		new SwitchBox(Button(1.f, &Program::eventSetRenderer, &Program::eventSetRenderer), Settings::getAvailibleRenderers(), World::winSys()->sets.renderer)
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("scroll speed")),
		new LineEdit(Button(1.f, &Program::eventSetScrollSpeed), World::winSys()->sets.getScrollSpeedString(), LineEdit::TextType::sFloatingSpaced)
	}, {
		new Label(Button(setsDescLength), World::drawSys()->translation("deadzone")),
		new Slider(Button(1.f, &Program::eventSetDeadzoneSL), World::winSys()->sets.getDeadzone(), 0, Default::axisLimit),
		new LineEdit(Button(100, &Program::eventSetDeadzoneLE), ntos(World::winSys()->sets.getDeadzone()), LineEdit::TextType::uInteger)
	} };
	sizt num = sizeof(lx) / sizeof(vector<Widget*>);
	vector<Widget*> lns(num+1);
	for (sizt i=0; i<num; i++)
		lns[i] = new Layout(itemHeight, lx[i], false);
	lns[num] = new Widget(0);

	const vector<Binding>& bindings = World::inputSys()->getBindings();
	sizt start = lns.size();
	lns.resize(lns.size() + bindings.size());
	for (sizt i=0; i<bindings.size(); i++) {
		Binding::Type type = static_cast<Binding::Type>(i);
		vector<Widget*> lin {
			new Label(Button(setsDescLength), bindingTypeToStr(type)),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::keyboard, type),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::joystick, type),
			new KeyGetter(Button(1.f), KeyGetter::AcceptType::gamepad, type),
		};
		lns[start+i] = new Layout(itemHeight, lin, false);
	}

	vector<Widget*> cont = {
		new Layout(topHeight, top, false, Layout::Selection::none, topSpacing),
		new ScrollArea(1.f, lns)
	};
	return new Layout(1.f, cont, true, Layout::Selection::none, topSpacing);
}
