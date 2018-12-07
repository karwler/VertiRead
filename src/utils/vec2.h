#pragma once

#include <cmath>
#include <string>

template <class T>
struct vec2 {
	vec2();
	template <class A> vec2(const A& n);
	template <class A, class B> vec2(const A& x, const B& y);
	template <class A, class B> vec2(const A& vx, const B& vy, bool swap);
	template <class A> vec2(const vec2<A>& v);
	template <class A> vec2(const vec2<A>& v, bool swap);
	~vec2() {}	// for some reason this needs to be here so msvc doesn't bitch around
	
	T& operator[](unsigned int i);
	const T& operator[](unsigned int i) const;

	vec2& operator=(const vec2& v);
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

	template <class F, class... A> vec2 set(const std::string& str, F strtox, A... args);
	bool has(const T& n) const;
	bool hasNot(const T& n) const;
	T length() const;
	vec2 swap() const;
	vec2 swap(bool yes) const;

	union { T x, w, u, b; };
	union { T y, h, v, t; };
};

template <class T>
vec2<T>::vec2() :
	x(T(0)),
	y(T(0))
{}

template <class T> template <class A>
vec2<T>::vec2(const A& n) :
	x(T(n)),
	y(T(n))
{}

template <class T> template <class A, class B>
vec2<T>::vec2(const A& x, const B& y) :
	x(T(x)),
	y(T(y))
{}

template <class T> template <class A, class B>
vec2<T>::vec2(const A& vx, const B& vy, bool swap) {
	if (swap) {
		y = T(vx);
		x = T(vy);
	} else {
		y = T(vy);
		x = T(vx);
	}
}

template <class T> template <class A>
vec2<T>::vec2(const vec2<A>& v) :
	x(T(v.x)),
	y(T(v.y))
{}

template <class T> template <class A>
vec2<T>::vec2(const vec2<A>& v, bool swap) {
	if (swap) {
		y = T(v.x);
		x = T(v.y);
	} else {
		y = T(v.y);
		x = T(v.x);
	}
}

template <class T>
T& vec2<T>::operator[](unsigned int i) {
	return (&x)[i];
}

template <class T>
const T& vec2<T>::operator[](unsigned int i) const {
	return (&x)[i];
}

template <class T>
vec2<T>& vec2<T>::operator=(const vec2& v) {
	x = v.x;
	y = v.y;
	return *this;
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
vec2<T> vec2<T>::set(const std::string& str, F strtox, A... args) {
	const char* pos = str.c_str();
	for (; *pos > '\0' && *pos <= ' '; pos++);

	for (unsigned int i = 0; *pos && i < 2; i++) {
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
bool vec2<T>::has(const T& n) const {
	return x == n || y == n;
}

template <class T>
bool vec2<T>::hasNot(const T& n) const {
	return x != n && y != n;
}

template <class T>
T vec2<T>::length() const {
	return sqrt(x*x + y*y);
}

template <class T>
vec2<T> vec2<T>::swap() const {
	return vec2(y, x);
}

template <class T>
vec2<T> vec2<T>::swap(bool yes) const {
	return yes ? swap() : *this;
}

template <class T>
vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x + b.x, a.y + b.y);
}

template <class T>
vec2<T> operator+(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x + b, a.y + b);
}

template <class T>
vec2<T> operator+(const T& a, const vec2<T>& b) {
	return vec2<T>(a + b.x, a + b.y);
}

template <class T>
vec2<T> operator-(const vec2<T>& a) {
	return vec2<T>(-a.x, -a.y);
}

template <class T>
vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x - b.x, a.y - b.y);
}

template <class T>
vec2<T> operator-(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x - b, a.y - b);
}

template <class T>
vec2<T> operator-(const T& a, const vec2<T>& b) {
	return vec2<T>(a - b.x, a - b.y);
}

template <class T>
vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x * b.x, a.y * b.y);
}

template <class T>
vec2<T> operator*(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x * b, a.y * b);
}

template <class T>
vec2<T> operator*(const T& a, const vec2<T>& b) {
	return vec2<T>(a * b.x, a * b.y);
}

template <class T>
vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x / b.x, a.y / b.y);
}

template <class T>
vec2<T> operator/(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x / b, a.y / b);
}

template <class T>
vec2<T> operator/(const T& a, const vec2<T>& b) {
	return vec2<T>(a % b.x, a % b.y);
}

template <class T>
vec2<T> operator%(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x % b.x, a.y % b.y);
}

template <class T>
vec2<T> operator%(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x % b, a.y % b);
}

template <class T>
vec2<T> operator%(const T& a, const vec2<T>& b) {
	return vec2<T>(a % b.x, a % b.y);
}

template <class T>
vec2<T> operator~(const vec2<T>& a) {
	return vec2<T>(~a.x, ~a.y);
}

template <class T>
vec2<T> operator&(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x & b.x, a.y & b.y);
}

template <class T>
vec2<T> operator&(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x & b, a.y & b);
}

template <class T>
vec2<T> operator&(const T& a, const vec2<T>& b) {
	return vec2<T>(a & b.x, a & b.y);
}

template <class T>
vec2<T> operator|(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x | b.x, a.y | b.y);
}

template <class T>
vec2<T> operator|(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x | b, a.y | b);
}

template <class T>
vec2<T> operator|(const T& a, const vec2<T>& b) {
	return vec2<T>(a | b.x, a | b.y);
}

template <class T>
vec2<T> operator^(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x ^ b.x, a.y ^ b.y);
}

template <class T>
vec2<T> operator^(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x ^ b, a.y ^ b);
}

template <class T>
vec2<T> operator^(const T& a, const vec2<T>& b) {
	return vec2<T>(a ^ b.x, a ^ b.y);
}

template <class T>
vec2<T> operator<<(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x << b.x, a.y << b.y);
}

template <class T>
vec2<T> operator<<(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x << b, a.y << b);
}

template <class T>
vec2<T> operator<<(const T& a, const vec2<T>& b) {
	return vec2<T>(a << b.x, a << b.y);
}

template <class T>
vec2<T> operator>>(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x >> b.x, a.y >> b.y);
}

template <class T>
vec2<T> operator>>(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x >> b, a.y >> b);
}

template <class T>
vec2<T> operator>>(const T& a, const vec2<T>& b) {
	return vec2<T>(a >> b.x, a >> b.y);
}

template <class T>
bool operator==(const vec2<T>& a, const vec2<T>& b) {
	return a.x == b.x && a.y == b.y;
}

template <class T>
bool operator==(const vec2<T>& a, const T& b) {
	return a.x == b && a.y == b;
}

template <class T>
bool operator==(const T& a, const vec2<T>& b) {
	return a == b.x && a == b.y;
}

template <class T>
bool operator!=(const vec2<T>& a, const vec2<T>& b) {
	return a.x != b.x || a.y != b.y;
}

template <class T>
bool operator!=(const vec2<T>& a, const T& b) {
	return a.x != b || a.y != b;
}

template <class T>
bool operator!=(const T& a, const vec2<T>& b) {
	return a != b.x || a != b.y;
}
