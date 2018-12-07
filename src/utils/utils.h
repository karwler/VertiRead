#pragma once

#include "prog/defaults.h"
#ifndef _WIN32
#include <strings.h>
#endif

// general wrappers

inline vec2i texSize(SDL_Texture* tex) {
	vec2i res;
	SDL_QueryTexture(tex, nullptr, nullptr, &res.x, &res.y);
	return res;
}

inline vec2i mousePos() {
	int px, py;
	SDL_GetMouseState(&px, &py);
	return vec2i(px, py);
}

inline bool mousePressed(uint8 mbutton) {
	return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(mbutton);
}

inline string getRendererName(int id) {
	SDL_RendererInfo info;
	SDL_GetRenderDriverInfo(id, &info);
	return info.name;
}

vector<string> getAvailibleRenderers();

// SDL_Rect wrapper

struct Rect : SDL_Rect {
	Rect(int x = 0, int y = 0, int w = 0, int h = 0);
	Rect(const vec2i& pos, const vec2i& size);

	vec2i& pos();
	const vec2i& pos() const;
	vec2i& size();
	const vec2i& size() const;
	vec2i end() const;
	vec2i back() const;

	bool overlap(const vec2i& point) const;
	bool overlap(const Rect& rect, vec2i& sback, vec2i& rback) const;
	Rect crop(const Rect& frame);		// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off
	Rect getOverlap(const Rect& frame) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify the rect
};

inline Rect::Rect(int x, int y, int w, int h) :
	SDL_Rect({x, y, w, h})
{}

inline Rect::Rect(const vec2i& pos, const vec2i& size) :
	SDL_Rect({pos.x, pos.y, size.w, size.h})
{}

inline vec2i& Rect::pos() {
	return *reinterpret_cast<vec2i*>(this);
}

inline const vec2i& Rect::pos() const {
	return *reinterpret_cast<const vec2i*>(this);
}

inline vec2i& Rect::size() {
	return reinterpret_cast<vec2i*>(this)[1];
}

inline const vec2i& Rect::size() const {
	return reinterpret_cast<const vec2i*>(this)[1];
}

inline vec2i Rect::end() const {
	return pos() + size();
}

inline vec2i Rect::back() const {
	return vec2i(x + w - 1, y + h - 1);
}

inline bool Rect::overlap(const vec2i& point) const {
	return point.x >= x && point.x < x + w && point.y >= y && point.y < y + h;
}

// SDL_Texture wrapper

struct Texture {
	Texture(const string& name = "", SDL_Texture* tex = nullptr);

	vec2i res() const;

	string name;
	SDL_Texture* tex;
};

inline Texture::Texture(const string& name, SDL_Texture* tex) :
	name(name),
	tex(tex)
{}

inline vec2i Texture::res() const {
	return texSize(tex);
}

// files and strings

int strnatcmp(const char* a, const char* b);	// natural string compare

inline bool strnatless(const string& a, const string& b) {
	return strnatcmp(a.c_str(), b.c_str()) < 0;
}

inline int strcicmp(const string& a, const string& b) {	// case insensitive check if strings are equal
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

string trim(const string& str);
string trimZero(const string& str);
bool pathCmp(const string& as, const string& bs);
bool isSubpath(const string& path, string parent);
string parentPath(const string& path);
string childPath(const string& parent, const string& child);
string getChild(const string& path, const string& parent);
string filename(const string& path);	// get filename from path
string getExt(const string& path);		// get file extension
bool hasExt(const string& path);
string delExt(const string& path);		// returns filepath without extension
string appendDsep(const string& path);	// append directory separator if necessary
#ifdef _WIN32
bool isDriveLetter(const string& path);	// check if path is a drive letter (plus colon and optionally dsep). only for windows

inline bool isDriveLetter(char c) {
	return c >= 'A' && c <= 'Z';
}
#endif
bool strchk(const string& str, bool (*cmp)(char));
string strEnclose(string str);
vector<string> strUnenclose(const string& str);
SDL_Color getColor(const string& line);
archive* openArchive(const string& file);
SDL_RWops* readArchiveEntry(archive* arch, archive_entry* entry);

inline bool isSpace(char c) {
	return c > '\0' && c <= ' ';
}

inline bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

// geometry?

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

// conversions

#ifdef _WIN32
string wtos(const wstring& wstr);
wstring stow(const string& str);
#endif
uint8 jtStrToHat(const string& str);

inline string jtHatToStr(uint8 jhat) {
	return Default::hatNames.count(jhat) ? Default::hatNames.at(jhat) : "invalid";
}

inline bool stob(const string& str) {
	return str == "true" || str == "1";
}

inline string btos(bool b) {
	return b ? "true" : "false";
}

template <class T>
string enumToStr(const vector<string>& names, T id) {
	return sizt(id) < names.size() ? names[sizt(id)] : "";
}

template <class T>
T strToEnum(const vector<string>& names, string str) {
	for (sizt i = 0; i < names.size(); i++)
		if (!strcicmp(names[i], str))
			return T(i);
	return T(-1);
}

inline long sstol(const string& str, int base = 0) {
	return strtol(str.c_str(), nullptr, base);
}

inline llong sstoll(const string& str, int base = 0) {
	return strtoll(str.c_str(), nullptr, base);
}

inline ulong sstoul(const string& str, int base = 0) {
	return strtoul(str.c_str(), nullptr, base);
}

inline ullong sstoull(const string& str, int base = 0) {
	return strtoull(str.c_str(), nullptr, base);
}

inline float sstof(const string& str) {
	return strtof(str.c_str(), nullptr);
}

inline double sstod(const string& str) {
	return strtod(str.c_str(), nullptr);
}

inline ldouble sstold(const string& str) {
	return strtold(str.c_str(), nullptr);
}

// container stuff

template <class T, class P, class F, class... A>
bool foreachAround(const vector<T>& vec, sizt start, bool fwd, P* parent, F func, A... args) {
	return fwd ? foreachFAround(vec, start, parent, func, args...) : foreachRAround(vec, start, parent, func, args...);
}

template <class T, class P, class F, class... A>
bool foreachFAround(const vector<T>& vec, sizt start, P* parent, F func, A... args) {
	for (sizt i = start+1; i < vec.size(); i++)
		if ((parent->*func)(vec[i], args...))
			return true;
	for (sizt i = 0; i < start; i++)
		if ((parent->*func)(vec[i], args...))
			return true;
	return false;
}

template <class T, class P, class F, class... A>
bool foreachRAround(const vector<T>& vec, sizt start, P* parent, F func, A... args) {
	for (sizt i = start-1; i < vec.size(); i--)
		if ((parent->*func)(vec[i], args...))
			return true;
	if (vec.size())
		for (sizt i = vec.size()-1; i > start; i--)
			if ((parent->*func)(vec[i], args...))
				return true;
	return false;
}

template <class T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}
