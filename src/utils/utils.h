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

// pi
#define PI M_PI

#define D2R(x) (x)/180*PI	// degrees to radians
#define R2D(x) (x)*180/PI	// radians to degrees

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
class ListBox;
class Capturer;

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
	kvec2& operator=(const kvec2<A>& v) {
		x = v.x;
		y = v.y;
		return *this;
	}

	friend kvec2 operator+(const kvec2& a, const kvec2& b) {
		return kvec2(a.x + b.x, a.y + b.y);
	}
	template <typename A>
	friend kvec2 operator+(const kvec2& a, const kvec2<A>& b) {
		return kvec2(a.x + b.x, a.y + b.y);
	}

	friend kvec2 operator-(const kvec2& a, const kvec2& b) {
		return kvec2(a.x - b.x, a.y - b.y);
	}
	template <typename A>
	friend kvec2 operator-(const kvec2& a, const kvec2<A>& b) {
		return kvec2(a.x - b.x, a.y - b.y);
	}

	friend kvec2 operator*(const kvec2& a, const kvec2& b) {
		return kvec2(a.x * b.x, a.y * b.y);
	}
	template <typename A>
	friend kvec2 operator*(const kvec2& a, const kvec2<A>& b) {
		return kvec2(a.x * b.x, a.y * b.y);
	}

	friend kvec2 operator/(const kvec2& a, const kvec2& b) {
		return kvec2(a.x / b.x, a.y / b.y);
	}
	template <typename A>
	friend kvec2 operator/(const kvec2& a, const kvec2<A>& b) {
		return kvec2(a.x / b.x, a.y / b.y);
	}

	kvec2& operator+=(const kvec2& v) {
		x += v.x;
		y += v.y;
		return *this;
	}
	template <typename A>
	kvec2& operator+=(const kvec2<A>& v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	kvec2& operator-=(const kvec2& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}
	template <typename A>
	kvec2& operator-=(const kvec2<A>& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}

	kvec2& operator*=(const kvec2& v) {
		x *= v.x;
		y *= v.y;
		return *this;
	}
	template <typename A>
	kvec2& operator*=(const kvec2<A>& v) {
		x *= v.x;
		y *= v.y;
		return *this;
	}

	kvec2& operator/=(const kvec2& v) {
		x /= v.x;
		y /= v.y;
		return *this;
	}
	template <typename A>
	kvec2& operator/=(const kvec2<A>& v) {
		x /= v.x;
		y /= v.y;
		return *this;
	}

	friend bool operator==(const kvec2& a, const kvec2& b) {
		return a.x == b.x && a.y == b.y;
	}
	template <typename A>
	friend bool operator==(const kvec2& a, const kvec2<A>& b) {
		return a.x == b.x && a.y == b.y;
	}

	friend bool operator!=(const kvec2& a, const kvec2& b) {
		return a.x != b.x || a.y != b.y;
	}
	template <typename A>
	friend bool operator!=(const kvec2& a, const kvec2<A>& b) {
		return a.x != b.x || a.y != b.y;
	}

	bool isNull() const {
		return x == 0 && y == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0;
	}

	T len() const {
		return length(*this);
	}
	kvec2 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
	}
	template <typename A>
	T dot(const kvec2<A>& vec) const {
		return dotP(*this, vec);
	}
	template <typename A>
	T cross(const kvec2<A>& vec) const {
		return crossP(*this, vec);
	}
	template <typename A>
	kvec2 refl(const kvec2<A>& nrm) const {
		return reflect(*this, nrm);
	}
	kvec2 rot(T ang) const {
		return rotate(*this, ang);
	}
};

template <typename A>
A length(const kvec2<A>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y);
}

template <typename A>
kvec2<A> normalize(const kvec2<A>& vec) {
	A l = length(vec);
	return kvec2<A>(vec.x/l, vec.y/l);
}

template <typename A>
bool isUnit(const kvec2<A>& vec) {
	return length(vec) == 1;
}

template <typename A, typename B>
A dotP(const kvec2<A>& v0, const kvec2<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y;
}

template <typename A, typename B>
A crossP(const kvec2<A>& v0, const kvec2<B>& v1) {
	return v0.x*v1.y - v0.y*v1.x;
}

template <typename A, typename B>
kvec2<A> reflect(const kvec2<A>& vec, kvec2<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - 2 * dotP(vec, nrm) * nrm;
}

template <typename A>
kvec2<A> rotate(const kvec2<A>& vec, A ang) {
	ang = D2R(ang);
	return kvec2<A>(vec.x*std::cos(ang) - vec.y*std::sin(ang), vec.x*std::sin(ang) + vec.y*std::cos(ang));
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
	kvec3& operator=(const kvec3<A>& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	friend kvec3 operator+(const kvec3& a, const kvec3& b) {
		return kvec3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	template <typename A>
	friend kvec3 operator+(const kvec3& a, const kvec3<A>& b) {
		return kvec3(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	friend kvec3 operator-(const kvec3& a, const kvec3& b) {
		return kvec3(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	template <typename A>
	friend kvec3 operator-(const kvec3& a, const kvec3<A>& b) {
		return kvec3(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	friend kvec3 operator*(const kvec3& a, const kvec3& b) {
		return kvec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}
	template <typename A>
	friend kvec3 operator*(const kvec3& a, const kvec3<A>& b) {
		return kvec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	friend kvec3 operator/(const kvec3& a, const kvec3& b) {
		return kvec3(a.x / b.x, a.y / b.y, a.z / b.z);
	}
	template <typename A>
	friend kvec3 operator/(const kvec3& a, const kvec3<A>& b) {
		return kvec3(a.x / b.x, a.y / b.y, a.z / b.z);
	}

	kvec3& operator+=(const kvec3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	template <typename A>
	kvec3& operator+=(const kvec3<A>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	kvec3& operator-=(const kvec3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	template <typename A>
	kvec3& operator-=(const kvec3<A>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	kvec3& operator*=(const kvec3& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	template <typename A>
	kvec3& operator*=(const kvec3<A>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	kvec3& operator/=(const kvec3& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}
	template <typename A>
	kvec3& operator/=(const kvec3<A>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	friend bool operator==(const kvec3& a, const kvec3& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
	template <typename A>
	friend bool operator==(const kvec3& a, const kvec3<A>& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	friend bool operator!=(const kvec3& a, const kvec3& b) {
		return a.x != b.x || a.y != b.y || a.z == b.z;
	}
	template <typename A>
	friend bool operator!=(const kvec3& a, const kvec3<A>& b) {
		return a.x != b.x || a.y != b.y || a.z != b.z;
	}

	bool isNull() const {
		return x == 0 && y == 0 && z == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0 || z == 0;
	}

	T len() const {
		return length(*this);
	}
	kvec3 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
	}
	template <typename A>
	T dot(const kvec3<A>& vec) const {
		return dotP(*this, vec);
	 }
	template <typename A>
	kvec3 cross(const kvec3<A>& vec) const {
		return crossP(*this, vec);
	}
	template <typename A>
	kvec3 refl(const kvec3<A>& nrm) const {
		return reflect(*this, nrm);
	}
	template <typename A>
	kvec3 rot(const kvec3<A>& ang) const {
		return rotate(*this, ang);
	}
};

template <typename T>
T length(const kvec3<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

template <typename T>
kvec3<T> normalize(const kvec3<T>& vec) {
	T l = vec.len();
	return kvec3<T>(vec.x/l, vec.y/l, vec.z/l);
}

template <typename T>
bool isUnit(const kvec3<T>& vec) {
	return vec.len() == 1;
}

template <typename A, typename B>
A dotP(const kvec3<A>& v0, const kvec3<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z;
}

template <typename A, typename B>
kvec3<A> crossP(const kvec3<A>& v0, const kvec3<B>& v1) {
	return kvec3<A>(v0.y*v1.z - v0.z*v1.y, v0.z*v1.x - v0.x*v1.z, v0.x*v1.y - v0.y*v1.x);
}

template <typename A, typename B>
kvec3<A> reflect(const kvec3<A>& vec, kvec3<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - 2 * dotP(vec, nrm) * nrm;
}

template <typename A, typename B>
kvec3<A> rotate(const kvec3<A>& vec, kvec3<B> ang) {
	ang = kvec3<B>(D2R(ang.x), D2R(ang.y), D2R(ang.z));
	kvec3<A> ret;

	// X-axis
	ret.y = vec.y*std::cos(ang.x) - vec.z*std::sin(ang.x);
	ret.z = vec.y*std::sin(ang.x) + vec.z*std::cos(ang.x);

	// Y-axis
	ret.x =  vec.x*std::cos(ang.y) + vec.z*std::sin(ang.y);
	ret.z = -vec.x*std::sin(ang.y) + vec.z*std::cos(ang.y);

	// Z-axis
	ret.x = vec.x*std::cos(ang.z) - vec.y*std::sin(ang.z);
	ret.y = vec.x*std::sin(ang.z) + vec.y*std::cos(ang.z);

	return ret;

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
	kvec4& operator=(const kvec4<A>& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		a = v.a;
		return *this;
	}

	friend kvec4 operator+(const kvec4& a, const kvec4& b) {
		return kvec4(a.x + b.x, a.y + b.y, a.z + b.z, a.a + b.a);
	}
	template <typename A>
	friend kvec4 operator+(const kvec4& a, const kvec4<A>& b) {
		return kvec4(a.x + b.x, a.y + b.y, a.z + b.z, a.a + b.a);
	}

	friend kvec4 operator-(const kvec4& a, const kvec4& b) {
		return kvec4(a.x - b.x, a.y - b.y, a.z - b.z, a.a - b.a);
	}
	template <typename A>
	friend kvec4 operator-(const kvec4& a, const kvec4<A>& b) {
		return kvec4(a.x - b.x, a.y - b.y, a.z - b.z, a.a - b.a);
	}

	friend kvec4 operator*(const kvec4& a, const kvec4& b) {
		return kvec4(a.x * b.x, a.y * b.y, a.z * b.z, a.a * b.a);
	}
	template <typename A>
	friend kvec4 operator*(const kvec4& a, const kvec4<A>& b) {
		return kvec4(a.x * b.x, a.y * b.y, a.z * b.z, a.a * b.a);
	}

	friend kvec4 operator/(const kvec4& a, const kvec4& b) {
		return kvec4(a.x / b.x, a.y / b.y, a.z / b.z, a.a / b.a);
	}
	template <typename A>
	friend kvec4 operator/(const kvec4& a, const kvec4<A>& b) {
		return kvec4(a.x / b.x, a.y / b.y, a.z / b.z, a.a / b.a);
	}

	kvec4& operator+=(const kvec4& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		a += v.a;
		return *this;
	}
	template <typename A>
	kvec4& operator+=(const kvec4<A>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		a += v.a;
		return *this;
	}

	kvec4& operator-=(const kvec4& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		a -= v.a;
		return *this;
	}
	template <typename A>
	kvec4& operator-=(const kvec4<A>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		a -= v.a;
		return *this;
	}

	kvec4& operator*=(const kvec4& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		a *= v.a;
		return *this;
	}
	template <typename A>
	kvec4& operator*=(const kvec4<A>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		a *= v.a;
		return *this;
	}

	kvec4& operator/=(const kvec4& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		a /= v.a;
		return *this;
	}
	template <typename A>
	kvec4& operator/=(const kvec4<A>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		a /= v.a;
		return *this;
	}

	friend bool operator==(const kvec4& a, const kvec4& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z && a.a == b.a;
	}
	template <typename A>
	friend bool operator==(const kvec4& a, const kvec4<A>& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z && a.a == b.a;
	}

	friend bool operator!=(const kvec4& a, const kvec4& b) {
		return a.x != b.x || a.y != b.y || a.z != b.z || a.a != b.a;
	}
	template <typename A>
	friend bool operator!=(const kvec4& a, const kvec4<A>& b) {
		return a.x != b.x || a.y != b.y || a.z != b.z || a.a != b.a;
	}

	T len() const {
		return length(*this);
	}
	kvec4 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
	}
	template <typename A>
	T dot(const kvec4& vec) const {
		return dotP(*this, vec);
	}
};

template <typename T>
T length(const kvec4<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z + vec.a*vec.a);
}

template <typename T>
kvec4<T> normalize(const kvec4<T>& vec) {
	T l = vec.len();
	return kvec4<T>(vec.x/l, vec.y/l, vec.z/l, vec.a/l);
}

template <typename T>
bool isUnit(const kvec4<T>& vec) {
	return vec.len() == 1;
}

template <typename A, typename B>
A dot(const kvec4<A>& v0, const kvec4<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z + v0.a*v1.a;
}

using vec4b = kvec4<uint8>;
using vec4i = kvec4<int>;

// files and strings
bool isNum(const std::string& str);
bool strcmpCI(const std::string& strl, const std::string& strr);
bool findChar(const std::string& str, char c, size_t* id=nullptr);
bool findString(const std::string& str, const std::string& c, size_t* id=nullptr);
std::vector<std::string> getWords(const std::string& line, char splitter, char spacer);
bool splitIniLine(const std::string& line, std::string* arg, std::string* val, std::string* key=nullptr, bool* isTitle=nullptr, size_t* id=nullptr);
std::istream& readLine(std::istream& ifs, std::string& str);

fs::path delExt(const fs::path& path);					// remove file extension from filepath/filename
string modifyCase(string str, ETextCase caseChange);

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
bool stob(const std::string& str);
std::string btos(bool b);

vec2i pix(const vec2f& p);
int pixX(float p);
int pixY(float p);
vec2f prc(const vec2i& p);
float prcX(int p);
float prcY(int p);

float axisToFloat(int16 axisValue);
int16 floatToAxis(float axisValue);

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
