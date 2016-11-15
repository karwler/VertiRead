#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

namespace kk {

template <typename T>
struct vec4 {
	vec4(T N=0) :
		x(N), y(N), z(N), a(N)
	{}
	vec4(T X, T Y, T Z, T A) :
		x(X), y(Y), z(Z), a(A)
	{}
	template <typename A>
	vec4(const vec4<A>& N) :
		x(N.x), y(N.y), z(N.z), a(N.a)
	{}

	T& operator[](char i) {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}
		return a;
	}
	const T& operator[](char i) const {
		return *this[i];
	}

	template <typename A>
	vec4& operator=(const vec4<A>& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		a = v.a;
		return *this;
	}
	template <typename A>
	vec4& operator+=(const vec4<A>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		a += v.a;
		return *this;
	}
	template <typename A>
	vec4& operator-=(const vec4<A>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		a -= v.a;
		return *this;
	}
	template <typename A>
	vec4& operator*=(const vec4<A>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		a *= v.a;
		return *this;
	}
	template <typename A>
	vec4& operator/=(const vec4<A>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		a /= v.a;
		return *this;
	}

	friend vec4 operator+(const vec4& a, const vec4& b) {
		return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.a + b.a);
	}
	friend vec4 operator-(const vec4& a, const vec4& b) {
		return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.a - b.a);
	}
	friend vec4 operator*(const vec4& a, const vec4& b) {
		return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.a * b.a);
	}
	friend vec4 operator/(const vec4& a, const vec4& b) {
		return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.a / b.a);
	}

	friend bool operator==(const vec4& a, const vec4& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z && a.a == b.a;
	}
	friend bool operator!=(const vec4& a, const vec4& b) {
		return a.x != b.x || a.y != b.y || a.z != b.z || a.a != b.a;
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
vec4<A> operator-(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x - b.x, a.y - b.y, a.z - b.z, a.a - b.a);
}
template <typename A, typename B>
vec4<A> operator*(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x * b.x, a.y * b.y, a.z * b.z, a.a * b.a);
}
template <typename A, typename B>
vec4<A> operator/(const vec4<A>& a, const vec4<B>& b) {
	return vec4<A>(a.x / b.x, a.y / b.y, a.z / b.z, a.a / b.a);
}

template <typename A, typename B>
bool operator==(const vec4<A>& a, const vec4<B>& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.a == b.a;
}
template <typename A, typename B>
bool operator!=(const vec4<A>& a, const vec4<B>& b) {
	return a.x != b.x || a.y != b.y || a.z != b.z || a.a != b.a;
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
