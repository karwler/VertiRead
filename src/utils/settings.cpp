#include "prog/progs.h"

// BINDING

void Binding::reset(Type newType) {
	switch (asg = ASG_NONE; type = newType) {
	case Type::enter:
		bcall = &ProgState::eventEnter;
		setKey(SDL_SCANCODE_RETURN);
		setJbutton(2);
		setGbutton(SDL_CONTROLLER_BUTTON_A);
		break;
	case Type::escape:
		bcall = &ProgState::eventEscape;
		setKey(SDL_SCANCODE_ESCAPE);
		setJbutton(1);
		setGbutton(SDL_CONTROLLER_BUTTON_B);
		break;
	case Type::up:
		bcall = &ProgState::eventUp;
		setKey(SDL_SCANCODE_UP);
		setJhat(0, SDL_HAT_UP);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_UP);
		break;
	case Type::down:
		bcall = &ProgState::eventDown;
		setKey(SDL_SCANCODE_DOWN);
		setJhat(0, SDL_HAT_DOWN);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		break;
	case Type::left:
		bcall = &ProgState::eventLeft;
		setKey(SDL_SCANCODE_LEFT);
		setJhat(0, SDL_HAT_LEFT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		break;
	case Type::right:
		bcall = &ProgState::eventRight;
		setKey(SDL_SCANCODE_RIGHT);
		setJhat(0, SDL_HAT_RIGHT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		break;
	case Type::centerView:
		bcall = &ProgState::eventCenterView;
		setKey(SDL_SCANCODE_C);
		setJbutton(10);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSTICK);
		break;
	case Type::nextPage:
		bcall = &ProgState::eventNextPage;
		setKey(SDL_SCANCODE_PAGEDOWN);
		break;
	case Type::prevPage:
		bcall = &ProgState::eventPrevPage;
		setKey(SDL_SCANCODE_PAGEUP);
		break;
	case Type::zoomIn:
		bcall = &ProgState::eventZoomIn;
		setKey(SDL_SCANCODE_W);
		setJbutton(5);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		break;
	case Type::zoomOut:
		bcall = &ProgState::eventZoomOut;
		setKey(SDL_SCANCODE_S);
		setJbutton(4);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		break;
	case Type::zoomReset:
		bcall = &ProgState::eventZoomReset;
		setKey(SDL_SCANCODE_R);
		setJbutton(11);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
		break;
	case Type::toStart:
		bcall = &ProgState::eventToStart;
		setKey(SDL_SCANCODE_HOME);
		break;
	case Type::toEnd:
		bcall = &ProgState::eventToEnd;
		setKey(SDL_SCANCODE_END);
		break;
	case Type::nextDir:
		bcall = &ProgState::eventNextDir;
		setKey(SDL_SCANCODE_D);
		setJbutton(7);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, true);
		break;
	case Type::prevDir:
		bcall = &ProgState::eventPrevDir;
		setKey(SDL_SCANCODE_A);
		setJbutton(6);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, true);
		break;
	case Type::fullscreen:
		bcall = &ProgState::eventFullscreen;
		setKey(SDL_SCANCODE_F);
		setJbutton(8);
		setGbutton(SDL_CONTROLLER_BUTTON_BACK);
		break;
	case Type::multiFullscreen:
		bcall = &ProgState::eventMultiFullscreen;
		setKey(SDL_SCANCODE_G);
		setJbutton(9);
		setGbutton(SDL_CONTROLLER_BUTTON_START);
		break;
	case Type::hide:
		bcall = &ProgState::eventHide;
		setKey(SDL_SCANCODE_H);
		break;
	case Type::boss:
		bcall = &ProgState::eventBoss;
		setKey(SDL_SCANCODE_B);
		break;
	case Type::refresh:
		bcall = &ProgState::eventRefresh;
		setKey(SDL_SCANCODE_F5);
		break;
	case Type::scrollUp:
		acall = &ProgState::eventScrollUp;
		setKey(SDL_SCANCODE_UP);
		setJaxis(1, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, false);
		break;
	case Type::scrollDown:
		acall = &ProgState::eventScrollDown;
		setKey(SDL_SCANCODE_DOWN);
		setJaxis(1, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, true);
		break;
	case Type::scrollLeft:
		acall = &ProgState::eventScrollLeft;
		setKey(SDL_SCANCODE_LEFT);
		setJaxis(0, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, false);
		break;
	case Type::scrollRight:
		acall = &ProgState::eventScrollRight;
		setKey(SDL_SCANCODE_RIGHT);
		setJaxis(0, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, true);
		break;
	case Type::cursorUp:
		acall = &ProgState::eventCursorUp;
		setJaxis(2, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, false);
		break;
	case Type::cursorDown:
		acall = &ProgState::eventCursorDown;
		setJaxis(2, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, true);
		break;
	case Type::cursorLeft:
		acall = &ProgState::eventCursorLeft;
		setJaxis(3, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, false);
		break;
	case Type::cursorRight:
		acall = &ProgState::eventCursorRight;
		setJaxis(3, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, true);
		break;
	case Type::scrollFast:
		acall = nullptr;
		setKey(SDL_SCANCODE_X);
		setJbutton(0);
		setGbutton(SDL_CONTROLLER_BUTTON_Y);
		break;
	case Type::scrollSlow:
		acall = nullptr;
		setKey(SDL_SCANCODE_Z);
		setJbutton(3);
		setGbutton(SDL_CONTROLLER_BUTTON_X);
		break;
	default:
		throw std::runtime_error("Invalid binding type: " + toStr(type));
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
	gctID = but;

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

void Binding::setGaxis(SDL_GameControllerAxis axis, bool positive) {
	gctID = axis;

	clearAsgGct();
	asg |= positive ? ASG_GAXIS_P : ASG_GAXIS_N;
}

// PICTURE LIMIT

PicLim::PicLim(Type ltype, uintptr_t cnt) :
	count(cnt),
	size(defaultSize()),
	type(ltype)
{}

void PicLim::set(string_view str) {
	vector<string_view> elems = getWords(str);
	type = !elems.empty() ? strToEnum(names, elems[0], Type::none) : Type::none;
	count = elems.size() > 1 ? toCount(elems[1]) : defaultCount;
	size = elems.size() > 2 ? toSize(elems[2]) : defaultSize();
}

uintptr_t PicLim::toCount(string_view str) {
	uintptr_t cnt = toNum<uintptr_t>(str);
	return cnt ? cnt : defaultCount;
}

uintptr_t PicLim::toSize(string_view str) {
	size_t i = 0;
	for (; i < str.length() && isSpace(str[i]); ++i);
	uintptr_t num = 0;
	std::from_chars_result res = std::from_chars(str.data() + i, str.data() + str.length(), num);
	if (!num)
		return defaultSize();

	string_view::iterator mit = std::find_if(str.begin() + (res.ptr - str.data()), str.end(), [](char c) -> bool { return std::find(sizeLetters.begin(), sizeLetters.end(), toupper(c)) != sizeLetters.end(); });
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
	return mit == str.end() || *mit != 'b' ? num : num / 8;
}

uint8 PicLim::memSizeMag(uintptr_t num) {
	uint8 m;
	for (m = 0; m + 1u < sizeLetters.size() && (!(num % 1000) && (num /= 1000)); ++m);
	return m;
}

string PicLim::memoryString(uintptr_t num, uint8 mag) {
	string str = toStr(num / sizeFactors[mag]);
	return (mag ? str + sizeLetters[mag] : str) + sizeLetters[0];
}

// SETTINGS

Settings::Settings(const fs::path& dirSets, vector<string>&& themes) :
	dirLib(dirSets / defaultDirLib)
{
	setTheme(string_view(), std::move(themes));
}

const string& Settings::setTheme(string_view name, vector<string>&& themes) {
	if (vector<string>::const_iterator it = std::find(themes.begin(), themes.end(), name); it != themes.end())
		return theme = name;
	return theme = themes.empty() ? string() : std::move(themes[0]);
}

const fs::path& Settings::setDirLib(const fs::path& drc, const fs::path& dirSets) {
	try {
		if (dirLib = drc; !fs::is_directory(dirLib) && !fs::create_directories(dirLib))
			if (dirLib = dirSets / defaultDirLib; !fs::is_directory(dirLib))
				fs::create_directories(dirLib);
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
		dirLib = dirSets / defaultDirLib;
	}
	return dirLib;
}

umap<int, Recti> Settings::displayArrangement() {
	ivec2 origin(INT_MAX);
	umap<int, Recti> dsps;
	for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i)
		if (Recti rect; !SDL_GetDisplayBounds(i, reinterpret_cast<SDL_Rect*>(&rect))) {
			dsps.emplace(i, rect);
			origin = glm::min(origin, rect.pos());
		}
	for (auto& [id, rect] : dsps)
		rect.pos() -= origin;
	return dsps;
}

void Settings::unionDisplays() {
	umap<int, Recti> dsps = displayArrangement();
	for (umap<int, Recti>::iterator it = displays.begin(); it != displays.end(); ++it)
		if (!dsps.count(it->first))
			displays.erase(it);
	for (umap<int, Recti>::const_iterator ds = dsps.begin(); ds != dsps.end(); ++ds)
		if (umap<int, Recti>::iterator it = displays.find(ds->first); it != displays.end() && it->second.size() != ds->second.size())
			displays.erase(it);
	if (displays.empty())
		displays = std::move(dsps);
}
