#include "engine/world.h"

// DIRECTION

const array<string, Direction::names.size()> Direction::names = {
	"Up",
	"Down",
	"Left",
	"Right"
};

// BINDING

const array<string, Binding::names.size()> Binding::names = {
	"enter",
	"escape",
	"up",
	"down",
	"left",
	"right",
	"scroll up",
	"scroll down",
	"scroll left",
	"scroll right",
	"cursor up",
	"cursor down",
	"cursor left",
	"cursor right",
	"center view",
	"scroll fast",
	"scroll slow",
	"next page",
	"prev page",
	"zoom in",
	"zoom out",
	"zoom reset",
	"to start",
	"to end",
	"next directory",
	"prev directory",
	"fullscreen",
	"show hidden",
	"boss",
	"refresh"
};

void Binding::setDefaultSelf(Type type) {
	switch (type) {
	case Type::enter:
		setBcall(&ProgState::eventEnter);
		setKey(SDL_SCANCODE_RETURN);
		setJbutton(2);
		setGbutton(SDL_CONTROLLER_BUTTON_A);
		break;
	case Type::escape:
		setBcall(&ProgState::eventEscape);
		setKey(SDL_SCANCODE_ESCAPE);
		setJbutton(1);
		setGbutton(SDL_CONTROLLER_BUTTON_B);
		break;
	case Type::up:
		setBcall(&ProgState::eventUp);
		setKey(SDL_SCANCODE_UP);
		setJhat(0, SDL_HAT_UP);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_UP);
		break;
	case Type::down:
		setBcall(&ProgState::eventDown);
		setKey(SDL_SCANCODE_DOWN);
		setJhat(0, SDL_HAT_DOWN);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		break;
	case Type::left:
		setBcall(&ProgState::eventLeft);
		setKey(SDL_SCANCODE_LEFT);
		setJhat(0, SDL_HAT_LEFT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		break;
	case Type::right:
		setBcall(&ProgState::eventRight);
		setKey(SDL_SCANCODE_RIGHT);
		setJhat(0, SDL_HAT_RIGHT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		break;
	case Type::scrollUp:
		setAcall(&ProgState::eventScrollUp);
		setKey(SDL_SCANCODE_UP);
		setJaxis(1, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, false);
		break;
	case Type::scrollDown:
		setAcall(&ProgState::eventScrollDown);
		setKey(SDL_SCANCODE_DOWN);
		setJaxis(1, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, true);
		break;
	case Type::scrollLeft:
		setAcall(&ProgState::eventScrollLeft);
		setKey(SDL_SCANCODE_LEFT);
		setJaxis(0, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, false);
		break;
	case Type::scrollRight:
		setAcall(&ProgState::eventScrollRight);
		setKey(SDL_SCANCODE_RIGHT);
		setJaxis(0, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, true);
		break;
	case Type::cursorUp:
		setAcall(&ProgState::eventCursorUp);
		setJaxis(2, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, false);
		break;
	case Type::cursorDown:
		setAcall(&ProgState::eventCursorDown);
		setJaxis(2, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, true);
		break;
	case Type::cursorLeft:
		setAcall(&ProgState::eventCursorLeft);
		setJaxis(3, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, false);
		break;
	case Type::cursorRight:
		setAcall(&ProgState::eventCursorRight);
		setJaxis(3, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, true);
		break;
	case Type::centerView:
		setBcall(&ProgState::eventCenterView);
		setKey(SDL_SCANCODE_C);
		setJbutton(10);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSTICK);
		break;
	case Type::scrollFast:
		setAcall(nullptr);
		setKey(SDL_SCANCODE_X);
		setJbutton(0);
		setGbutton(SDL_CONTROLLER_BUTTON_Y);
		break;
	case Type::scrollSlow:
		setAcall(nullptr);
		setKey(SDL_SCANCODE_Z);
		setJbutton(3);
		setGbutton(SDL_CONTROLLER_BUTTON_X);
		break;
	case Type::nextPage:
		setBcall(&ProgState::eventNextPage);
		setKey(SDL_SCANCODE_PAGEDOWN);
		break;
	case Type::prevPage:
		setBcall(&ProgState::eventPrevPage);
		setKey(SDL_SCANCODE_PAGEUP);
		break;
	case Type::zoomIn:
		setBcall(&ProgState::eventZoomIn);
		setKey(SDL_SCANCODE_W);
		setJbutton(5);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		break;
	case Type::zoomOut:
		setBcall(&ProgState::eventZoomOut);
		setKey(SDL_SCANCODE_S);
		setJbutton(4);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		break;
	case Type::zoomReset:
		setBcall(&ProgState::eventZoomReset);
		setKey(SDL_SCANCODE_R);
		setJbutton(11);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
		break;
	case Type::toStart:
		setBcall(&ProgState::eventToStart);
		setKey(SDL_SCANCODE_HOME);
		break;
	case Type::toEnd:
		setBcall(&ProgState::eventToEnd);
		setKey(SDL_SCANCODE_END);
		break;
	case Type::nextDir:
		setBcall(&ProgState::eventNextDir);
		setKey(SDL_SCANCODE_D);
		setJbutton(7);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, true);
		break;
	case Type::prevDir:
		setBcall(&ProgState::eventPrevDir);
		setKey(SDL_SCANCODE_A);
		setJbutton(6);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, true);
		break;
	case Type::fullscreen:
		setBcall(&ProgState::eventFullscreen);
		setKey(SDL_SCANCODE_F);
		setJbutton(8);
		setGbutton(SDL_CONTROLLER_BUTTON_BACK);
		break;
	case Type::hide:
		setBcall(&ProgState::eventHide);
		setKey(SDL_SCANCODE_H);
		break;
	case Type::boss:
		setBcall(&ProgState::eventBoss);
		setKey(SDL_SCANCODE_B);
		break;
	case Type::refresh:
		setBcall(&ProgState::eventRefresh);
		setKey(SDL_SCANCODE_F5);
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

// PICTURE LIMIT

const array<string, PicLim::names.size()> PicLim::names = {
	"none",
	"count",
	"size"
};

PicLim::PicLim(Type type, uptrt count) :
	type(type),
	count(count),
	size(defaultSize())
{}

void PicLim::set(const string& str) {
	vector<string> elems = getWords(str);
	type = elems.size() > 0 ? strToEnum<Type>(names, elems[0]) : Type::none;
	count = elems.size() > 1 ? toCount(elems[1]) : defaultCount;
	size = elems.size() > 2 ? toSize(elems[2]) : defaultSize();
}

uptrt PicLim::toCount(const string& str) {
	uptrt cnt = uptrt(sstoull(str));
	return cnt ? cnt : defaultCount;
}

uptrt PicLim::toSize(const string& str) {
	const char* pos;
	char* end;
	for (pos = str.c_str(); notDigit(*pos) && *pos; pos++);
	uptrt num = uptrt(strtoull(pos, &end, 0));
	if (!num)
		return defaultSize();

	string::const_iterator mit = std::find_if(str.begin() + pdift(end - str.c_str()), str.end(), [](char c) -> bool { return std::find(sizeLetters.begin(), sizeLetters.end(), toupper(c)) != sizeLetters.end(); });
	if (mit != str.end())
		switch (toupper(*mit)) {
		case sizeLetters[1]:
			num *= sizeFactors[1];
			break;
		case sizeLetters[2]:
			num *= sizeFactors[2];
			break;
		case sizeLetters[3]:
			num *= sizeFactors[3];
		}
	mit = std::find_if(mit, str.end(), [](char c) -> bool { return toupper(c) == sizeLetters[0]; });
	return mit == str.end() || *mit != 'b' ? num : num /= 8;
}

// SETTINGS

Settings::Settings() :
	maximized(false),
	fullscreen(false),
	showHidden(true),
	direction(Direction::down),
	zoom(defaultZoom),
	spacing(defaultSpacing),
	resolution(800, 600),
	renderer(emptyStr),
	scrollSpeed(1600.f, 1600.f),
	deadzone(256),
	font(defaultFont),
	dirLib(World::fileSys()->getDirSets() + defaultDirLib)
{
	setTheme(emptyStr);
}

const string& Settings::setTheme(const string& name) {
	vector<string> themes = World::fileSys()->getAvailibleThemes();
	if (vector<string>::iterator it = std::find(themes.begin(), themes.end(), name); it != themes.end())
		return theme = name;
	return theme = themes.empty() ? emptyStr : themes[0];
}

const string& Settings::setFont(const string& newFont) {
	string path = World::fileSys()->findFont(newFont);
	return font = FileSys::isFont(path) ? newFont : defaultFont;
}

const string& Settings::setDirLib(const string& drc) {
	dirLib = drc;
	if (FileSys::fileType(dirLib) != FTYPE_DIR && !FileSys::createDir(dirLib))
		if (dirLib = World::fileSys()->getDirSets() + defaultDirLib; FileSys::fileType(dirLib) != FTYPE_DIR)
			FileSys::createDir(dirLib);
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
