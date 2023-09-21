#include "compare.h"
#include <cwctype>

char32_t mbstowc(string_view::iterator& mb, size_t& len) {
	if (!len)
		return '\0';

	uchar c = *mb++;
	--len;
	if (c < 0x80)
		return c;
	if ((c >= 0x80 && c <= 0xBF) || c >= 0xF5) {
		for (; len && ((uchar(*mb) >= 0x80 && uchar(*mb) <= 0xBF) || uchar(*mb) >= 0xF5); ++mb, --len);
		return '\0';
	}

	char32_t w;
	size_t cnt;
	if (c >= 0xF0) {
		w = c & 0x07;
		cnt = 3;
	} else if (c >= 0xE0) {
		w = c & 0x0F;
		cnt = 2;
	} else {
		w = c & 0x1F;
		cnt = 1;
	}

	if (len < cnt) {
		for (; len && ((uchar(*mb) >= 0x80 && uchar(*mb) <= 0xBF) || uchar(*mb) >= 0xF5); ++mb, --len);
		return '\0';
	}
	len -= cnt;

	for (size_t l = cnt; l; --l, ++mb) {
		c = *mb;
		if (c < 0x80 || c > 0xBF) {
			len += l;
			return '\0';
		}
		w = (w << 6) | (c & 0x3F);
	}
	return !(w >= 0x110000 || (w >= 0x00D800 && w <= 0x00DFFF)) ? w : '\0';
}

#ifdef WITH_ICU
void StrNatCmp::init() {
	UErrorCode status = U_ZERO_ERROR;
	collator = icu::Collator::createInstance(icu::Locale::getDefault(), status);
	if (U_FAILURE(status))
		throw std::runtime_error(u_errorName(status));

	collator->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);
	collator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
}

#else

std::strong_ordering StrNatCmp::cmp(string_view sa, string_view sb) {
	string_view::iterator a = sa.begin(), b = sb.begin();
	size_t alen = sa.length(), blen = sb.length();
	while (alen && blen) {
		char32_t ca = skipSpaces(a, alen), cb = skipSpaces(b, blen);
		if (std::iswdigit(ca) && std::iswdigit(cb))
			if (std::strong_ordering dif = ca == '0' || cb == '0' ? cmpLeft(ca, a, alen, cb, b, blen) : cmpRight(ca, a, alen, cb, b, blen); dif != 0)
				return dif;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != 0)
			return dif;
	}
	return alen == 0 && blen == 0 ? std::strong_ordering::equal : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
}

char32_t StrNatCmp::skipSpaces(string_view::iterator& p, size_t& l) {
	char32_t c;
	do {
		c = mbstowc(p, l);
	} while (l && std::iswspace(c));
	return c;
}

std::strong_ordering StrNatCmp::cmpLeft(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen) {
	for (;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !std::iswdigit(ca), nbd = !std::iswdigit(cb);
		if (nad && nbd)
			return std::strong_ordering::equal;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != 0)
			return dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? std::strong_ordering::equal : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
	}
}

std::strong_ordering StrNatCmp::cmpRight(char32_t ca, string_view::iterator a, size_t alen, char32_t cb, string_view::iterator b, size_t blen) {
	for (std::strong_ordering bias = std::strong_ordering::equal;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !std::iswdigit(ca), nbd = !std::iswdigit(cb);
		if (nad && nbd)
			return bias;
		if (nad)
			return std::strong_ordering::less;
		if (nbd)
			return std::strong_ordering::greater;
		if (std::strong_ordering dif = cmpLetter(ca, cb); dif != 0 && bias == 0)
			bias = dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? bias : alen == 0 ? std::strong_ordering::less : std::strong_ordering::greater;
	}
}

std::strong_ordering StrNatCmp::cmpLetter(char32_t a, char32_t b) {
	if (a != b) {
		wint_t au = std::towlower(a), bu = std::towlower(b);
		return au != bu ? au <=> bu : a <=> b;
	}
	return std::strong_ordering::equal;
}
#endif
