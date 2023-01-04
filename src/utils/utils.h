#pragma once

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;

// to make life easier
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using wchar = wchar_t;

using std::array;
using std::optional;
using std::pair;
using std::string;
using std::string_view;
using std::tuple;
using std::vector;
using std::wstring;
using std::wstring_view;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

template <class T> using initlist = std::initializer_list<T>;
template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uptr = std::unique_ptr<T...>;

using sizet = size_t;
using pdift = ptrdiff_t;
using iptrt = intptr_t;
using uptrt = uintptr_t;
using pairStr = pair<string, string>;
using pairPath = pair<fs::path, fs::path>;
using mapFiles = umap<string, pair<sizet, uptrt>>;

using glm::vec2;
using glm::uvec2;
using glm::u32vec2;
using glm::ivec2;
using glm::vec4;
using glm::u8vec4;
using glm::ivec4;
using mvec2 = glm::vec<2, sizet, glm::defaultp>;

// forward declarations
struct archive;
struct archive_entry;
class Browser;
class Button;
class CheckBox;
class ComboBox;
class Context;
class DrawSys;
class FileSys;
class InputSys;
class Label;
class LabelEdit;
class Layout;
class Overlay;
struct PictureLoader;
class Picture;
class Popup;
class Program;
class ProgressBar;
class ProgState;
class ReaderBox;
class RootLayout;
class Scene;
class ScrollArea;
class Settings;
class Slider;
class Texture;
class Widget;
class WindowArranger;

// events

using PCall = void (Program::*)(Button*);
using LCall = void (Program::*)(Layout*);
using SBCall = void (ProgState::*)();
using SACall = void (ProgState::*)(float);

// general wrappers

#ifdef _WIN32
constexpr string_view linend = "\r\n";
#else
constexpr string_view linend = "\n";
#endif

enum class UserCode : int32 {
	readerProgress,
	readerFinished,
	downloadProgress,
	downloadNext,
	downlaodFinished,
	moveProgress,
	moveFinished
};

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator~(T a) {
	return T(~std::underlying_type_t<T>(a));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&(T a, T b) {
	return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|(T a, T b) {
	return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^(T a, T b) {
	return T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
constexpr bool outRange(T val, T min, T max) {
	return val < min || val > max;
}

template <class T>
string operator+(std::basic_string<T>&& a, std::basic_string_view<T> b) {
	sizet alen = a.length();
	a.resize(alen + b.length());
	std::copy(b.begin(), b.end(), a.begin() + alen);
	return a;
}

template <class T>
string operator+(const std::basic_string<T>& a, std::basic_string_view<T> b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	std::copy(a.begin(), a.end(), r.begin());
	std::copy(b.begin(), b.end(), r.begin() + a.length());
	return r;
}

template <class T>
string operator+(std::basic_string_view<T> a, std::basic_string<T>&& b) {
	sizet blen = b.length();
	b.resize(a.length() + blen);
	std::move_backward(b.begin(), b.begin() + blen, b.end());
	std::copy(a.begin(), a.end(), b.begin());
	return b;
}

template <class T>
string operator+(std::basic_string_view<T> a, const std::basic_string<T>& b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	std::copy(a.begin(), a.end(), r.begin());
	std::copy(b.begin(), b.end(), r.begin() + a.length());
	return r;
}

inline void pushEvent(UserCode code, void* data1 = nullptr, void* data2 = nullptr) {
	SDL_Event event;
	event.user = { SDL_USEREVENT, SDL_GetTicks(), 0, int32(code), data1, data2 };
	SDL_PushEvent(&event);
}

#ifndef NDEBUG
inline void dbgPass() {}
#endif

// SDL_Rect wrapper
template <class T>
struct Rect {
	using tvec2 = glm::vec<2, T, glm::defaultp>;
	using tvec4 = glm::vec<4, T, glm::defaultp>;

	T x, y, w, h;

	Rect() = default;
	constexpr Rect(T n);
	constexpr Rect(T px, T py, T sw, T sh);
	constexpr Rect(T px, T py, const tvec2& sv);
	constexpr Rect(const tvec2& pv, T sw, T sh);
	constexpr Rect(const tvec2& pv, const tvec2& sv);
	constexpr Rect(const tvec4& rv);

	tvec2& pos();
	constexpr tvec2 pos() const;
	tvec2& size();
	constexpr tvec2 size() const;
	constexpr tvec2 end() const;
	tvec4& toVec();
	constexpr tvec4 toVec() const;

	bool operator==(const Rect& rect) const;
	constexpr bool empty() const;
	constexpr bool contains(const tvec2& point) const;
	constexpr bool overlaps(const Rect& rect) const;
	constexpr Rect<T> intersect(const Rect& rect) const;
	constexpr Rect<T> translate(const tvec2& mov) const;
};

using Recti = Rect<int>;
using Rectu = Rect<uint>;

template <class T>
constexpr Rect<T>::Rect(T n) :
	x(n), y(n), w(n), h(n)
{}

template <class T>
constexpr Rect<T>::Rect(T px, T py, T sw, T sh) :
	x(px), y(py), w(sw), h(sh)
{}

template <class T>
constexpr Rect<T>::Rect(T px, T py, const tvec2& sv) :
	x(px), y(py), w(sv.x), h(sv.y)
{}

template <class T>
constexpr Rect<T>::Rect(const tvec2& pv, T sw, T sh) :
	x(pv.x), y(pv.y), w(sw), h(sh)
{}

template <class T>
constexpr Rect<T>::Rect(const tvec2& pv, const tvec2& sv) :
	x(pv.x), y(pv.y), w(sv.x), h(sv.y)
{}

template <class T>
constexpr Rect<T>::Rect(const tvec4& rv) :
	x(rv.x), y(rv.y), w(rv.z), h(rv.w)
{}

template <class T>
typename Rect<T>::tvec2& Rect<T>::pos() {
	return *reinterpret_cast<tvec2*>(this);
}

template <class T>
constexpr typename Rect<T>::tvec2 Rect<T>::pos() const {
	return tvec2(x, y);
}

template <class T>
inline typename Rect<T>::tvec2& Rect<T>::size() {
	return reinterpret_cast<tvec2*>(this)[1];
}

template <class T>
constexpr typename Rect<T>::tvec2 Rect<T>::size() const {
	return tvec2(w, h);
}

template <class T>
constexpr typename Rect<T>::tvec2 Rect<T>::end() const {
	return pos() + size();
}

template <class T>
typename Rect<T>::tvec4& Rect<T>::toVec() {
	return *reinterpret_cast<tvec4*>(this);
}

template <class T>
constexpr typename Rect<T>::tvec4 Rect<T>::toVec() const {
	return tvec4(x, y, w, h);
}

template <class T>
bool Rect<T>::operator==(const Rect& rect) const {
	return x == rect.x && y == rect.y && w == rect.w && h == rect.h;
}

template <class T>
constexpr bool Rect<T>::empty() const {
	return w <= T(0) || h <= T(0);
}

template <class T>
constexpr bool Rect<T>::contains(const tvec2& point) const {
	return point.x >= x && point.x < x + w && point.y >= y && point.y < y + h;
}

template <class T>
constexpr bool Rect<T>::overlaps(const Rect& rect) const {
	if (!empty() && !rect.empty()) {
		tvec2 dpos = glm::max(pos(), rect.pos());
		tvec2 dend = glm::min(end(), rect.end());
		return dend.x > dpos.x && dend.y > dpos.y;
	}
	return false;
}

template <class T>
constexpr Rect<T> Rect<T>::intersect(const Rect& rect) const {
	if (!empty() && !rect.empty()) {
		tvec2 dpos = glm::max(pos(), rect.pos());
		tvec2 dend = glm::min(end(), rect.end());
		return Rect(dpos, dend - dpos);
	}
	return false;
}

template <class T>
constexpr Rect<T> Rect<T>::translate(const tvec2& mov) const {
	return Rect(pos() + mov, size());
}

// files and strings

bool isDriveLetter(const fs::path& path);
fs::path parentPath(const fs::path& path);
string strEnclose(string_view str);
vector<string> strUnenclose(string_view str);
vector<string_view> getWords(string_view str);
tm currentDateTime();

inline bool isSpace(int c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(int c) {
	return uint(c) > ' ' && c != 0x7F;
}

inline string_view trim(string_view str) {
	string_view::iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string_view(str.data() + (pos - str.begin()), std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base() - pos);
}

inline bool isDsep(int c) {
#ifdef _WIN32
	return c == '\\' || c == '/';
#else
	return c == '/';
#endif
}

inline bool notDsep(int c) {
#ifdef _WIN32
	return c != '\\' && c != '/';
#else
	return c != '/';
#endif
}

inline fs::path relativePath(const fs::path& path, const fs::path& base) {
	return !base.empty() ? path.lexically_relative(base) : path;
}

inline bool isSubpath(const fs::path& path, const fs::path& base) {
	return base.empty() || !path.lexically_relative(base).empty();
}

// conversions
#ifdef _WIN32
string swtos(wstring_view wstr);
wstring sstow(string_view str);
#endif
uint32 ceilPow2(uint32 val);

inline string stos(string_view str) {	// dummy function for World::setArgs
	return string(str);
}

inline const char* toStr(bool b) {
	return b ? "true" : "false";
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(T num) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num, base);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);
	return string(buf.data(), res.ptr);
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
string toStr(T num) {
	return toStr<base>(std::underlying_type_t<T>(num));
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(T num, uint8 pad) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);

	uint8 len = res.ptr - buf.data();
	if (len >= pad)
		return string(buf.data(), res.ptr);

	string str;
	str.resize(pad);
	pad -= len;
	std::fill_n(str.begin(), pad, '0');
	std::copy(buf.data(), res.ptr, str.begin() + pad);
	return str;
}

template <class T = float, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
constexpr sizet recommendedCharBufferSize() {
	if constexpr (std::is_same_v<T, float>)
		return 16;
	if constexpr (std::is_same_v<T, double>)
		return 24;
	return 30;
}

template <class T = float, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
string toStr(T num) {
	array<char, recommendedCharBufferSize<T>()> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num);
	return string(buf.data(), res.ptr);
}

template <uint8 base = 10, glm::length_t L, class T, glm::qualifier Q, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr<base>(v[i]) + sep;
	return str + toStr<base>(v[L - 1]);
}

template <glm::length_t L, class T, glm::qualifier Q, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L - 1]);
}

inline string tmToDateStr(const tm& tim, char sep = '-') {
	return toStr(tim.tm_year + 1900) + sep + toStr(tim.tm_mon, 2) + sep + toStr(tim.tm_mday, 2);
}

inline string tmToTimeStr(const tm& tim, char sep = ':') {
	return toStr(tim.tm_hour, 2) + sep + toStr(tim.tm_min, 2) + sep + toStr(tim.tm_sec, 2);
}

template <class T, sizet N, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, int> = 0>
T strToEnum(const array<const char*, N>& names, string_view str, T defaultValue = T(N)) {
	typename array<const char*, N>::const_iterator p = std::find_if(names.begin(), names.end(), [str](const char* it) -> bool { return !SDL_strncasecmp(it, str.data(), str.length()) && !it[str.length()]; });
	return p != names.end() ? T(p - names.begin()) : defaultValue;
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>, int> = 0>
T strToVal(const umap<T, const char*>& names, string_view str, T defaultValue = T(0)) {
	typename umap<T, const char*>::const_iterator it = std::find_if(names.begin(), names.end(), [str](const pair<const T, const char*>& nit) -> bool { return !SDL_strncasecmp(nit.second, str.data(), str.length()) && !nit.second[str.length()]; });
	return it != names.end() ? it->first : defaultValue;
}

inline bool toBool(string_view str) {
	return (str.length() >= 4 && !SDL_strncasecmp(str.data(), "true", 4)) || (str.length() >= 2 && !SDL_strncasecmp(str.data(), "on", 2)) || (!str.empty() && !SDL_strncasecmp(str.data(), "y", 1)) || std::any_of(str.begin(), str.end(), [](char c) -> bool { return c >= '1' && c <= '9'; });
}

template <class T, class... A, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
T toNum(string_view str, A... args) {
	T val = T(0);
	sizet i = 0;
	for (; i < str.length() && isSpace(str[i]); ++i);
	std::from_chars(str.data() + i, str.data() + str.length(), val, args...);
	return val;
}

template <class T, class... A, std::enable_if_t<(std::is_integral_v<typename T::value_type> && !std::is_same_v<typename T::value_type, bool>) || std::is_floating_point_v<typename T::value_type>, int> = 0>
T toVec(string_view str, typename T::value_type fill = typename T::value_type(0), A... args) {
	T vec(fill);
	sizet p = 0;
	for (glm::length_t i = 0; p < str.length() && i < vec.length(); ++i) {
		for (; p < str.length() && isSpace(str[p]); ++p);
		for (; p < str.length(); ++p)
			if (std::from_chars_result res = std::from_chars(str.data() + p, str.data() + str.length(), vec[i], args...); res.ec == std::errc(0)) {
				p = res.ptr - str.data();
				break;
			}
	}
	return vec;
}

// other

template <class T>
T btom(bool b) {
	return T(b) * T(2) - T(1);	// b needs to be 0 or 1
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> vswap(const T& x, const T& y, bool swap) {
	return swap ? glm::vec<2, T, Q>(y, x) : glm::vec<2, T, Q>(x, y);
}

template <class... A>
void logInfo(A&&... args) {
	std::ostringstream ss;
	(ss << ... << std::forward<A>(args));
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", ss.str().c_str());
}

template <class... A>
void logError(A&&... args) {
	std::ostringstream ss;
	(ss << ... << std::forward<A>(args));
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", ss.str().c_str());
}
