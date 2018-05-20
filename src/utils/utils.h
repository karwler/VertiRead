#pragma once

#include "settings.h"

// files and strings
bool strcmpCI(const string& strl, const string& strr);	// case insensitive check if strings are equal
bool isAbsolute(const string& path);
string parentPath(const string& path);
string filename(const string& path);	// get filename from path
string getExt(const string& path);		// get file extension
bool hasExt(const string& path);
bool hasExt(const string& path, const string& ext);	// whether file has an extension
string delExt(const string& path);		// returns filepath without extension
string appendDsep(const string& path);	// append directory separator if necessary
bool isDriveLetter(const string& path);	// check if path is a drive letter (plus colon and optionally dsep). only for windows
vector<vec2t> getWords(const string& line, char spacer=' ');	// returns index of first character and length of words in line
string getBook(const string& picPath);
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
inline bool isCapitalLetter(char c) { return c >= 'A' && c <= 'Z'; }

// geometry?
SDL_Rect cropRect(SDL_Rect& rect, const SDL_Rect& frame);	// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off
SDL_Rect overlapRect(SDL_Rect rect, const SDL_Rect& frame);	// same as above except it returns the overlap instead of the crop

inline vec2i rectEnd(const SDL_Rect& rect) {
	return vec2i(rect.x + rect.w - 1, rect.y + rect.h - 1);
}

inline bool inRect(const vec2i& point, const SDL_Rect& rect) {	// check if point is in rect
	return point.x >= rect.x && point.x < rect.x + rect.w && point.y >= rect.y && point.y < rect.y + rect.h;
}

template <typename T>
bool inRange(T val, T min, T max) {
	return val >= min && val <= max;
}

template <typename T>
bool outRange(T val, T min, T max) {
	return val < min || val > max;
}

template <typename T>	// correct val if out of range. returns true if value already in range
T bringIn(T val, T min, T max) {
	return (val < min) ? min : (val > max) ? max : val;
}

template <typename T>
vec2<T> bringIn(const vec2<T>& val, const vec2<T>& min, const vec2<T>& max) {
	return vec2<T>(bringIn(val.x, min.x, max.x), bringIn(val.y, min.y, max.y));
}

template <typename T>	// correct val if out of range. returns true if value already in range
T bringUnder(T val, T max) {
	return (val > max) ? max : val;
}

template <typename T>
vec2<T> bringUnder(const vec2<T>& val, const vec2<T>& max) {
	return vec2<T>(bringUnder(val.x, max.x), bringUnder(val.y, max.y));
}

// convertions
string wtos(const wstring& wstr);
wstring stow(const string& str);
inline bool stob(const string& str) { return str == "true" || str == "1"; }
inline string btos(bool b) { return b ? "true" : "false"; }
string jtHatToStr(uint8 jhat);
uint8 jtStrToHat(const string& str);

template <typename T>
string enumToStr(const vector<string>& names, T id) {
	return (T(id) >= names.size()) ? "invalid" : names[T(id)];
}

template <typename T>
T strToEnum(const vector<string>& names, string str) {
	for (sizt i=0; i<names.size(); i++)
		if (strcmpCI(names[i], str))
			return T(i);
	return T(names.size());
}

inline float axisToFloat(int axisValue) {	// input axis value to float from -1 to 1
	return float(axisValue) / float(Default::axisLimit);
}

template <typename T>
string ntos(T num) {
	ostringstream ss;
	ss << +num;
	return ss.str();
}

// container stuff

template <typename T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}
