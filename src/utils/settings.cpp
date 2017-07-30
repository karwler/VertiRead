#include "settings.h"
#include "engine/filer.h"
#include "prog/program.h"
#include <algorithm>
#include <cctype>

// GENEREAL SETTINGS

GeneralSettings::GeneralSettings(const string& LANG, const string& LIB, const string& PST)
{
	Lang(LANG);
	DirLib(LIB);
	DirPlist(PST);
}

string GeneralSettings::Lang() const {
	return lang;
}

void GeneralSettings::Lang(const string& language) {
	lang = language;
	std::transform(lang.begin(), lang.end(), lang.begin(), tolower);

	if (!Filer::Exists(Filer::dirLangs+lang+".ini"))
		lang = "english";
}

string GeneralSettings::DirLib() const {
	return dirLib;
}

string GeneralSettings::LibraryPath() const {
	return isAbsolute(dirLib) ? dirLib : Filer::dirExec + dirLib;
}

void GeneralSettings::DirLib(const string& dir) {
	dirLib = dir.empty() ? Filer::dirSets + "library"+dsep : appendDsep(dir);
}

string GeneralSettings::DirPlist() const {
	return dirPlist;
}

string GeneralSettings::PlaylistPath() const {
	return isAbsolute(dirPlist) ? dirPlist : Filer::dirExec + dirPlist;
}

void GeneralSettings::DirPlist(const string& dir) {
	dirPlist = dir.empty() ? Filer::dirSets + "playlists"+dsep : appendDsep(dir);
}

// VIDEO SETTINGS

VideoSettings::VideoSettings(bool MAX, bool FSC, const vec2i& RES, const string& FNT, const string& RNDR) :
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	renderer(RNDR)
{
	SetFont(FNT);
	SetDefaultTheme();
}

string VideoSettings::Font() const {
	return font;
}

string VideoSettings::Fontpath() const {
	return fontpath;
}

void VideoSettings::SetFont(const string& newFont) {
	fontpath = Filer::FindFont(newFont);
	font = fontpath.empty() ? "" : newFont;
}

void VideoSettings::SetDefaultTheme() {
	vector<string> themes = Filer::GetAvailibleThemes();
	if (themes.empty()) {
		theme.clear();
		colors = GetDefaultColors();
	}
	else {
		theme = themes[0];
		Filer::GetColors(colors, theme);
	}
}

map<EColor, vec4c> VideoSettings::GetDefaultColors() {
	map<EColor, vec4c> colors = {
		make_pair(EColor::background, GetDefaultColor(EColor::background)),
		make_pair(EColor::rectangle, GetDefaultColor(EColor::rectangle)),
		make_pair(EColor::highlighted, GetDefaultColor(EColor::highlighted)),
		make_pair(EColor::darkened, GetDefaultColor(EColor::darkened)),
		make_pair(EColor::text, GetDefaultColor(EColor::text))
	};
	return colors;
}

vec4c VideoSettings::GetDefaultColor(EColor color) {
	switch (color) {
	case EColor::background:
		return DEFAULT_COLOR_BACKGROUND;
	case EColor::rectangle:
		return DEFAULT_COLOR_RECTANGLE;
	case EColor::highlighted:
		return DEFAULT_COLOR_HIGHLIGHTED;
	case EColor::darkened:
		return DEFAULT_COLOR_DARKENED;
	case EColor::text:
		return DEFAULT_COLOR_TEXT;
	}
	return 0;	// just so msvc doesn't bitch around
}

int VideoSettings::GetRenderDriverIndex() {
	// get index of currently selected renderer (if name doesn't match, choose the first renderer)
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == renderer)
			return i;

	renderer = getRendererName(0);
	return 0;
}

// AUDIO SETTINGS

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

// CONTROLS SETTINGS

ControlsSettings::ControlsSettings(const vec2f& SSP, int16 DDZ, bool fillMissingBindings, const map<string, Shortcut*>& CTS) :
	scrollSpeed(SSP),
	deadzone(DDZ),
	shortcuts(CTS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
	vector<string> names = { SHORTCUT_OK, SHORTCUT_BACK, SHORTCUT_ZOOM_IN, SHORTCUT_ZOOM_OUT, SHORTCUT_ZOOM_RESET, SHORTCUT_CENTER_VIEW, SHORTCUT_FAST, SHORTCUT_SLOW, SHORTCUT_PLAY_PAUSE, SHORTCUT_FULLSCREEN, SHORTCUT_NEXT_DIR, SHORTCUT_PREV_DIR, SHORTCUT_NEXT_SONG, SHORTCUT_PREV_SONG, SHORTCUT_VOLUME_UP, SHORTCUT_VOLUME_DOWN, SHORTCUT_PAGE_UP, SHORTCUT_PAGE_DOWN, SHORTCUT_UP, SHORTCUT_DOWN, SHORTCUT_RIGHT, SHORTCUT_LEFT, SHORTCUT_CURSOR_UP, SHORTCUT_CURSOR_DOWN, SHORTCUT_CURSOR_RIGHT, SHORTCUT_CURSOR_LEFT };
	for (string& it : names)
		if (shortcuts.count(it) == 0) {
			Shortcut* sc = GetDefaultShortcut(it);
			if (sc)
				shortcuts.insert(make_pair(it, sc));
		}
}

Shortcut* ControlsSettings::GetDefaultShortcut(const string& name) {
	Shortcut* shortcut = nullptr;
	if (name == SHORTCUT_OK) {
		shortcut = new ShortcutKey(&Program::Event_Ok);
		shortcut->Key(DEFAULT_KEY_OK);
		shortcut->JButton(DEFAULT_JBUTTON_OK);
		shortcut->GButton(DEFAULT_GBUTTON_OK);
	} else if (name == SHORTCUT_BACK) {
		shortcut = new ShortcutKey(&Program::Event_Back);
		shortcut->Key(DEFAULT_KEY_BACK);
		shortcut->JButton(DEFAULT_JBUTTON_BACK);
		shortcut->GButton(DEFAULT_GBUTTON_BACK);
	} else if (name == SHORTCUT_ZOOM_IN) {
		shortcut = new ShortcutKey(&Program::Event_ZoomIn);
		shortcut->Key(DEFAULT_KEY_ZOOM_IN);
		shortcut->JButton(DEFAULT_JBUTTON_ZOOM_IN);
		shortcut->GButton(DEFAULT_GBUTTON_ZOOM_IN);
	} else if (name == SHORTCUT_ZOOM_OUT) {
		shortcut = new ShortcutKey(&Program::Event_ZoomOut);
		shortcut->Key(DEFAULT_KEY_ZOOM_OUT);
		shortcut->JButton(DEFAULT_JBUTTON_ZOOM_OUT);
		shortcut->GButton(DEFAULT_GBUTTON_ZOOM_OUT);
	} else if (name == SHORTCUT_ZOOM_RESET) {
		shortcut = new ShortcutKey(&Program::Event_ZoomReset);
		shortcut->Key(DEFAULT_KEY_ZOOM_RESET);
		shortcut->JButton(DEFAULT_JBUTTON_ZOOM_RESET);
		shortcut->GButton(DEFAULT_GBUTTON_ZOOM_RESET);
	} else if (name == SHORTCUT_CENTER_VIEW) {
		shortcut = new ShortcutKey(&Program::Event_CenterView);
		shortcut->Key(DEFAULT_KEY_CENTER_VIEW);
		shortcut->JButton(DEFAULT_JBUTTON_CENTER_VIEW);
		shortcut->GButton(DEFAULT_GBUTTON_CENTER_VIEW);
	} else if (name == SHORTCUT_FAST) {
		shortcut = new ShortcutAxis();
		shortcut->Key(DEFAULT_KEY_FAST);
		shortcut->JButton(DEFAULT_JBUTTON_FAST);
		shortcut->GButton(DEFAULT_GBUTTON_FAST);
	} else if (name == SHORTCUT_SLOW) {
		shortcut = new ShortcutAxis();
		shortcut->Key(DEFAULT_KEY_SLOW);
		shortcut->JButton(DEFAULT_JBUTTON_SLOW);
		shortcut->GButton(DEFAULT_GBUTTON_SLOW);
	} else if (name == SHORTCUT_PLAY_PAUSE) {
		shortcut = new ShortcutKey(&Program::Event_PlayPause);
		shortcut->Key(DEFAULT_KEY_PLAY_PAUSE);
		shortcut->JButton(DEFAULT_JBUTTON_PLAY_PAUSE);
		shortcut->GButton(DEFAULT_GBUTTON_PLAY_PAUSE);
	} else if (name == SHORTCUT_FULLSCREEN) {
		shortcut = new ShortcutKey(&Program::Event_ScreenMode);
		shortcut->Key(DEFAULT_KEY_FULLSCREEN);
		shortcut->JButton(DEFAULT_JBUTTON_FULLSCREEN);
		shortcut->GButton(DEFAULT_GBUTTON_FULLSCREEN);
	} else if (name == SHORTCUT_NEXT_DIR) {
		shortcut = new ShortcutKey(&Program::Event_NextDir);
		shortcut->Key(DEFAULT_KEY_NEXT_DIR);
		shortcut->JButton(DEFAULT_JBUTTON_NEXT_DIR);
		shortcut->GAxis(DEFAULT_GAXIS_NEXT_DIR, true);
	} else if (name == SHORTCUT_PREV_DIR) {
		shortcut = new ShortcutKey(&Program::Event_PrevDir);
		shortcut->Key(DEFAULT_KEY_PREV_DIR);
		shortcut->JButton(DEFAULT_JBUTTON_PREV_DIR);
		shortcut->GAxis(DEFAULT_GAXIS_PREV_DIR, true);
	} else if (name == SHORTCUT_NEXT_SONG) {
		shortcut = new ShortcutKey(&Program::Event_NextSong);
		shortcut->Key(DEFAULT_KEY_NEXT_SONG);
		shortcut->JHat(DEFAULT_JHAT_ID, DEFAULT_JHAT_DPAD_RIGHT);
		shortcut->GButton(DEFAULT_GBUTTON_DPAD_RIGHT);
	} else if (name == SHORTCUT_PREV_SONG) {
		shortcut = new ShortcutKey(&Program::Event_PrevSong);
		shortcut->Key(DEFAULT_KEY_PREV_SONG);
		shortcut->JHat(DEFAULT_JHAT_ID, DEFAULT_JHAT_DPAD_LEFT);
		shortcut->GButton(DEFAULT_GBUTTON_DPAD_LEFT);
	} else if (name == SHORTCUT_VOLUME_UP) {
		shortcut = new ShortcutKey(&Program::Event_VolumeUp);
		shortcut->Key(DEFAULT_KEY_VOLUME_UP);
		shortcut->JHat(DEFAULT_JHAT_ID, DEFAULT_JHAT_DPAD_UP);
		shortcut->GButton(DEFAULT_GBUTTON_DPAD_UP);
	} else if (name == SHORTCUT_VOLUME_DOWN) {
		shortcut = new ShortcutKey(&Program::Event_VolumeDown);
		shortcut->Key(DEFAULT_KEY_VOLUME_DOWN);
		shortcut->JHat(DEFAULT_JHAT_ID, DEFAULT_JHAT_DPAD_DOWN);
		shortcut->GButton(DEFAULT_GBUTTON_DPAD_DOWN);
	} else if (name == SHORTCUT_PAGE_UP) {
		shortcut = new ShortcutKey(&Program::Event_PageUp);
		shortcut->Key(DEFAULT_KEY_PAGE_UP);
	} else if (name == SHORTCUT_PAGE_DOWN) {
		shortcut = new ShortcutKey(&Program::Event_PageDown);
		shortcut->Key(DEFAULT_KEY_PAGE_DOWN);
	} else if (name == SHORTCUT_UP) {
		shortcut = new ShortcutAxis(&Program::Event_Up);
		shortcut->Key(DEFAULT_KEY_UP);
		shortcut->JAxis(DEFAULT_JAXIS_VERTICAL, DEFAULT_AXIS_DIR_UP);
		shortcut->GAxis(DEFAULT_GAXIS_VERTICAL, DEFAULT_AXIS_DIR_UP);
	} else if (name == SHORTCUT_DOWN) {
		shortcut = new ShortcutAxis(&Program::Event_Down);
		shortcut->Key(DEFAULT_KEY_DOWN);
		shortcut->JAxis(DEFAULT_JAXIS_VERTICAL, DEFAULT_AXIS_DIR_DOWN);
		shortcut->GAxis(DEFAULT_GAXIS_VERTICAL, DEFAULT_AXIS_DIR_DOWN);
	} else if (name == SHORTCUT_RIGHT) {
		shortcut = new ShortcutAxis(&Program::Event_Right);
		shortcut->Key(DEFAULT_KEY_RIGHT);
		shortcut->JAxis(DEFAULT_JAXIS_HORIZONTAL, DEFAULT_AXIS_DIR_RIGHT);
		shortcut->GAxis(DEFAULT_GAXIS_HORIZONTAL, DEFAULT_AXIS_DIR_RIGHT);
	} else if (name == SHORTCUT_LEFT) {
		shortcut = new ShortcutAxis(&Program::Event_Left);
		shortcut->Key(DEFAULT_KEY_LEFT);
		shortcut->JAxis(DEFAULT_JAXIS_HORIZONTAL, DEFAULT_AXIS_DIR_LEFT);
		shortcut->GAxis(DEFAULT_GAXIS_HORIZONTAL, DEFAULT_AXIS_DIR_LEFT);
	} else if (name == SHORTCUT_CURSOR_UP) {
		shortcut = new ShortcutAxis(&Program::Event_CursorUp);
		shortcut->JAxis(DEFAULT_JAXIS_CURSOR_VERT, DEFAULT_AXIS_DIR_UP);
		shortcut->GAxis(DEFAULT_GAXIS_CURSOR_VERT, DEFAULT_AXIS_DIR_UP);
	} else if (name == SHORTCUT_CURSOR_DOWN) {
		shortcut = new ShortcutAxis(&Program::Event_CursorDown);
		shortcut->JAxis(DEFAULT_JAXIS_CURSOR_VERT, DEFAULT_AXIS_DIR_DOWN);
		shortcut->GAxis(DEFAULT_GAXIS_CURSOR_VERT, DEFAULT_AXIS_DIR_DOWN);
	} else if (name == SHORTCUT_CURSOR_RIGHT) {
		shortcut = new ShortcutAxis(&Program::Event_CursorRight);
		shortcut->JAxis(DEFAULT_JAXIS_CURSOR_HORI, DEFAULT_AXIS_DIR_RIGHT);
		shortcut->GAxis(DEFAULT_GAXIS_CURSOR_HORI, DEFAULT_AXIS_DIR_RIGHT);
	} else if (name == SHORTCUT_CURSOR_LEFT) {
		shortcut = new ShortcutAxis(&Program::Event_CursorLeft);
		shortcut->JAxis(DEFAULT_JAXIS_CURSOR_HORI, DEFAULT_AXIS_DIR_LEFT);
		shortcut->GAxis(DEFAULT_GAXIS_CURSOR_HORI, DEFAULT_AXIS_DIR_LEFT);
	}
	return shortcut;
}
