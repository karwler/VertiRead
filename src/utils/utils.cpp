#include "utils.h"
#include <cwctype>
#ifdef _WIN32
#include <windows.h>
#endif

void pushEvent(UserEvent code, void* data1, void* data2) {
	SDL_Event event = { .user = {
		.type = code,
		.timestamp = SDL_GetTicks(),
		.data1 = data1,
		.data2 = data2
	} };
	if (int rc = SDL_PushEvent(&event); rc <= 0)
		throw std::runtime_error(rc ? SDL_GetError() : "Event queue full");
}

template <Integer C>
bool tstrciequal(std::basic_string_view<C> a, std::basic_string_view<C> b) {
	if (a.length() != b.length())
		return false;
	for (size_t i = 0; i < a.length(); ++i) {
		if constexpr (sizeof(C) == sizeof(char)) {
			if (std::tolower(a[i]) != std::tolower(b[i]))
				return false;
		} else
			if (std::towlower(a[i]) != std::towlower(b[i]))
				return false;
	}
	return true;
}

bool strciequal(string_view a, string_view b) {
	return tstrciequal(a, b);
}

bool strciequal(wstring_view a, wstring_view b) {
	return tstrciequal(a, b);
}

bool strnciequal(string_view a, string_view b, size_t n) {
	size_t alen = std::min(a.length(), n);
	if (alen != std::min(b.length(), n))
		return false;
	for (size_t i = 0; i < alen; ++i)
		if (std::tolower(a[i]) != std::tolower(b[i]))
			return false;
	return true;
}

string_view parentPath(string_view path) {
	string_view::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), notDsep);
	it = std::find_if(it, path.rend(), isDsep);
	it = std::find_if(it, path.rend(), notDsep);
	size_t len = it.base() - path.begin();
	return len || path.empty() || notDsep(path[0]) ? string_view(path.data(), len) : "/";
}

static bool pathCompareLoop(string_view as, string_view bs, string_view::iterator& ai, string_view::iterator& bi) {
	while (ai != as.end() && bi != bs.end()) {
		// comparee names of next entry
		string_view::iterator an = std::find_if(ai, as.end(), isDsep);
		string_view::iterator bn = std::find_if(bi, bs.end(), isDsep);
		if (!std::equal(ai, an, bi, bn))
			return false;

		// skip directory separators
		ai = std::find_if(an, as.end(), notDsep);
		bi = std::find_if(bn, bs.end(), notDsep);
	}
	return true;	// one has reached it's end so don't forget to check later which one (paths are equal if both have ended)
}

string_view relativePath(string_view path, string_view base) {
	string_view::iterator ai = path.begin(), bi = base.begin();
	return pathCompareLoop(path, base, ai, bi) && bi == base.end() ? string_view(ai, path.end()) : string_view();
}

bool isSubpath(string_view path, string_view base) {
	string_view::iterator ai = path.begin(), bi = base.begin();	// parent has to have reached its end while path was still matching
	return pathCompareLoop(path, base, ai, bi) && bi == base.end();
}

string strEnclose(string_view str) {
	string txt(str);
	for (size_t i = txt.find_first_of("\"\\"); i < txt.length(); i = txt.find_first_of("\"\\", i + 2))
		txt.insert(txt.begin() + i, '\\');
	return '"' + txt + '"';
}

vector<string> strUnenclose(string_view str) {
	vector<string> words;
	for (size_t pos = 0;;) {
		// find next start
		if (pos = str.find_first_of('"', pos); pos == string::npos)
			break;

		// find start's end
		size_t end = ++pos;
		for (;; ++end)
			if (end = str.find_first_of('"', end); end == string::npos || str[end - 1] != '\\')
				break;
		if (end >= str.length())
			break;

		// remove escapes and add to quote list
		string quote(str.data() + pos, end - pos);
		for (size_t i = quote.find_first_of('\\'); i < quote.length(); i = quote.find_first_of('\\', i + 1))
			if (quote[i + 1] == '"')
				quote.erase(i, 1);
		words.push_back(std::move(quote));
		pos = end + 1;
	}
	return words;
}

vector<string_view> getWords(string_view str) {
	vector<string_view> words;
	size_t p = 0;
	for (; p < str.length() && isSpace(str[p]); ++p);
	while (p < str.length()) {
		size_t i = p;
		for (; i < str.length() && notSpace(str[i]); ++i);
		words.emplace_back(str.data() + p, i - p);
		for (p = i; p < str.length() && isSpace(str[p]); ++p);
	}
	return words;
}

tm currentDateTime() {
	time_t rawt = time(nullptr);
	tm tim;
#ifdef _WIN32
	localtime_s(&tim, &rawt);
#else
	localtime_r(&rawt, &tim);
#endif
	return tim;
}

#ifdef _WIN32
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
#endif
