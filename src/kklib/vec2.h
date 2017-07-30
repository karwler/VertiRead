#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace kk {

template <typename T>
struct vec2 {
	vec2(const T& N=T(0)) :
		x(N), y(N)
	{}
	vec2(const T& X, const T& Y) :
		x(X), y(Y)
	{}
	template <typename A>
	vec2(const vec2<A>& V) :
		x(V.x), y(V.y)
	{}

	T& operator[](char i) {
		if (i == 0)
			return x;
		return y;
	}
	const T& operator[](char i) const {
		if (i == 0)
			return x;
		return y;
	}

	vec2& operator=(const vec2& v) {
		x = v.x;
		y = v.y;
		return *this;
	}
	vec2& operator+=(const vec2& v) {
		x += v.x;
		y += v.y;
		return *this;
	}
	vec2& operator-=(const vec2& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}
	vec2& operator*=(const vec2& v) {
		x *= v.x;
		y *= v.y;
		return *this;
	}
	vec2& operator/=(const vec2& v) {
		x /= v.x;
		y /= v.y;
		return *this;
	}

	bool isNull() const {
		return x == T(0) && y == T(0);
	}
	bool hasNull() const {
		return x == T(0) || y == T(0);
	}

	T len() const {
		return std::sqrt(x*x + y*y);
	}
	vec2 norm() const {
		T l = len();
		return vec2(x/l, y/l);
	}
	bool isUnit() const {
		return len() == T(1);
	}
	template <typename A>
	T dot(const vec2<A>& vec) const {
		return dotP(*this, vec);
	}
	template <typename A>
	T cross(const vec2<A>& vec) const {
		return crossP(*this, vec);
	}
	template <typename A>
	vec2 refl(const vec2<A>& nrm) const {
		return reflect(*this, nrm);
	}
	vec2 rot(T ang) const {
		return vec2(x*std::cos(ang) - y*std::sin(ang), x*std::sin(ang) + y*std::cos(ang));
	}

	union { T x, w; };
	union { T y, h; };
};

template <typename A, typename B>
vec2<A> operator+(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x + b.x, a.y + b.y);
}
template <typename A, typename B>
vec2<A> operator+(const vec2<A>& a, const B& b) {
	return vec2<A>(a.x + b, a.y + b);
}
template <typename A, typename B>
vec2<A> operator+(const A& a, const vec2<B>& b) {
	return vec2<A>(a + b.x, a + b.y);
}

template <typename A, typename B>
vec2<A> operator-(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x - b.x, a.y - b.y);
}
template <typename A, typename B>
vec2<A> operator-(const vec2<A>& a, const B& b) {
	return vec2<A>(a.x - b, a.y - b);
}
template <typename A, typename B>
vec2<A> operator-(const A& a, const vec2<B>& b) {
	return vec2<A>(a - b.x, a - b.y);
}

template <typename A, typename B>
vec2<A> operator*(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x * b.x, a.y * b.y);
}
template <typename A, typename B>
vec2<A> operator*(const vec2<A>& a, const B& b) {
	return vec2<A>(a.x * b, a.y * b);
}
template <typename A, typename B>
vec2<A> operator*(const A& a, const vec2<B>& b) {
	return vec2<A>(a * b.x, a * b.y);
}

template <typename A, typename B>
vec2<A> operator/(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x / b.x, a.y / b.y);
}
template <typename A, typename B>
vec2<A> operator/(const vec2<A>& a, const B& b) {
	return vec2<A>(a.x / b, a.y / b);
}
template <typename A, typename B>
vec2<A> operator/(const A& a, const vec2<B>& b) {
	return vec2<A>(a / b.x, a / b.y);
}

template <typename A, typename B>
bool operator==(const vec2<A>& a, const vec2<B>& b) {
	return a.x == b.x && a.y == b.y;
}
template <typename A, typename B>
bool operator==(const vec2<A>& a, const B& b) {
	return a.x == b && a.y == b;
}
template <typename A, typename B>
bool operator==(const A& a, const vec2<B>& b) {
	return a == b.x && a == b.y;
}

template <typename A, typename B>
bool operator!=(const vec2<A>& a, const vec2<B>& b) {
	return a.x != b.x || a.y != b.y;
}
template <typename A, typename B>
bool operator!=(const vec2<A>& a, const B& b) {
	return a.x != b || a.y != b;
}
template <typename A, typename B>
bool operator!=(const A& a, const vec2<B>& b) {
	return a != b.x || a != b.y;
}

template <typename A, typename B>
A dotP(const vec2<A>& v0, const vec2<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y;
}

template <typename A, typename B>
A crossP(const vec2<A>& v0, const vec2<B>& v1) {
	return v0.x*v1.y - v0.y*v1.x;
}

template <typename A, typename B>
vec2<A> reflect(const vec2<A>& vec, vec2<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - A(2) * dotP(vec, nrm) * nrm;
}

}
