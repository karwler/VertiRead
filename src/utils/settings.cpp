#include "settings.h"
#include "engine/filer.h"
#include "prog/program.h"
#include <algorithm>

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
	theme.clear();
	colors = GetDefaultColors();
}

map<EColor, vec4b> VideoSettings::GetDefaultColors() {
	map<EColor, vec4b> colors = {
		make_pair(EColor::background, GetDefaultColor(EColor::background)),
		make_pair(EColor::rectangle, GetDefaultColor(EColor::rectangle)),
		make_pair(EColor::highlighted, GetDefaultColor(EColor::highlighted)),
		make_pair(EColor::darkened, GetDefaultColor(EColor::darkened)),
		make_pair(EColor::text, GetDefaultColor(EColor::text))
	};
	return colors;
}

vec4b VideoSettings::GetDefaultColor(EColor color) {
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

// AUDIO SETTINGS

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

// CONTROLS SETTINGS

ControlsSettings::ControlsSettings(const vec2f& SSP, bool fillMissingBindings, const map<string, Shortcut*>& CTS) :
	scrollSpeed(SSP),
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
	if (name == SHORTCUT_OK)
		return new ShortcutKey(DEFAULT_KEY_OK, DEFAULT_CBINDING_OK, DEFAULT_CTYPE_0, &Program::Event_Ok);
	else if (name == SHORTCUT_BACK)
		return new ShortcutKey(DEFAULT_KEY_BACK, DEFAULT_CBINDING_BACK, DEFAULT_CTYPE_0, &Program::Event_Back);
	else if (name == SHORTCUT_ZOOM_IN)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_IN, DEFAULT_CBINDING_ZOOM_IN, DEFAULT_CTYPE_0, &Program::Event_ZoomIn);
	else if (name == SHORTCUT_ZOOM_OUT)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_OUT, DEFAULT_CBINDING_ZOOM_OUT, DEFAULT_CTYPE_0, &Program::Event_ZoomOut);
	else if (name == SHORTCUT_ZOOM_RESET)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_RESET, DEFAULT_CBINDING_ZOOM_RESET, DEFAULT_CTYPE_0, &Program::Event_ZoomReset);
	else if (name == SHORTCUT_CENTER_VIEW)
		return new ShortcutKey(DEFAULT_KEY_CENTER_VIEW, DEFAULT_CBINDING_CENTER_VIEW, DEFAULT_CTYPE_0, &Program::Event_CenterView);
	else if (name == SHORTCUT_FAST)
		return new ShortcutAxis(DEFAULT_KEY_FAST, DEFAULT_CBINDING_FAST, DEFAULT_CTYPE_0);
	else if (name == SHORTCUT_SLOW)
		return new ShortcutAxis(DEFAULT_KEY_SLOW, DEFAULT_CBINDING_SLOW, DEFAULT_CTYPE_0);
	else if (name == SHORTCUT_PLAY_PAUSE)
		return new ShortcutKey(DEFAULT_KEY_PLAY_PAUSE, DEFAULT_CBINDING_PLAY_PAUSE, DEFAULT_CTYPE_0, &Program::Event_PlayPause);
	else if (name == SHORTCUT_FULLSCREEN)
		return new ShortcutKey(DEFAULT_KEY_FULLSCREEN, DEFAULT_CBINDING_FULLSCREEN, DEFAULT_CTYPE_0, &Program::Event_ScreenMode);
	else if (name == SHORTCUT_NEXT_DIR)
		return new ShortcutKey(DEFAULT_KEY_NEXT_DIR, DEFAULT_CBINDING_NEXT_DIR, DEFAULT_CTYPE_1, &Program::Event_NextDir);
	else if (name == SHORTCUT_PREV_DIR)
		return new ShortcutKey(DEFAULT_KEY_PREV_DIR, DEFAULT_CBINDING_PREV_DIR, DEFAULT_CTYPE_1, &Program::Event_PrevDir);
	else if (name == SHORTCUT_NEXT_SONG)
		return new ShortcutKey(DEFAULT_KEY_NEXT_SONG, DEFAULT_CBINDING_DPAD_RIGHT, DEFAULT_CTYPE_2, &Program::Event_NextSong);
	else if (name == SHORTCUT_PREV_SONG)
		return new ShortcutKey(DEFAULT_KEY_PREV_SONG, DEFAULT_CBINDING_DPAD_LEFT, DEFAULT_CTYPE_2, &Program::Event_PrevSong);
	else if (name == SHORTCUT_VOLUME_UP)
		return new ShortcutKey(DEFAULT_KEY_VOLUME_UP, DEFAULT_CBINDING_DPAD_UP, DEFAULT_CTYPE_2, &Program::Event_VolumeUp);
	else if (name == SHORTCUT_VOLUME_DOWN)
		return new ShortcutKey(DEFAULT_KEY_VOLUME_DOWN, DEFAULT_CBINDING_DPAD_DOWN, DEFAULT_CTYPE_2, &Program::Event_VolumeDown);
	else if (name == SHORTCUT_PAGE_UP)
		return new ShortcutKey(DEFAULT_KEY_PAGE_UP, &Program::Event_PageUp);
	else if (name == SHORTCUT_PAGE_DOWN)
		return new ShortcutKey(DEFAULT_KEY_PAGE_DOWN, &Program::Event_PageDown);
	else if (name == SHORTCUT_UP)
		return new ShortcutAxis(DEFAULT_KEY_UP, DEFAULT_CBINDING_VERTICAL, DEFAULT_CTYPE_4, &Program::Event_Up);
	else if (name == SHORTCUT_DOWN)
		return new ShortcutAxis(DEFAULT_KEY_DOWN, DEFAULT_CBINDING_VERTICAL, DEFAULT_CTYPE_3, &Program::Event_Down);
	else if (name == SHORTCUT_RIGHT)
		return new ShortcutAxis(DEFAULT_KEY_RIGHT, DEFAULT_CBINDING_HORIZONTAL, DEFAULT_CTYPE_3, &Program::Event_Right);
	else if (name == SHORTCUT_LEFT)
		return new ShortcutAxis(DEFAULT_KEY_LEFT, DEFAULT_CBINDING_HORIZONTAL, DEFAULT_CTYPE_4, &Program::Event_Left);
	else if (name == SHORTCUT_CURSOR_UP)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_VERT, DEFAULT_CTYPE_4, &Program::Event_CursorUp);
	else if (name == SHORTCUT_CURSOR_DOWN)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_VERT, DEFAULT_CTYPE_3, &Program::Event_CursorDown);
	else if (name == SHORTCUT_CURSOR_RIGHT)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_HORI, DEFAULT_CTYPE_3, &Program::Event_CursorRight);
	else if (name == SHORTCUT_CURSOR_LEFT)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_HORI, DEFAULT_CTYPE_4, &Program::Event_CursorLeft);
	return nullptr;
}
