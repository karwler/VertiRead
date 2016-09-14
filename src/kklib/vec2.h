#pragma once

#include <cmath>

namespace kk {

template <typename T>
struct vec2 {
	vec2(T N=0) :
		x(N), y(N)
	{}
	vec2(T X, T Y) :
		x(X), y(Y)
	{}
	template <typename A>
	vec2(const vec2<A>& N) :
		x(N.x), y(N.y)
	{}

	T& operator[](char i) {
		if (i == 0)
			return x;
		return y;
	}
	const T& operator[](char i) const {
		return *this[i];
	}

	template <typename A>
	vec2& operator=(const vec2<A>& v) {
		x = v.x;
		y = v.y;
		return *this;
	}
	template <typename A>
	vec2& operator+=(const vec2<A>& v) {
		x += v.x;
		y += v.y;
		return *this;
	}
	template <typename A>
	vec2& operator-=(const vec2<A>& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}
	template <typename A>
	vec2& operator*=(const vec2<A>& v) {
		x *= v.x;
		y *= v.y;
		return *this;
	}
	template <typename A>
	vec2& operator/=(const vec2<A>& v) {
		x /= v.x;
		y /= v.y;
		return *this;
	}

	friend vec2 operator+(const vec2& a, const vec2& b) {
		return vec2(a.x + b.x, a.y + b.y);
	}
	friend vec2 operator-(const vec2& a, const vec2& b) {
		return vec2(a.x - b.x, a.y - b.y);
	}
	friend vec2 operator*(const vec2& a, const vec2& b) {
		return vec2(a.x * b.x, a.y * b.y);
	}
	friend vec2 operator/(const vec2& a, const vec2& b) {
		return vec2(a.x / b.x, a.y / b.y);
	}

	friend bool operator==(const vec2& a, const vec2& b) {
		return a.x == b.x && a.y == b.y;
	}
	friend bool operator!=(const vec2& a, const vec2& b) {
		return a.x != b.x || a.y != b.y;
	}

	bool isNull() const {
		return x == 0 && y == 0;
	}
	bool hasNull() const {
		return x == 0 || y == 0;
	}

	T len() const {
		return length(*this);
	}
	vec2 norm() const {
		return normalize(*this);
	}
	bool unit() const {
		return isUnit(*this);
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
		return rotate(*this, ang);
	}

	union { T x, w; };
	union { T y, h; };
};

template <typename A, typename B>
vec2<A> operator+(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x + b.x, a.y + b.y);
}
template <typename A, typename B>
vec2<A> operator-(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x - b.x, a.y - b.y);
}
template <typename A, typename B>
vec2<A> operator*(const vec2<A>& a, const vec2<B>& b) {
	return vec2<A>(a.x * b.x, a.y * b.y);
}
template <typename A, typename B>
vec2<A> operator/(const vec2<A>& a, const vec2<A>& b) {
	return vec2<A>(a.x / b.x, a.y / b.y);
}

template <typename A, typename B>
bool operator==(const vec2<A>& a, const vec2<B>& b) {
	return a.x == b.x && a.y == b.y;
}
template <typename A, typename B>
bool operator!=(const vec2<A>& a, const vec2<B>& b) {
	return a.x != b.x || a.y != b.y;
}

template <typename A>
A length(const vec2<A>& vec) {
	return std::sqrt(vec.x*vec.x + vec.y*vec.y);
}

template <typename A>
vec2<A> normalize(const vec2<A>& vec) {
	A l = length(vec);
	return vec2<A>(vec.x/l, vec.y/l);
}

template <typename A>
bool isUnit(const vec2<A>& vec) {
	return length(vec) == 1;
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
	return vec - 2 * dotP(vec, nrm) * nrm;
}

template <typename A>
vec2<A> rotate(const vec2<A>& vec, A ang) {
	return vec2<A>(vec.x*std::cos(ang) - vec.y*std::sin(ang), vec.x*std::sin(ang) + vec.y*std::cos(ang));
}

template <typename A>
bool findIsct(const vec2<A>& p0, const vec2<A>& p1, const vec2<A>& v0, const vec2<A>& v1, vec2<A>& isct) {
	A vCross = crossP(v0, v1);
	if (vCross == 0)
		return false;

	A r = crossP(p1 - p0, v1) / vCross;
	A s = crossP(p1 - p0, v0) / vCross;

	if (r < 0 || r > 1 || s < 0 || s > 1)
		return false;

	isct = p0 + r*v0;
	return true;
}

template <typename A>
A distancePointLine(const vec2<A>& v, const vec2<A>& w, const vec2<A>& p) {
	A l2 = pow(length(w-v), 2);
	if (l2 == 0)
		return length(v-p);

	A t = std::max(A(0), std::min(A(1), dotP(p - v, w - v) / l2));
	vec2f projection = v + t * (w - v);
	return length(projection - p);
}

template <typename A>
A findLookAtRotation(const vec2<A>& a, const vec2<A>& b) {
	kvec2<A> point = b - a;
	if (point.isNull())
		return 0;

	A ang = acos(point.x / point.len());
	return ang = (point.y < 0) ? 2*M_PI-ang : ang;
}

}
