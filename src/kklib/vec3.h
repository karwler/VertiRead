#pragma once

#include <cmath>

namespace kk {

template <typename T>
struct vec3 {
	vec3(T N=0) :
		x(N), y(N), z(N)
	{}
	vec3(T X, T Y, T Z) :
		x(X), y(Y), z(Z)
	{}
	template <typename A>
	vec3(const vec3<A>& N) :
		x(N.x), y(N.y), z(N.z)
	{}

	T& operator[](char i) {
		switch (i) {
		case 0:
			return x;
		case 1:
			return y;
		}
		return z;
	}
	const T& operator[](char i) const {
		return *this[i];
	}

	template <typename A>
	vec3& operator=(const vec3<A>& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}
	template <typename A>
	vec3& operator+=(const vec3<A>& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	template <typename A>
	vec3& operator-=(const vec3<A>& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	template <typename A>
	vec3& operator*=(const vec3<A>& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	template <typename A>
	vec3& operator/=(const vec3<A>& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	friend vec3 operator+(const vec3& a, const vec3& b) {
		return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	friend vec3 operator-(const vec3& a, const vec3& b) {
		return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	friend vec3 operator*(const vec3& a, const vec3& b) {
		return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}
	friend vec3 operator/(const vec3& a, const vec3& b) {
		return vec3(a.x / b.x, a.y / b.y, a.z / b.z);
	}

	friend bool operator==(const vec3& a, const vec3& b) {
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
	friend bool operator!=(const vec3& a, const vec3& b) {
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
	vec3 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
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
		return rotate(*this, ang);
	}

	union { T x, r; };
	union { T y, g; };
	union { T z, b; };
};

template <typename A, typename B>
vec3<A> operator+(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x + b.x, a.y + b.y, a.z + b.z);
}
template <typename A, typename B>
vec3<A> operator-(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x - b.x, a.y - b.y, a.z - b.z);
}
template <typename A, typename B>
vec3<A> operator*(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x * b.x, a.y * b.y, a.z * b.z);
}
template <typename A, typename B>
vec3<A> operator/(const vec3<A>& a, const vec3<B>& b) {
	return vec3<A>(a.x / b.x, a.y / b.y, a.z / b.z);
}

template <typename A, typename B>
bool operator==(const vec3<A>& a, const vec3<B>& b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
template <typename A, typename B>
bool operator!=(const vec3<A>& a, const vec3<B>& b) {
	return a.x != b.x || a.y != b.y || a.z != b.z;
}

template <typename T>
T length(const vec3<T>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

template <typename T>
vec3<T> normalize(const vec3<T>& vec) {
	T l = vec.len();
	return vec3<T>(vec.x/l, vec.y/l, vec.z/l);
}

template <typename T>
bool isUnit(const vec3<T>& vec) {
	return vec.len() == 1;
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
	return vec - 2 * dotP(vec, nrm) * nrm;
}

template <typename A, typename B>
vec3<A> rotate(const vec3<A>& vec, vec3<B> ang) {
	vec3<A> ret;

	ret.y = vec.y*std::cos(ang.x) - vec.z*std::sin(ang.x);
	ret.z = vec.y*std::sin(ang.x) + vec.z*std::cos(ang.x);

	ret.x =  vec.x*std::cos(ang.y) + vec.z*std::sin(ang.y);
	ret.z = -vec.x*std::sin(ang.y) + vec.z*std::cos(ang.y);

	ret.x = vec.x*std::cos(ang.z) - vec.y*std::sin(ang.z);
	ret.y = vec.x*std::sin(ang.z) + vec.y*std::cos(ang.z);

	return ret;
}

}
