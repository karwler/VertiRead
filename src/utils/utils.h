#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/common.hpp>
#include <glm/fwd.hpp>
#define SDL_MAIN_HANDLED
#include <SDL_log.h>
#include <SDL_rect.h>
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace fs = std::filesystem;
namespace rng = std::ranges;

// to make life easier
using schar = signed char;
using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;

using std::array;
using std::optional;
using std::pair;
using std::string;
using std::string_view;
using std::tuple;
using std::vector;
using std::wstring;
using std::wstring_view;

using byte_t = std::byte;
using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using uptr = std::unique_ptr<T...>;

using glm::mat4;
using glm::vec2;
using glm::dvec2;
using glm::uvec2;
using glm::u32vec2;
using glm::ivec2;
using glm::vec3;
using glm::vec4;
using glm::u8vec4;
using glm::ivec4;

// forward declarations
class Browser;
class Button;
class CheckBox;
struct Children;
class ComboBox;
class Context;
class CredentialManager;
class DrawSys;
class FileOps;
class FileSys;
class IconButton;
class IconPushButton;
class InputSys;
class Label;
class LabelEdit;
class Layout;
class Overlay;
class Picture;
struct PicLim;
class Popup;
class Program;
class ProgState;
class PushButton;
class ReaderBox;
class Renderer;
class RootLayout;
class Scene;
class ScrollArea;
class Settings;
class Slider;
class Texture;
class Widget;
class WindowArranger;

// general wrappers

template <class T> concept Class = std::is_class_v<T>;
template <class T, class B> concept Derived = std::is_base_of_v<B, T>;
template <class T> concept Enumeration = std::is_enum_v<T>;
template <class T> concept Integer = std::is_integral_v<T> && !std::is_same_v<T, bool>;
template <class T> concept IntEnum = std::is_integral_v<T> || std::is_enum_v<T>;
template <class T> concept Floating = std::is_floating_point_v<T>;
template <class T> concept IntEnumFloat = IntEnum<T> || Floating<T>;
template <class T> concept Number = Integer<T> || Floating<T>;
template <class T> concept Pointer = std::is_pointer_v<T>;
template <class T> concept IntegerPointer = Integer<T> || Pointer<T>;
template <class T> concept VecNumber = Number<typename T::value_type>;
template <class T> concept Iterator = requires(T t) { ++t; *t; };
template <class T> concept MemberFunction = std::is_member_function_pointer_v<T>;
template <class T, class... A> concept Invocable = std::is_invocable_v<T, A...>;
template <class T, class R, class... A> concept InvocableR = std::is_invocable_r_v<R, T, A...>;

#ifdef _WIN32
#define LINEND "\r\n"
inline constexpr char directorySeparator = '\\';
#else
#define LINEND "\n"
inline constexpr char directorySeparator = '/';
#endif

template <Enumeration T>
constexpr T operator~(T a) {
	return T(~std::underlying_type_t<T>(a));
}

template <Enumeration T>
constexpr T operator&(T a, T b) {
	return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr T operator&=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr T operator|(T a, T b) {
	return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr T operator|=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr T operator^(T a, T b) {
	return T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr T operator^=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <Enumeration T>
constexpr std::underlying_type_t<T> eint(T e) {
	return std::underlying_type_t<T>(e);
}

template <Number T>
constexpr bool outRange(T val, T min, T max) {
	return val < min || val > max;
}

template <IntegerPointer T>
T coalesce(T val, T alt) {
	return val ? val : alt;
}

template <Integer T>
string operator+(std::basic_string<T>&& a, std::basic_string_view<T> b) {
	size_t alen = a.length();
	a.resize(alen + b.length());
	rng::copy(b, a.begin() + alen);
	return a;
}

template <Integer T>
string operator+(const std::basic_string<T>& a, std::basic_string_view<T> b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	rng::copy(a, r.begin());
	rng::copy(b, r.begin() + a.length());
	return r;
}

template <Integer T>
string operator+(std::basic_string_view<T> a, std::basic_string<T>&& b) {
	size_t blen = b.length();
	b.resize(a.length() + blen);
	std::move_backward(b.begin(), b.begin() + blen, b.end());
	rng::copy(a, b.begin());
	return b;
}

template <Integer T>
string operator+(std::basic_string_view<T> a, const std::basic_string<T>& b) {
	std::basic_string<T> r;
	r.resize(a.length() + b.length());
	rng::copy(a, r.begin());
	rng::copy(b, r.begin() + a.length());
	return r;
}

template <Class T>
T valcp(const T& v) {
	return v;	// to avoid useless type cast warning for copying
}

#ifndef NDEBUG
inline void dbgPass() { /* for setting break points in awkward places */ }
#endif

// SDL_Rect equivalent (must be reinterpretable)
template <Number T>
struct Rect {
	using tvec2 = glm::vec<2, T, glm::defaultp>;
	using tvec4 = glm::vec<4, T, glm::defaultp>;

	T x, y, w, h;

	Rect() = default;
	template <Number U> constexpr Rect(const Rect<U>& r) : x(T(r.x)), y(T(r.y)), w(T(r.w)), h(T(r.h)) {}
	constexpr Rect(T n) : x(n), y(n), w(n), h(n) {}
	constexpr Rect(T px, T py, T sw, T sh) : x(px), y(py), w(sw), h(sh) {}
	constexpr Rect(T px, T py, const tvec2& sv) : x(px), y(py), w(sv.x), h(sv.y) {}
	constexpr Rect(const tvec2& pv, T sw, T sh) : x(pv.x), y(pv.y), w(sw), h(sh) {}
	constexpr Rect(const tvec2& pv, const tvec2& sv) : x(pv.x), y(pv.y), w(sv.x), h(sv.y) {}
	constexpr Rect(const tvec4& rv) : x(rv.x), y(rv.y), w(rv.z), h(rv.w) {}

	tvec2& pos() { return *reinterpret_cast<tvec2*>(this); }
	constexpr tvec2 pos() const { return tvec2(x, y); }
	tvec2& size() { return reinterpret_cast<tvec2*>(this)[1]; }
	constexpr tvec2 size() const { return tvec2(w, h); }
	constexpr tvec2 end() const { return pos() + size(); }
	tvec4& asVec() { return *reinterpret_cast<tvec4*>(this); }
	const tvec4& asVec() const { return *reinterpret_cast<const tvec4*>(this); }
	SDL_Rect& asRect() { return *reinterpret_cast<SDL_Rect*>(this); }
	const SDL_Rect& asRect() const { return *reinterpret_cast<const SDL_Rect*>(this); }

	constexpr bool operator==(const Rect& rect) const;
	constexpr bool empty() const;
	constexpr bool contains(const tvec2& point) const;
	constexpr bool overlaps(const Rect& rect) const;
	constexpr Rect<T> intersect(const Rect& rect) const;
	constexpr Rect<T> translate(const tvec2& mov) const;
};

using Recti = Rect<int>;
using Rectu = Rect<uint>;
using Rectf = Rect<float>;

template <Number T>
constexpr bool Rect<T>::operator==(const Rect& rect) const {
	return x == rect.x && y == rect.y && w == rect.w && h == rect.h;
}

template <Number T>
constexpr bool Rect<T>::empty() const {
	return w <= T(0) || h <= T(0);
}

template <Number T>
constexpr bool Rect<T>::contains(const tvec2& point) const {
	return point.x >= x && point.x < x + w && point.y >= y && point.y < y + h;
}

template <Number T>
constexpr bool Rect<T>::overlaps(const Rect& rect) const {
	if (!empty() && !rect.empty()) {
		tvec2 dpos = glm::max(pos(), rect.pos());
		tvec2 dend = glm::min(end(), rect.end());
		return dend.x > dpos.x && dend.y > dpos.y;
	}
	return false;
}

template <Number T>
constexpr Rect<T> Rect<T>::intersect(const Rect& rect) const {
	if (!empty() && !rect.empty()) {
		tvec2 dpos = glm::max(pos(), rect.pos());
		tvec2 dend = glm::min(end(), rect.end());
		return Rect(dpos, dend - dpos);
	}
	return Rect(T(0));
}

template <Number T>
constexpr Rect<T> Rect<T>::translate(const tvec2& mov) const {
	return Rect(pos() + mov, size());
}

// size of a widget in pixels or relative to it's parent
struct Size {
	enum Mode : uint8 {
		rela,	// relative to parent size
		pixv,	// absolute pixel value
		calc	// use the calculation function
	};

	union {
		float prc;
		int pix;
		int (*cfn)(const Widget*);
	};
	uint id = UINT_MAX;	// a widget's id in its parent's widget list
	Mode mod;

	template <Floating T = float> constexpr Size(T percent = T(1)) : prc(percent), mod(rela) {}
	template <Integer T> constexpr Size(T pixels) : pix(pixels), mod(pixv) {}
	constexpr Size(int (*calcul)(const Widget*)) : cfn(calcul), mod(calc) {}

	int operator()(const Widget* wgt) const { return cfn(wgt); }
};

using svec2 = glm::vec<2, Size, glm::defaultp>;

// byte sequence that shouldn't change its location
class Data {
protected:
	uptr<byte_t[]> ptr;
	size_t len = 0;

public:
	Data() = default;
	Data(size_t siz) : ptr(std::make_unique_for_overwrite<byte_t[]>(siz)), len(siz) {}
	Data(uptr<byte_t[]>&& p, size_t l) : ptr(std::move(p)), len(l) {}

	byte_t* data() { return ptr.get(); }
	const byte_t* data() const { return ptr.get(); }
	size_t size() const { return len; }
	void resize(size_t siz);
	void clear() noexcept;
};

// constant dynamic C string
class Cstring {
public:
	static inline char nullch = '\0';

private:
	char* ptr = &nullch;

public:
	Cstring() = default;
	Cstring(const Cstring& s) { set(s); }
	Cstring(Cstring&& s) noexcept;
	Cstring(const char* s) { set(s); }
	Cstring(const char* s, size_t l) { set(s, l); }
	Cstring(const char* p, const char* e) { set(p, e - p); }
	Cstring(const string& s) { set(s); }
#ifdef _WIN32
	Cstring(const wchar_t* s) { set(s); }
	Cstring(wstring_view s) { set(s); }
#endif
	Cstring(const fs::path& s) { set(s); }
	Cstring(string_view s) { set(s); }
	Cstring(std::initializer_list<char> s) { set(s); }
	~Cstring();

	Cstring& operator=(const Cstring& s);
	Cstring& operator=(Cstring&& s) noexcept;
	Cstring& operator=(const char* s);
	Cstring& operator=(const string& s);
#ifdef _WIN32
	Cstring& operator=(const wchar_t* s);
	Cstring& operator=(wstring_view s);
#endif
	Cstring& operator=(const fs::path& s);
	Cstring& operator=(string_view s);
	Cstring& operator=(std::initializer_list<char> s);

	bool operator==(const Cstring& s) const { return !strcmp(ptr, s.ptr); }
	bool operator==(const char* s) const { return !strcmp(ptr, s); }
	bool operator==(const string& s) const { return !strcmp(ptr, s.data()); }
	bool operator==(string_view s) const;

	char& operator[](size_t i) { return ptr[i]; }
	char operator[](size_t i) const { return ptr[i]; }
	char* data() { return ptr; }
	const char* data() const { return ptr; }
	size_t length() const { return strlen(ptr); }
	bool filled() const { return *ptr; }
	void clear() noexcept;

private:
	template <class T> Cstring& assign(T&& s);
	void set(const Cstring& s) { set(s.data()); }
	void set(const char* s);
	void set(const char* s, size_t l);
	void set(const string& s);
#ifdef _WIN32
	void set(const wchar_t* s);
	void set(wstring_view s);
#endif
	void set(const fs::path& s);
	void set(string_view s) { set(s.data(), s.length()); }
	void set(std::initializer_list<char> s);
};

inline bool Cstring::operator==(string_view s) const {
	return !strncmp(ptr, s.data(), s.length()) && !ptr[s.length()];
}

// files and strings

bool strciequal(string_view a, string_view b) noexcept;
bool strciequal(wstring_view a, wstring_view b) noexcept;
string_view parentPath(string_view path) noexcept;
bool pathEqual(string_view a, string_view b) noexcept;
bool pathEqual(const char* a, const char* b) noexcept;
string_view relativePath(string_view path, string_view base) noexcept;
bool isSubpath(string_view path, string_view base) noexcept;
#ifdef _WIN32
bool isAbsolute(string_view path) noexcept;
#endif

template <Integer T>
bool strfilled(const T* str) {
	return str && str[0];
}

inline bool isSpace(int c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(int c) {
	return uint(c) > ' ' && c != 0x7F;
}

inline bool isDsep(int c) {
#ifdef _WIN32
	return c == directorySeparator || c == '/';
#else
	return c == directorySeparator;
#endif
}

inline bool notDsep(int c) {
#ifdef _WIN32
	return c != directorySeparator && c != '/';
#else
	return c != directorySeparator;
#endif
}

#ifdef _WIN32
inline bool isDriveLetter(string_view path) {
	return path.length() >= 2 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':' && std::all_of(path.begin() + 2, path.end(), isDsep);
}
#else
inline bool isAbsolute(string_view path) {
	return !path.empty() && path[0] == '/';
}
#endif

template <Integer T>
std::basic_string_view<T> filename(std::basic_string_view<T> path) {
	typename std::basic_string_view<T>::reverse_iterator end = std::find_if(path.rbegin(), path.rend(), notDsep);
	return std::basic_string_view<T>(std::find_if(end, path.rend(), isDsep).base(), end.base());
}

template <Integer T>
std::basic_string_view<T> filename(const std::basic_string<T>& path) {
	return filename(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> filename(const T* path) {
	return filename(std::basic_string_view<T>(path));
}

template <Integer T>
const T* filenamePtr(std::basic_string_view<T> path) {
	return std::to_address(std::find_if(path.rbegin(), path.rend(), isDsep).base());
}

template <Integer T>
const T* filenamePtr(const std::basic_string<T>& path) {
	return filenamePtr(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> fileExtension(std::basic_string_view<T> path) {
	typename std::basic_string_view<T>::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() && *it == '.' && it + 1 != path.rend() && notDsep(it[1]) ? std::basic_string_view<T>(it.base(), path.end()) : std::basic_string_view<T>();
}

template <Integer T>
std::basic_string_view<T> fileExtension(const std::basic_string<T>& path) {
	return fileExtension(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> fileExtension(const T* path) {
	return fileExtension(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> delExtension(std::basic_string_view<T> path) {
	typename std::basic_string_view<T>::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() ? *it == '.' && it + 1 != path.rend() && notDsep(it[1]) ? std::basic_string_view<T>(path.begin(), it.base() - 1) : path : std::basic_string_view<T>();
}

template <Integer T>
std::basic_string_view<T> delExtension(const std::basic_string<T>& path) {
	return delExtension(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> delExtension(const T* path) {
	return delExtension(std::basic_string_view<T>(path));
}

template <Integer T>
std::basic_string_view<T> trim(std::basic_string_view<T> str) {
	typename std::basic_string_view<T>::iterator pos = rng::find_if(str, notSpace);
	return std::basic_string_view<T>(pos, std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base());
}

template <Integer T>
std::basic_string_view<T> trim(const std::basic_string<T>& str) {
	return trim(std::basic_string_view<T>(str));
}

template <Integer T>
std::basic_string_view<T> trim(const T* str) {
	return trim(std::basic_string_view<T>(str));
}

template <Integer T>
std::basic_string<T> operator/(std::basic_string<T>&& a, std::basic_string_view<T> b) {
	size_t alen = a.length();
	uint nds = alen && !b.empty() && notDsep(a.back()) && notDsep(b[0]);
	a.resize(alen + nds + b.length());
	if (nds)
		a[alen] = directorySeparator;
	rng::copy(b, a.begin() + alen + nds);
	return a;
}

template <Integer T>
std::basic_string<T> operator/(const std::basic_string<T>& a, std::basic_string_view<T> b) {
	std::basic_string<T> r;
	uint nds = !a.empty() && !b.empty() && notDsep(a.back()) && notDsep(b[0]);
	r.resize(a.length() + nds + b.length());
	rng::copy(a, r.begin());
	if (nds)
		r[a.length()] = directorySeparator;
	rng::copy(b, r.begin() + a.length() + nds);
	return r;
}

template <Integer T>
std::basic_string<T> operator/(std::basic_string_view<T> a, std::basic_string<T>&& b) {
	size_t blen = b.length();
	uint nds = !a.empty() && blen && notDsep(a.back()) && notDsep(b[0]);
	b.resize(a.length() + nds + blen);
	std::move_backward(b.begin(), b.begin() + blen, b.end());
	rng::copy(a, b.begin());
	if (nds)
		b[a.length()] = directorySeparator;
	return b;
}

template <Integer T>
std::basic_string<T> operator/(std::basic_string_view<T> a, const std::basic_string<T>& b) {
	std::basic_string<T> r;
	uint nds = !a.empty() && !b.empty() && notDsep(a.back()) && notDsep(b[0]);
	r.resize(a.length() + nds + b.length());
	rng::copy(a, r.begin());
	if (nds)
		r[a.length()] = directorySeparator;
	rng::copy(b, r.begin() + a.length() + nds);
	return r;
}

template <Integer T>
std::basic_string<T> operator/(std::basic_string<T>&& a, const std::basic_string<T>& b) {
	return std::move(a) / std::basic_string_view<T>(b);
}

template <Integer T>
std::basic_string<T> operator/(const std::basic_string<T>& a, const std::basic_string<T>& b) {
	return a / std::basic_string_view<T>(b);
}

template <Integer T>
std::basic_string<T> operator/(std::basic_string<T>&& a, const T* b) {
	return std::move(a) / std::basic_string_view<T>(b);
}

template <Integer T>
std::basic_string<T> operator/(const std::basic_string<T>& a, const T* b) {
	return a / std::basic_string_view<T>(b);
}

template <Integer T>
std::basic_string<T> operator/(const T* a, std::basic_string<T>&& b) {
	return std::basic_string_view<T>(a) / std::move(b);
}

template <Integer T>
std::basic_string<T> operator/(const T* a, const std::basic_string<T>& b) {
	return std::basic_string_view<T>(a) / b;
}

// conversions
#ifdef _WIN32
string swtos(wstring_view wstr);
wstring sstow(string_view str);
string winErrorMessage(uint32 msgId);
#endif

inline fs::path toPath(string_view path) {
#ifdef _WIN32
	return fs::path(sstow(path));
#else
	return fs::path(path);
#endif
}

#ifdef _WIN32
inline string fromPath(const fs::path& path) {
	return swtos(path.native());
#else
inline const string& fromPath(const fs::path& path) {
	return path.native();
#endif
}

inline const char* toStr(bool b) {
	return b ? "true" : "false";
}

template <Class S = string, uint8 base = 10, Integer T>
S toStr(T num) {
	array<char, sizeof(T) * 8 + std::is_signed_v<T>> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num, base);
	if constexpr (base > 10)
		std::transform(buf.data(), res.ptr, buf.data(), toupper);
	return S(buf.data(), res.ptr);
}

template <Class S = string, uint8 base = 10, Enumeration T>
S toStr(T num) {
	return toStr<S, base>(eint(num));
}

template <Class S = string, Floating T>
S toStr(T num, std::chars_format fmt = std::chars_format::fixed) {
	array<char, 64> buf;
	std::to_chars_result res = std::to_chars(buf.data(), buf.data() + buf.size(), num, fmt);
	return S(buf.data(), res.ptr);
}

template <uint8 base = 10, glm::length_t L, Integer T, glm::qualifier Q>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr<string, base>(v[i]) + sep;
	return str + toStr<string, base>(v[L - 1]);
}

template <glm::length_t L, Floating T, glm::qualifier Q>
string toStr(const glm::vec<L, T, Q>& v, const char* sep = " ") {
	string str;
	for (glm::length_t i = 0; i < L - 1; ++i)
		str += toStr(v[i]) + sep;
	return str + toStr(v[L - 1]);
}

template <Number T, class... A>
T toNum(string_view str, A... args) {
	T val = T(0);
	size_t i = 0;
	for (; i < str.length() && isSpace(str[i]); ++i);
	std::from_chars(str.data() + i, str.data() + str.length(), val, args...);
	return val;
}

template <VecNumber T, class... A>
T toVec(string_view str, typename T::value_type fill = typename T::value_type(0), A... args) {
	T vec(fill);
	size_t p = 0;
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

tm currentDateTime() noexcept;
void copyPixels(void* dst, const void* src, uint dpitch, uint spitch, uint bwidth, uint height) noexcept;

template <Number T>
T btom(bool b) {
	return T(b) * T(2) - T(1);	// b needs to be 0 or 1
}

template <Integer T>
T roundToMultiple(T val, T mul) {
	T rem = val % mul;
	return rem ? val + mul - rem : val;
}

template <class... A>
void logInfo(A&&... args) {
	try {
		std::ostringstream ss;
		(ss << ... << std::forward<A>(args));
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", ss.str().data());
	} catch (...) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to log");
	}
}

template <class... A>
void logError(A&&... args) {
	try {
		std::ostringstream ss;
		(ss << ... << std::forward<A>(args));
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", ss.str().data());
	} catch (...) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to log");
	}
}
