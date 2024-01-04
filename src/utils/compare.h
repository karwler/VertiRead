#pragma once

#include "utils.h"
#ifdef WITH_ICU
#include <unicode/coll.h>
#endif

char32_t mbstowc(string_view::iterator& pos, size_t& len);
char32_t mbstowc(const char*& pos);

class Strcomp {
#ifdef WITH_ICU
private:
	static inline icu::Collator* collator = nullptr;
#endif

public:
#ifdef WITH_ICU
	static void init();
	static void free() { delete collator; }
#endif

	bool operator()(const Cstring& a, const Cstring& b) const;

	static bool less(const char* a, const char* b);
	static bool less(string_view a, string_view b);

#ifndef WITH_ICU
private:
	static std::strong_ordering cmp(string_view sa, string_view sb);
	static std::strong_ordering cmpLeft(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen);
	static std::strong_ordering cmpRight(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen);
	static std::strong_ordering cmpLetter(char32_t a, char32_t b);
	static char32_t skipSpaces(string_view::iterator& p, size_t& l);

	static std::strong_ordering cmp(const char* a, const char* b);
	static std::strong_ordering cmpLeft(char32_t ca, const char* a, char32_t cb, const char* b);
	static std::strong_ordering cmpRight(char32_t ca, const char* a, char32_t cb, const char* b);
	static char32_t skipSpaces(const char*& p);
#endif
};

inline bool Strcomp::operator()(const Cstring& a, const Cstring& b) const {
	return less(a.data(), b.data());
}

inline bool Strcomp::less(const char* a, const char* b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	return collator->compareUTF8(a, b, status) == UCOL_LESS;
#else
	return cmp(a, b) == std::strong_ordering::less;
#endif
}

inline bool Strcomp::less(string_view a, string_view b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	return collator->compareUTF8(a, b, status) == UCOL_LESS;
#else
	return cmp(a, b) == std::strong_ordering::less;
#endif
}
