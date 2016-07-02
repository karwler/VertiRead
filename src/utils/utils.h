#pragma once

// include system headers
#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

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
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <cstdlib>
#include <climits>
#include <cstdint>
#include <cmath>
#include <algorithm>
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
namespace fs = boost::filesystem;

// undefine some stuff to prevent conflicts
#ifdef main
#undef main
#endif

#ifdef CreateWindow
#undef CreateWindow
#endif

#ifdef PlaySound
#undef PlaySound
#endif

#ifdef DrawText
#undef DrawText
#endif

// not sure why, but why not
#ifndef PI
#ifdef M_PI
#define PI M_PI
#endif
#else
#ifndef M_PI
#define M_PI PI
#endif
#endif

// directory separator
#ifdef _WIN32
const char dsep = '\\';
#else
const char dsep = '/';
#endif

using byte = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using cstr = const char*;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using char16 = char16_t;
using char32 = char32_t;
using wchar = wchar_t;

// predeclarations
enum class ETextCase : uint8;

class Engine;
class AudioSys;
class InputSys;
class WinSys;

class Scene;
class Program;

class Button;
class ScrollArea;
class ListBox;
class PopupChoice;
class Capturer;
class LineEditor;

// smart pointer

template <typename T>
class kptr {
public:
	kptr(T* p=nullptr) : ptr(p) {}
	kptr(T v) : ptr(new T(v)) {}
	~kptr() {
		if (ptr)
			delete ptr;
	}

	T* get() const {
		return ptr;
	}
	T& val() const {
		return *ptr;
	}

	operator T*() const {
		return ptr;
	}
	operator T() const {
		return *ptr;
	}

	T* operator->() const {
		return ptr;
	}
	T& operator*() const {
		return *ptr;
	}

	T* reset(T* p=nullptr) {
		if (ptr)
			delete ptr;
		ptr = p;
		return ptr;
	}
	T* operator=(kptr& b) {
		if (ptr)
			delete ptr;
		ptr = b.p;
		return ptr;
	}
	T* operator=(T* p) {
		if (ptr)
			delete ptr;
		ptr = p;
		return ptr;
	}

	void swap(kptr& b) {
		T* tmp = b.ptr;
		b.ptr = ptr;
		ptr = tmp;
	}
	void swap(T* p) {
		T* tmp = p;
		p = ptr;
		ptr = tmp;
	}

	T* release() {
		T* t = ptr;
		ptr = nullptr;
		return t;
	}
private:
	T* ptr;
};

// 2D vector

template <typename T>
struct kvec2 {
	kvec2(T N=0) : x(N), y(N) {}
	kvec2(T X, T Y) : x(X), y(Y) {}
	template <typename A>
	kvec2(const kvec2<A>& N) : x(N.x), y(N.y) {}

	T x, y;

	template <typename A>
	kvec2& operator=(const kvec2<A>& n) {
		x = n.x;
		y = n.y;
		return *this;
	}
	kvec2& operator=(T n) {
		x = n;
		y = n;
		return *this;
	}
	template <typename A>
	kvec2 operator+(const kvec2<A>& n) const {
		return kvec2(x + n.x, y + n.y);
	}
	kvec2 operator+(T n) const {
		return kvec2(x+n, y+n);
	}
	template <typename A>
	kvec2 operator-(const kvec2<A>& n) const {
		return kvec2(x - n.x, y - n.y);
	}
	kvec2 operator-(T n) const {
		return kvec2(x-n, y-n);
	}
	template <typename A>
	kvec2 operator*(const kvec2<A>& n) const {
		return kvec2(x * n.x, y * n.y);
	}
	kvec2 operator*(T n) const {
		return kvec2(x*n, y*n);
	}
	template <typename A>
	kvec2 operator/(const kvec2<A>& n) const {
		return kvec2(x / n.x, y / n.y);
	}
	kvec2 operator/(T n) const {
		return kvec2(x/n, y/n);
	}
	template <typename A>
	kvec2& operator+=(const kvec2<A>& n) {
		x += n.x;
		y += n.y;
		return *this;
	}
	kvec2& operator+=(T n) {
		x += n;
		y += n;
		return *this;
	}
	template <typename A>
	kvec2& operator-=(const kvec2<A>& n) {
		x -= n.x;
		y -= n.y;
		return *this;
	}
	kvec2& operator-=(T n) {
		x -= n;
		y -= n;
		return *this;
	}
	template <typename A>
	kvec2& operator*=(const kvec2<A>& n) {
		x *= n.x;
		y *= n.y;
		return *this;
	}
	kvec2& operator*=(T n) {
		x *= n;
		y *= n;
		return *this;
	}
	template <typename A>
	kvec2& operator/=(const kvec2<A>& n) {
		x /= n.x;
		y /= n.y;
		return *this;
	}
	kvec2& operator/=(T n) {
		x /= n;
		y /= n;
		return *this;
	}

	template <typename A>
	bool operator==(const kvec2<A>& n) const {
		return x == n.x && y == n.y;
	}
	bool operator==(T n) const {
		return x == n && y == n;
	}
	template <typename A>
	bool operator!=(const kvec2<A>& n) const {
		return x != n.x || y != n.y;
	}
	bool operator!=(T n) const {
		return x != n || y != n;
	}
	bool isNull() const {
		return x == 0 && y == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0;
	}

	double len() const { return length(*this); }
	kvec2 norm() const { return normalize(*this); }
	bool isUnit() const { return isUnit(*this); }
	template <typename A>
	T dot(const kvec2<A>& n) const { return dot(*this, n); }
	template <typename A>
	kvec2 reflect(const kvec2<A>& n) const { return reflect(*this, n); }
};

template <typename T>
double length(const kvec2<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y);
}

template <typename T>
kvec2<T> normalize(const kvec2<T>& vec) {
	double l = length(vec);
	return kvec2<T>(double(vec.x)/l, double(vec.y)/l);
}

template <typename T>
bool isUnit(const kvec2<T>& vec) {
	return length(vec) == 1.0;
}

template <typename A, typename B>
A dotProduct(const kvec2<A>& v1, const kvec2<B>& v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

template <typename A, typename B>
bool reflect(const kvec2<A>& vec, kvec2<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - 2 * dotProduct(vec, nrm) * nrm;
}

using vec2i = kvec2<int>;
using vec2f = kvec2<float>;

// 3D vector
template <typename T>
struct kvec3 {
	kvec3(T N=0) : x(N), y(N), z(N) {}
	kvec3(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
	template <typename A>
	kvec3(const kvec3<A>& N) : x(N.x), y(N.y), z(N.z) {}

	T x, y, z;

	template <typename A>
	kvec3& operator=(const kvec3<A>& n) {
		x = n.x;
		y = n.y;
		z = n.z;
		return *this;
	}
	kvec3& operator=(T n) {
		x = n;
		y = n;
		z = n;
		return *this;
	}
	template <typename A>
	kvec3 operator+(const kvec3<A>& n) const {
		return kvec3(x + n.x, y + n.y, z + n.z);
	}
	kvec3 operator+(T n) const {
		return kvec3(x+n, y+n, z+n);
	}
	template <typename A>
	kvec3 operator-(const kvec3<A>& n) const {
		return kvec3(x - n.x, y - n.y, z - n.z);
	}
	kvec3 operator-(T n) const {
		return kvec3(x-n, y-n, z-n);
	}
	template <typename A>
	kvec3 operator*(const kvec3<A>& n) const {
		return kvec3(x * n.x, y * n.y, z * n.z);
	}
	kvec3 operator*(T n) const {
		return kvec3(x*n, y*n, z*n);
	}
	template <typename A>
	kvec3 operator/(const kvec3<A>& n) const {
		return kvec3(x / n.x, y / n.y, z / n.z);
	}
	kvec3 operator/(T n) const {
		return kvec3(x/n, y/n, z/n);
	}
	template <typename A>
	kvec3& operator+=(const kvec3<A>& n) {
		x += n.x;
		y += n.y;
		z += n.z;
		return *this;
	}
	kvec3& operator+=(T n) {
		x += n;
		y += n;
		z += n;
		return *this;
	}
	template <typename A>
	kvec3& operator-=(const kvec3<A>& n) {
		x -= n.x;
		y -= n.y;
		z -= n.z;
		return *this;
	}
	kvec3& operator-=(T n) {
		x -= n;
		y -= n;
		z -= n;
		return *this;
	}
	template <typename A>
	kvec3& operator*=(const kvec3<A>& n) {
		x *= n.x;
		y *= n.y;
		z *= n.z;
		return *this;
	}
	kvec3& operator*=(T n) {
		x *= n;
		y *= n;
		z *= n;
		return *this;
	}
	template <typename A>
	kvec3& operator/=(const kvec3<A>& n) {
		x /= n.x;
		y /= n.y;
		z /= n.z;
		return *this;
	}
	kvec3& operator/=(T n) {
		x /= n;
		y /= n;
		z /= n;
		return *this;
	}

	template <typename A>
	bool operator==(const kvec3<A>& n) const {
		return x == n.x && y == n.y && z == n.z;
	}
	bool operator==(T n) const {
		return x == n && y == n && z == n;
	}
	template <typename A>
	bool operator!=(const kvec3<A>& n) const {
		return x != n.x || y != n.y || z != n.z;
	}
	bool operator!=(T n) const {
		return x != n || y != n || z != n;
	}
	bool isNull() const {
		return x == 0 && y == 0 && z == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0 || z == 0;
	}

	double len() const { return length(*this); }
	kvec3 norm() const { return normalize(*this); }
	bool isUnit() const { return isUnit(*this); }
	template <typename A>
	double dot(const kvec3<A>& n) const { return dotProduct(*this, n); }
	template <typename A>
	kvec3 cross(const kvec3<A>& n) const { return crossProduct(*this, n); }
	template <typename A>
	kvec3 reflect(const kvec3<A>& n) const { return reflect(*this, n); }
};

template <typename T>
double length(const kvec3<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

template <typename T>
kvec3<T> normalize(const kvec3<T>& vec) {
	double l = vec.len();
	return kvec3<T>(double(vec.x)/l, double(vec.y)/l, double(vec.z)/l);
}

template <typename T>
bool isUnit(const kvec3<T>& vec) {
	return vec.len() == 1.0;
}

template <typename A, typename B>
A dotProduct(const kvec3<A>& v1, const kvec3<B>& v2) {
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

template <typename A, typename B>
kvec3<A> crossProduct(const kvec3<A>& v1, const kvec3<B>& v2) {
	return kvec3<A>(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
}

template <typename A, typename B>
bool reflect(const kvec3<A>& vec, kvec3<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - 2 * dotProduct(vec, nrm) * nrm;
}

using vec3i = kvec3<int>;
using vec3f = kvec3<float>;

// 4D vector

template <typename T>
struct kvec4 {
	kvec4(T N=0) : x(N), y(N), z(N), a(N) {}
	kvec4(T X, T Y, T Z, T A) : x(X), y(Y), z(Z), a(A) {}
	template <typename A>
	kvec4(const kvec4<A>& N) : x(N.x), y(N.y), z(N.z), a(N.a) {}

	T x, y, z, a;

	template <typename A>
	kvec4& operator=(const kvec4<A>& n) {
		x = n.x;
		y = n.y;
		z = n.z;
		a = n.a;
		return *this;
	}
	kvec4& operator=(T n) {
		x = n;
		y = n;
		z = n;
		a = n;
		return *this;
	}
	template <typename A>
	kvec4 operator+(const kvec4<A>& n) const {
		return kvec4(x + n.x, y + n.y, z + n.z, a + n.a);
	}
	kvec4 operator+(T n) const {
		return kvec4(x+n, y+n, z+n, a+n);
	}
	template <typename A>
	kvec4 operator-(const kvec4<A>& n) const {
		return kvec4(x - n.x, y - n.y, z - n.z, a - n.a);
	}
	kvec4 operator-(T n) const {
		return kvec4(x-n, y-n, z-n, a-n);
	}
	template <typename A>
	kvec4 operator*(const kvec4<A>& n) const {
		return kvec4(x * n.x, y * n.y, z * n.z, a * n.a);
	}
	kvec4 operator*(T n) const {
		return kvec4(x*n, y*n, z*n, a*n);
	}
	template <typename A>
	kvec4 operator/(const kvec4<A>& n) const {
		return kvec4(x / n.x, y / n.y, z / n.z, a / n.a);
	}
	kvec4 operator/(T n) const {
		return kvec4(x/n, y/n, z/n, a/n);
	}
	template <typename A>
	kvec4& operator+=(const kvec4<A>& n) {
		x += n.x;
		y += n.y;
		z += n.z;
		a += n.a;
		return *this;
	}
	kvec4& operator+=(T n) {
		x += n;
		y += n;
		z += n;
		a += n;
		return *this;
	}
	template <typename A>
	kvec4& operator-=(const kvec4<A>& n) {
		x -= n.x;
		y -= n.y;
		z -= n.z;
		a -= n.a;
		return *this;
	}
	kvec4& operator-=(T n) {
		x -= n;
		y -= n;
		z -= n;
		a -= n;
		return *this;
	}
	template <typename A>
	kvec4& operator*=(const kvec4<A>& n) {
		x *= n.x;
		y *= n.y;
		z *= n.z;
		a *= n.a;
		return *this;
	}
	kvec4& operator*=(T n) {
		x *= n;
		y *= n;
		z *= n;
		a *= n;
		return *this;
	}
	template <typename A>
	kvec4& operator/=(const kvec4<A>& n) {
		x /= n.x;
		y /= n.y;
		z /= n.z;
		a /= n.a;
		return *this;
	}
	kvec4& operator/=(T n) {
		x /= n;
		y /= n;
		z /= n;
		a /= n;
		return *this;
	}

	template <typename A>
	bool operator==(const kvec4<A>& n) const {
		return x == n.x && y == n.y && z == n.z && a == n.a;
	}
	bool operator==(T n) const {
		return x == n && y == n && z == n && a == n;
	}
	template <typename A>
	bool operator!=(const kvec4<A>& n) const {
		return x != n.x || y != n.y || z != n.z || a != n.a;
	}
	bool operator!=(T n) const {
		return x != n || y != n || z != n || a != n;
	}
	bool isNull() const {
		return x == 0 && y == 0 && z == 0 && a == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0 || z == 0 || a == 0;
	}

	double len() const { return length(*this); }
	kvec4 norm() const { return normalize(*this); }
	bool isUnit() const { return isUnit(*this); }
	template <typename A>
	double dot(const kvec4& n) const { return dotProduct(*this, n); }
};

template <typename T>
double length(const kvec4<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z + vec.a*vec.a);
}

template <typename T>
kvec4<T> normalize(const kvec4<T>& vec) {
	double l = vec.len();
	return kvec4<T>(double(vec.x)/l, double(vec.y)/l, double(vec.z)/l, double(vec.a)/l);
}

template <typename T>
bool isUnit(const kvec4<T>& vec) {
	return vec.len() == 1.0;
}

template <typename A, typename B>
A dotProduct(const kvec4<A>& v1, const kvec4<B>& v2) {
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.a*v2.a;
}

using vec4b = kvec4<uint8>;
using vec4i = kvec4<int>;

// files and strings
bool strcmpCI(const string& strl, const string& strr);	// case insensitive string comparison
bool is_num(const string& str);
bool findChar(const string& str, char c, size_t* id=nullptr);
bool findString(const string& str, const string& c, size_t* id=nullptr);
string modifyCase(string str, ETextCase caseChange);
vector<string> getWords(const string& line, bool skipCommas=false);
bool splitIniLine(const string& line, string* arg, string* val, string* key=nullptr, size_t* id=nullptr);
fs::path delExt(const fs::path& path);					// remove file extension from filepath/filename
std::istream& readLine(std::istream& ifs, string& str);

// graphics
bool inRect(const SDL_Rect& rect, vec2i point);
bool needsCrop(const SDL_Rect& crop);
SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame);
SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop);
SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop);

// other
void PrintInfo();
string getRendererName(int id);
vector<string> getAvailibleRenderers(bool trustedOnly=false);

// convertions
bool stob(const string& str);
string btos(bool b);
vec2i pix(const vec2f& p);
int pixX(float p);
int pixY(float p);
vec2f prc(const vec2i& p);
float prcX(int p);
float prcY(int p);

template <typename T>
T to_num(const string& str) {
	T val;
	std::stringstream ss(str);
	ss >> val;
	return val;
}

template <typename T>
string to_str(T val) {
	std::stringstream ss;
	ss << val;
	return ss.str();
}

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
