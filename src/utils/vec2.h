#pragma once

#include <cmath>

template <typename T>
struct vec2;

template <typename T>
T dot(const vec2<T>& a, const vec2<T>& b);
template <typename T>
T cross(const vec2<T>& a, const vec2<T>& b);
template <typename T>
vec2<T> reflect(const vec2<T>& vec, vec2<T> nrm);
template <typename T>
vec2<T> rotate(const vec2<T>& vec, const T& ang);
template <typename T>
char intersect(vec2<T>& in, vec2<T>& im, const vec2<T>& ap, const vec2<T>& av, const vec2<T>& bp, const vec2<T>& bv);

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
		if (i == 1)
			return y;
		throw "out of range";
	}

	const T& operator[](char i) const {
		if (i == 0)
			return x;
		if (i == 1)
			return y;
		throw "out of range";
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

	vec2& operator%=(const vec2& v) {
		x %= v.x;
		y %= v.y;
		return *this;
	}

	vec2& operator&=(const vec2& v) {
		x &= v.x;
		y &= v.y;
		return *this;
	}

	vec2& operator|=(const vec2& v) {
		x |= v.x;
		y |= v.y;
		return *this;
	}

	vec2& operator^=(const vec2& v) {
		x ^= v.x;
		y ^= v.y;
		return *this;
	}

	vec2& operator<<=(const vec2& v) {
		x <<= v.x;
		y <<= v.y;
		return *this;
	}

	vec2& operator>>=(const vec2& v) {
		x >>= v.x;
		y >>= v.y;
		return *this;
	}

	vec2& operator++() {
		x++;
		y++;
		return *this;
	}

	vec2 operator++(int) {
		vec2 t = *this;
		x++;
		y++;
		return t;
	}

	vec2& operator--() {
		x--;
		y--;
		return *this;
	}

	vec2 operator--(int) {
		vec2 t = *this;
		x--;
		y--;
		return t;
	}

	bool has(const T& n) const {
		return x == n || y == n;
	}

	bool hasNot(const T& n) const {
		return x != n && y != n;
	}

	T length() const {
		return std::sqrt(x*x + y*y);
	}

	vec2 normalize() const {
		return *this / length();
	}

	T dot(const vec2& vec) const {
		return ::dot(*this, vec);
	}

	T cross(const vec2& vec) const {
		return ::cross(*this, vec);
	}

	vec2 reflect(const vec2& nrm) const {
		return ::reflect(*this, nrm);
	}

	vec2 rotate(const T& ang) const {
		return ::rotate(*this, ang);
	}

	union { T x, w, l; };
	union { T y, h, u; };
};

template <typename T>
vec2<T> operator+(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x + b.x, a.y + b.y);
}

template <typename T>
vec2<T> operator+(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x + b, a.y + b);
}

template <typename T>
vec2<T> operator+(const T& a, const vec2<T>& b) {
	return vec2<T>(a + b.x, a + b.y);
}

template <typename T>
vec2<T> operator-(const vec2<T>& a) {
	return vec2<T>(-a.x, -a.y);
}

template <typename T>
vec2<T> operator-(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
vec2<T> operator-(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x - b, a.y - b);
}

template <typename T>
vec2<T> operator-(const T& a, const vec2<T>& b) {
	return vec2<T>(a - b.x, a - b.y);
}

template <typename T>
vec2<T> operator*(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x * b.x, a.y * b.y);
}

template <typename T>
vec2<T> operator*(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x * b, a.y * b);
}

template <typename T>
vec2<T> operator*(const T& a, const vec2<T>& b) {
	return vec2<T>(a * b.x, a * b.y);
}

template <typename T>
vec2<T> operator/(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x / b.x, a.y / b.y);
}

template <typename T>
vec2<T> operator/(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x / b, a.y / b);
}

template <typename T>
vec2<T> operator/(const T& a, const vec2<T>& b) {
	return vec2<T>(a % b.x, a % b.y);
}

template <typename T>
vec2<T> operator%(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x % b.x, a.y % b.y);
}

template <typename T>
vec2<T> operator%(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x % b, a.y % b);
}

template <typename T>
vec2<T> operator%(const T& a, const vec2<T>& b) {
	return vec2<T>(a % b.x, a % b.y);
}

template <typename T>
vec2<T> operator~(const vec2<T>& a) {
	return vec2<T>(~a.x, ~a.y);
}

template <typename T>
vec2<T> operator&(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x & b.x, a.y & b.y);
}

template <typename T>
vec2<T> operator&(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x & b, a.y & b);
}

template <typename T>
vec2<T> operator&(const T& a, const vec2<T>& b) {
	return vec2<T>(a & b.x, a & b.y);
}

template <typename T>
vec2<T> operator|(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x | b.x, a.y | b.y);
}

template <typename T>
vec2<T> operator|(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x | b, a.y | b);
}

template <typename T>
vec2<T> operator|(const T& a, const vec2<T>& b) {
	return vec2<T>(a | b.x, a | b.y);
}

template <typename T>
vec2<T> operator^(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x ^ b.x, a.y ^ b.y);
}

template <typename T>
vec2<T> operator^(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x ^ b, a.y ^ b);
}

template <typename T>
vec2<T> operator^(const T& a, const vec2<T>& b) {
	return vec2<T>(a ^ b.x, a ^ b.y);
}

template <typename T>
vec2<T> operator<<(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x << b.x, a.y << b.y);
}

template <typename T>
vec2<T> operator<<(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x << b, a.y << b);
}

template <typename T>
vec2<T> operator<<(const T& a, const vec2<T>& b) {
	return vec2<T>(a << b.x, a << b.y);
}

template <typename T>
vec2<T> operator>>(const vec2<T>& a, const vec2<T>& b) {
	return vec2<T>(a.x >> b.x, a.y >> b.y);
}

template <typename T>
vec2<T> operator>>(const vec2<T>& a, const T& b) {
	return vec2<T>(a.x >> b, a.y >> b);
}

template <typename T>
vec2<T> operator>>(const T& a, const vec2<T>& b) {
	return vec2<T>(a >> b.x, a >> b.y);
}

template <typename T>
bool operator==(const vec2<T>& a, const vec2<T>& b) {
	return a.x == b.x && a.y == b.y;
}

template <typename T>
bool operator==(const vec2<T>& a, const T& b) {
	return a.x == b && a.y == b;
}

template <typename T>
bool operator==(const T& a, const vec2<T>& b) {
	return a == b.x && a == b.y;
}

template <typename T>
bool operator!=(const vec2<T>& a, const vec2<T>& b) {
	return a.x != b.x || a.y != b.y;
}

template <typename T>
bool operator!=(const vec2<T>& a, const T& b) {
	return a.x != b || a.y != b;
}

template <typename T>
bool operator!=(const T& a, const vec2<T>& b) {
	return a != b.x || a != b.y;
}

template <typename T>
T dot(const vec2<T>& a, const vec2<T>& b) {
	return a.x * b.x + a.y * b.y;
}

template <typename T>
T cross(const vec2<T>& a, const vec2<T>& b) {
	return a.x * b.y - a.y * b.x;
}

template <typename T>
vec2<T> reflect(const vec2<T>& vec, vec2<T> nrm) {
	nrm = nrm.normalize();
	return vec - T(2) * dot(vec, nrm) * nrm;
}

template <typename T>
vec2<T> rotate(const vec2<T>& vec, const T& ang) {
	T sa = std::sin(ang);
	T ca = std::cos(ang);
	return vec2<T>(vec.x * ca - vec.y * sa, vec.x * sa + vec.y * ca);
}

template <typename T>
char intersect(vec2<T>& in, vec2<T>& im, const vec2<T>& ap, const vec2<T>& av, const vec2<T>& bp, const vec2<T>& bv) {	// return 0 if no intersection, 1 if lines intersect, 2 if lines overlap
	vec2<T> dp = bp - ap;
	T dt = cross(av, bv);
	T ta = cross(dp, bv);
	T tb = cross(dp, av);

	if (dt == T(0)) {	// lines are parallel
		if (ta == T(0)) {	// lines might overlap
			T ad = dot(av, av);
			T t0 = dot(dp, av) / ad;
			T t1 = t0 + dot(bv, av) / ad;

			if ((t0 >= T(0) && t0 <= T(1)) || (t1 >= T(0) && t1 <= T(1))) {	// liens overlap
				vec2<T> iv = (dot(av, bv) < T(0)) ? vec2<T>(t1, t0) : vec2<T>(t0, t1);
				in = (iv.l < T(0)) ? ap : ap + iv.l * av;
				im = (iv.u > T(1)) ? ap + av : ap + iv.u * av;
				return 2;
			}
		}
		return 0;
	}

	T af = ta / dt;
	T bf = tb / dt;
	if (af < T(0) || af > T(1) || bf < T(0) || bf > T(1))	// lines don't intersect
		return 0;

	in = ap + af * av;	// get intersection
	return 1;
}
