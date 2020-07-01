#pragma once

// stuff that's used pretty much everywhere
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#include <strings.h>
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <climits>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace fs = std::filesystem;

#ifdef main
#undef main
#endif

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
using std::vector;
using std::wstring;
using std::to_string;

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
class Button;
class DrawSys;
class Layout;
struct PictureLoader;
class Program;
class ProgState;
class Scene;

// events
using PCall = void (Program::*)(Button*);
using SBCall = void (ProgState::*)();
using SACall = void (ProgState::*)(float);

// global constants
#ifdef _WIN32
constexpr wchar topDir[] = L"";
#else
constexpr char topDir[] = "/";
#endif

constexpr array<char, 4> sizeLetters = {
	'B',
	'K',
	'M',
	'G'
};

constexpr array<uptrt, 4> sizeFactors = {
	1,
	1'000,
	1'000'000,
	1'000'000'000
};

// general wrappers

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

inline ivec2 texSize(SDL_Texture* tex) {
	ivec2 s;
	return !SDL_QueryTexture(tex, nullptr, nullptr, &s.x, &s.y) ? s : ivec2(0);
}

inline ivec2 mousePos() {
	ivec2 p;
	SDL_GetMouseState(&p.x, &p.y);
	return p;
}

inline string getRendererName(int id) {
	SDL_RendererInfo info;
	return !SDL_GetRenderDriverInfo(id, &info) ? info.name : "";
}

void pushEvent(UserCode code, void* data1 = nullptr, void* data2 = nullptr);

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

	bool contain(const ivec2& point) const;
	Rect crop(const Rect& rect);			// crop rect so it fits in the frame (aka set rect to the area where they overlap) and return how much was cut off
	Rect intersect(const Rect& rect) const;	// same as above except it returns the overlap instead of the crop and it doesn't modify itself
};

inline constexpr Rect::Rect(int n) :
	SDL_Rect({ n, n, n, n })
{}

inline constexpr Rect::Rect(int px, int py, int sw, int sh) :
	SDL_Rect({ px, py, sw, sh })
{}

inline constexpr Rect::Rect(ivec2 pos, ivec2 size) :
	SDL_Rect({ pos.x, pos.y, size.x, size.y })
{}

inline ivec2& Rect::pos() {
	return *reinterpret_cast<ivec2*>(this);
}

inline constexpr ivec2 Rect::pos() const {
	return ivec2(x, y);
}

inline ivec2& Rect::size() {
	return reinterpret_cast<ivec2*>(this)[1];
}

inline constexpr ivec2 Rect::size() const {
	return ivec2(w, h);
}

inline constexpr ivec2 Rect::end() const {
	return pos() + size();
}

inline bool Rect::contain(const ivec2& point) const {
	return SDL_PointInRect(reinterpret_cast<const SDL_Point*>(&point), this);
}

inline Rect Rect::intersect(const Rect& rect) const {
	Rect isct;
	return SDL_IntersectRect(this, &rect, &isct) ? isct : Rect(0);
}

// SDL_Thread wrapper

class Thread {
public:
	Thread(int (*func)(void*), void* pdata = nullptr);
	~Thread();

	bool start(int (*func)(void*));
	bool getRun() const;
	void interrupt();
	int finish();

	void* data;
private:
	SDL_Thread* proc;
	bool run;
};

inline Thread::~Thread() {
	finish();
}

inline bool Thread::getRun() const {
	return run;
}

inline void Thread::interrupt() {
	run = false;
}


// files and strings

inline int strcicmp(const string& a, const string& b) {	// case insensitive check if strings are equal
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

inline int strncicmp(const string& a, const string& b, sizet n) {	// case insensitive check if strings are equal
#ifdef _WIN32
	return _strnicmp(a.c_str(), b.c_str(), n);
#else
	return strncasecmp(a.c_str(), b.c_str(), n);
#endif
}

bool isDriveLetter(const fs::path& path);
fs::path parentPath(const fs::path& path);
string strEnclose(string str);
vector<string> strUnenclose(const string& str);
vector<string> getWords(const string& str);

inline bool isSpace(int c) {
	return (c > '\0' && c <= ' ') || c == 0x7F;
}

inline bool notSpace(int c) {
	return uint(c) > ' ' && c != 0x7F;
}

inline string trim(const string& str) {
	string::const_iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string(pos, std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base());
}

inline string trimZero(const string& str) {
	sizet id = str.find_last_not_of('0');
	return str.substr(0, str[id] == '.' ? id : id + 1);
}

inline string firstUpper(string str) {
	str[0] = char(toupper(str[0]));
	return str;
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

inline sizet memSizeMag(uptrt num) {
	sizet m;
	for (m = 0; m + 1 < sizeLetters.size() && (!(num % 1000) && (num /= 1000)); m++);
	return m;
}

inline string memoryString(uptrt num, sizet mag) {
	string str = to_string(num / sizeFactors[mag]);
	return (mag ? str + sizeLetters[mag] : str) + sizeLetters[0];
}

inline string memoryString(uptrt num) {
	return memoryString(num, memSizeMag(num));
}

// conversions
#ifdef _WIN32
string cwtos(const wchar* wstr);
string swtos(const wstring& wstr);
wstring cstow(const char* str);
wstring sstow(const string& str);
#endif

inline string stos(const char* str) {	// dummy function for World::setArgs
	return str;
}

inline bool stob(const string& str) {
	return str == "true" || str == "1";
}

inline const char* btos(bool b) {
	return b ? "true" : "false";
}

template <class T, sizet N>
T strToEnum(const array<const char*, N>& names, const string& str) {
	return T(std::find_if(names.begin(), names.end(), [str](const string& it) -> bool { return !strcicmp(it, str); }) - names.begin());
}

template <class T>
T strToVal(const umap<T, const char*>& names, const string& str) {
	umap<uint8, const char*>::const_iterator it = std::find_if(names.begin(), names.end(), [str](const pair<T, const char*>& nit) -> bool { return !strcicmp(nit.second, str); });
	return it != names.end() ? it->first : T(0);
}

inline long sstol(const string& str, int base = 0) {
	return strtol(str.c_str(), nullptr, base);
}

inline llong sstoll(const string& str, int base = 0) {
	return strtoll(str.c_str(), nullptr, base);
}

inline ulong sstoul(const string& str, int base = 0) {
	return strtoul(str.c_str(), nullptr, base);
}

inline ullong sstoull(const string& str, int base = 0) {
	return strtoull(str.c_str(), nullptr, base);
}

inline float sstof(const string& str) {
	return strtof(str.c_str(), nullptr);
}

inline double sstod(const string& str) {
	return strtod(str.c_str(), nullptr);
}

inline ldouble sstold(const string& str) {
	return strtold(str.c_str(), nullptr);
}

template <class T>
T btom(bool b) {
	return T(b) * T(2) - T(1);	// b needs to be 0 or 1
}

template <class T, class F, class... A>
T readNumber(const char*& pos, F strtox, A... args) {
	T num = T(0);
	for (char* end; *pos; pos++)
		if (num = T(strtox(pos, &end, args...)); pos != end) {
			pos = end;
			break;
		}
	return num;
}

template <class T, class F, class... A>
T stoxv(const char* str, F strtox, typename T::value_type fill, A... args) {
	T vec(fill);
	for (glm::length_t i = 0; *str && i < vec.length();)
		vec[i++] = readNumber<typename T::value_type>(str, strtox, args...);
	return vec;
}

template <class T, class F = decltype(strtof)>
T stofv(const char* str, F strtox = strtof, typename T::value_type fill = typename T::value_type(0)) {
	return stoxv<T>(str, strtox, fill);
}

template <class T, class F>
T stoiv(const char* str, F strtox, typename T::value_type fill = typename T::value_type(0), int base = 0) {
	return stoxv<T>(str, strtox, fill, base);
}

// container stuff

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> min(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min) {
	return glm::vec<2, T, Q>(std::min(val.x, min.x), std::min(val.y, min.y));
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> max(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& max) {
	return glm::vec<2, T, Q>(std::max(val.x, max.x), std::max(val.y, max.y));
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> clamp(const glm::vec<2, T, Q>& val, const glm::vec<2, T, Q>& min, const glm::vec<2, T, Q>& max) {
	return glm::vec<2, T, Q>(std::clamp(val.x, min.x, max.x), std::clamp(val.y, min.y, max.y));
}

template <class T, glm::qualifier Q = glm::defaultp>
glm::vec<2, T, Q> vswap(const T& x, const T& y, bool swap) {
	return swap ? glm::vec<2, T, Q>(y, x) : glm::vec<2, T, Q>(x, y);
}
