#pragma once

#include "prog/defaults.h"

// files and strings
bool strcmpCI(const string& strl, const string& strr);
bool findChar(const string& str, char c);
bool findChar(const string& str, char c, size_t& id);
bool findString(const string& str, const string& c);
bool findString(const string& str, const string& c, size_t& id);
bool isAbsolute(const string& path);
string parentPath(const string& path);
string filename(const string& path);
string getExt(const string& path);
bool hasExt(const string& path, const string& ext);
string delExt(const string& path);
string appendDsep(const string& path);
bool isDriveLetter(const string& path);
string modifyCase(string str, ETextCase caseChange);
vector<string> getWords(const string& line, char splitter);
bool splitIniLine(const string& line, string* arg, string* val, string* key=nullptr, bool* isTitle=nullptr, size_t* id=nullptr);

// graphics
bool inRect(const SDL_Rect& rect, vec2i point);
bool needsCrop(const SDL_Rect& crop);
SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame);
void textCropRight(SDL_Rect& crop, int textLen, int rectWidth);
SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop);
SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop);

// other
string getRendererName(int id);
vector<string> getAvailibleRenderers();

// convertions
string wtos(const wstring& wstr);
wstring stow(const string& str);
bool stob(const string& str);
string btos(bool b);
string jtHatToStr(uint8 jhat);
uint8 jtStrToHat(string str);
string gpButtonToStr(uint8 gbutton);
uint8 gpStrToButton(string str);
string gpAxisToStr(uint8 gaxis);
uint8 gpStrToAxis(string str);

vec2i pix(const vec2f& p);
int pixX(float p);
int pixY(float p);
vec2f prc(const vec2i& p);
float prcX(int p);
float prcY(int p);

float axisToFloat(int16 axisValue);
int16 floatToAxis(float axisValue);

// pointer container cleaners
template <typename T>
void erase(vector<T*>& vec, size_t i) {
	delete vec[i];
	vec.erase(vec.begin() + i);
}

template <typename T>
void clear(vector<T*>& vec) {
	for (T* it : vec)
		delete it;
	vec.clear();
}

template <typename T>
bool contains(const vector<T>& vec, const T& elem) {
	for (const T& it : vec)
		if (it == elem)
			return true;
	return false;
}

template <typename T>
bool contains(const vector<T>& vec, const T& elem, size_t& id) {
	for (size_t i=0; i!=vec.size(); i++)
		if (vec[i] == elem) {
			id = i;
			return true;
		}
	return false;
}

template <typename A, typename B>
void erase(map<A, B*>& mp, const A& key) {
	delete mp[key];
	mp.erase(key);
}

template <typename A, typename B>
void clear(map<A, B*>& mp) {
	for (const pair<A, B*>& it : mp)
		delete it.second;
	mp.clear();
}

template <typename T>
void clear(grid2<T*>& gd) {
	for (T* it : gd)
		delete it;
	gd.clear();
}
