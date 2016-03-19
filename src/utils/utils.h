#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <vector>
#include <list>
#include <map>
#include <string>
using namespace std;
namespace fs = boost::filesystem;

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

using byte = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using cchar = const char;
using cstr = const char*;

template<class T>
class kptr {
public:
	kptr(T* p = nullptr) : ptr(p) {}
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
	T* operator->() const {
		return ptr;
	}
	T& operator*() const {
		return *ptr;
	}

	T* reset(T* p = nullptr) {
		if (ptr)
			delete ptr;
		ptr = p;
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

template<typename T>
struct kvec2 {
	kvec2(T X = 0, T Y = 0) : x(X), y(Y) {}
	T x, y;

	kvec2 operator=(T n) {
		x = n;
		y = n;
		return kvec2(x, y);
	}
	kvec2 operator+(const kvec2& n) {
		return kvec2(x + n.x, y + n.y);
	}
	kvec2 operator-(const kvec2& n) {
		return kvec2(x - n.x, y - n.y);
	}
	kvec2 operator*(const kvec2& n) {
		return kvec2(x * n.x, y * n.y);
	}
	kvec2 operator*(T n) {
		return kvec2(x*n, y*n);
	}
	kvec2 operator/(const kvec2& n) {
		return vec2(x / n.x, y / n.y);
	}
	kvec2 operator+=(const kvec2& n) {
		x += n.x;
		y += n.y;
		return kvec2(x, y);
	}
	kvec2 operator-=(const kvec2& n) {
		x -= n.x;
		y -= n.y;
		return kvec2(x, y);
	}
	kvec2 operator*=(const kvec2& n) {
		x *= n.x;
		y *= n.y;
		return kvec2(x, y);
	}
	kvec2 operator*=(T n) {
		x *= n;
		y *= n;
		return kvec2(x, y);
	}
	kvec2 operator/=(const kvec2& n) {
		x /= n.x;
		y /= n.y;
		return kvec2(x, y);
	}
	bool operator==(const kvec2& n) {
		return x == n.x && y == n.y;
	}
	bool isNull() {
		return x == NULL && y == NULL;
	}
	double len() const { return sqrt(x*x + y*y); }
};
using vec2i = kvec2<int>;
using vec2f = kvec2<float>;

template<typename T>
struct kvec3 {
	kvec3(T X = 0, T Y = 0, T Z = 0) : x(X), y(Y), z(Z) {}
	T x, y, z;
	
	kvec3 operator=(T n) {
		x = n;
		y = n;
		z = n;
		return kvec3(x, y, z);
	}
	kvec3 operator+(const kvec3& n) {
		return kvec3(x + n.x, y + n.y, z + n.z);
	}
	kvec3 operator-(const kvec3& n) {
		return kvec3(x - n.x, y - n.y, z - n.z);
	}
	kvec3 operator*(T n) {
		return kvec3(x*n, y*n, z*n);
	}
	kvec3 operator+=(const kvec3& n) {
		x += n.x;
		y += n.y;
		z += n.z;
		return kvec3(x, y, z);
	}
	kvec3 operator-=(const kvec3& n) {
		x -= n.x;
		y -= n.y;
		z -= n.z;
		return kvec3(x, y, z);
	}
	kvec3 operator*=(T n) {
		x *= n;
		y *= n;
		z *= n;
		return kvec3(x, y, z);
	}
	bool operator==(const kvec3& n) {
		return x == n.x && y == n.y && z == n.z;
	}
	bool isNull() {
		return x == NULL && y == NULL && z == NULL;
	}
};
using vec3b = kvec3<byte>;
using vec3i = kvec3<int>;

template<typename T>
struct kvec4 {
	kvec4(T X = 0, T Y = 0, T Z = 0, T A = 0) : x(X), y(Y), z(Z), a(A) {}
	T x, y, z, a;
	
	kvec4 operator=(T n) {
		x = n;
		y = n;
		z = n;
		a = n;
		return kvec4(x, y, z, a);
	}
	kvec4 operator+(const kvec4& n) {
		return kvec4(x + n.x, y + n.y, z + n.z, a + n.a);
	}
	kvec4 operator-(const kvec4& n) {
		return kvec4(x - n.x, y - n.y, z - n.z, a - n.a);
	}
	kvec4 operator*(T n) {
		return kvec4(x*n, y*n, z*n, a*n);
	}
	kvec4 operator+=(const kvec4& n) {
		x += n.x;
		y += n.y;
		z += n.z;
		a += n.a;
		return kvec4(x, y, z, a);
	}
	kvec4 operator-=(const kvec4& n) {
		x -= n.x;
		y -= n.y;
		z -= n.z;
		a -= n.a;
		return kvec4(x, y, z, a);
	}
	kvec4 operator*=(T n) {
		x *= n;
		y *= n;
		z *= n;
		a *= n;
		return kvec4(x, y, z, a);
	}
	bool operator==(const kvec4& n) {
		return x == n.x && y == n.y && z == n.z && a == n.a;
	}
	bool isNull() {
		return x == NULL && y == NULL && z == NULL && a == NULL;
	}
};
using vec4b = kvec4<byte>;
using vec4i = kvec4<int>;

bool isNumber(string str);
int findChar(string str, char c);
vector<string> getWords(string line, bool skipCommas=true);
int SplitIniLine(string line, string* arg, string* val, string* key = nullptr);
fs::path removeExtension(fs::path path);
bool inRect(const SDL_Rect& rect, vec2i point);
bool needsCrop(const SDL_Rect& crop);
SDL_Rect GetCrop(SDL_Rect item, SDL_Rect frame);
SDL_Surface* CropSurface(SDL_Surface* surface, SDL_Rect& rect, SDL_Rect crop);

string wtos(wstring wstr);
bool stob(string str);
string btos(bool b);
SDL_Keysym stok(string str);
string ktos(SDL_Keysym key);
vec2i pix(vec2f p);
int pixX(float p);
int pixY(float p);
vec2f prc(vec2i p);
float prcX(int p);
float prcY(int p);
string getRendererName(int id);

template<class T>
void Erase(vector<T*>& vec, uint i) {
	delete vec[i];
	vec.erase(vec.begin() + i);
}

template<class T>
void Clear(vector<T*>& vec) {
	for (uint i = 0; i != vec.size(); i++)
		delete vec[i];
	vec.clear();
}

template<class T>
void Erase(list<T*>& lst, uint i) {
	delete lst[i];
	lst.erase(lst.begin() + i);
}

template<class T>
void Clear(list<T*>& lst) {
	for (uint i = 0; i != lst.size(); i++)
		delete lst[i];
	lst.clear();
}
