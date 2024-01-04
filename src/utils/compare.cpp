#include "compare.h"
#include <cwctype>

char32_t mbstowc(string_view::iterator& pos, size_t& len) {
	if (!len)
		return '\0';

	uchar c = *pos++;
	--len;
	if (c < 0x80)
		return c;
	if ((c >= 0x80 && c <= 0xBF) || c >= 0xF5) {
		for (; len && (c = *pos, (c >= 0x80 && c <= 0xBF) || c >= 0xF5); ++pos, --len);
		return '\0';
	}

	char32_t w;
	size_t l;
	if (c >= 0xF0) {
		w = c & 0x07;
		l = 3;
	} else if (c >= 0xE0) {
		w = c & 0x0F;
		l = 2;
	} else {
		w = c & 0x1F;
		l = 1;
	}

	if (len < l) {
		for (; len && (c = *pos, (c >= 0x80 && c <= 0xBF) || c >= 0xF5); ++pos, --len);
		return '\0';
	}
	len -= l;

	for (; l; --l, ++pos) {
		if (c = *pos; c < 0x80 || c > 0xBF) {
			len += l;
			return '\0';
		}
		w = (w << 6) | (c & 0x3F);
	}
	return !(w >= 0x110000 || (w >= 0x00D800 && w <= 0x00DFFF)) ? w : '\0';
}

char32_t mbstowc(const char*& pos) {
	uchar c = *pos;
	if (!c)
		return '\0';

	++pos;
	if (c < 0x80)
		return c;
	if ((c >= 0x80 && c <= 0xBF) || c >= 0xF5) {
		for (; (c = *pos) && ((c >= 0x80 && c <= 0xBF) || c >= 0xF5); ++pos);
		return '\0';
	}

	char32_t w;
	size_t l;
	if (c >= 0xF0) {
		w = c & 0x07;
		l = 3;
	} else if (c >= 0xE0) {
		w = c & 0x0F;
		l = 2;
	} else {
		w = c & 0x1F;
		l = 1;
	}

	for (; l; --l, ++pos) {
		if (c = *pos; c < 0x80 || c > 0xBF) {
			for (; (c = *pos) && c >= 0xF5; ++pos);
			return '\0';
		}
		w = (w << 6) | (c & 0x3F);
	}
	return !(w >= 0x110000 || (w >= 0x00D800 && w <= 0x00DFFF)) ? w : '\0';
}

#ifdef WITH_ICU
void Strcomp::init() {
	UErrorCode status = U_ZERO_ERROR;
	collator = icu::Collator::createInstance(icu::Locale::getDefault(), status);
	if (U_FAILURE(status))
		throw std::runtime_error(u_errorName(status));

	collator->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);
	collator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
}

#else

std::strong_ordering Strcomp::cmp(string_view sa, string_view sb) {
	string_view::iterator a = sa.begin(), b = sb.begin();
	size_t alen = sa.length(), blen = sb.length();
	while (alen && blen) {
		char32_t ca = skipSpaces(a, alen), cb = skipSpaces(b, blen);
		if (iswdigit(ca) && iswdigit(cb))
			if (std::strong_ordering dif = ca == '0' || cb == '0' ? cmpLeft(ca, a, alen, cb, b, blen) : cmpRight(ca, a, alen, cb, b, blen); dif != std::strong_ordering::equal)
				return dif;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal)
			return dif;
	}
	return alen == 0 && blen == 0 ? std::strong_ordering::equal : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
}

std::strong_ordering Strcomp::cmpLeft(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen) {
	for (;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !iswdigit(ca), nbd = !iswdigit(cb);
		if (nad && nbd)
			return std::strong_ordering::equal;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal)
			return dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? std::strong_ordering::equal : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
	}
}

std::strong_ordering Strcomp::cmpRight(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen) {
	for (std::strong_ordering bias = std::strong_ordering::equal;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !iswdigit(ca), nbd = !iswdigit(cb);
		if (nad && nbd)
			return bias;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal && bias == std::strong_ordering::equal)
			bias = dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? bias : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
	}
}

char32_t Strcomp::skipSpaces(string_view::iterator& p, size_t& l) {
	char32_t c;
	do {
		c = mbstowc(p, l);
	} while (l && iswspace(c));
	return c;
}

std::strong_ordering Strcomp::cmpLetter(char32_t a, char32_t b) {
	if (a != b) {
		wint_t au = towupper(a), bu = towupper(b);
		return au != bu ? au <=> bu : a <=> b;
	}
	return std::strong_ordering::equal;
}

std::strong_ordering Strcomp::cmp(const char* a, const char* b) {
	for (;;) {
		char32_t ca = skipSpaces(a), cb = skipSpaces(b);
		if (iswdigit(ca) && iswdigit(cb))
			if (std::strong_ordering dif = ca == '0' || cb == '0' ? cmpLeft(ca, a, cb, b) : cmpRight(ca, a, cb, b); dif != std::strong_ordering::equal)
				return dif;
		if (!(ca || cb))
			return std::strong_ordering::equal;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal)
			return dif;
	}
}

std::strong_ordering Strcomp::cmpLeft(char32_t ca, const char* a, char32_t cb, const char* b) {
	for (;; ca = mbstowc(a), cb = mbstowc(b)) {
		bool nad = !iswdigit(ca), nbd = !iswdigit(cb);
		if (nad && nbd)
			return std::strong_ordering::equal;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal)
			return dif;
	}
}

std::strong_ordering Strcomp::cmpRight(char32_t ca, const char* a, char32_t cb, const char* b) {
	for (std::strong_ordering bias = std::strong_ordering::equal;; ca = mbstowc(a), cb = mbstowc(b)) {
		bool nad = !iswdigit(ca), nbd = !iswdigit(cb);
		if (nad && nbd)
			return bias;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (!(ca || cb))
			return bias;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != std::strong_ordering::equal && bias == std::strong_ordering::equal)
			bias = dif;
	}
}

char32_t Strcomp::skipSpaces(const char*& p) {
	char32_t c;
	do {
		c = mbstowc(p);
	} while (iswspace(c));
	return c;
}
#endif
