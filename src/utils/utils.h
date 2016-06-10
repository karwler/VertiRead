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
#include <sstream>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::list;
using std::map;
using std::pair;
using std::make_pair;
using std::string;
using std::to_string;
namespace fs = boost::filesystem;

// undefine main cause it sometimes can cause conflicts
#ifdef main
#undef main
#endif

#ifndef PI
#ifdef M_PI
#define PI M_PI
#endif
#else
#ifndef M_PI
#define M_PI PI
#endif
#endif

#ifdef _WIN32
const string dsep = "\\";
#else
const string dsep = "/";
#endif

using byte = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using cchar = const char;
using cstr = const char*;

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
	T x, y;

	kvec2& operator=(T n) {
		x = n;
		y = n;
		return *this;
	}
	kvec2 operator+(const kvec2& n) const {
		return kvec2(x + n.x, y + n.y);
	}
	kvec2 operator+(T n) const {
		return kvec2(x+n, y+n);
	}
	kvec2 operator-(const kvec2& n) const {
		return kvec2(x - n.x, y - n.y);
	}
	kvec2 operator-(T n) const {
		return kvec2(x-n, y-n);
	}
	kvec2 operator*(const kvec2& n) const {
		return kvec2(x * n.x, y * n.y);
	}
	kvec2 operator*(T n) const {
		return kvec2(x*n, y*n);
	}
	kvec2 operator/(const kvec2& n) const {
		return kvec2(x / n.x, y / n.y);
	}
	kvec2 operator/(T n) const {
		return kvec2(x/n, y/n);
	}
	kvec2& operator+=(const kvec2& n) {
		x += n.x;
		y += n.y;
		return *this;
	}
	kvec2& operator+=(T n) {
		x += n;
		y += n;
		return *this;
	}
	kvec2& operator-=(const kvec2& n) {
		x -= n.x;
		y -= n.y;
		return *this;
	}
	kvec2& operator-=(T n) {
		x -= n;
		y -= n;
		return *this;
	}
	kvec2& operator*=(const kvec2& n) {
		x *= n.x;
		y *= n.y;
		return *this;
	}
	kvec2& operator*=(T n) {
		x *= n;
		y *= n;
		return *this;
	}
	kvec2& operator/=(const kvec2& n) {
		x /= n.x;
		y /= n.y;
		return *this;
	}
	kvec2& operator/=(T n) {
		x /= n;
		y /= n;
		return *this;
	}

	bool operator==(const kvec2& n) const {
		return x == n.x && y == n.y;
	}
	bool operator!=(const kvec2& n) const {
		return x != n.x || y != n.y;
	}
	bool isNull() const {
		return x == 0 && y == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0;
	}

	static double len(const kvec2& v) {
		return sqrt(v.x*v.x + v.y*v.y);
	}
	double len() const { return len(*this); }

	static kvec2 norm(const kvec2& v) {
		double l = len();
		return kvec2(v.x/l, v.y/l);
	}
	kvec2 norm() const { return norm(*this); }

	static double dot(const kvec2& v1, const kvec2& v2) {
		return v1.x*v2.x + v1.y*v2.y;
	}
	double dot(const kvec2& n) const { return dot(*this, n); }
};
using vec2i = kvec2<int>;
using vec2f = kvec2<float>;

// 3D vector
template <typename T>
struct kvec3 {
	kvec3(T N=0) : x(N), y(N), z(N) {}
	kvec3(T X, T Y, T Z) : x(X), y(Y), z(Z) {}
	T x, y, z;
	
	kvec3& operator=(T n) {
		x = n;
		y = n;
		z = n;
		return *this;
	}
	kvec3 operator+(const kvec3& n) const {
		return kvec3(x + n.x, y + n.y, z + n.z);
	}
	kvec3 operator+(T n) const {
		return kvec3(x+n, y+n, z+n);
	}
	kvec3 operator-(const kvec3& n) const {
		return kvec3(x - n.x, y - n.y, z - n.z);
	}
	kvec3 operator-(T n) const {
		return kvec3(x-n, y-n, z-n);
	}
	kvec3 operator*(const kvec3& n) const {
		return kvec3(x * n.x, y * n.y, z * n.z);
	}
	kvec3 operator*(T n) const {
		return kvec3(x*n, y*n, z*n);
	}
	kvec3 operator/(const kvec3& n) const {
		return kvec3(x / n.x, y / n.y, z / n.z);
	}
	kvec3 operator/(T n) const {
		return kvec3(x/n, y/n, z/n);
	}
	kvec3& operator+=(const kvec3& n) {
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
	kvec3& operator-=(const kvec3& n) {
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
	kvec3& operator*=(const kvec3& n) {
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
	kvec3& operator/=(const kvec3& n) {
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

	bool operator==(const kvec3& n) const {
		return x == n.x && y == n.y && z == n.z;
	}
	bool operator!=(const kvec3& n) const {
		return x != n.x || y != n.y || z != n.z;
	}
	bool isNull() const {
		return x == 0 && y == 0 && z == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0 || z == 0;
	}

	static double len(const kvec3& v) {
		return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	}
	double len() const { return len(*this); }

	static kvec3 norm(const kvec3& v) {
		double l = len();
		return kvec3(v.x/l, v.y/l, v.z/l);
	}
	kvec3 norm() const { return norm(*this); }

	static double dot(const kvec3& v1, const kvec3& v2) {
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
	}
	double dot(const kvec3& n) const { return dot(*this, n); }

	static kvec3 cross(const kvec3& v1, const kvec3& v2) {
		return kvec3(v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x);
	}
	kvec3 cross(const kvec3& n) const { return cross(*this, n); }
};
using vec3b = kvec3<byte>;
using vec3i = kvec3<int>;

// 4D vector
template <typename T>
struct kvec4 {
	kvec4(T N=0) : x(N), y(N), z(N), a(N) {}
	kvec4(T X, T Y, T Z, T A) : x(X), y(Y), z(Z), a(A) {}
	T x, y, z, a;
	
	kvec4& operator=(T n) {
		x = n;
		y = n;
		z = n;
		a = n;
		return *this;
	}
	kvec4 operator+(const kvec4& n) const {
		return kvec4(x + n.x, y + n.y, z + n.z, a + n.a);
	}
	kvec4 operator+(T n) const {
		return kvec4(x+n, y+n, z+n, a+n);
	}
	kvec4 operator-(const kvec4& n) const {
		return kvec4(x - n.x, y - n.y, z - n.z, a - n.a);
	}
	kvec4 operator-(T n) const {
		return kvec4(x-n, y-n, z-n, a-n);
	}
	kvec4 operator*(const kvec4& n) const {
		return kvec4(x * n.x, y * n.y, z * n.z, a * n.a);
	}
	kvec4 operator*(T n) const {
		return kvec4(x*n, y*n, z*n, a*n);
	}
	kvec4 operator/(const kvec4& n) const {
		return kvec4(x / n.x, y / n.y, z / n.z, a / n.a);
	}
	kvec4 operator/(T n) const {
		return kvec4(x/n, y/n, z/n, a/n);
	}
	kvec4& operator+=(const kvec4& n) {
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
	kvec4& operator-=(const kvec4& n) {
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
	kvec4& operator*=(const kvec4& n) {
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
	kvec4& operator/=(const kvec4& n) {
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

	bool operator==(const kvec4& n) const {
		return x == n.x && y == n.y && z == n.z && a == n.a;
	}
	bool operator!=(const kvec4& n) const {
		return x != n.x || y != n.y || z != n.z || a != n.a;
	}
	bool isNull() const {
		return x == 0 && y == 0 && z == 0 && a == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0 || z == 0 || a == 0;
	}

	static double len(const kvec4 v) {
		return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.a*v.a);
	}
	double len() const { return len(*this); }

	static kvec4 norm(const kvec4& v) {
		double l = len();
		return kvec4(v.x/l, v.y/l, v.z/l, v.a/l);
	}
	kvec4 norm() const { return norm(*this); }

	static double dot(const kvec4& v1, const kvec4& v2) {
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.a*v2.a;
	}
	double dot(const kvec4& n) const { return dot(*this, n); }
};
using vec4b = kvec4<byte>;
using vec4i = kvec4<int>;

// files and strings
bool is_num(const string& str);
int findChar(const string& str, char c);
int findString(const string& str, const string& c);
vector<string> getWords(const string& line, bool skipCommas=false);
int splitIniLine(const string& line, string* arg, string* val, string* key=nullptr);
fs::path removeExtension(const fs::path& path);

// graphics
bool inRect(const SDL_Rect& rect, vec2i point);
bool needsCrop(const SDL_Rect& crop);
SDL_Rect getCrop(SDL_Rect item, SDL_Rect frame);
SDL_Rect cropRect(const SDL_Rect& rect, const SDL_Rect& crop);
SDL_Surface* cropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop);

// other
void PrintInfo();
string getRendererName(int id);

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
T sto(const string& str) {
	T val;
	std::stringstream ss(str);
	ss >> val;
	return val;
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

template <typename T>
void erase(list<T*>& lst, uint i) {
	delete lst[i];
	lst.erase(lst.begin() + i);
}

template <typename T>
void clear(list<T*>& lst) {
	for (T* it : lst)
		delete it;
	lst.clear();
}
