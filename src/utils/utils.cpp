#include "engine/world.h"
#include <algorithm>
#include <locale>
#include <codecvt>

bool strcmpCI(const string& strl, const string& strr) {
	size_t i = 0;
	do {
		if (tolower(strl[i]) != tolower(strr[i]))
			return false;
	} while (strl[i++] != '\0');
	return true;
}

bool findChar(const string& str, char c) {
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] == c)
			return true;
	return false;
}

bool findChar(const string& str, char c, size_t& id) {
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] == c) {
			id = i;
			return true;
		}
	return false;
}

bool findString(const string& str, const string& c) {
	size_t check = 0;
	for (size_t i=0; i!=str.length(); i++) {
		if (str[i] == c[check]) {
			if (++check == c.length())
				return true;
		} else
			check = 0;
	}
	return false;
}

bool findString(const string& str, const string& c, size_t& id) {
	size_t check = 0;
	for (size_t i=0; i!=str.length(); i++) {
		if (str[i] == c[check]) {
			if (++check == c.length()) {
				id = i-c.length();
				return true;
			}
		}
		else
			check = 0;
	}
	return false;
}

bool isAbsolute(const string& path) {
	return path[0] == dsep || (path[1] == ':' && path[2] == dsep);
}

string parentPath(const string& path) {
	size_t start = (path[path.length()-1] == dsep) ? path.length()-2 : path.length()-1;
	for (size_t i=start; i!=SIZE_MAX; i--)
		if (path[i] == dsep)
			return path.substr(0, i+1);
	return path;
}

string filename(const string& path) {
	for (size_t i=path.length()-1; i!=SIZE_MAX; i--)
		if (path[i] == dsep)
			return path.substr(i+1);
	return path;
}

string getExt(const string& path) {
	for (size_t i=path.length()-1; i!=SIZE_MAX; i--)
		if (path[i] == '.')
			return path.substr(i+1);
	return "";
}

bool hasExt(const string& path, const string& ext) {
	if (path.length() < ext.length())
		return false;
	
	size_t pos = path.length() - ext.length();
	for (size_t i=0; i!=ext.length(); i++)
		if (path[pos+i] != ext[i])
			return false;
	return true;
}

string delExt(const string& path) {
	for (size_t i=path.length()-1; i!=SIZE_MAX; i--)
		if (path[i] == '.')
			return path.substr(0, i);
	return path;
}

string appendDsep(const string& path) {
	return (path[path.length()-1] == dsep) ? path : path + dsep;
}

bool isDriveLetter(const string& path) {
	return (path.length() == 2 && path[1] == ':') || (path.length() == 3 && path[1] == ':' && path[2] == dsep);
}

string modifyCase(string str, ETextCase caseChange) {
	if (caseChange == ETextCase::first_upper) {
		if (!str.empty())
			str[0] = toupper(str[0]);
	} else if (caseChange == ETextCase::all_upper)
		std::transform(str.begin(), str.end(), str.begin(), toupper);
	else if (caseChange == ETextCase::all_lower)
		std::transform(str.begin(), str.end(), str.begin(), tolower);
	return str;
}

vector<string> getWords(const string& line, char splitter) {
	// skip first splitter chars
	size_t i = 0;
	while (i != line.length() && line[i] == splitter)
		i++;
	size_t start = i;

	vector<string> words;
	for (; i<=line.length(); i++)
		if (line[i] == splitter) {	// end of word
			words.push_back(line.substr(start, i-start));

			// skip first splitter chars
			while (i != line.length() && line[i] == splitter)
				i++;
			start = i;	// new starting point for next word
		}
	return words;
}

string getBook(const string& picPath) {
	size_t start = World::library()->getSettings().libraryPath().length();
	for (size_t i=start; i!=picPath.length(); i++)
		if (picPath[i] == dsep)
			return picPath.substr(start, i-start);
	return "";
}

bool inRect(const SDL_Rect& rect, const vec2i& point) {
	return rect.w != 0 && rect.h != 0 && point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}

SDL_Rect cropRect(SDL_Rect& rect, const SDL_Rect& frame) {
	// ends of each rect and frame
	vec2i rend(rect.x + rect.w, rect.y + rect.h);
	vec2i fend(frame.x + frame.w, frame.y + frame.h);

	// crop rect if it's boundaries are out of frame
	SDL_Rect crop = {0, 0, 0, 0};
	if (rect.x < frame.x) {	// left
		crop.x = frame.x - rect.x;

		rect.x = frame.x;
		rect.w -= crop.x;
	}
	if (rend.x > fend.x) {	// right
		crop.w = rend.x - fend.x;
		rect.w -= crop.w;
	}
	if (rect.y < frame.y) {	// top
		crop.y = frame.y - rect.y;

		rect.y = frame.y;
		rect.h -= crop.y;
	}
	if (rend.y > fend.y) {	// bottom
		crop.h = rend.y - fend.y;
		rect.h -= crop.h;
	}
	// get full width and height of crop
	crop.w += crop.x;
	crop.h += crop.y;
	return crop;
}

string getRendererName(int id) {
	SDL_RendererInfo info;
	SDL_GetRenderDriverInfo(id, &info);
	return info.name;
}

vector<string> getAvailibleRenderers() {
	vector<string> renderers(SDL_GetNumRenderDrivers());
	for (int i=0; i!=renderers.size(); i++)
		renderers[i] = getRendererName(i);
	return renderers;
}

string wtos(const wstring& wstr) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().to_bytes(wstr);
}

wstring stow(const string& str) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().from_bytes(str);
}

bool stob(const string& str) {
	return str == "true";
}

string btos(bool b) {
	return b ? "true" : "false";
}

string jtHatToStr(uint8 jhat) {
	switch (jhat) {
	case SDL_HAT_CENTERED:
		return "Center";
	case SDL_HAT_UP:
		return "Up";
	case SDL_HAT_RIGHT:
		return "Right";
	case SDL_HAT_DOWN:
		return "Down";
	case SDL_HAT_LEFT:
		return "Left";
	case SDL_HAT_RIGHTUP:
		return "Right-Up";
	case SDL_HAT_RIGHTDOWN:
		return "Right-Down";
	case SDL_HAT_LEFTDOWN:
		return "Left-Down";
	case SDL_HAT_LEFTUP:
		return "Left-Up";
	}
	return "invalid";
}

uint8 jtStrToHat(string str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	if (str == "center")
		return SDL_HAT_CENTERED;
	if (str == "up")
		return SDL_HAT_UP;
	if (str == "right")
		return SDL_HAT_RIGHT;
	if (str == "down")
		return SDL_HAT_DOWN;
	if (str == "left")
		return SDL_HAT_LEFT;
	if (str == "right-up")
		return SDL_HAT_RIGHTUP;
	if (str == "right-down")
		return SDL_HAT_RIGHTDOWN;
	if (str == "left-down")
		return SDL_HAT_LEFTDOWN;
	if (str == "left-up")
		return SDL_HAT_LEFTUP;
	return 0x10;
}

string gpButtonToStr(uint8 gbutton) {
	switch (gbutton) {
	case SDL_CONTROLLER_BUTTON_A:
		return "A";
	case SDL_CONTROLLER_BUTTON_B:
		return "B";
	case SDL_CONTROLLER_BUTTON_X:
		return "X";
	case SDL_CONTROLLER_BUTTON_Y:
		return "Y";
	case SDL_CONTROLLER_BUTTON_BACK:
		return "Back";
	case SDL_CONTROLLER_BUTTON_GUIDE:
		return "Guide";
	case SDL_CONTROLLER_BUTTON_START:
		return "Start";
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return "LS";
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return "RS";
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return "LB";
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return "RB";
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return "Up";
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return "Down";
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return "Left";
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return "Right";
	}
	return "invalid";
}

uint8 gpStrToButton(string str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	if (str == "a")
		return SDL_CONTROLLER_BUTTON_A;
	if (str == "b")
		return SDL_CONTROLLER_BUTTON_B;
	if (str == "x")
		return SDL_CONTROLLER_BUTTON_X;
	if (str == "y")
		return SDL_CONTROLLER_BUTTON_Y;
	if (str == "back")
		return SDL_CONTROLLER_BUTTON_BACK;
	if (str == "guide")
		return SDL_CONTROLLER_BUTTON_GUIDE;
	if (str == "start")
		return SDL_CONTROLLER_BUTTON_START;
	if (str == "ls")
		return SDL_CONTROLLER_BUTTON_LEFTSTICK;
	if (str == "rs")
		return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
	if (str == "lb")
		return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
	if (str == "rb")
		return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
	if (str == "up")
		return SDL_CONTROLLER_BUTTON_DPAD_UP;
	if (str == "down")
		return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
	if (str == "left")
		return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	if (str == "right")
		return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	return SDL_CONTROLLER_BUTTON_MAX;
}

string gpAxisToStr(uint8 gaxis) {
	switch (gaxis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		return "LX";
	case SDL_CONTROLLER_AXIS_LEFTY:
		return "LY";
	case SDL_CONTROLLER_AXIS_RIGHTX:
		return "RX";
	case SDL_CONTROLLER_AXIS_RIGHTY:
		return "RY";
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		return "LT";
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		return "RT";
	}
	return "invalid";
}

uint8 gpStrToAxis(string str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	if (str == "lx")
		return SDL_CONTROLLER_AXIS_LEFTX;
	if (str == "ly")
		return SDL_CONTROLLER_AXIS_LEFTY;
	if (str == "rx")
		return SDL_CONTROLLER_AXIS_RIGHTX;
	if (str == "ry")
		return SDL_CONTROLLER_AXIS_RIGHTY;
	if (str == "lt")
		return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
	if (str == "rt")
		return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
	return SDL_CONTROLLER_AXIS_MAX;
}

vec2i pix(const vec2f& p) {
	vec2i res = World::winSys()->resolution();
	return vec2i(p.x * res.x, p.y *res.y);
}

int pixX(float p) {
	return p * World::winSys()->resolution().x;
}

int pixY(float p) {
	return p * World::winSys()->resolution().y;
}

vec2f prc(const vec2i& p) {
	vec2i res = World::winSys()->resolution();
	return vec2f(float(p.x) / float(res.x), float(p.y) / float(res.y));
}

float prcX(int p) {
	return float(p) / float(World::winSys()->resolution().x);
}

float prcY(int p) {
	return float(p) / float(World::winSys()->resolution().y);
}

float axisToFloat(int16 axisValue) {
	return float(axisValue) / float(32768);
}

int16 floatToAxis(float axisValue) {
	return axisValue * float(32768);
}
