#include "engine/world.h"
#include <locale>
#include <codecvt>

bool strcmpCI(const string& strl, const string& strr) {
	sizt i = 0;
	do {
		if (tolower(strl[i]) != tolower(strr[i]))
			return false;
	} while (strl[i++] != '\0');
	return true;
}

bool isAbsolute(const string& path) {
	if (path.empty())
		return false;
	return path[0] == dsep || (path.length() >= 3 && isCapitalLetter(path[0]) && path[1] == ':' && path[2] == dsep);
}

string parentPath(const string& path) {
	sizt i = (path[path.length()-1] == dsep) ? path.length()-2 : path.length()-1;
	for (; i!=SIZE_MAX; i--)
		if (path[i] == dsep)
			return path.substr(0, i+1);
	return path;
}

string filename(const string& path) {
	for (sizt i=path.length()-1; i!=SIZE_MAX; i--)
		if (path[i] == dsep)
			return path.substr(i+1);
	return path;
}

string getExt(const string& path) {
	for (sizt i=path.length()-1; i!=SIZE_MAX; i--)
		if (path[i] == '.')
			return path.substr(i+1);
	return "";
}

bool hasExt(const string& path) {
	for (sizt i=path.length()-1; i!=SIZE_MAX; i--) {
		if (path[i] == '.')
			return true;
		else if (path[i] == dsep)
			return false;
	}
	return false;
}

bool hasExt(const string& path, const string& ext) {
	if (path.length() < ext.length())
		return false;

	sizt pos = path.length() - ext.length();
	for (sizt i=0; i<ext.length(); i++)
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
	if (path.empty())
		return string(1, dsep);
	return (path.back() == dsep) ? path : path + dsep;
}

bool isDriveLetter(const string& path) {
	return (path.length() == 2 && isCapitalLetter(path[0]) && path[1] == ':') || (path.length() == 3 && isCapitalLetter(path[0]) && path[1] == ':' && path[2] == dsep);
}

vector<vec2t> getWords(const string& line, char spacer) {
	sizt i = 0;
	while (line[i] == spacer)
		i++;

	sizt start = i;
	vector<vec2t> words;
	while (i < line.length()) {
		if (line[i] == spacer) {
			words.push_back(vec2t(start, i-start));
			while (line[++i] == spacer);
			start = i;
		} else
			i++;
	}
	if (start < i)
		words.push_back(vec2t(start, i-start));
	return words;
}

string getBook(const string& picPath) {
	sizt start = appendDsep(World::winSys()->sets.getDirLib()).length();
	for (sizt i=start; i<picPath.length(); i++)
		if (picPath[i] == dsep)
			return picPath.substr(start, i-start);
	return "";
}

SDL_Rect cropRect(SDL_Rect& rect, const SDL_Rect& frame) {
	if (rect.w <= 0 || rect.h <= 0 || frame.w <= 0 || frame.h <= 0) {	// idfk
		rect = {0, 0, 0, 0};
		return rect;
	}

	// ends of each rect and frame
	vec2i rend = rectEnd(rect);
	vec2i fend = rectEnd(frame);
	if (rect.x > fend.x || rect.y > fend.y || rend.x < frame.x || rend.y < frame.y) {	// if rect is out of frame
		rect = {0, 0, 0, 0};
		return rect;
	}

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

SDL_Rect overlapRect(SDL_Rect rect, const SDL_Rect& frame) {
	if (rect.w <= 0 || rect.h <= 0 || frame.w <= 0 || frame.h <= 0)		// idfk
		return {0, 0, 0, 0};

	// ends of both rects
	vec2i rend = rectEnd(rect);
	vec2i fend = rectEnd(frame);
	if (rect.x > fend.x || rect.y > fend.y || rend.x < frame.x || rend.y < frame.y)	// if they don't overlap
		return {0, 0, 0, 0};

	// crop rect if it's boundaries are out of frame
	if (rect.x < frame.x) {	// left
		rect.w -= frame.x - rect.x;
		rect.x = frame.x;
	}
	if (rend.x > fend.x)	// right
		rect.w -= rend.x - fend.x;
	if (rect.y < frame.y) {	// top
		rect.h -= frame.y - rect.y;
		rect.y = frame.y;
	}
	if (rend.y > fend.y)	// bottom
		rect.h -= rend.y - fend.y;
	return rect;
}

string wtos(const wstring& wstr) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().to_bytes(wstr);
}

wstring stow(const string& str) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().from_bytes(str);
}

string colorToStr(Color color) {
	switch (color) {
	case Color::background:
		return "background";
	case Color::normal:
		return "normal";
	case Color::dark:
		return "dark";
	case Color::light:
		return "light";
	case Color::text:
		return "text";
	}
	return "invalid";
}

Color strToColor(string str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	if (str == "background")
		return Color::background;
	if (str == "normal")
		return Color::normal;
	if (str == "dark")
		return Color::dark;
	if (str == "light")
		return Color::light;
	if (str == "text")
		return Color::text;
	return Color::numColors;
}

string bindingTypeToStr(Binding::Type type) {
	switch (type) {
	case Binding::Type::back:
		return "back";
	case Binding::Type::zoomIn:
		return "zoom_in";
	case Binding::Type::zoomOut:
		return "zoom_out";
	case Binding::Type::zoomReset:
		return "zoom_reset";
	case Binding::Type::centerView:
		return "center_view";
	case Binding::Type::fast:
		return "fast";
	case Binding::Type::slow:
		return "slow";
	case Binding::Type::playPause:
		return "play_pause";
	case Binding::Type::fullscreen:
		return "fullscreen";
	case Binding::Type::nextDir:
		return "next_dir";
	case Binding::Type::prevDir:
		return "prev_dir";
	case Binding::Type::nextSong:
		return "next_song";
	case Binding::Type::prevSong:
		return "prev_song";
	case Binding::Type::volumeUp:
		return "volume_up";
	case Binding::Type::volumeDown:
		return "volume_down";
	case Binding::Type::pageUp:
		return "page_up";
	case Binding::Type::pageDown:
		return "page_down";
	case Binding::Type::up:
		return "up";
	case Binding::Type::down:
		return "down";
	case Binding::Type::right:
		return "right";
	case Binding::Type::left:
		return "left";
	}
	return "invalid";
}

Binding::Type strToBindingType(string str) {
	std::transform(str.begin(), str.end(), str.begin(), tolower);
	if (str == "back")
		return Binding::Type::back;
	if (str == "zoom_in")
		return Binding::Type::zoomIn;
	if (str == "zoom_out")
		return Binding::Type::zoomOut;
	if (str == "zoom_reset")
		return Binding::Type::zoomReset;
	if (str == "center_view")
		return Binding::Type::centerView;
	if (str == "fast")
		return Binding::Type::fast;
	if (str == "slow")
		return Binding::Type::slow;
	if (str == "play_pause")
		return Binding::Type::playPause;
	if (str == "fullscreen")
		return Binding::Type::fullscreen;
	if (str == "next_dir")
		return Binding::Type::nextDir;
	if (str == "prev_dir")
		return Binding::Type::prevDir;
	if (str == "next_song")
		return Binding::Type::nextSong;
	if (str == "prev_song")
		return Binding::Type::prevSong;
	if (str == "volume_up")
		return Binding::Type::volumeUp;
	if (str == "volume_down")
		return Binding::Type::volumeDown;
	if (str == "page_up")
		return Binding::Type::pageUp;
	if (str == "page_down")
		return Binding::Type::pageDown;
	if (str == "up")
		return Binding::Type::up;
	if (str == "down")
		return Binding::Type::down;
	if (str == "right")
		return Binding::Type::right;
	if (str == "left")
		return Binding::Type::left;
	return Binding::Type::numBindings;
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
