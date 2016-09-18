#pragma once

#include <cstdint>
#include <cmath>

using byte = unsigned char;
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

using char8 = char;
using char16 = char16_t;
using char32 = char32_t;
using wchar = wchar_t;

namespace kk {

template <typename T>
struct vec2;

using vec2i = vec2<int>;
using vec2u = vec2<uint>;
using vec2f = vec2<float>;
using vec2d = vec2<double>;

template <typename T>
struct vec3;

using vec3b = vec3<byte>;
using vec3i = vec3<int>;
using vec3u = vec3<uint>;
using vec3f = vec3<float>;
using vec3d = vec3<double>;

template <typename T>
struct vec4;

using vec4b = vec4<byte>;
using vec4i = vec4<int>;
using vec4u = vec4<uint>;
using vec4f = vec4<float>;
using vec4d = vec4<double>;

}

#define PI M_PI

#define D2R(x) (x)/180*PI	// degrees to radians
#define R2D(x) (x)*180/PI	// radians to degrees
