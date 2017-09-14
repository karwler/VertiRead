#include "settings.h"
#include "engine/filer.h"
#include "prog/program.h"
#include <algorithm>

// GENEREAL SETTINGS

GeneralSettings::GeneralSettings(const string& LANG, const string& LIB, const string& PST)
{
	setLang(LANG);
	setDirLib(LIB);
	setDirPlist(PST);
}

string GeneralSettings::getLang() const {
	return lang;
}

void GeneralSettings::setLang(const string& language) {
	lang = language;
	std::transform(lang.begin(), lang.end(), lang.begin(), tolower);

	if (!Filer::fileExists(Filer::dirLangs+lang+".ini"))
		lang = Default::language;
}

string GeneralSettings::getDirLib() const {
	return dirLib;
}

string GeneralSettings::libraryPath() const {
	return isAbsolute(dirLib) ? dirLib : Filer::dirExec + dirLib;
}

void GeneralSettings::setDirLib(const string& dir) {
	dirLib = dir.empty() ? Filer::dirSets + Default::dirLibrary + dsep : appendDsep(dir);
}

string GeneralSettings::getDirPlist() const {
	return dirPlist;
}

string GeneralSettings::playlistPath() const {
	return isAbsolute(dirPlist) ? dirPlist : Filer::dirExec + dirPlist;
}

void GeneralSettings::setDirPlist(const string& dir) {
	dirPlist = dir.empty() ? Filer::dirSets + Default::dirPlaylists + dsep : appendDsep(dir);
}

// VIDEO SETTINGS

VideoSettings::VideoSettings(bool MAX, bool FSC, const vec2i& RES, const string& FNT, const string& RNDR) :
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	renderer(RNDR)
{
	setFont(FNT);
	setDefaultTheme();
}

string VideoSettings::getFont() const {
	return font;
}

string VideoSettings::getFontpath() const {
	return fontpath;
}

void VideoSettings::setFont(const string& newFont) {
	fontpath = Filer::findFont(newFont);
	font = fontpath.empty() ? "" : newFont;
}

void VideoSettings::setDefaultTheme() {
	vector<string> themes = Filer::getAvailibleThemes();
	if (themes.empty()) {
		theme.clear();
		colors = getDefaultColors();
	}
	else {
		theme = themes[0];
		Filer::getColors(colors, theme);
	}
}

map<EColor, SDL_Color> VideoSettings::getDefaultColors() {
	map<EColor, SDL_Color> colors = {
		make_pair(EColor::background, getDefaultColor(EColor::background)),
		make_pair(EColor::rectangle, getDefaultColor(EColor::rectangle)),
		make_pair(EColor::highlighted, getDefaultColor(EColor::highlighted)),
		make_pair(EColor::darkened, getDefaultColor(EColor::darkened)),
		make_pair(EColor::text, getDefaultColor(EColor::text))
	};
	return colors;
}

SDL_Color VideoSettings::getDefaultColor(EColor color) {
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
	return {0, 0, 0, 0};	// just so msvc doesn't bitch around
}

int VideoSettings::getRenderDriverIndex() {
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

ControlsSettings::ControlsSettings(const vec2f& SSP, int16 DDZ, bool fillMissing, const map<string, Shortcut*>& CTS) :
	scrollSpeed(SSP),
	deadzone(DDZ),
	shortcuts(CTS)
{
	if (fillMissing)
		fillMissingBindings();
}

void ControlsSettings::fillMissingBindings() {
	for (string& it : vector<string>({Default::shortcutOk, Default::shortcutBack, Default::shortcutZoomIn, Default::shortcutZoomOut, Default::shortcutZoomReset, Default::shortcutCenterView, Default::shortcutFast, Default::shortcutSlow, Default::shortcutPlayPause, Default::shortcutFullscreen, Default::shortcutNextDir, Default::shortcutPrevDir, Default::shortcutNextSong, Default::shortcutPrevSong, Default::shortcutVolumeUp, Default::shortcutVolumeDown, Default::shortcutPageUp, Default::shortcutPageDown, Default::shortcutUp, Default::shortcutDown, Default::shortcutRight, Default::shortcutLeft}))
		if (shortcuts.count(it) == 0) {
			Shortcut* sc = getDefaultShortcut(it);
			if (sc)
				shortcuts.insert(make_pair(it, sc));
		}
}

Shortcut* ControlsSettings::getDefaultShortcut(const string& name) {
	Shortcut* shortcut = nullptr;
	if (name == Default::shortcutOk) {
		shortcut = new ShortcutKey(&Program::eventOk);
		shortcut->setKey(Default::keyOk);
		shortcut->setJbutton(Default::jbuttonOk);
		shortcut->gbutton(Default::gbuttonOk);
	} else if (name == Default::shortcutBack) {
		shortcut = new ShortcutKey(&Program::eventBack);
		shortcut->setKey(Default::keyBack);
		shortcut->setJbutton(Default::jbuttonBack);
		shortcut->gbutton(Default::gbuttonBack);
	} else if (name == Default::shortcutZoomIn) {
		shortcut = new ShortcutKey(&Program::eventZoomIn);
		shortcut->setKey(Default::keyZoomOut);
		shortcut->setJbutton(Default::jbuttonZoomIn);
		shortcut->gbutton(Default::gbuttonZoomOut);
	} else if (name == Default::shortcutZoomOut) {
		shortcut = new ShortcutKey(&Program::eventZoomOut);
		shortcut->setKey(Default::keyZoomOut);
		shortcut->setJbutton(Default::jbuttonZoomOut);
		shortcut->gbutton(Default::gbuttonZoomOut);
	} else if (name == Default::shortcutZoomReset) {
		shortcut = new ShortcutKey(&Program::eventZoomReset);
		shortcut->setKey(Default::keyZoomReset);
		shortcut->setJbutton(Default::jbuttonZoomReset);
		shortcut->gbutton(Default::gbuttonZoomReset);
	} else if (name == Default::shortcutCenterView) {
		shortcut = new ShortcutKey(&Program::eventCenterView);
		shortcut->setKey(Default::keyCenterView);
		shortcut->setJbutton(Default::jbuttonCenterView);
		shortcut->gbutton(Default::gbuttonCenterView);
	} else if (name == Default::shortcutFast) {
		shortcut = new ShortcutAxis();
		shortcut->setKey(Default::keyFast);
		shortcut->setJbutton(Default::jbuttonFast);
		shortcut->gbutton(Default::gbuttonFast);
	} else if (name == Default::shortcutSlow) {
		shortcut = new ShortcutAxis();
		shortcut->setKey(Default::keySlow);
		shortcut->setJbutton(Default::jbuttonSlow);
		shortcut->gbutton(Default::gbuttonSlow);
	} else if (name == Default::shortcutPlayPause) {
		shortcut = new ShortcutKey(&Program::eventPlayPause);
		shortcut->setKey(Default::keyPlayPause);
		shortcut->setJbutton(Default::jbuttonPlayPause);
		shortcut->gbutton(Default::gbuttonPlayPause);
	} else if (name == Default::shortcutFullscreen) {
		shortcut = new ShortcutKey(&Program::eventScreenMode);
		shortcut->setKey(Default::keyFullscreen);
		shortcut->setJbutton(Default::jbuttonFullscreen);
		shortcut->gbutton(Default::gbuttonFullscreen);
	} else if (name == Default::shortcutNextDir) {
		shortcut = new ShortcutKey(&Program::eventNextDir);
		shortcut->setKey(Default::keyNextDir);
		shortcut->setJbutton(Default::jbuttonNextDir);
		shortcut->setGaxis(Default::gaxisNextDir, true);
	} else if (name == Default::shortcutPrevDir) {
		shortcut = new ShortcutKey(&Program::eventPrevDir);
		shortcut->setKey(Default::keyPrevDir);
		shortcut->setJbutton(Default::jbuttonPrevDir);
		shortcut->setGaxis(Default::gaxisPrevDir, true);
	} else if (name == Default::shortcutNextSong) {
		shortcut = new ShortcutKey(&Program::eventNextSong);
		shortcut->setKey(Default::keyNextSong);
		shortcut->setJhat(Default::jhatID, Default::jhatDpadRight);
		shortcut->gbutton(Default::gbuttonDpadRight);
	} else if (name == Default::shortcutPrevSong) {
		shortcut = new ShortcutKey(&Program::eventPrevSong);
		shortcut->setKey(Default::keyPrevSong);
		shortcut->setJhat(Default::jhatID, Default::jhatDpadLeft);
		shortcut->gbutton(Default::gbuttonDpadLeft);
	} else if (name == Default::shortcutVolumeUp) {
		shortcut = new ShortcutKey(&Program::eventVolumeUp);
		shortcut->setKey(Default::keyVolumeUp);
		shortcut->setJhat(Default::jhatID, Default::jhatDpadUp);
		shortcut->gbutton(Default::gbuttonDpadUp);
	} else if (name == Default::shortcutVolumeDown) {
		shortcut = new ShortcutKey(&Program::eventVolumeDown);
		shortcut->setKey(Default::keyVolumeDown);
		shortcut->setJhat(Default::jhatID, Default::jhatDpadDown);
		shortcut->gbutton(Default::gbuttonDpadDown);
	} else if (name == Default::shortcutPageUp) {
		shortcut = new ShortcutKey(&Program::eventPageUp);
		shortcut->setKey(Default::keyPageUp);
	} else if (name == Default::shortcutPageDown) {
		shortcut = new ShortcutKey(&Program::eventPageDown);
		shortcut->setKey(Default::keyPageDown);
	} else if (name == Default::shortcutUp) {
		shortcut = new ShortcutAxis(&Program::eventUp);
		shortcut->setKey(Default::keyUp);
		shortcut->setJaxis(Default::jaxisVertical, Default::axisDirUp);
		shortcut->setGaxis(Default::gaxisVertical, Default::axisDirUp);
	} else if (name == Default::shortcutDown) {
		shortcut = new ShortcutAxis(&Program::eventDown);
		shortcut->setKey(Default::keyDown);
		shortcut->setJaxis(Default::jaxisVertical, Default::axisDirDown);
		shortcut->setGaxis(Default::gaxisVertical, Default::axisDirDown);
	} else if (name == Default::shortcutRight) {
		shortcut = new ShortcutAxis(&Program::eventRight);
		shortcut->setKey(Default::keyRight);
		shortcut->setJaxis(Default::jaxisHorizontal, Default::axisDirRight);
		shortcut->setGaxis(Default::gaxisHorizontal, Default::axisDirRight);
	} else if (name == Default::shortcutLeft) {
		shortcut = new ShortcutAxis(&Program::eventLeft);
		shortcut->setKey(Default::keyLeft);
		shortcut->setJaxis(Default::jaxisHorizontal, Default::axisDirLeft);
		shortcut->setGaxis(Default::gaxisHorizontal, Default::axisDirLeft);
	}
	return shortcut;
}
