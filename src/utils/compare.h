#pragma once

#include "utils.h"

class StrNatCmp {
public:
	bool operator()(const fs::path& a, const fs::path& b) const;
	template <class C, class T, class A> bool operator()(const std::basic_string<C, T, A>& a, const std::basic_string<C, T, A>& b) const;
	template <class C> bool operator()(const C* a, const C* b) const;

	static bool less(const fs::path& a, const fs::path& b);
	template <class C, class T, class A> static bool less(const std::basic_string<C, T, A>& a, const std::basic_string<C, T, A>& b);
	template <class C> static bool less(const C* a, const C* b);

private:
	template <class C> static int cmp(const C* a, const C* b);
	template <class C> static int cmpLeft(const C* a, const C* b);
	template <class C> static int cmpRight(const C* a, const C* b);
	template <class C> static int cmpLetter(C a, C b);
};

inline bool StrNatCmp::operator()(const fs::path& a, const fs::path& b) const {
	return less(a, b);
}

template <class C, class T, class A>
bool StrNatCmp::operator()(const std::basic_string<C, T, A>& a, const std::basic_string<C, T, A>& b) const {
	return less(a, b);
}

template <class C>
bool StrNatCmp::operator()(const C* a, const C* b) const {
	return less(a, b);
}

inline bool StrNatCmp::less(const fs::path& a, const fs::path& b) {
	return cmp(a.c_str(), b.c_str()) < 0;
}

template <class C, class T, class A>
bool StrNatCmp::less(const std::basic_string<C, T, A>& a, const std::basic_string<C, T, A>& b) {
	return cmp(a.c_str(), b.c_str()) < 0;
}

template <class C>
bool StrNatCmp::less(const C* a, const C* b) {
	return cmp(a, b) < 0;
}

template <class C>
int StrNatCmp::cmp(const C* a, const C* b) {
	for (;; ++a, ++b) {
		C ca = *a, cb = *b;
		for (; isSpace(ca); ca = *++a);
		for (; isSpace(cb); cb = *++b);

		if (isdigit(ca) && isdigit(cb))
			if (int dif = ca == '0' || cb == '0' ? cmpLeft(a, b) : cmpRight(a, b))
				return dif;
		if (!(ca || cb))
			return 0;
		if (int dif = cmpLetter(*a, *b))
			return dif;
	}
}

template <class C>
int StrNatCmp::cmpLeft(const C* a, const C* b) {
	for (;; ++a, ++b) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = cmpLetter(*a, *b))
			return dif;
	}
}

template <class C>
int StrNatCmp::cmpRight(const C* a, const C* b) {
	for (int bias = 0;; ++a, ++b) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (!(*a || *b))
			return bias;
		if (int dif = cmpLetter(*a, *b); dif && !bias)
			bias = dif;
	}
}

template <class C>
int StrNatCmp::cmpLetter(C a, C b) {
	if (a != b) {
		int au = toupper(a), bu = toupper(b);
		return au != bu ? (int(au > bu) - int(au < bu)) : (int(a > b) - int(a < b));
	}
	return 0;
}
