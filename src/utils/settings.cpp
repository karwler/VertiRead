#include "prog/progs.h"
#include <format>
#include <regex>

// BINDING

void Binding::reset(Type newType) {
	switch (asg = ASG_NONE; type = newType) {
	using enum Type;
	case enter:
		bcall = &ProgState::eventEnter;
		setKey(SDL_SCANCODE_RETURN);
		setJbutton(2);
		setGbutton(SDL_CONTROLLER_BUTTON_A);
		break;
	case escape:
		bcall = &ProgState::eventEscape;
		setKey(SDL_SCANCODE_ESCAPE);
		setJbutton(1);
		setGbutton(SDL_CONTROLLER_BUTTON_B);
		break;
	case up:
		bcall = &ProgState::eventUp;
		setKey(SDL_SCANCODE_UP);
		setJhat(0, SDL_HAT_UP);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_UP);
		break;
	case down:
		bcall = &ProgState::eventDown;
		setKey(SDL_SCANCODE_DOWN);
		setJhat(0, SDL_HAT_DOWN);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		break;
	case left:
		bcall = &ProgState::eventLeft;
		setKey(SDL_SCANCODE_LEFT);
		setJhat(0, SDL_HAT_LEFT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		break;
	case right:
		bcall = &ProgState::eventRight;
		setKey(SDL_SCANCODE_RIGHT);
		setJhat(0, SDL_HAT_RIGHT);
		setGbutton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		break;
	case centerView:
		bcall = &ProgState::eventCenterView;
		setKey(SDL_SCANCODE_C);
		setJbutton(10);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSTICK);
		break;
	case nextPage:
		bcall = &ProgState::eventNextPage;
		setKey(SDL_SCANCODE_PAGEDOWN);
		break;
	case prevPage:
		bcall = &ProgState::eventPrevPage;
		setKey(SDL_SCANCODE_PAGEUP);
		break;
	case zoomIn:
		bcall = &ProgState::eventZoomIn;
		setKey(SDL_SCANCODE_W);
		setJbutton(5);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		break;
	case zoomOut:
		bcall = &ProgState::eventZoomOut;
		setKey(SDL_SCANCODE_S);
		setJbutton(4);
		setGbutton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		break;
	case zoomReset:
		bcall = &ProgState::eventZoomReset;
		setKey(SDL_SCANCODE_R);
		setJbutton(11);
		setGbutton(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
		break;
	case zoomFit:
		bcall = &ProgState::eventZoomFit;
		setKey(SDL_SCANCODE_T);
		break;
	case toStart:
		bcall = &ProgState::eventToStart;
		setKey(SDL_SCANCODE_HOME);
		break;
	case toEnd:
		bcall = &ProgState::eventToEnd;
		setKey(SDL_SCANCODE_END);
		break;
	case nextDir:
		bcall = &ProgState::eventNextDir;
		setKey(SDL_SCANCODE_D);
		setJbutton(7);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, true);
		break;
	case prevDir:
		bcall = &ProgState::eventPrevDir;
		setKey(SDL_SCANCODE_A);
		setJbutton(6);
		setGaxis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, true);
		break;
	case fullscreen:
		bcall = &ProgState::eventFullscreen;
		setKey(SDL_SCANCODE_F);
		setJbutton(8);
		setGbutton(SDL_CONTROLLER_BUTTON_BACK);
		break;
	case multiFullscreen:
		bcall = &ProgState::eventMultiFullscreen;
		setKey(SDL_SCANCODE_G);
		setJbutton(9);
		setGbutton(SDL_CONTROLLER_BUTTON_START);
		break;
	case hide:
		bcall = &ProgState::eventHide;
		setKey(SDL_SCANCODE_H);
		break;
	case boss:
		bcall = &ProgState::eventBoss;
		setKey(SDL_SCANCODE_B);
		break;
	case refresh:
		bcall = &ProgState::eventRefresh;
		setKey(SDL_SCANCODE_F5);
		break;
	case scrollUp:
		acall = &ProgState::eventScrollUp;
		setKey(SDL_SCANCODE_UP);
		setJaxis(1, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, false);
		break;
	case scrollDown:
		acall = &ProgState::eventScrollDown;
		setKey(SDL_SCANCODE_DOWN);
		setJaxis(1, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTY, true);
		break;
	case scrollLeft:
		acall = &ProgState::eventScrollLeft;
		setKey(SDL_SCANCODE_LEFT);
		setJaxis(0, false);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, false);
		break;
	case scrollRight:
		acall = &ProgState::eventScrollRight;
		setKey(SDL_SCANCODE_RIGHT);
		setJaxis(0, true);
		setGaxis(SDL_CONTROLLER_AXIS_LEFTX, true);
		break;
	case cursorUp:
		acall = &ProgState::eventCursorUp;
		setJaxis(2, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, false);
		break;
	case cursorDown:
		acall = &ProgState::eventCursorDown;
		setJaxis(2, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTY, true);
		break;
	case cursorLeft:
		acall = &ProgState::eventCursorLeft;
		setJaxis(3, false);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, false);
		break;
	case cursorRight:
		acall = &ProgState::eventCursorRight;
		setJaxis(3, true);
		setGaxis(SDL_CONTROLLER_AXIS_RIGHTX, true);
		break;
	case scrollFast:
		acall = nullptr;
		setKey(SDL_SCANCODE_X);
		setJbutton(0);
		setGbutton(SDL_CONTROLLER_BUTTON_Y);
		break;
	case scrollSlow:
		acall = nullptr;
		setKey(SDL_SCANCODE_Z);
		setJbutton(3);
		setGbutton(SDL_CONTROLLER_BUTTON_X);
		break;
	default:
		throw std::runtime_error(std::format("Invalid binding type: {}", uint(type)));
	}
}

void Binding::setKey(SDL_Scancode kkey) noexcept {
	key = kkey;
	asg |= ASG_KEY;
}

void Binding::setJbutton(uint8 but) noexcept {
	jctID = but;

	clearAsgJct();
	asg |= ASG_JBUTTON;
}

void Binding::setJaxis(uint8 axis, bool positive) noexcept {
	jctID = axis;

	clearAsgJct();
	asg |= positive ? ASG_JAXIS_P : ASG_JAXIS_N;
}

void Binding::setJhat(uint8 hat, uint8 val) noexcept {
	jctID = hat;
	jHatVal = val;

	clearAsgJct();
	asg |= ASG_JHAT;
}

void Binding::setGbutton(SDL_GameControllerButton but) noexcept {
	gctID = but;

	clearAsgGct();
	asg |= ASG_GBUTTON;
}

void Binding::setGaxis(SDL_GameControllerAxis axis, bool positive) noexcept {
	gctID = axis;

	clearAsgGct();
	asg |= positive ? ASG_GAXIS_P : ASG_GAXIS_N;
}

uint8 Binding::hatNameToValue(string_view name) noexcept {
	for (size_t i = 0; i < hatNames.size(); ++i)
		if (strciequal(hatNames[i], name))
			return hatValues[i];
	return SDL_HAT_CENTERED;
}

const char* Binding::hatValueToName(uint8 val) noexcept {
	for (size_t i = 0; i < hatValues.size(); ++i)
		if (hatValues[i] == val)
			return hatNames[i];
	return "Center";
}

// PICTURE LIMIT

void PicLim::set(string_view str) {
	size_t p, e;
	for (p = 0; p < str.length() && isSpace(str[p]); ++p);
	for (e = p; e < str.length() && notSpace(str[e]); ++e);
	type = strToEnum(names, str.substr(p, e - p), Type::none);
	for (p = e; p < str.length() && isSpace(str[p]); ++p);
	for (e = p; e < str.length() && notSpace(str[e]); ++e);
	count = toCount(str.substr(p, e - p));
	size = toSize(str.substr(e));
}

uintptr_t PicLim::toSize(string_view str) {
	uintptr_t num = 0;
	const char* end = str.data() + str.length();
	std::from_chars_result res = std::from_chars(std::find_if(str.data(), end, [](char ch) -> bool { return notSpace(ch); }), end, num);
	if (!num)
		return 0;

	const char* beg = std::find_if(res.ptr, end, [](char ch) -> bool { return notSpace(ch); });
	const char* fin = std::find_if(beg, end, [](char ch) -> bool { return !isalpha(ch); });
	if (string_view unit(beg, fin); (unit.length() == 2 || unit.length() == 3) && toupper(unit.back()) == 'B') {
		constexpr array letters = { 'K', 'M', 'G' };
		for (uint i = 0; i < letters.size(); ++i)
			if (toupper(unit[0]) == letters[i])
				return num * uintptr_t(std::pow(unit.length() == 3 && toupper(unit[1]) == 'I' ? 1024u : 1000u, i + 1));
	}
	return num;
}

pair<uint8, uint8> PicLim::memSizeMag(uintptr_t num) {
	if (!num)
		return pair(0, 0);

	uint8 m;
	for (m = 0; m < 3 && num % 1000 == 0; num /= 1000, ++m);
	if (m)
		return pair(m, 0);
	for (m = 0; m < 3 && num % 1024 == 0; num /= 1024, ++m);
	return pair(0, m);
}

string PicLim::memoryString(uintptr_t num, uint8 dmag, uint8 smag) {
	if (!(dmag || smag))
		return std::format("{} B", num);
	return dmag
		? std::format("{} {}", num / uintptr_t(std::pow(1000u, dmag)), array{ "KB", "MB", "GB" }[dmag - 1])
		: std::format("{} {}", num / uintptr_t(std::pow(1024u, smag)), array{ "KiB", "MiB", "GiB" }[smag - 1]);
}

string PicLim::memoryString(uintptr_t num) {
	auto [dmag, smag] = memSizeMag(num);
	return memoryString(num, dmag, smag);
}

// SETTINGS

Settings::Settings(const fs::path& dirSets, vector<string>&& themes) :
	dirLib(fromPath(dirSets / defaultDirLib))
{
	setTheme(string_view(), std::move(themes));
}

void Settings::setZoom(string_view str) {
	size_t p, e;
	for (p = 0; p < str.length() && isSpace(str[p]); ++p);
	for (e = p; e < str.length() && notSpace(str[e]); ++e);
	zoomType = strToEnum(zoomNames, string_view(str.data() + p, e - p), defaultZoomType);
	zoom = std::clamp(toNum<int8>(str.substr(e)), int8(-Settings::zoomLimit), Settings::zoomLimit);
}

const string& Settings::setTheme(string_view name, vector<string>&& themes) {
	if (vector<string>::const_iterator it = rng::find(themes, name); it != themes.end())
		return theme = name;
	return theme = themes.empty() ? string() : std::move(themes[0]);
}

umap<int, Recti> Settings::displayArrangement() {
	ivec2 origin(INT_MAX);
	umap<int, Recti> dsps;
	for (int i = 0, e = SDL_GetNumVideoDisplays(); i < e; ++i)
		if (Recti rect; !SDL_GetDisplayBounds(i, &rect.asRect())) {
			dsps.emplace(i, rect);
			origin = glm::min(origin, rect.pos());
		}
	for (auto& [id, rect] : dsps)
		rect.pos() -= origin;
	return dsps;
}

void Settings::unionDisplays() {
	umap<int, Recti> dsps = displayArrangement();
	vector<int> invalids;
	for (const auto& [id, rect] : displays)
		if (umap<int, Recti>::iterator it = dsps.find(id); it == dsps.end() || it->second.size() != rect.size())
			invalids.push_back(id);

	if (invalids.size() < displays.size()) {
		for (int id : invalids)
			displays.erase(id);
	} else
		displays = std::move(dsps);
}

Settings::Renderer Settings::getRenderer(string_view name) {
#ifdef WITH_DIRECT3D
	if (std::regex_match(name.begin(), name.end(), std::regex(R"r(\s*d(irect)?(3d|x).*)r", std::regex::icase)))
		return Renderer::direct3d11;
#endif
#ifdef WITH_OPENGL
	std::match_results<string_view::iterator> mr;
	if (std::regex_match(name.begin(), name.end(), mr, std::regex(R"r(\s*(open)?gl[^a-z]*?(es[^a-z]*?)?(\d).*)r", std::regex::icase))) {
		if (mr.length(2))
			return Renderer::opengles3;
		return name[mr.position(3)] >= '3' ? Renderer::opengl3 : Renderer::opengl1;
	}
#endif
#ifdef WITH_VULKAN
	if (std::regex_match(name.begin(), name.end(), std::regex(R"r(\s*v(ulkan|k).*)r", std::regex::icase)))
		return Renderer::vulkan;
#endif
	return Renderer::software;
}

void Settings::setRenderer(const uset<string>& cmdFlags) {
#ifdef WITH_DIRECT3D
	if (cmdFlags.contains(flagDirect3d11))
		renderer = Renderer::direct3d11;
	else
#endif
#ifdef WITH_OPENGL
	if (cmdFlags.contains(flagOpenGl1))
		renderer = Renderer::opengl1;
	else if (cmdFlags.contains(flagOpenGl3))
		renderer = Renderer::opengl3;
	else if (cmdFlags.contains(flagOpenEs3))
		renderer = Renderer::opengles3;
	else
#endif
#ifdef WITH_VULKAN
	if (cmdFlags.contains(flagVulkan))
		renderer = Renderer::vulkan;
	else
#endif
	if (cmdFlags.contains(flagSoftware))
		renderer = Renderer::software;
}
