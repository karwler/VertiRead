#pragma once

#include "utils.h"
#ifdef WITH_ICU
#include <unicode/coll.h>
#endif

char32_t mbstowc(string_view::iterator& mb, size_t& len);

class StrNatCmp {
#ifdef WITH_ICU
private:
	static inline icu::Collator* collator = nullptr;
#endif

public:
#ifdef WITH_ICU
	static void init();
	static void free();
#endif

	bool operator()(string_view a, string_view b) const;
	bool operator()(const string& a, const string& b) const;
	bool operator()(const string& a, string_view b) const;

	static bool less(string_view a, string_view b);
	static bool less(const string& a, const string& b);
	static bool less(const string& a, string_view b);

#ifndef WITH_ICU
private:
	static std::strong_ordering cmp(string_view sa, string_view sb);
	static std::strong_ordering cmpLeft(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen);
	static std::strong_ordering cmpRight(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen);
	static std::strong_ordering cmpLetter(char32_t a, char32_t b);
	static char32_t skipSpaces(string_view::iterator& p, size_t& l);
#endif
};

#ifdef WITH_ICU
inline void StrNatCmp::free() {
	delete collator;
}
#endif

inline bool StrNatCmp::operator()(string_view a, string_view b) const {
	return less(a, b);
}

inline bool StrNatCmp::operator()(const string& a, const string& b) const {
	return less(string_view(a), string_view(b));
}

inline bool StrNatCmp::operator()(const string& a, string_view b) const {
	return less(string_view(a), b);
}

inline bool StrNatCmp::less(string_view a, string_view b) {
#ifdef WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	return collator->compareUTF8(a, b, status) == UCOL_LESS;
#else
	return cmp(a, b) < 0;
#endif
}

inline bool StrNatCmp::less(const string& a, const string& b) {
	return less(string_view(a), string_view(b));
}

inline bool StrNatCmp::less(const string& a, string_view b) {
	return less(string_view(a), b);
}
