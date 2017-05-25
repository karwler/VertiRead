#pragma once

// include SDL
#include <SDL2/SDL.h>
#ifdef __APPLE__
#include <SDL2_image/SDL_image.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <SDL2_mixer/SDL_mixer.h>
#else
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#endif

// include other useful stuff
#include "kklib/aliases.h"
#include "kklib/grid2.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::to_string;

using kk::vec2i;
using kk::vec2u;
using kk::vec2f;
using kk::vec4c;

using vec2t = kk::vec2<size_t>;

using kk::grid2;

#ifdef main
#undef main
#endif

// forward declaraions
enum class EColor : uint8;
enum class ETextCase : uint8;

class AudioSys;
class Engine;
class InputSys;
class WinSys;

class Scene;
class Program;

class ScrollArea;
class ScrollAreaX1;
class Capturer;

// files and strings
bool alphanumStrCompare(const string& sa, const string& sb);
bool strcmpCI(const string& strl, const string& strr);
bool findChar(const string& str, char c, size_t* id=nullptr);
bool findString(const string& str, const string& c, size_t* id=nullptr);
bool isAbsolute(const string& path);
string parentPath(const string& path);
string filename(const string& path);
string getExt(const string& path);
string delExt(const string& path);
string appendDsep(const string& path);
bool isDriveLetter(const string& path);
string modifyCase(string str, ETextCase caseChange);
vector<string> getWords(const string& line, char splitter, char spacer);
bool splitIniLine(const string& line, string* arg, string* val, string* key=nullptr, bool* isTitle=nullptr, size_t* id=nullptr);
void sortStrVec(vector<string>& vec);

// graphics
bool inRect(const SDL_Rect& rect, vec2i point);
bool needsCrop(const SDL_Rect& crop);
SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame);
void textCropRight(SDL_Rect& crop, int textLen, int rectWidth);
SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop);
SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop);

// other
void PrintInfo();
string getRendererName(int id);
vector<string> getAvailibleRenderers(bool trustedOnly=false);

// convertions
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
void erase(vector<T*>& vec, uint i) {
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
bool contains(const vector<T>& vec, const T& elem, size_t* id=nullptr) {
	for (size_t i=0; i!=vec.size(); i++)
		if (vec[i] == elem) {
			if (id)
				*id = i;
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
void clear(grid2<T>& gd) {
	for (T& it : gd)
		delete it;
	gd.clear();
}
