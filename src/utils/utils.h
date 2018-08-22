#pragma once

#include "settings.h"

// files and strings
int strcicmp(const string& a, const string& b);	// case insensitive check if strings are equal
int strnatcmp(const char* a, const char* b);	// natural string compare
inline bool strnatless(const string& a, const string& b) { return strnatcmp(a.c_str(), b.c_str()) < 0; }
bool isAbsolute(const string& path);
string absolutePath(const string& path);
bool isSubpath(const string& path, string parent);
string parentPath(const string& path);
string childPath(const string& parent, const string& child);
string filename(const string& path);	// get filename from path
string getExt(const string& path);		// get file extension
bool hasExt(const string& path);
string delExt(const string& path);		// returns filepath without extension
string appendDsep(const string& path);	// append directory separator if necessary
inline bool directoryCmp(const string& a, const string& b) { return appendDsep(a) == appendDsep(b); }
#ifdef _WIN32
bool isDriveLetter(const string& path);	// check if path is a drive letter (plus colon and optionally dsep). only for windows
inline bool isDriveLetter(char c) { return c >= 'A' && c <= 'Z'; }
#endif
inline bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
vector<string> getWords(const string& line);
string getBook(const string& pic);

// geometry?
SDL_Rect cropRect(SDL_Rect& rect, const SDL_Rect& frame);	// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off
SDL_Rect overlapRect(SDL_Rect rect, const SDL_Rect& frame);	// same as above except it returns the overlap instead of the crop

inline vec2i rectEnd(const SDL_Rect& rect) {
	return vec2i(rect.x + rect.w - 1, rect.y + rect.h - 1);
}

inline bool inRect(const vec2i& point, const SDL_Rect& rect) {	// check if point is in rect
	return point.x >= rect.x && point.x < rect.x + rect.w && point.y >= rect.y && point.y < rect.y + rect.h;
}

template <class T>
bool inRange(T val, T min, T max) {
	return val >= min && val <= max;
}

template <class T>
bool outRange(T val, T min, T max) {
	return val < min || val > max;
}

template <class T>	// correct val if out of range. returns true if value already in range
T bringIn(T val, T min, T max) {
	return val < min ? min : val > max ? max : val;
}

template <class T>
vec2<T> bringIn(const vec2<T>& val, const vec2<T>& min, const vec2<T>& max) {
	return vec2<T>(bringIn(val.x, min.x, max.x), bringIn(val.y, min.y, max.y));
}

template <class T>	// correct val if out of range. returns true if value already in range
T bringUnder(T val, T max) {
	return (val > max) ? max : val;
}

template <class T>
vec2<T> bringUnder(const vec2<T>& val, const vec2<T>& max) {
	return vec2<T>(bringUnder(val.x, max.x), bringUnder(val.y, max.y));
}

template <class T>	// correct val if out of range. returns true if value already in range
T bringOver(T val, T min) {
	return (val < min) ? min : val;
}

template <class T>
vec2<T> bringOver(const vec2<T>& val, const vec2<T>& min) {
	return vec2<T>(bringUnder(val.x, min.x), bringUnder(val.y, min.y));
}

// convertions
string wtos(const wstring& wstr);
wstring stow(const string& str);
inline bool stob(const string& str) { return str == "true" || str == "1"; }
inline string btos(bool b) { return b ? "true" : "false"; }
string jtHatToStr(uint8 jhat);
uint8 jtStrToHat(const string& str);

template <class T>
string enumToStr(const vector<string>& names, T id) {
	return static_cast<sizt>(id) < names.size() ? names[static_cast<sizt>(id)] : "";
}

template <class T>
T strToEnum(const vector<string>& names, string str) {
	for (sizt i = 0; i < names.size(); i++)
		if (!strcicmp(names[i], str))
			return static_cast<T>(i);
	return static_cast<T>(SIZE_MAX);
}

template <class T>
string ntos(T num) {
	ostringstream ss;
	ss << +num;
	return ss.str();
}

// container stuff

sizt nextIndex(sizt id, sizt lim, bool fwd);

template <class T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}
