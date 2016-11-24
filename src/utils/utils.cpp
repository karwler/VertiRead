#include "engine/world.h"
#include <algorithm>
#include <cctype>

ullong NSC_GetNextNum(const string& str, size_t start, size_t& length) {
	size_t i = start;
	while (i != str.size() && isdigit(str[i]))
		i++;

	length = i - start;
	return stoull(str.substr(start, length));
}

bool numStrCompare(const string& sa, const string& sb) {
	size_t ia = 0, ib = 0;
	while (ia != sa.size() && ib != sb.size()) {
		if (isdigit(sa[ia]) && isdigit(sb[ib])) {
			size_t la, lb;
			ullong na = NSC_GetNextNum(sa, ia, la);
			ullong nb = NSC_GetNextNum(sb, ib, lb);

			if (na != nb)
				return na < nb;
			ia += la;
			ib += lb;
		}
		else if (sa[ia] != sb[ib])
			return sa[ia] <= sb[ib];
		else {
			ia++;
			ib++;
		}
	}
	return sa.size() <= sb.size();
}

bool strcmpCI(const string& strl, const string& strr) {
	if (strl.length() != strr.length())
		return false;

	for (size_t i=0; i!=strl.length(); i++)
		if (tolower(strl[i]) != tolower(strr[i]))
			return false;
	return true;
}

bool findChar(const string& str, char c, size_t* id) {
	for (size_t i=0; i!=str.length(); i++)
		if (str[i] == c) {
			if (id)
				*id = i;
			return true;
		}
	return false;
}

bool findString(const string& str, const string& c, size_t* id) {
	size_t check = 0;
	for (size_t i=0; i!=str.length(); i++) {
		if (str[i] == c[check]) {
			if (++check == c.length()) {
				if (id)
					*id = i-c.length();
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
	switch (caseChange) {
	case ETextCase::first_upper:
		if (!str.empty())
			str[0] = toupper(str[0]);
		break;
	case ETextCase::all_upper:
		std::transform(str.begin(), str.end(), str.begin(), toupper);
		break;
	case ETextCase::all_lower:
		std::transform(str.begin(), str.end(), str.begin(), tolower);
	}
	return str;
}

vector<string> getWords(const string& line, char splitter, char spacer) {
	vector<std::string> words;

	size_t i = 0;
	while (i != line.length() && line[i] == spacer)
		i++;
	size_t start = i;
	size_t end = line.length();

	for (; i<=line.length(); i++) {
		if (line[i] == splitter || i == line.length()) {
			if (end > i)
				end = i;

			if (start < end)
				words.push_back(line.substr(start, end-start));

			while (i != line.length() && (line[i] == spacer || line[i] == splitter))
				i++;
			start = i;
			end = line.length();
		}
		else if (line[i] == spacer) {
			if (end > i)
				end = i;
		}
		else
			end = line.length();
	}
	return words;
}

bool splitIniLine(const string& line, string* arg, string* val, string* key, bool* isTitle, size_t* id) {
	if (line[0] == '[' && line[line.length()-1] == ']') {
		if (arg)
			*arg = line.substr(1, line.length()-2);
		if (isTitle)
			*isTitle = true;
		return true;
	}
	
	size_t i0;;
	if (!findChar(line, '=', &i0))
		return false;

	if (val)
		*val = line.substr(i0 + 1);
	string left = line.substr(0, i0);

	size_t i1 = 0, i2 = 0;
	findChar(left, '[', &i1);
	findChar(left, ']', &i2);
	if (i1 < i2) {
		if (arg)
			*arg = line.substr(0, i1);
		if (key)
			*key = line.substr(i1+1, i2-i1-1);
	}
	else if (arg)
		*arg = left;

	if (isTitle)
		*isTitle = false;
	if (id)
		*id = i0;
	return true;
}

void sortStrVec(vector<string>& vec) {
	std::sort(vec.begin(), vec.end(), numStrCompare);
}

bool inRect(const SDL_Rect& rect, vec2i point) {
	return rect.w != 0 && rect.h != 0 && point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y && point.y <= rect.y + rect.h;
}

bool needsCrop(const SDL_Rect& crop) {
	return crop.x != 0 || crop.y != 0 || crop.w != 0 || crop.h != 0;
}

SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame) {
	item.w += item.x;
	item.h += item.y;
	frame.w += frame.x;
	frame.h += frame.y;

	vec2i outRet(item.w-item.x, item.h-item.y);	// return values in case that the item is outta frame
	SDL_Rect crop = { 0, 0, 0, 0 };

	if (item.w < frame.x)	// left
		crop.w = -outRet.x;
	else if (item.x < frame.x)
		crop.x = frame.x - item.x;

	if (item.x > frame.w)	// right
		crop.w = outRet.x;
	else if (item.w > frame.w)
		crop.w = item.w - frame.w;

	if (item.h < frame.y)	// top
		crop.h = -outRet.y;
	else if (item.y < frame.y)
		crop.y = frame.y - item.y;

	if (item.y > frame.h)	// bottom
		crop.h = outRet.y;
	else if (item.h > frame.h)
		crop.h = item.h - frame.h;

	return crop;
}

SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop) {
	return { rect.x+crop.x, rect.y+crop.y, rect.w-crop.x-crop.w, rect.h-crop.y-crop.h };
}

SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop) {
	vec2i temp(rect.w, rect.h);
	rect = cropRect(rect, crop);
	crop = { crop.x, crop.y, temp.x - crop.x - crop.w, temp.y - crop.y - crop.h };

	SDL_Surface* sheet = SDL_CreateRGBSurface(surface->flags, crop.w, crop.h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
	SDL_BlitSurface(surface, &crop, sheet, 0);
	return sheet;
}

void PrintInfo() {
	SDL_version ver;
	SDL_GetVersion(&ver);
	cout << "\nSDL version: min=" << to_string(ver.minor) << " max=" << to_string(ver.major) << " patch=" << to_string(ver.patch) << endl;
	cout << "Platform: " << SDL_GetPlatform() << endl;
	cout << "CPU count: " << SDL_GetCPUCount() << " - " << SDL_GetCPUCacheLineSize() << endl;
	cout << "RAM: " << SDL_GetSystemRAM() << "MB" << endl;
	cout << "\nVideo Drivers:" << endl;
	for (int i=0; i!=SDL_GetNumVideoDrivers(); i++)
		cout << SDL_GetVideoDriver(i) << endl;
	cout << "\nRenderers:" << endl;
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++)
		cout << getRendererName(i) << endl;
	cout << "\nAudio Devices:" << endl;
	for (int i=0; i!=SDL_GetNumAudioDevices(0); i++)
		cout << SDL_GetAudioDeviceName(i, 0) << endl;
	cout << "\nAudio Drivers:" << endl;
	for (int i=0; i!=SDL_GetNumAudioDrivers(); i++)
		cout << SDL_GetAudioDriver(i) << endl;
	cout << endl;
}

string getRendererName(int id) {
	SDL_RendererInfo info;
	if (!SDL_GetRenderDriverInfo(id, &info))
		SDL_GetRenderDriverInfo(-1, &info);
	return info.name;
}

vector<string> getAvailibleRenderers(bool trustedOnly) {
	vector<string> renderers;
	for (int i=0; i!=SDL_GetNumRenderDrivers(); i++) {
		string name = getRendererName(i);
		if (!trustedOnly || (trustedOnly && strcmpCI(name, "direct3d")) || (trustedOnly && strcmpCI(name, "opengl")) || (trustedOnly && strcmpCI(name, "software")))
			renderers.push_back(name);
	}
	return renderers;
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
	vec2i res = World::winSys()->Resolution();
	return vec2i(p.x * res.x, p.y *res.y);
}

int pixX(float p) {
	return p * World::winSys()->Resolution().x;
}

int pixY(float p) {
	return p * World::winSys()->Resolution().y;
}

vec2f prc(const vec2i& p) {
	vec2i res = World::winSys()->Resolution();
	return vec2f(float(p.x) / float(res.x), float(p.y) / float(res.y));
}

float prcX(int p) {
	return float(p) / float(World::winSys()->Resolution().x);
}

float prcY(int p) {
	return float(p) / float(World::winSys()->Resolution().y);
}

float axisToFloat(int16 axisValue) {
	return float(axisValue) / float(32768);
}

int16 floatToAxis(float axisValue) {
	return axisValue * float(32768);
}
