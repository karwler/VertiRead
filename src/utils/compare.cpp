#include "compare.h"

char32_t mbstowc(const char*& mb, size_t& len) {
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

int StrNatCmp::u8strcicmp(string_view ls, string_view rs) {
	size_t llen = ls.length();
	size_t rlen = rs.length();
	const char* lmb = ls.data();
	const char* rmb = rs.data();
	while (llen && rlen)
		if (char32_t lwc = mbstowc(lmb, llen), rwc = mbstowc(rmb, rlen); lwc != rwc)
			if (wint_t ll = std::towlower(lwc), rl = std::towlower(rwc); ll != rl)
				return int(ll) - int(rl);
	return llen == 0 && rlen == 0 ? 0 : llen == 0 ? -1 : 1;
}

int StrNatCmp::u8cmp(const char* a, size_t alen, const char* b, size_t blen) {
	while (alen && blen) {
		char32_t ca, cb;
		do {
			ca = mbstowc(a, alen);
		} while (alen && std::iswspace(ca));
		do {
			cb = mbstowc(b, blen);
		} while (blen && std::iswspace(cb));

		if (std::iswdigit(ca) && std::iswdigit(cb))
			if (int dif = ca == '0' || cb == '0' ? u8cmpLeft(ca, a, alen, cb, b, blen) : u8cmpRight(ca, a, alen, cb, b, blen))
				return dif;
		if (int dif = cmpLetter(ca, cb))
			return dif;
	}
	return alen == 0 && blen == 0 ? 0 : alen == 0 ? -1 : 1;
}

int StrNatCmp::u8cmpLeft(char32_t ca, const char* a, size_t alen, char32_t cb, const char* b, size_t blen) {
	for (;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !std::iswdigit(ca), nbd = !std::iswdigit(cb);
		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = cmpLetter(ca, cb))
			return dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? 0 : alen == 0 ? -1 : 1;
	}
}

int StrNatCmp::u8cmpRight(char32_t ca, const char* a, size_t alen, char32_t cb, const char* b, size_t blen) {
	for (int bias = 0;; ca = mbstowc(a, alen), cb = mbstowc(b, blen)) {
		bool nad = !std::iswdigit(ca), nbd = !std::iswdigit(cb);
		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = cmpLetter(ca, cb); dif && !bias)
			bias = dif;
		if (!(alen && blen))
			return alen == 0 && blen == 0 ? bias : alen == 0 ? -1 : 1;
	}
}
#endif
