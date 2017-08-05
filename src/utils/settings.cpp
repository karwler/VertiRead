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
		lang = Default::language;
}

string GeneralSettings::DirLib() const {
	return dirLib;
}

string GeneralSettings::LibraryPath() const {
	return isAbsolute(dirLib) ? dirLib : Filer::dirExec + dirLib;
}

void GeneralSettings::DirLib(const string& dir) {
	dirLib = dir.empty() ? Filer::dirSets + Default::dirLibrary + dsep : appendDsep(dir);
}

string GeneralSettings::DirPlist() const {
	return dirPlist;
}

string GeneralSettings::PlaylistPath() const {
	return isAbsolute(dirPlist) ? dirPlist : Filer::dirExec + dirPlist;
}

void GeneralSettings::DirPlist(const string& dir) {
	dirPlist = dir.empty() ? Filer::dirSets + Default::dirPlaylists + dsep : appendDsep(dir);
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
		return Default::colorBackground;
	case EColor::rectangle:
		return Default::colorRectangle;
	case EColor::highlighted:
		return Default::colorHighlighted;
	case EColor::darkened:
		return Default::colorDarkened;
	case EColor::text:
		return Default::colorText;
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
	for (string& it : vector<string>({Default::shortcutOk, Default::shortcutBack, Default::shortcutZoomIn, Default::shortcutZoomOut, Default::shortcutZoomReset, Default::shortcutCenterView, Default::shortcutFast, Default::shortcutSlow, Default::shortcutPlayPause, Default::shortcutFullscreen, Default::shortcutNextDir, Default::shortcutPrevDir, Default::shortcutNextSong, Default::shortcutPrevSong, Default::shortcutVolumeUp, Default::shortcutVolumeDown, Default::shortcutPageUp, Default::shortcutPageDown, Default::shortcutUp, Default::shortcutDown, Default::shortcutRight, Default::shortcutLeft}))
		if (shortcuts.count(it) == 0) {
			Shortcut* sc = GetDefaultShortcut(it);
			if (sc)
				shortcuts.insert(make_pair(it, sc));
		}
}

Shortcut* ControlsSettings::GetDefaultShortcut(const string& name) {
	Shortcut* shortcut = nullptr;
	if (name == Default::shortcutOk) {
		shortcut = new ShortcutKey(&Program::Event_Ok);
		shortcut->Key(Default::keyOk);
		shortcut->JButton(Default::jbuttonOk);
		shortcut->GButton(Default::gbuttonOk);
	} else if (name == Default::shortcutBack) {
		shortcut = new ShortcutKey(&Program::Event_Back);
		shortcut->Key(Default::keyBack);
		shortcut->JButton(Default::jbuttonBack);
		shortcut->GButton(Default::gbuttonBack);
	} else if (name == Default::shortcutZoomIn) {
		shortcut = new ShortcutKey(&Program::Event_ZoomIn);
		shortcut->Key(Default::keyZoomOut);
		shortcut->JButton(Default::jbuttonZoomIn);
		shortcut->GButton(Default::gbuttonZoomOut);
	} else if (name == Default::shortcutZoomOut) {
		shortcut = new ShortcutKey(&Program::Event_ZoomOut);
		shortcut->Key(Default::keyZoomOut);
		shortcut->JButton(Default::jbuttonZoomOut);
		shortcut->GButton(Default::gbuttonZoomOut);
	} else if (name == Default::shortcutZoomReset) {
		shortcut = new ShortcutKey(&Program::Event_ZoomReset);
		shortcut->Key(Default::keyZoomReset);
		shortcut->JButton(Default::jbuttonZoomReset);
		shortcut->GButton(Default::gbuttonZoomReset);
	} else if (name == Default::shortcutCenterView) {
		shortcut = new ShortcutKey(&Program::Event_CenterView);
		shortcut->Key(Default::keyCenterView);
		shortcut->JButton(Default::jbuttonCenterView);
		shortcut->GButton(Default::gbuttonCenterView);
	} else if (name == Default::shortcutFast) {
		shortcut = new ShortcutAxis();
		shortcut->Key(Default::keyFast);
		shortcut->JButton(Default::jbuttonFast);
		shortcut->GButton(Default::gbuttonFast);
	} else if (name == Default::shortcutSlow) {
		shortcut = new ShortcutAxis();
		shortcut->Key(Default::keySlow);
		shortcut->JButton(Default::jbuttonSlow);
		shortcut->GButton(Default::gbuttonSlow);
	} else if (name == Default::shortcutPlayPause) {
		shortcut = new ShortcutKey(&Program::Event_PlayPause);
		shortcut->Key(Default::keyPlayPause);
		shortcut->JButton(Default::jbuttonPlayPause);
		shortcut->GButton(Default::gbuttonPlayPause);
	} else if (name == Default::shortcutFullscreen) {
		shortcut = new ShortcutKey(&Program::Event_ScreenMode);
		shortcut->Key(Default::keyFullscreen);
		shortcut->JButton(Default::jbuttonFullscreen);
		shortcut->GButton(Default::gbuttonFullscreen);
	} else if (name == Default::shortcutNextDir) {
		shortcut = new ShortcutKey(&Program::Event_NextDir);
		shortcut->Key(Default::keyNextDir);
		shortcut->JButton(Default::jbuttonNextDir);
		shortcut->GAxis(Default::gaxisNextDir, true);
	} else if (name == Default::shortcutPrevDir) {
		shortcut = new ShortcutKey(&Program::Event_PrevDir);
		shortcut->Key(Default::keyPrevDir);
		shortcut->JButton(Default::jbuttonPrevDir);
		shortcut->GAxis(Default::gaxisPrevDir, true);
	} else if (name == Default::shortcutNextSong) {
		shortcut = new ShortcutKey(&Program::Event_NextSong);
		shortcut->Key(Default::keyNextSong);
		shortcut->JHat(Default::jhatID, Default::jhatDpadRight);
		shortcut->GButton(Default::gbuttonDpadRight);
	} else if (name == Default::shortcutPrevSong) {
		shortcut = new ShortcutKey(&Program::Event_PrevSong);
		shortcut->Key(Default::keyPrevSong);
		shortcut->JHat(Default::jhatID, Default::jhatDpadLeft);
		shortcut->GButton(Default::gbuttonDpadLeft);
	} else if (name == Default::shortcutVolumeUp) {
		shortcut = new ShortcutKey(&Program::Event_VolumeUp);
		shortcut->Key(Default::keyVolumeUp);
		shortcut->JHat(Default::jhatID, Default::jhatDpadUp);
		shortcut->GButton(Default::gbuttonDpadUp);
	} else if (name == Default::shortcutVolumeDown) {
		shortcut = new ShortcutKey(&Program::Event_VolumeDown);
		shortcut->Key(Default::keyVolumeDown);
		shortcut->JHat(Default::jhatID, Default::jhatDpadDown);
		shortcut->GButton(Default::gbuttonDpadDown);
	} else if (name == Default::shortcutPageUp) {
		shortcut = new ShortcutKey(&Program::Event_PageUp);
		shortcut->Key(Default::keyPageUp);
	} else if (name == Default::shortcutPageDown) {
		shortcut = new ShortcutKey(&Program::Event_PageDown);
		shortcut->Key(Default::keyPageDown);
	} else if (name == Default::shortcutUp) {
		shortcut = new ShortcutAxis(&Program::Event_Up);
		shortcut->Key(Default::keyUp);
		shortcut->JAxis(Default::jaxisVertical, Default::axisDirUp);
		shortcut->GAxis(Default::gaxisVertical, Default::axisDirUp);
	} else if (name == Default::shortcutDown) {
		shortcut = new ShortcutAxis(&Program::Event_Down);
		shortcut->Key(Default::keyDown);
		shortcut->JAxis(Default::jaxisVertical, Default::axisDirDown);
		shortcut->GAxis(Default::gaxisVertical, Default::axisDirDown);
	} else if (name == Default::shortcutRight) {
		shortcut = new ShortcutAxis(&Program::Event_Right);
		shortcut->Key(Default::keyRight);
		shortcut->JAxis(Default::jaxisHorizontal, Default::axisDirRight);
		shortcut->GAxis(Default::gaxisHorizontal, Default::axisDirRight);
	} else if (name == Default::shortcutLeft) {
		shortcut = new ShortcutAxis(&Program::Event_Left);
		shortcut->Key(Default::keyLeft);
		shortcut->JAxis(Default::jaxisHorizontal, Default::axisDirLeft);
		shortcut->GAxis(Default::gaxisHorizontal, Default::axisDirLeft);
	}
	return shortcut;
}
