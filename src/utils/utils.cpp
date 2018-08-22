#include "engine/world.h"
#include <locale>
#include <codecvt>
#ifndef _WIN32
#include <strings.h>
#endif

int strcicmp(const string& a, const string& b) {
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

static int natCompareRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		if (!isDigit(*a)  &&  !isDigit(*b))
			return bias;
		else if (!isDigit(*a))
			return -1;
		else if (!isDigit(*b))
			return +1;
		else if (*a < *b) {
			if (!bias)
				bias = -1;
		} else if (*a > *b) {
			if (!bias)
				bias = +1;
		} else if (!*a  &&  !*b)
			return bias;
	}
	return 0;
}

static int natCompareLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		if (!isDigit(*a)  &&  !isDigit(*b))
			return 0;
		else if (!isDigit(*a))
			return -1;
		else if (!isDigit(*b))
			return +1;
		else if (*a < *b)
			return -1;
		else if (*a > *b)
			return +1;
	}
	return 0;
}

int strnatcmp(const char* a, const char* b) {
	for (;; a++, b++) {
		char ca = *a;
		char cb = *b;
		while (isSpace(ca))
			ca = *++a;
		while (isSpace(cb))
			cb = *++b;

		if (isDigit(ca) && isDigit(cb)) {
			if (ca == '0' || cb == '0') {
				if (int result = natCompareLeft(a, b))
					return result;
			} else {
				if (int result = natCompareRight(a, b))
					return result;
			}
		}

		if (!ca && !cb)
			return 0;
		if (ca < cb)
			return -1;
		if (ca > cb)
			return +1;
	}
}

bool isAbsolute(const string& path) {
#ifdef _WIN32
	return path.size() && (path[0] == '\\' || (path.length() == 2 && isDriveLetter(path[0]) && path[1] == ':') || (path.length() > 2 && isDriveLetter(path[0]) && path[1] == ':' && path[2] == '\\'));
#else
	return path.size() && path[0] == '/';
#endif
}

string absolutePath(const string& path) {
	if (path.empty())
		return dseps;
	return isAbsolute(path) ? path : childPath(Filer::getWorkingDir(), path);
}

bool isSubpath(const string& path, string parent) {
	if (!(isAbsolute(path) && isAbsolute(parent)))
		return false;
	if (parent == dseps)
		return true;

	parent = appendDsep(parent);
	return !appendDsep(path).compare(0, parent.length(), parent);
}

string parentPath(const string& path) {
	if (path.empty())
		return "";

	string sds = dseps;
	if (path == sds)
		return sds;
#ifdef _WIN32
	if (isDriveLetter(path))
		return sds;
#endif
	sizt pos = path.find_last_of(dsep, path.length() - (path.back() == dsep ? 2 : 1));
	return pos == string::npos ? "" : path.substr(0, pos);
}

string childPath(const string& parent, const string& child) {
#ifdef _WIN32
	return parent == "\\" && isDriveLetter(child) ? child : appendDsep(parent) + child;
#else
	return appendDsep(parent) + child;
#endif
}

string filename(const string& path) {
	if (path.empty() || path == dseps)
		return "";

	sizt end = path.back() == dsep ? path.length()-1 : path.length();
	sizt pos = path.find_last_of(dsep, end-1);
	return pos == string::npos ? path.substr(0, end) : path.substr(pos+1, end-pos-1);
}

string getExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return path.substr(i+1);
		else if (path[i] == dsep)
			return "";
	}
	return "";
}

bool hasExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return true;
		else if (path[i] == dsep)
			return false;
	}
	return false;
}

string delExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return path.substr(0, i);
		else if (path[i] == dsep)
			return path;
	}
	return path;
}

string appendDsep(const string& path) {
	return path.size() && path.back() == dsep ? path : path + dsep;
}
#ifdef _WIN32
bool isDriveLetter(const string& path) {
	return (path.length() == 2 && isDriveLetter(path[0]) && path[1] == ':') || (path.length() == 3 && isDriveLetter(path[0]) && path[1] == ':' && path[2] == dsep);
}
#endif
vector<string> getWords(const string& line) {
	sizt i = 0;
	while (isSpace(line[i]))
		i++;

	sizt start = i;
	vector<string> words;
	while (i < line.length()) {
		if (isSpace(line[i])) {
			words.push_back(line.substr(start, i-start));
			while (isSpace(line[++i]));
			start = i;
		} else
			i++;
	}
	if (start < i)
		words.push_back(line.substr(start, i-start));
	return words;
}

string getBook(const string& pic) {
	string dirLib = appendDsep(World::winSys()->sets.getDirLib());
	if (!isSubpath(pic, dirLib) || pic.length() == dirLib.length())
		return ".";
	return pic.substr(dirLib.length(), pic.find_first_of(dsep, dirLib.length()) - dirLib.length());
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

string jtHatToStr(uint8 jhat) {
	return Default::hatNames.count(jhat) ? Default::hatNames.at(jhat) : "invalid";
}

uint8 jtStrToHat(const string& str) {
	for (const pair<uint8, string>& it : Default::hatNames)
		if (!strcicmp(it.second, str))
			return it.first;
	return 0x10;
}

sizt nextIndex(sizt id, sizt lim, bool fwd) {
	if (fwd)
		return id == lim - 1 ? 0 : id + 1;
	return id == 0 ? lim - 1 : id - 1;
}
