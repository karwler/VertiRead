#include "engine/world.h"

// BINDING

Binding::Binding() :
	asg(ASG_NONE)
{}

void Binding::setDefaultSelf(Type type) {
	if (type == Type::enter) {
		setBcall(&ProgState::eventEnter);
		setKey(Default::keyEnter);
		setJbutton(Default::jbuttonEnter);
		setGbutton(Default::gbuttonEnter);
	} else if (type == Type::escape) {
		setBcall(&ProgState::eventEscape);
		setKey(Default::keyEscape);
		setJbutton(Default::jbuttonEscape);
		setGbutton(Default::gbuttonEscape);
	} else if (type == Type::up) {
		setBcall(&ProgState::eventUp);
		setKey(Default::keyUp);
		setJhat(Default::jhatID, Default::jhatUp);
		setGbutton(Default::gbuttonUp);
	} else if (type == Type::down) {
		setBcall(&ProgState::eventDown);
		setKey(Default::keyDown);
		setJhat(Default::jhatID, Default::jhatDown);
		setGbutton(Default::gbuttonDown);
	} else if (type == Type::left) {
		setBcall(&ProgState::eventLeft);
		setKey(Default::keyLeft);
		setJhat(Default::jhatID, Default::jhatLeft);
		setGbutton(Default::gbuttonLeft);
	} else if (type == Type::right) {
		setBcall(&ProgState::eventRight);
		setKey(Default::keyRight);
		setJhat(Default::jhatID, Default::jhatRight);
		setGbutton(Default::gbuttonRight);
	} else if (type == Type::scrollUp) {
		setAcall(&ProgState::eventScrollUp);
		setKey(Default::keyUp);
		setJaxis(Default::jaxisScrollVertical, Default::axisDirUp);
		setGaxis(Default::gaxisScrollVertical, Default::axisDirUp);
	} else if (type == Type::scrollDown) {
		setAcall(&ProgState::eventScrollDown);
		setKey(Default::keyDown);
		setJaxis(Default::jaxisScrollVertical, Default::axisDirDown);
		setGaxis(Default::gaxisScrollVertical, Default::axisDirDown);
	} else if (type == Type::scrollLeft) {
		setAcall(&ProgState::eventScrollLeft);
		setKey(Default::keyLeft);
		setJaxis(Default::jaxisScrollHorizontal, Default::axisDirLeft);
		setGaxis(Default::gaxisScrollHorizontal, Default::axisDirLeft);
	} else if (type == Type::scrollRight) {
		setAcall(&ProgState::eventScrollRight);
		setKey(Default::keyRight);
		setJaxis(Default::jaxisScrollHorizontal, Default::axisDirRight);
		setGaxis(Default::gaxisScrollHorizontal, Default::axisDirRight);
	} else if (type == Type::cursorUp) {
		setAcall(&ProgState::eventCursorUp);
		setJaxis(Default::jaxisCursorVertical, Default::axisDirUp);
		setGaxis(Default::gaxisCursorVertical, Default::axisDirUp);
	} else if (type == Type::cursorDown) {
		setAcall(&ProgState::eventCursorDown);
		setJaxis(Default::jaxisCursorVertical, Default::axisDirDown);
		setGaxis(Default::gaxisCursorVertical, Default::axisDirDown);
	} else if (type == Type::cursorLeft) {
		setAcall(&ProgState::eventCursorLeft);
		setJaxis(Default::jaxisCursorHorizontal, Default::axisDirLeft);
		setGaxis(Default::gaxisCursorHorizontal, Default::axisDirLeft);
	} else if (type == Type::cursorRight) {
		setAcall(&ProgState::eventCursorRight);
		setJaxis(Default::jaxisCursorHorizontal, Default::axisDirRight);
		setGaxis(Default::gaxisCursorHorizontal, Default::axisDirRight);
	} else if (type == Type::centerView) {
		setBcall(&ProgState::eventCenterView);
		setKey(Default::keyCenterView);
		setJbutton(Default::jbuttonCenterView);
		setGbutton(Default::gbuttonCenterView);
	} else if (type == Type::scrollFast) {
		setAcall(nullptr);
		setKey(Default::keyScrollFast);
		setJbutton(Default::jbuttonScrollFast);
		setGbutton(Default::gbuttonScrollFast);
	} else if (type == Type::scrollSlow) {
		setAcall(nullptr);
		setKey(Default::keyScrollSlow);
		setJbutton(Default::jbuttonScrollSlow);
		setGbutton(Default::gbuttonScrollSlow);
	} else if (type == Type::nextPage) {
		setBcall(&ProgState::eventNextPage);
		setKey(Default::keyNextPage);
	} else if (type == Type::prevPage) {
		setBcall(&ProgState::eventPrevPage);
		setKey(Default::keyPrevPage);
	} else if (type == Type::zoomIn) {
		setBcall(&ProgState::eventZoomIn);
		setKey(Default::keyZoomIn);
		setJbutton(Default::jbuttonZoomIn);
		setGbutton(Default::gbuttonZoomIn);
	} else if (type == Type::zoomOut) {
		setBcall(&ProgState::eventZoomOut);
		setKey(Default::keyZoomOut);
		setJbutton(Default::jbuttonZoomOut);
		setGbutton(Default::gbuttonZoomOut);
	} else if (type == Type::zoomReset) {
		setBcall(&ProgState::eventZoomReset);
		setKey(Default::keyZoomReset);
		setJbutton(Default::jbuttonZoomReset);
		setGbutton(Default::gbuttonZoomReset);
	} else if (type == Type::toStart) {
		setBcall(&ProgState::eventToStart);
		setKey(Default::keyToStart);
	} else if (type == Type::toEnd) {
		setBcall(&ProgState::eventToEnd);
		setKey(Default::keyToEnd);
	} else if (type == Type::nextDir) {
		setBcall(&ProgState::eventNextDir);
		setKey(Default::keyNextDir);
		setJbutton(Default::jbuttonNextDir);
		setGaxis(Default::gaxisNextDir, true);
	} else if (type == Type::prevDir) {
		setBcall(&ProgState::eventPrevDir);
		setKey(Default::keyPrevDir);
		setJbutton(Default::jbuttonPrevDir);
		setGaxis(Default::gaxisPrevDir, true);
	} else if (type == Type::fullscreen) {
		setBcall(&ProgState::eventFullscreen);
		setKey(Default::keyFullscreen);
		setJbutton(Default::jbuttonFullscreen);
		setGbutton(Default::gbuttonFullscreen);
	} else if (type == Type::refresh) {
		setBcall(&ProgState::eventRefresh);
		setKey(Default::keyRefresh);
	}
}

void Binding::clearAsgKey() {
	asg &= ~ASG_KEY;
}

void Binding::setKey(SDL_Scancode kkey) {
	key = kkey;
	asg |= ASG_KEY;
}

bool Binding::jctAssigned() const {
	return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

void Binding::clearAsgJct() {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
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

void Binding::clearAsgGct() {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

void Binding::setGbutton(SDL_GameControllerButton but) {
	gctID = but;

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

void Binding::setGaxis(SDL_GameControllerAxis axis, bool positive) {
	gctID = axis;

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

string Settings::getResolutionString() const {
	return to_string(resolution.x) + ' ' + to_string(resolution.y);
}

void Settings::setResolution(const string& line) {
	vector<string> elems = getWords(line);
	for (sizt i = 0; i < elems.size() && i < 2; i++)
		resolution[i] = sstoul(elems[i]);
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

vector<string> Settings::getAvailibleRenderers() {
	vector<string> renderers(SDL_GetNumRenderDrivers());
	for (sizt i = 0; i < renderers.size(); i++)
		renderers[i] = getRendererName(i);
	return renderers;
}

string Settings::getRendererName(int id) {
	SDL_RendererInfo info;
	SDL_GetRenderDriverInfo(id, &info);
	return info.name;
}

string Settings::getScrollSpeedString() const {
	return trimZero(to_string(scrollSpeed.x)) + ' ' + trimZero(to_string(scrollSpeed.y));
}

void Settings::setScrollSpeed(const string& line) {
	vector<string> elems = getWords(line);
	for (sizt i = 0; i < elems.size() && i < 2; i++)
		scrollSpeed[i] = sstof(elems[i].c_str());
}

void Settings::setDeadzone(int zone) {
	deadzone = bringIn(zone, 0, Default::axisLimit);
}
