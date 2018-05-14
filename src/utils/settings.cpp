#include "engine/world.h"

// PLAYLIST

Playlist::Playlist(const string& NAME, const vector<string>& SGS, const uset<string>& BKS) :
	name(NAME),
	songs(SGS),
	books(BKS)
{}

// BINDING

Binding::Binding() :
	asg(ASG_NONE)
{}

void Binding::setDefaultSelf(Type type) {
	if (type == Type::back) {
		setBcall(&ProgState::eventBack);
		setKey(Default::keyBack);
		setJbutton(Default::jbuttonBack);
		setGbutton(Default::gbuttonBack);
	} else if (type == Type::zoomIn) {
		setBcall(&ProgState::eventZoomIn);
		setKey(Default::keyZoomOut);
		setJbutton(Default::jbuttonZoomIn);
		setGbutton(Default::gbuttonZoomOut);
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
	} else if (type == Type::centerView) {
		setBcall(&ProgState::eventCenterView);
		setKey(Default::keyCenterView);
		setJbutton(Default::jbuttonCenterView);
		setGbutton(Default::gbuttonCenterView);
	} else if (type == Type::fast) {
		setAcall(nullptr);
		setKey(Default::keyFast);
		setJbutton(Default::jbuttonFast);
		setGbutton(Default::gbuttonFast);
	} else if (type == Type::slow) {
		setAcall(nullptr);
		setKey(Default::keySlow);
		setJbutton(Default::jbuttonSlow);
		setGbutton(Default::gbuttonSlow);
	} else if (type == Type::playPause) {
		setBcall(&ProgState::eventPlayPause);
		setKey(Default::keyPlayPause);
		setJbutton(Default::jbuttonPlayPause);
		setGbutton(Default::gbuttonPlayPause);
	} else if (type == Type::fullscreen) {
		setBcall(&ProgState::eventScreenMode);
		setKey(Default::keyFullscreen);
		setJbutton(Default::jbuttonFullscreen);
		setGbutton(Default::gbuttonFullscreen);
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
	} else if (type == Type::nextSong) {
		setBcall(&ProgState::eventNextSong);
		setKey(Default::keyNextSong);
		setJhat(Default::jhatID, Default::jhatDpadRight);
		setGbutton(Default::gbuttonDpadRight);
	} else if (type == Type::prevSong) {
		setBcall(&ProgState::eventPrevSong);
		setKey(Default::keyPrevSong);
		setJhat(Default::jhatID, Default::jhatDpadLeft);
		setGbutton(Default::gbuttonDpadLeft);
	} else if (type == Type::volumeUp) {
		setBcall(&ProgState::eventVolumeUp);
		setKey(Default::keyVolumeUp);
		setJhat(Default::jhatID, Default::jhatDpadUp);
		setGbutton(Default::gbuttonDpadUp);
	} else if (type == Type::volumeDown) {
		setBcall(&ProgState::eventVolumeDown);
		setKey(Default::keyVolumeDown);
		setJhat(Default::jhatID, Default::jhatDpadDown);
		setGbutton(Default::gbuttonDpadDown);
	} else if (type == Type::pageUp) {
		setBcall(&ProgState::eventPageUp);
		setKey(Default::keyPageUp);
	} else if (type == Type::pageDown) {
		setBcall(&ProgState::eventPageDown);
		setKey(Default::keyPageDown);
	} else if (type == Type::up) {
		setAcall(&ProgState::eventUp);
		setKey(Default::keyUp);
		setJaxis(Default::jaxisVertical, Default::axisDirUp);
		setGaxis(Default::gaxisVertical, Default::axisDirUp);
	} else if (type == Type::down) {
		setAcall(&ProgState::eventDown);
		setKey(Default::keyDown);
		setJaxis(Default::jaxisVertical, Default::axisDirDown);
		setGaxis(Default::gaxisVertical, Default::axisDirDown);
	} else if (type == Type::right) {
		setAcall(&ProgState::eventRight);
		setKey(Default::keyRight);
		setJaxis(Default::jaxisHorizontal, Default::axisDirRight);
		setGaxis(Default::gaxisHorizontal, Default::axisDirRight);
	} else if (type == Type::left) {
		setAcall(&ProgState::eventLeft);
		setKey(Default::keyLeft);
		setJaxis(Default::jaxisHorizontal, Default::axisDirLeft);
		setGaxis(Default::gaxisHorizontal, Default::axisDirLeft);
	}
}

void Binding::clearAsgKey() {
	asg &= ~ASG_KEY;
}

void Binding::setKey(SDL_Scancode KEY) {
	key = KEY;
	asg |= ASG_KEY;
}

bool Binding::jctAssigned() const {
	return asg & (ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

void Binding::clearAsgJct() {
	asg &= ~(ASG_JBUTTON | ASG_JHAT | ASG_JAXIS_P | ASG_JAXIS_N);
}

void Binding::setJbutton(uint8 BUT) {
	jctID = BUT;

	clearAsgJct();
	asg |= ASG_JBUTTON;
}

void Binding::setJaxis(uint8 AXIS, bool positive) {
	jctID = AXIS;

	clearAsgJct();
	asg |= (positive) ? ASG_JAXIS_P : ASG_JAXIS_N;
}

void Binding::setJhat(uint8 HAT, uint8 VAL) {
	jctID = HAT;
	jHatVal = VAL;

	clearAsgJct();
	asg |= ASG_JHAT;
}

void Binding::clearAsgGct() {
	asg &= ~(ASG_GBUTTON | ASG_GAXIS_P | ASG_GAXIS_N);
}

void Binding::setGbutton(uint8 BUT) {
	gctID = BUT;

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

void Binding::setGaxis(uint8 AXIS, bool positive) {
	gctID = AXIS;

	clearAsgGct();
	asg |= (positive) ? ASG_GAXIS_P : ASG_GAXIS_N;
}

void Binding::setBcall(void (ProgState::*call)()) {
	callAxis = false;
	bcall = call;
}

void Binding::setAcall(void (ProgState::*call)(float)) {
	callAxis = true;
	acall = call;
}

// SETTINGS

Settings::Settings(bool MAX, bool FSC, const vec2i& RES, const string& THM, const string& FNT, const string& LANG, int VOL, const string& LIB, const string& PST, const string& RNDR, const vec2f& SSP, int16 DDZ) :
	maximized(MAX),
	fullscreen(FSC),
	resolution(RES),
	renderer(RNDR),
	scrollSpeed(SSP)
{
	setTheme(THM);
	setFont(FNT);
	setLang(LANG);
	setDirLib(LIB);
	setDirPlist(PST);
	setVolume(VOL);
	setDeadzone(DDZ);
}

string Settings::getResolutionString() const {
	ostringstream ss;
	ss << resolution.x << ' ' << resolution.y;
	return ss.str();
}

void Settings::setResolution(const string& line) {
	vector<vec2t> elems = getWords(line);
	if (elems.size() >= 1)
		resolution.x = stoi(line.substr(elems[0].l, elems[0].u));
	if (elems.size() >= 2)
		resolution.y = stoi(line.substr(elems[1].l, elems[1].u));
}

const string& Settings::setTheme(const string& name) {
	vector<string> themes = Filer::getAvailibleThemes();
	for (string& it : themes)
		if (it == name)
			return theme = name;
	return theme = themes.empty() ? "" : themes[0];
}

string Settings::setFont(const string& newFont) {
	string path = Filer::findFont(newFont);
	TTF_Font* tmp = TTF_OpenFont(path.c_str(), Default::fontTestHeight);
	if (tmp) {
		TTF_CloseFont(tmp);
		font = newFont;
		return path;
	}
	font = Default::font;
	return Filer::findFont(font);
}

const string& Settings::setLang(const string& language) {
	return lang = (Filer::fileType(Filer::dirLangs + language + ".ini") == FTYPE_FILE) ? language : Default::language;
}

uint8 Settings::setDirLib(const string& dir) {
	return setDirectory(dirLib, dir, Filer::dirSets + Default::dirLibrary);
}

uint8 Settings::setDirPlist(const string& dir) {
	return setDirectory(dirPlist, dir, Filer::dirSets + Default::dirPlaylists);
}

uint8 Settings::setDirectory(string& dir, const string& newDir, const string& defaultDir) {
	if (Filer::fileType(newDir) != FTYPE_DIR)
		if (!Filer::mkDir(newDir)) {
			dir = defaultDir;
			if (Filer::fileType(dir) != FTYPE_DIR)
				return Filer::mkDir(dir) ? 1 : 2;
			return 1;
		}
	dir = newDir;
	return 0;
}

int Settings::getRendererIndex() {
	// get index of currently selected renderer (if name doesn't match, choose the first renderer)
	for (int i=0; i<SDL_GetNumRenderDrivers(); i++)
		if (getRendererName(i) == renderer)
			return i;

	renderer = getRendererName(0);
	return 0;
}

vector<string> Settings::getAvailibleRenderers() {
	vector<string> renderers(SDL_GetNumRenderDrivers());
	for (int i=0; i<renderers.size(); i++)
		renderers[i] = getRendererName(i);
	return renderers;
}

string Settings::getRendererName(int id) {
	SDL_RendererInfo info;
	SDL_GetRenderDriverInfo(id, &info);
	return info.name;
}

int Settings::setVolume(int vol) {
	return volume = bringIn(vol, 0, MIX_MAX_VOLUME);
}

string Settings::getScrollSpeedString() const {
	ostringstream ss;
	ss << scrollSpeed.x << ' ' << scrollSpeed.y;
	return ss.str();
}

void Settings::setScrollSpeed(const string& line) {
	vector<vec2t> elems = getWords(line);
	if (elems.size() >= 1)
		scrollSpeed.x = stof(line.substr(elems[0].l, elems[0].u));
	if (elems.size() >= 2)
		scrollSpeed.y = stof(line.substr(elems[1].l, elems[1].u));
}

void Settings::setDeadzone(int zone) {
	deadzone = bringIn(zone, 0, Default::axisLimit);
}
