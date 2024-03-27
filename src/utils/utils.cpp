#include "utils.h"
#include <cwctype>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// DATA

void Data::resize(size_t siz) {
	if (siz) {
		uptr<byte_t[]> dat = std::make_unique_for_overwrite<byte_t[]>(siz);
		if (ptr)
			std::copy_n(ptr.get(), std::min(len, siz), dat.get());
		ptr = std::move(dat);
		len = siz;
	} else
		clear();
}

void Data::clear() noexcept {
	ptr.reset();
	len = 0;
}

// CSTRING

Cstring::Cstring(Cstring&& s) noexcept :
	ptr(s.ptr)
{
	s.ptr = &nullch;
}

Cstring::~Cstring() {
	if (ptr != &nullch)
		delete[] ptr;
}

Cstring& Cstring::operator=(const Cstring& s) {
	return assign(s);
}

Cstring& Cstring::operator=(Cstring&& s) noexcept {
	if (ptr != &nullch)
		delete[] ptr;
	ptr = s.ptr;
	s.ptr = &nullch;
	return *this;
}

Cstring& Cstring::operator=(const char* s) {
	return assign(s);
}

Cstring& Cstring::operator=(const string& s) {
	return assign(s);
}

#ifdef _WIN32
Cstring& Cstring::operator=(const wchar_t* s) {
	return assign(s);
}

Cstring& Cstring::operator=(wstring_view s) {
	return assign(s);
}
#endif

Cstring& Cstring::operator=(const fs::path& s) {
	return assign(s);
}

Cstring& Cstring::operator=(string_view s) {
	return assign(s);
}

Cstring& Cstring::operator=(std::initializer_list<char> s) {
	return assign(s);
}

template <class T>
Cstring& Cstring::assign(T&& s) {
	clear();
	set(std::forward<T>(s));
	return *this;
}

void Cstring::set(const char* s) {
	if (size_t len = strlen(s)) {
		ptr = new char[++len];
		std::copy_n(s, len, ptr);
	}
}

void Cstring::set(const char* s, size_t l) {
	if (l) {
		ptr = new char[l + 1];
		std::copy_n(s, l, ptr);
		ptr[l] = '\0';
	}
}

void Cstring::set(const string& s) {
	if (size_t len = s.length()) {
		ptr = new char[++len];
		std::copy_n(s.data(), len, ptr);
	}
}

#ifdef _WIN32
void Cstring::set(const wchar_t* s) {
	if (*s)
		if (int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr); len > 1) {
			ptr = new char[len];
			WideCharToMultiByte(CP_UTF8, 0, s, -1, ptr, len, nullptr, nullptr);
		}
}

void Cstring::set(wstring_view s) {
	if (!s.empty())
		if (int len = WideCharToMultiByte(CP_UTF8, 0, s.data(), s.length(), nullptr, 0, nullptr, nullptr); len > 0) {
			ptr = new char[len + 1];
			WideCharToMultiByte(CP_UTF8, 0, s.data(), s.length(), ptr, len, nullptr, nullptr);
			ptr[len] = '\0';
		}
}
#endif

void Cstring::set(const fs::path& s) {
	if (size_t slen = s.native().length()) {
#ifdef _WIN32
		if (int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), ++slen, nullptr, 0, nullptr, nullptr); len > 1) {
			ptr = new char[len];
			WideCharToMultiByte(CP_UTF8, 0, s.c_str(), slen, ptr, len, nullptr, nullptr);
		}
#else
		ptr = new char[++slen];
		std::copy_n(s.c_str(), slen, ptr);
#endif
	}
}

void Cstring::set(std::initializer_list<char> s) {
	if (size_t len = s.size()) {
		ptr = new char[len + 1];
		rng::copy(s, ptr);
		ptr[len] = '\0';
	}
}

void Cstring::clear() noexcept {
	if (ptr != &nullch) {
		delete[] ptr;
		ptr = &nullch;
	}
}

// FUNCTIONS

template <Integer C>
bool tstrciequal(std::basic_string_view<C> a, std::basic_string_view<C> b) {
	if (a.length() != b.length())
		return false;
	for (size_t i = 0; i < a.length(); ++i) {
		if constexpr (sizeof(C) == sizeof(char)) {
			if (toupper(a[i]) != toupper(b[i]))
				return false;
		} else
			if (towupper(a[i]) != towupper(b[i]))
				return false;
	}
	return true;
}

bool strciequal(string_view a, string_view b) noexcept {
	return tstrciequal(a, b);
}

bool strciequal(wstring_view a, wstring_view b) noexcept {
	return tstrciequal(a, b);
}

string_view parentPath(string_view path) noexcept {
	string_view::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), notDsep);
	it = std::find_if(it, path.rend(), isDsep);
	it = std::find_if(it, path.rend(), notDsep);
	size_t len = it.base() - path.begin();
	return len || path.empty() || notDsep(path[0]) ? string_view(path.data(), len) : "/";
}

static bool pathCompareLoop(string_view::iterator& ai, string_view::iterator ae, string_view::iterator& bi, string_view::iterator be) {
	while (ai != ae && bi != be) {
		// comparee names of next entry
		string_view::iterator an = std::find_if(ai, ae, isDsep);
		string_view::iterator bn = std::find_if(bi, be, isDsep);
		if (!std::equal(ai, an, bi, bn))
			return false;

		// skip directory separators
		ai = std::find_if(an, ae, notDsep);
		bi = std::find_if(bn, be, notDsep);
	}
	return true;	// one has reached it's end so don't forget to check later which one (paths are equal if both have ended)
}

static bool pathCompareLoop(const char*& ai, const char*& bi) {
#ifdef _WIN32
	constexpr char dseps[] = "\\/";
#else
	constexpr char dseps[] = "/";
#endif
	while (*ai && *bi) {
		const char* an = ai + strcspn(ai, dseps);
		const char* bn = bi + strcspn(bi, dseps);
		if (!std::equal(ai, an, bi, bn))
			return false;
		ai = an + strspn(an, dseps);
		bi = bn + strspn(bn, dseps);
	}
	return true;
}

bool pathEqual(string_view a, string_view b) noexcept {
	string_view::iterator ai = a.begin(), bi = b.begin();	// check if both paths have reached their ends simultaneously
	return pathCompareLoop(ai, a.end(), bi, b.end()) && ai == a.end() && bi == b.end();
}

bool pathEqual(const char* a, const char* b) noexcept {
	return pathCompareLoop(a, b) && !*a && !*b;
}

string_view relativePath(string_view path, string_view base) noexcept {
	string_view::iterator ai = path.begin(), bi = base.begin();
	return pathCompareLoop(ai, path.end(), bi, base.end()) && bi == base.end() ? string_view(ai, path.end()) : string_view();
}

bool isSubpath(string_view path, string_view base) noexcept {
	string_view::iterator ai = path.begin(), bi = base.begin();	// parent has to have reached its end while path was still matching
	return pathCompareLoop(ai, path.end(), bi, base.end()) && bi == base.end();
}

tm currentDateTime() noexcept {
	time_t rawt = time(nullptr);
	tm tim;
#ifdef _WIN32
	localtime_s(&tim, &rawt);
#else
	localtime_r(&rawt, &tim);
#endif
	return tim;
}

void copyPixels(void* dst, const void* src, uint dpitch, uint spitch, uint bwidth, uint height) noexcept {
	if (dpitch == spitch)
		memcpy(dst, src, size_t(dpitch) * size_t(height));
	else {
		auto dp = static_cast<uint8*>(dst);
		auto sp = static_cast<const uint8*>(src);
		for (uint r = 0; r < height; ++r, dp += dpitch, sp += spitch)
			memcpy(dp, sp, bwidth);
	}
}

#ifdef _WIN32
static bool hasDriveLetter(const char* path) {
	return ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':';
}

static bool isUnc(const char* path) {
	return isDsep(path[0]) && isDsep(path[1]) && notDsep(path[2]);
}

bool isAbsolute(string_view path) noexcept {
	return path.length() >= 3 && ((hasDriveLetter(path.data()) && isDsep(path[2])) || isUnc(path.data()));
}

string swtos(wstring_view src) {
	string dst;
	if (int len = WideCharToMultiByte(CP_UTF8, 0, src.data(), src.length(), nullptr, 0, nullptr, nullptr); len > 0) {
		dst.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, src.data(), src.length(), dst.data(), len, nullptr, nullptr);
	}
	return dst;
}

wstring sstow(string_view src) {
	wstring dst;
	if (int len = MultiByteToWideChar(CP_UTF8, 0, src.data(), src.length(), nullptr, 0); len > 0) {
		dst.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, src.data(), src.length(), dst.data(), len);
	}
	return dst;
}

string winErrorMessage(uint32 msgId) {
	wchar_t* buff = nullptr;
	string msg = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, msgId, 0, reinterpret_cast<LPWSTR>(&buff), 0, nullptr) ? swtos(trim(buff)) : string();
	if (buff)
		LocalFree(buff);
	return msg;
}
#endif
