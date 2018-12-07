#include "engine/world.h"

// BINDING

void Binding::setDefaultSelf(Type type) {
	switch (type) {
	case Type::enter:
		setBcall(&ProgState::eventEnter);
		setKey(Default::keyEnter);
		setJbutton(Default::jbuttonEnter);
		setGbutton(Default::gbuttonEnter);
		break;
	case Type::escape:
		setBcall(&ProgState::eventEscape);
		setKey(Default::keyEscape);
		setJbutton(Default::jbuttonEscape);
		setGbutton(Default::gbuttonEscape);
		break;
	case Type::up:
		setBcall(&ProgState::eventUp);
		setKey(Default::keyUp);
		setJhat(Default::jhatID, Default::jhatUp);
		setGbutton(Default::gbuttonUp);
		break;
	case Type::down:
		setBcall(&ProgState::eventDown);
		setKey(Default::keyDown);
		setJhat(Default::jhatID, Default::jhatDown);
		setGbutton(Default::gbuttonDown);
		break;
	case Type::left:
		setBcall(&ProgState::eventLeft);
		setKey(Default::keyLeft);
		setJhat(Default::jhatID, Default::jhatLeft);
		setGbutton(Default::gbuttonLeft);
		break;
	case Type::right:
		setBcall(&ProgState::eventRight);
		setKey(Default::keyRight);
		setJhat(Default::jhatID, Default::jhatRight);
		setGbutton(Default::gbuttonRight);
		break;
	case Type::scrollUp:
		setAcall(&ProgState::eventScrollUp);
		setKey(Default::keyUp);
		setJaxis(Default::jaxisScrollVertical, Default::axisDirUp);
		setGaxis(Default::gaxisScrollVertical, Default::axisDirUp);
		break;
	case Type::scrollDown:
		setAcall(&ProgState::eventScrollDown);
		setKey(Default::keyDown);
		setJaxis(Default::jaxisScrollVertical, Default::axisDirDown);
		setGaxis(Default::gaxisScrollVertical, Default::axisDirDown);
		break;
	case Type::scrollLeft:
		setAcall(&ProgState::eventScrollLeft);
		setKey(Default::keyLeft);
		setJaxis(Default::jaxisScrollHorizontal, Default::axisDirLeft);
		setGaxis(Default::gaxisScrollHorizontal, Default::axisDirLeft);
		break;
	case Type::scrollRight:
		setAcall(&ProgState::eventScrollRight);
		setKey(Default::keyRight);
		setJaxis(Default::jaxisScrollHorizontal, Default::axisDirRight);
		setGaxis(Default::gaxisScrollHorizontal, Default::axisDirRight);
		break;
	case Type::cursorUp:
		setAcall(&ProgState::eventCursorUp);
		setJaxis(Default::jaxisCursorVertical, Default::axisDirUp);
		setGaxis(Default::gaxisCursorVertical, Default::axisDirUp);
		break;
	case Type::cursorDown:
		setAcall(&ProgState::eventCursorDown);
		setJaxis(Default::jaxisCursorVertical, Default::axisDirDown);
		setGaxis(Default::gaxisCursorVertical, Default::axisDirDown);
		break;
	case Type::cursorLeft:
		setAcall(&ProgState::eventCursorLeft);
		setJaxis(Default::jaxisCursorHorizontal, Default::axisDirLeft);
		setGaxis(Default::gaxisCursorHorizontal, Default::axisDirLeft);
		break;
	case Type::cursorRight:
		setAcall(&ProgState::eventCursorRight);
		setJaxis(Default::jaxisCursorHorizontal, Default::axisDirRight);
		setGaxis(Default::gaxisCursorHorizontal, Default::axisDirRight);
		break;
	case Type::centerView:
		setBcall(&ProgState::eventCenterView);
		setKey(Default::keyCenterView);
		setJbutton(Default::jbuttonCenterView);
		setGbutton(Default::gbuttonCenterView);
		break;
	case Type::scrollFast:
		setAcall(nullptr);
		setKey(Default::keyScrollFast);
		setJbutton(Default::jbuttonScrollFast);
		setGbutton(Default::gbuttonScrollFast);
		break;
	case Type::scrollSlow:
		setAcall(nullptr);
		setKey(Default::keyScrollSlow);
		setJbutton(Default::jbuttonScrollSlow);
		setGbutton(Default::gbuttonScrollSlow);
		break;
	case Type::nextPage:
		setBcall(&ProgState::eventNextPage);
		setKey(Default::keyNextPage);
		break;
	case Type::prevPage:
		setBcall(&ProgState::eventPrevPage);
		setKey(Default::keyPrevPage);
		break;
	case Type::zoomIn:
		setBcall(&ProgState::eventZoomIn);
		setKey(Default::keyZoomIn);
		setJbutton(Default::jbuttonZoomIn);
		setGbutton(Default::gbuttonZoomIn);
		break;
	case Type::zoomOut:
		setBcall(&ProgState::eventZoomOut);
		setKey(Default::keyZoomOut);
		setJbutton(Default::jbuttonZoomOut);
		setGbutton(Default::gbuttonZoomOut);
		break;
	case Type::zoomReset:
		setBcall(&ProgState::eventZoomReset);
		setKey(Default::keyZoomReset);
		setJbutton(Default::jbuttonZoomReset);
		setGbutton(Default::gbuttonZoomReset);
		break;
	case Type::toStart:
		setBcall(&ProgState::eventToStart);
		setKey(Default::keyToStart);
		break;
	case Type::toEnd:
		setBcall(&ProgState::eventToEnd);
		setKey(Default::keyToEnd);
		break;
	case Type::nextDir:
		setBcall(&ProgState::eventNextDir);
		setKey(Default::keyNextDir);
		setJbutton(Default::jbuttonNextDir);
		setGaxis(Default::gaxisNextDir, true);
		break;
	case Type::prevDir:
		setBcall(&ProgState::eventPrevDir);
		setKey(Default::keyPrevDir);
		setJbutton(Default::jbuttonPrevDir);
		setGaxis(Default::gaxisPrevDir, true);
		break;
	case Type::fullscreen:
		setBcall(&ProgState::eventFullscreen);
		setKey(Default::keyFullscreen);
		setJbutton(Default::jbuttonFullscreen);
		setGbutton(Default::gbuttonFullscreen);
		break;
	case Type::refresh:
		setBcall(&ProgState::eventRefresh);
		setKey(Default::keyRefresh);
	}
}

void Binding::setKey(SDL_Scancode kkey) {
	key = kkey;
	asg |= ASG_KEY;
}

void Binding::setJbutton(uint8 but) {
	jctID = but;

	clearAsgJct();
	asg |= ASG_JBUTTON;
}

void Binding::setJaxis(uint8 axis, bool positive) {
	jctID = axis;

	clearAsgJct();
	asg |= positive ? ASG_JAXIS_P : ASG_JAXIS_N;
}

void Binding::setJhat(uint8 hat, uint8 val) {
	jctID = hat;
	jHatVal = val;

	clearAsgJct();
	asg |= ASG_JHAT;
}

void Binding::setGbutton(SDL_GameControllerButton but) {
	gctID = uint8(but);

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

void Binding::setGaxis(SDL_GameControllerAxis axis, bool positive) {
	gctID = uint8(axis);

	clearAsgGct();
	asg |= positive ? ASG_GAXIS_P : ASG_GAXIS_N;
}

void Binding::setBcall(SBCall call) {
	callAxis = false;
	bcall = call;
}

void Binding::setAcall(SACall call) {
	callAxis = true;
	acall = call;
}

// SETTINGS

Settings::Settings(bool maximized, bool fullscreen, const vec2i& resolution, const Direction& direction, float zoom, int spacing, const string& theme, const string& font, const string& language, const string& library, const string& renderer, const vec2f& speed, int16 deadzone) :
	maximized(maximized),
	fullscreen(fullscreen),
	direction(direction),
	zoom(zoom),
	spacing(spacing),
	resolution(resolution),
	renderer(renderer),
	scrollSpeed(speed)
{
	setTheme(theme);
	setFont(font);
	setLang(language);
	setDirLib(library);
	setDeadzone(deadzone);
}

const string& Settings::setTheme(const string& name) {
	vector<string> themes = World::fileSys()->getAvailibleThemes();
	for (string& it : themes)
		if (it == name)
			return theme = name;
	return theme = themes.empty() ? "" : themes[0];
}

string Settings::setFont(const string& newFont) {
	if (string path = World::fileSys()->findFont(newFont); FileSys::isFont(path)) {
		font = newFont;
		return path;
	}
	font = Default::font;
	return World::fileSys()->findFont(font);
}

const string& Settings::setLang(const string& language) {
	return lang = FileSys::fileType(World::fileSys()->getDirLangs() + language + ".ini") == FTYPE_FILE ? language : Default::language;
}

const string& Settings::setDirLib(const string& drc) {
	dirLib = drc;
	if (FileSys::fileType(dirLib) != FTYPE_DIR)
		if (!FileSys::createDir(dirLib)) {
			dirLib = World::fileSys()->getDirSets() + Default::dirLibrary;
			if (FileSys::fileType(dirLib) != FTYPE_DIR)
				FileSys::createDir(dirLib);
		}
	return dirLib;
}

int Settings::getRendererIndex() {
	// get index of currently selected renderer (if name doesn't match, choose the first renderer)
	for (int i = 0; i < SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == renderer)
			return i;
	renderer = getRendererName(0);
	return 0;
}

void Settings::setDeadzone(int zone) {
	deadzone = bringIn(zone, 0, Default::axisLimit);
}
