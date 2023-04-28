#pragma once

#include "utils.h"
#include <cwctype>
#ifdef WITH_ICU
#include <unicode/coll.h>
#endif

char32_t mbstowc(const char*& mb, size_t& len);

class StrNatCmp {
private:
#ifdef WITH_ICU
	static inline icu::Collator* collator = nullptr;
#endif

public:
#ifdef WITH_ICU
	static void init();
	static void free();
#endif

	static int u8strcicmp(string_view a, string_view b);

	bool operator()(const fs::path& a, const fs::path& b) const;
	template <class C, class T> bool operator()(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b) const;
	template <class C> bool operator()(const C* a, const C* b) const;

	static bool less(const fs::path& a, const fs::path& b);
	template <class C, class T> static bool less(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b);
	template <class C> static bool less(const C* a, const C* b);

private:
	template <class C> static int cmp(const C* a, const C* b);
	template <class C> static int cmpLeft(const C* a, const C* b);
	template <class C> static int cmpRight(const C* a, const C* b);
	template <class C> static int cmpLetter(C a, C b);

#ifndef WITH_ICU
	static int u8cmp(const char* a, size_t alen, const char* b, size_t blen);
	static int u8cmpLeft(char32_t ca, const char* a, size_t alen, char32_t cb, const char* b, size_t blen);
	static int u8cmpRight(char32_t ca, const char* a, size_t alen, char32_t cb, const char* b, size_t blen);
#endif
};

#ifdef WITH_ICU
inline void StrNatCmp::free() {
	delete collator;
}

inline int StrNatCmp::u8strcicmp(string_view ls, string_view rs) {
	UErrorCode status = U_ZERO_ERROR;
	return collator->compareUTF8(ls, rs, status);
}
#endif

inline bool StrNatCmp::operator()(const fs::path& a, const fs::path& b) const {
	return less(a, b);
}

template <class C, class T>
bool StrNatCmp::operator()(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b) const {
	return less(a, b);
}

template <class C>
bool StrNatCmp::operator()(const C* a, const C* b) const {
	return less(a, b);
}

inline bool StrNatCmp::less(const fs::path& a, const fs::path& b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
#ifdef _WIN32
	return collator->compare(reinterpret_cast<const char16_t*>(a.c_str()), a.native().length(), reinterpret_cast<const char16_t*>(b.c_str()), b.native().length(), status) == UCOL_LESS;
#else
	return collator->compareUTF8(icu::StringPiece(a.c_str(), a.native().length()), icu::StringPiece(b.c_str(), b.native().length()), status) == UCOL_LESS;
#endif
#else
#ifdef _WIN32
	return cmp(a.c_str(), b.c_str()) < 0;
#else
	return u8cmp(a.c_str(), a.native().length(), b.c_str(), b.native().length()) < 0;
#endif
#endif
}

template <class C, class T>
bool StrNatCmp::less(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	if constexpr (std::is_same_v<C, char>)
		return collator->compareUTF8(a, b, status) == UCOL_LESS;
	if constexpr (sizeof(C) == sizeof(char))
		return collator->compareUTF8(icu::StringPiece(reinterpret_cast<const char*>(a.data()), a.length()), icu::StringPiece(reinterpret_cast<const char*>(b.data()), b.length()), status) == UCOL_LESS;
	if constexpr (std::is_same_v<C, char16_t>)
		return collator->compare(a.c_str(), a.length(), b.c_str(), b.length(), status) == UCOL_LESS;
	if constexpr (sizeof(C) == sizeof(char16_t))
		return collator->compare(reinterpret_cast<const char16_t*>(a.c_str()), a.length(), reinterpret_cast<const char16_t*>(b.c_str()), b.length(), status) == UCOL_LESS;
#else
	if constexpr (std::is_same_v<C, char>)
		return u8cmp(a.data(), a.length(), b.data(), b.length()) < 0;
	if constexpr (sizeof(C) == sizeof(char))
		return u8cmp(reinterpret_cast<const char*>(a.data()), a.length(), reinterpret_cast<const char*>(b.data()), b.length());
#endif
	return cmp(a.c_str(), b.c_str()) < 0;
}

template <class C>
bool StrNatCmp::less(const C* a, const C* b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	if constexpr (std::is_same_v<C, char>)
		return collator->compareUTF8(a, b, status) == UCOL_LESS;
	if constexpr (sizeof(C) == sizeof(char))
		return collator->compareUTF8(reinterpret_cast<const char*>(a), strlen(a), reinterpret_cast<const char*>(b), strlen(b), status) == UCOL_LESS;
	if constexpr (std::is_same_v<C, char16_t>)
		return collator->compare(a, strlen(a), b, strlen(b), status) == UCOL_LESS;
	if constexpr (sizeof(C) == sizeof(char16_t))
		return collator->compare(reinterpret_cast<const char16_t*>(a), strlen(a), reinterpret_cast<const char16_t*>(b), strlen(b), status) == UCOL_LESS;
#else
	if constexpr (std::is_same_v<C, char>)
		return u8cmp(a, strlen(a), b, strlen(b)) < 0;
	if constexpr (sizeof(C) == sizeof(char))
		return u8cmp(reinterpret_cast<const char*>(a), strlen(a), reinterpret_cast<const char*>(b), strlen(b)) < 0;
#endif
	return cmp(a, b) < 0;
}

template <class C>
int StrNatCmp::cmp(const C* a, const C* b) {
	for (;; ++a, ++b) {
		C ca = *a, cb = *b;
		bool dnum;
		if constexpr (sizeof(C) == sizeof(char)) {
			for (; std::isspace(ca); ca = *++a);
			for (; std::isspace(cb); cb = *++b);
			dnum = std::isdigit(ca) && std::isdigit(cb);
		} else {
			for (; std::iswspace(ca); ca = *++a);
			for (; std::iswspace(cb); cb = *++b);
			dnum = std::iswdigit(ca) && std::iswdigit(cb);
		}

		if (dnum)
			if (int dif = ca == '0' || cb == '0' ? cmpLeft(a, b) : cmpRight(a, b))
				return dif;
		if (!(ca || cb))
			return 0;
		if (int dif = cmpLetter(ca, cb))
			return dif;
	}
}

template <class C>
int StrNatCmp::cmpLeft(const C* a, const C* b) {
	for (;; ++a, ++b) {
		C ca = *a, cb = *b;
		bool nad, nbd;
		if constexpr (sizeof(C) == sizeof(char)) {
			nad = !std::isdigit(ca);
			nbd = !std::isdigit(cb);
		} else {
			nad = !std::iswdigit(ca);
			nbd = !std::iswdigit(cb);
		}

		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = cmpLetter(ca, cb))
			return dif;
	}
}

template <class C>
int StrNatCmp::cmpRight(const C* a, const C* b) {
	for (int bias = 0;; ++a, ++b) {
		C ca = *a, cb = *b;
		bool nad, nbd;
		if constexpr (sizeof(C) == sizeof(char)) {
			nad = !std::isdigit(ca);
			nbd = !std::isdigit(cb);
		} else {
			nad = !std::iswdigit(ca);
			nbd = !std::iswdigit(cb);
		}

		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (!(ca || cb))
			return bias;
		if (int dif = cmpLetter(ca, cb); dif && !bias)
			bias = dif;
	}
}

template <class C>
int StrNatCmp::cmpLetter(C a, C b) {
	if (a != b) {
		int au, bu;
		if constexpr (sizeof(C) == sizeof(char)) {
			au = std::tolower(a);
			bu = std::tolower(b);
		} else {
			au = std::towlower(a);
			bu = std::towlower(b);
		}
		return au != bu ? (int(au > bu) - int(au < bu)) : (int(a > b) - int(a < b));
	}
	return 0;
}
