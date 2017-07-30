#pragma once

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace kk {

template <typename T>
struct vec3 {
	vec3(const T& N=T(0)) :
		x(N), y(N), z(N)
	{}
	vec3(const T& X, const T& Y, const T& Z) :
		x(X), y(Y), z(Z)
	{}
	template <typename A>
	vec3(const vec3<A>& V) :
		x(V.x), y(V.y), z(V.z)
	{}

	T& operator[](char i) {
		if (i == 0)
			return x;
		if (i == 1)
			return y;
		return z;
	}
	const T& operator[](char i) const {
		if (i == 0)
			return x;
		if (i == 1)
			return y;
		return z;
	}

	vec3& operator=(const vec3& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}
	vec3& operator+=(const vec3& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	vec3& operator-=(const vec3& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	vec3& operator*=(const vec3& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	vec3& operator/=(const vec3& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	bool isNull() const {
		return x == T(0) && y == T(0) && z == T(0);
	}
	bool hasNull() const {
		return x == T(0) || y == T(0) || z == T(0);
	}

	T len() const {
		return std::sqrt(x*x + y*y + z*z);
	}
	vec3 norm() const {
		T l = len();
		return vec3(x/l, y/l, z/l);
	}
	bool isUnit() const {
		return len() == T(1);
	}
	template <typename A>
	T dot(const vec3<A>& vec) const {
		return dotP(*this, vec);
	 }
	template <typename A>
	vec3 cross(const vec3<A>& vec) const {
		return crossP(*this, vec);
	}
	template <typename A>
	vec3 refl(const vec3<A>& nrm) const {
		return reflect(*this, nrm);
	}
	template <typename A>
	vec3 rot(const vec3<A>& ang) const {
		vec3 ret;

		ret.y = y*std::cos(ang.x) - z*std::sin(ang.x);
		ret.z = y*std::sin(ang.x) + z*std::cos(ang.x);

		ret.x =  x*std::cos(ang.y) + z*std::sin(ang.y);
		ret.z = -x*std::sin(ang.y) + z*std::cos(ang.y);

		ret.x = x*std::cos(ang.z) - y*std::sin(ang.z);
		ret.y = x*std::sin(ang.z) + y*std::cos(ang.z);

		return ret;
	}

	union { T x, w, r; };
	union { T y, h, g; };
	union { T z, d, b; };
};

template <typename A, typename B>
vec3<A> operator+(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x + b.x, a.y + b.y, a.z + b.z);
}
template <typename A, typename B>
vec3<A> operator+(const vec3<A>& a, const B& b) {
	return vec3<A>(a.x + b, a.y + b, a.z + b);
}
template <typename A, typename B>
vec3<A> operator+(const A& a, const vec3<B>& b) {
	return vec3<A>(a + b.x, a + b.y, a + b.z);
}

template <typename A, typename B>
vec3<A> operator-(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x - b.x, a.y - b.y, a.z - b.z);
}
template <typename A, typename B>
vec3<A> operator-(const vec3<A>& a, const B& b) {
	return vec3<A>(a.x - b, a.y - b, a.z - b);
}
template <typename A, typename B>
vec3<A> operator-(const A& a, const vec3<B>& b) {
	return vec3<A>(a - b.x, a - b.y, a - b.z);
}

template <typename A, typename B>
vec3<A> operator*(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x * b.x, a.y * b.y, a.z * b.z);
}
template <typename A, typename B>
vec3<A> operator*(const vec3<A>& a, const B& b) {
	return vec3<A>(a.x * b, a.y * b, a.z * b);
}
template <typename A, typename B>
vec3<A> operator*(const A& a, const vec3<B>& b) {
	return vec3<A>(a * b.x, a * b.y, a * b.z);
}

template <typename A, typename B>
vec3<A> operator/(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x / b.x, a.y / b.y, a.z / b.z);
}
template <typename A, typename B>
vec3<A> operator/(const vec3<A>& a, const B& b) {
	return vec3<A>(a.x / b, a.y / b, a.z / b);
}
template <typename A, typename B>
vec3<A> operator/(const A& a, const vec3<B>& b) {
	return vec3<A>(a / b.x, a / b.y, a / b.z);
}

template <typename A, typename B>
bool operator==(const vec3<A>& a, const vec3<B>& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template <typename A, typename B>
bool operator==(const vec3<A>& a, const B& b) {
	return a.x == b && a.y == b && a.z == b;
}
template <typename A, typename B>
bool operator==(const A& a, const vec3<B>& b) {
	return a == b.x && a == b.y && a == b.z;
}

template <typename A, typename B>
bool operator!=(const vec3<A>& a, const vec3<B>& b) {
	return a.x != b.x || a.y != b.y || a.z != b.z;
}
template <typename A, typename B>
bool operator!=(const vec3<A>& a, const B& b) {
	return a.x != b || a.y != b || a.z != b;
}
template <typename A, typename B>
bool operator!=(const A& a, const vec3<B>& b) {
	return a != b.x || a != b.y || a != b.z;
}

template <typename A, typename B>
A dotP(const vec3<A>& v0, const vec3<B>& v1) {
	return v0.x*v1.x + v0.y*v1.y + v0.z*v1.z;
}

template <typename A, typename B>
vec3<A> crossP(const vec3<A>& v0, const vec3<B>& v1) {
	return vec3<A>(v0.y*v1.z - v0.z*v1.y, v0.z*v1.x - v0.x*v1.z, v0.x*v1.y - v0.y*v1.x);
}

template <typename A, typename B>
vec3<A> reflect(const vec3<A>& vec, vec3<B> nrm) {
	if (!isUnit(nrm))
		nrm = normalize(nrm);
	return vec - A(2) * dotP(vec, nrm) * nrm;
}

}
