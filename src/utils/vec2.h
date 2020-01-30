#pragma once

#include <algorithm>
#include <cmath>
#include <string>

using std::string;
using std::wstring;
using std::to_string;

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using wchar = wchar_t;

template <class T>
struct vec2 {
	union { T x, w, u, b; };
	union { T y, h, v, t; };

	vec2() = default;
	template <class A> constexpr vec2(A n);
	template <class A, class B> constexpr vec2(A x, B y);
	template <class A> constexpr vec2(const vec2<A>& v);

	template <class I> T& operator[](I i);
	template <class I> constexpr T operator[](I i) const;

	vec2& operator+=(const vec2& v);
	vec2& operator-=(const vec2& v);
	vec2& operator*=(const vec2& v);
	vec2& operator/=(const vec2& v);
	vec2& operator%=(const vec2& v);
	vec2& operator&=(const vec2& v);
	vec2& operator|=(const vec2& v);
	vec2& operator^=(const vec2& v);
	vec2& operator<<=(const vec2& v);
	vec2& operator>>=(const vec2& v);
	vec2& operator++();
	vec2 operator++(int);
	vec2& operator--();
	vec2 operator--(int);

	template <class F, class... A> vec2& set(const string& str, F strtox, A... args);
	constexpr bool has(T n) const;
	constexpr bool hasNot(T n) const;
	constexpr T length() const;
	constexpr T area() const;
	constexpr vec2 swap() const;
	vec2 swap(bool yes) const;
	static vec2 swap(T x, T y, bool yes);
	constexpr vec2 min(const vec2& min) const;
	constexpr vec2 max(const vec2& max) const;
	constexpr vec2 clamp(const vec2& min, const vec2& max) const;
};

template <class T> template <class A>
constexpr vec2<T>::vec2(A n) :
	x(T(n)),
	y(T(n))
{}

template <class T> template <class A, class B>
constexpr vec2<T>::vec2(A x, B y) :
	x(T(x)),
	y(T(y))
{}

template <class T> template <class A>
constexpr vec2<T>::vec2(const vec2<A>& v) :
	x(T(v.x)),
	y(T(v.y))
{}

template <class T> template <class I>
T& vec2<T>::operator[](I i) {
	return (&x)[i];
}

template <class T> template <class I>
constexpr T vec2<T>::operator[](I i) const {
	return (&x)[i];
}

template <class T>
vec2<T>& vec2<T>::operator+=(const vec2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator-=(const vec2& v) {
	x -= v.x;
	y -= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator*=(const vec2& v) {
	x *= v.x;
	y *= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator/=(const vec2& v) {
	x /= v.x;
	y /= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator%=(const vec2& v) {
	x %= v.x;
	y %= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator&=(const vec2& v) {
	x &= v.x;
	y &= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator|=(const vec2& v) {
	x |= v.x;
	y |= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator^=(const vec2& v) {
	x ^= v.x;
	y ^= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator<<=(const vec2& v) {
	x <<= v.x;
	y <<= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator>>=(const vec2& v) {
	x >>= v.x;
	y >>= v.y;
	return *this;
}

template <class T>
vec2<T>& vec2<T>::operator++() {
	x++;
	y++;
	return *this;
}

template <class T>
vec2<T> vec2<T>::operator++(int) {
	vec2 t = *this;
	x++;
	y++;
	return t;
}

template <class T>
vec2<T>& vec2<T>::operator--() {
	x--;
	y--;
	return *this;
}

template <class T>
vec2<T> vec2<T>::operator--(int) {
	vec2 t = *this;
	x--;
	y--;
	return t;
}

template <class T> template <class F, class... A>
vec2<T>& vec2<T>::set(const string& str, F strtox, A... args) {
	const char* pos = str.c_str();
	for (; *pos > '\0' && *pos <= ' '; pos++);

	for (uint i = 0; *pos && i < 2; i++) {
		char* end;
		if (T num = T(strtox(pos, &end, args...)); end != pos) {
			(*this)[i] = num;
			for (pos = end; *pos > '\0' && *pos <= ' '; pos++);
		} else
			pos++;
	}
	return *this;
}

template <class T>
constexpr bool vec2<T>::has(T n) const {
	return x == n || y == n;
}

template <class T>
constexpr bool vec2<T>::hasNot(T n) const {
	return x != n && y != n;
}

template <class T>
constexpr T vec2<T>::length() const {
	return std::sqrt(x*x + y*y);
}

template <class T>
constexpr T vec2<T>::area() const {
	return x * y;
}

template <class T>
constexpr vec2<T> vec2<T>::swap() const {
	return vec2(y, x);
}

template <class T>
vec2<T> vec2<T>::swap(bool yes) const {
	return yes ? swap() : *this;
}

template <class T>
vec2<T> vec2<T>::swap(T x, T y, bool yes) {
	return yes ? vec2(y, x) : vec2(x, y);
}

template<class T>
constexpr vec2<T> vec2<T>::min(const vec2<T>& min) const {
	return vec2<T>(std::min(x, min.x), std::min(y, min.y));
}

template<class T>
constexpr vec2<T> vec2<T>::max(const vec2<T>& max) const {
	return vec2<T>(std::max(x, max.x), std::max(y, max.y));
}

template<class T>
constexpr vec2<T> vec2<T>::clamp(const vec2<T>& min, const vec2<T>& max) const {
	return vec2<T>(std::clamp(x, min.x, max.x), std::clamp(y, min.y, max.y));
}

template <class T>
constexpr vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x + b.x, a.y + b.y);
}

template <class T>
constexpr vec2<T> operator+(const vec2<T>& a, T b) {
	return vec2<T>(a.x + b, a.y + b);
}

template <class T>
constexpr vec2<T> operator+(T a, const vec2<T>& b) {
	return vec2<T>(a + b.x, a + b.y);
}

template <class T>
constexpr vec2<T> operator-(const vec2<T>& a) {
	return vec2<T>(-a.x, -a.y);
}

template <class T>
constexpr vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x - b.x, a.y - b.y);
}

template <class T>
constexpr vec2<T> operator-(const vec2<T>& a, T b) {
	return vec2<T>(a.x - b, a.y - b);
}

template <class T>
constexpr vec2<T> operator-(T a, const vec2<T>& b) {
	return vec2<T>(a - b.x, a - b.y);
}

template <class T>
constexpr vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x * b.x, a.y * b.y);
}

template <class T>
constexpr vec2<T> operator*(const vec2<T>& a, T b) {
	return vec2<T>(a.x * b, a.y * b);
}

template <class T>
constexpr vec2<T> operator*(T a, const vec2<T>& b) {
	return vec2<T>(a * b.x, a * b.y);
}

template <class T>
constexpr vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x / b.x, a.y / b.y);
}

template <class T>
constexpr vec2<T> operator/(const vec2<T>& a, T b) {
	return vec2<T>(a.x / b, a.y / b);
}

template <class T>
constexpr vec2<T> operator/(T a, const vec2<T>& b) {
	return vec2<T>(a / b.x, a / b.y);
}

template <class T>
constexpr vec2<T> operator%(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x % b.x, a.y % b.y);
}

template <class T>
constexpr vec2<T> operator%(const vec2<T>& a, T b) {
	return vec2<T>(a.x % b, a.y % b);
}

template <class T>
constexpr vec2<T> operator%(T a, const vec2<T>& b) {
	return vec2<T>(a % b.x, a % b.y);
}

template <class T>
constexpr vec2<T> operator~(const vec2<T>& a) {
	return vec2<T>(~a.x, ~a.y);
}

template <class T>
constexpr vec2<T> operator&(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x & b.x, a.y & b.y);
}

template <class T>
constexpr vec2<T> operator&(const vec2<T>& a, T b) {
	return vec2<T>(a.x & b, a.y & b);
}

template <class T>
constexpr vec2<T> operator&(T a, const vec2<T>& b) {
	return vec2<T>(a & b.x, a & b.y);
}

template <class T>
constexpr vec2<T> operator|(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x | b.x, a.y | b.y);
}

template <class T>
constexpr vec2<T> operator|(const vec2<T>& a, T b) {
	return vec2<T>(a.x | b, a.y | b);
}

template <class T>
constexpr vec2<T> operator|(T a, const vec2<T>& b) {
	return vec2<T>(a | b.x, a | b.y);
}

template <class T>
constexpr vec2<T> operator^(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x ^ b.x, a.y ^ b.y);
}

template <class T>
constexpr vec2<T> operator^(const vec2<T>& a, T b) {
	return vec2<T>(a.x ^ b, a.y ^ b);
}

template <class T>
constexpr vec2<T> operator^(T a, const vec2<T>& b) {
	return vec2<T>(a ^ b.x, a ^ b.y);
}

template <class T>
constexpr bool operator==(const vec2<T>& a, const vec2<T>& b) {
	return a.x == b.x && a.y == b.y;
}

template <class T>
constexpr bool operator==(const vec2<T>& a, T b) {
	return a.x == b && a.y == b;
}

template <class T>
constexpr bool operator==(T a, const vec2<T>& b) {
	return a == b.x && a == b.y;
}

template <class T>
constexpr bool operator!=(const vec2<T>& a, const vec2<T>& b) {
	return a.x != b.x || a.y != b.y;
}

template <class T>
constexpr bool operator!=(const vec2<T>& a, T b) {
	return a.x != b || a.y != b;
}

template <class T>
constexpr bool operator!=(T a, const vec2<T>& b) {
	return a != b.x || a != b.y;
}
