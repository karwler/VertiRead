#pragma once

#include <cmath>

template <class T>
struct vec2 {
	vec2(const T& n = T(0)) :
		x(n), y(n)
	{}

	vec2(const T& x, const T& y) :
		x(x), y(y)
	{}

	vec2(const T& vx, const T& vy, bool swap) {
		if (swap) {
			y = vx;
			x = vy;
		} else {
			y = vy;
			x = vx;
		}
	}

	template <class A>
	vec2(const vec2<A>& v) :
		x(v.x), y(v.y)
	{}

	template <class A>
	vec2(const vec2<A>& v, bool swap) {
		if (swap) {
			y = v.x;
			x = v.y;
		} else {
			y = v.y;
			x = v.x;
		}
	}

	~vec2() {}	// for some reason this needs to be here so msvc doesn't bitch around
	
	T& operator[](size_t i) {
		return reinterpret_cast<T*>(this)[i];
	}

	const T& operator[](size_t i) const {
		return reinterpret_cast<const T*>(this)[i];
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
		return sqrt(x*x + y*y);
	}

	vec2 swap() const {
		return vec2(y, x);
	}

	vec2 swap(bool yes) const {
		return yes ? swap() : *this;
	}

	union { T x, w, u, b; };
	union { T y, h, v, t; };
};

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
