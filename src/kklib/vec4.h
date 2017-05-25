#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

namespace kk {

template <typename T>
struct vec4 {
	vec4(const T& N=0) :
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

	T len() const {
		return length(*this);
	}
	vec4 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
	}
	template <typename A>
	T dot(const vec4& vec) const {
		return dotP(*this, vec);
	}

	union { T x, r; };
	union { T y, g; };
	union { T z, b, w; };
	union { T a,    h; };
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

template <typename T>
T length(const vec4<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z + vec.a*vec.a);
}

template <typename T>
vec4<T> normalize(const vec4<T>& vec) {
	T l = vec.len();
	return vec4<T>(vec.x/l, vec.y/l, vec.z/l, vec.a/l);
}

template <typename T>
bool isUnit(const vec4<T>& vec) {
	return vec.len() == 1;
}

template <typename A, typename B>
A dot(const vec4<A>& v0, const vec4<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z + v0.a*v1.a;
}

}
