#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace kk {

template <typename T>
struct vec4 {
	vec4(const T& N=T(0)) :
		x(N), y(N), z(N), a(N)
	{}
	vec4(const T& X, const T& Y, const T& Z, const T& A) :
		x(X), y(Y), z(Z), a(A)
	{}
	template <typename A>
	vec4(const vec4<A>& V) :
		x(V.x), y(V.y), z(V.z), a(V.a)
	{}

	T& operator[](char i) {
		if (i == 0)
			return x;
		if (i == 1)
			return y;
		if (i ==2)
			return z;
		return a;
	}
	const T& operator[](char i) const {
		if (i == 0)
			return x;
		if (i == 1)
			return y;
		if (i ==2)
			return z;
		return a;
	}

	vec4& operator=(const vec4& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		a = v.a;
		return *this;
	}
	vec4& operator+=(const vec4& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		a += v.a;
		return *this;
	}
	vec4& operator-=(const vec4& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		a -= v.a;
		return *this;
	}
	vec4& operator*=(const vec4& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		a *= v.a;
		return *this;
	}
	vec4& operator/=(const vec4& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		a /= v.a;
		return *this;
	}

	bool isNull() const {
		return x == T(0) && y == T(0) && z == T(0) && a == T(0);
	}
	bool hasNull() const {
		return x == T(0) || y == T(0) || z == T(0) || a == T(0);
	}
	
	T len() const {
		return std::sqrt(x*x + y*y + z*z + a*a);
	}
	vec4 norm() const {
		T l = len();
		return vec4(x/l, y/l, z/l, a/l);
	}
	bool isUnit() const {
		return len() == T(1);
	}
	template <typename A>
	T dot(const vec4& vec) const {
		return dotP(*this, vec);
	}

	union { T x, w, r; };
	union { T y, h, g; };
	union { T z, d, b; };
	union { T a; };
};

template <typename A, typename B>
vec4<A> operator+(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x + b.x, a.y + b.y, a.z + b.z, a.a + b.a);
}
template <typename A, typename B>
vec4<A> operator+(const vec4<A>& a, const B& b) {
	return vec4<A>(a.x + b, a.y + b, a.z + b, a.a + b);
}
template <typename A, typename B>
vec4<A> operator+(const A& a, const vec4<B>& b) {
	return vec4<A>(a + b.x, a + b.y, a + b.z, a + b.a);
}

template <typename A, typename B>
vec4<A> operator-(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x - b.x, a.y - b.y, a.z - b.z, a.a - b.a);
}
template <typename A, typename B>
vec4<A> operator-(const vec4<A>& a, const B& b) {
	return vec4<A>(a.x - b, a.y - b, a.z - b, a.a - b);
}
template <typename A, typename B>
vec4<A> operator-(const A& a, const vec4<B>& b) {
	return vec4<A>(a - b.x, a - b.y, a - b.z, a - b.a);
}

template <typename A, typename B>
vec4<A> operator*(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x * b.x, a.y * b.y, a.z * b.z, a.a * b.a);
}
template <typename A, typename B>
vec4<A> operator*(const vec4<A>& a, const B& b) {
	return vec4<A>(a.x * b, a.y * b, a.z * b, a.a * b);
}
template <typename A, typename B>
vec4<A> operator*(const A& a, const vec4<B>& b) {
	return vec4<A>(a * b.x, a * b.y, a * b.z, a * b.a);
}

template <typename A, typename B>
vec4<A> operator/(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x / b.x, a.y / b.y, a.z / b.z, a.a / b.a);
}
template <typename A, typename B>
vec4<A> operator/(const vec4<A>& a, const B& b) {
	return vec4<A>(a.x / b, a.y / b, a.z / b, a.a / b);
}
template <typename A, typename B>
vec4<A> operator/(const A& a, const vec4<B>& b) {
	return vec4<A>(a / b.x, a / b.y, a / b.z, a / b.a);
}

template <typename A, typename B>
bool operator==(const vec4<A>& a, const vec4<B>& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.a == b.a;
}
template <typename A, typename B>
bool operator==(const vec4<A>& a, const B& b) {
	return a.x == b && a.y == b && a.z == b && a.a == b;
}
template <typename A, typename B>
bool operator==(const A& a, const vec4<B>& b) {
	return a == b.x && a == b.y && a == b.z && a == b.a;
}

template <typename A, typename B>
bool operator!=(const vec4<A>& a, const vec4<B>& b) {
	return a.x != b.x || a.y != b.y || a.z != b.z || a.a != b.a;
}
template <typename A, typename B>
bool operator!=(const vec4<A>& a, const B& b) {
	return a.x != b || a.y != b || a.z != b || a.a != b;
}
template <typename A, typename B>
bool operator!=(const A& a, const vec4<B>& b) {
	return a != b.x || a != b.y || a != b.z || a != b.a;
}

template <typename A, typename B>
A dot(const vec4<A>& v0, const vec4<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z + v0.a*v1.a;
}

}
