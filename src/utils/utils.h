#pragma once

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
using std::pair;
using std::string;
using std::string_view;
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
template <class... T> using uset = std::unordered_set<T...>;

using sizet = size_t;
using pdift = ptrdiff_t;
using uptrt = uintptr_t;
using pairStr = pair<string, string>;
using pairPath = pair<fs::path, fs::path>;
using mapFiles = umap<string, pair<sizet, uptrt>>;

using glm::vec2;
using glm::ivec2;
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
class Widget;

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
	std::move_backward(b.begin(), b.begin() + blen, b.begin() + a.length());
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

inline ivec2 texSize(SDL_Texture* tex) {
	ivec2 s;
	return !SDL_QueryTexture(tex, nullptr, nullptr, &s.x, &s.y) ? s : ivec2(0);
}

inline ivec2 mousePos() {
	ivec2 p;
	SDL_GetMouseState(&p.x, &p.y);
	return p;
}

inline void pushEvent(UserCode code, void* data1 = nullptr, void* data2 = nullptr) {
	SDL_Event event;
	event.user = { SDL_USEREVENT, SDL_GetTicks(), 0, int32(code), data1, data2 };
	SDL_PushEvent(&event);
}

// SDL_Rect wrapper
struct Rect : SDL_Rect {
	Rect() = default;
	constexpr Rect(int n);
	constexpr Rect(int px, int py, int sw, int sh);
	constexpr Rect(ivec2 pos, ivec2 size);

	ivec2& pos();
	constexpr ivec2 pos() const;
	ivec2& size();
	constexpr ivec2 size() const;
	constexpr ivec2 end() const;

	bool contain(ivec2 point) const;
	Rect crop(const Rect& rect);			// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify itself
};

constexpr Rect::Rect(int n) :
	SDL_Rect{ n, n, n, n }
{}

constexpr Rect::Rect(int px, int py, int sw, int sh) :
	SDL_Rect{ px, py, sw, sh }
{}

constexpr Rect::Rect(ivec2 pos, ivec2 size) :
	SDL_Rect{ pos.x, pos.y, size.x, size.y }
{}

inline ivec2& Rect::pos() {
	return *reinterpret_cast<ivec2*>(this);
}

constexpr ivec2 Rect::pos() const {
	return ivec2(x, y);
}

inline ivec2& Rect::size() {
	return reinterpret_cast<ivec2*>(this)[1];
}

constexpr ivec2 Rect::size() const {
	return ivec2(w, h);
}

constexpr ivec2 Rect::end() const {
	return pos() + size();
}

inline bool Rect::contain(ivec2 point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// reader picture
struct Texture {
	string name;
	SDL_Texture* tex;

	Texture(string&& tname = string(), SDL_Texture* texture = nullptr);

	ivec2 res() const;
};

inline Texture::Texture(string&& tname, SDL_Texture* texture) :
	name(std::move(tname)),
	tex(texture)
{}

inline ivec2 Texture::res() const {
	return texSize(tex);
}

// files and strings

bool isDriveLetter(const fs::path& path);
fs::path parentPath(const fs::path& path);
string strEnclose(string_view str);
vector<string> strUnenclose(string_view str);
vector<string_view> getWords(string_view str);

inline bool isSpace(int c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(int c) {
	return uint(c) > ' ' && c != 0x7F;
}

inline string_view trim(string_view str) {
	string_view::iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string_view(&*pos, std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base() - pos);
}

inline string firstUpper(string_view str) {
	string txt(str);
	txt[0] = toupper(txt[0]);
	return txt;
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
	return toStr(std::underlying_type_t<T>(num));
}

template <uint8 base = 10, class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
string toStr(T num, uint8 pad) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);

	uint8 len = res.ptr - buf.data();
	if (pad > len) {
		pad = std::min(uint8(pad - len), uint8(buf.size()));
		std::move_backward(buf.data(), res.ptr, buf.data() + pad);
		std::fill_n(buf.begin(), pad, '0');
		len += pad;
	}
	return string(buf.data(), len);
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

template <glm::length_t L, class T, glm::qualifier Q, std::enable_if_t<(std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_floating_point_v<T>, int> = 0>
string toStr(const glm::vec<L, T, Q>& v, string_view sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L - 1]);
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

template <class T, class N = typename T::value_type, class... A, std::enable_if_t<(std::is_integral_v<N> && !std::is_same_v<N, bool>) || std::is_floating_point_v<N>, int> = 0>
T toVec(string_view str, typename T::value_type fill = typename T::value_type(0), A... args) {
	T vec(fill);
	sizet p = 0;
	for (glm::length_t i = 0; p < str.length() && i < vec.length(); ++i) {
		for (; p < str.length() && isSpace(str[p]); ++p);
		for (; p < str.length(); ++p)
			if (std::from_chars_result res = std::from_chars(str.data() + p, str.data() + str.length(), vec[i], args...); res.ec == std::errc()) {
				p = res.ptr - str.data();
				break;
			}
	}
	return vec;
}

// container stuff

template <class T>
T btom(bool b) {
	return T(b) * T(2) - T(1);	// b needs to be 0 or 1
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> vswap(const T& x, const T& y, bool swap) {
	return swap ? glm::vec<2, T, Q>(y, x) : glm::vec<2, T, Q>(x, y);
}
