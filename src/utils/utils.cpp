#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

bool isDriveLetter(const fs::path& path) {
	if (const fs::path::value_type* p = path.c_str(); isalpha(p[0]) && p[1] == ':') {
		for (p += 2; isDsep(*p); ++p);
		return !*p;
	}
	return false;
}

fs::path parentPath(const fs::path& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return fs::path();
#endif
	const fs::path::value_type* p = path.c_str();
	if (sizet len = std::char_traits<fs::path::value_type>::length(p); len && isDsep(p[--len])) {
		while (len && isDsep(p[--len]));
		if (len)
			return fs::path(p, p + len).parent_path();
	}
	return path.parent_path();
}

string strEnclose(string_view str) {
	string txt(str);
	for (sizet i = txt.find_first_of("\"\\"); i < txt.length(); i = txt.find_first_of("\"\\", i + 2))
		txt.insert(txt.begin() + i, '\\');
	return '"' + txt + '"';
}

vector<string> strUnenclose(string_view str) {
	vector<string> words;
	for (sizet pos = 0;;) {
		// find next start
		if (pos = str.find_first_of('"', pos); pos == string::npos)
			break;

		// find start's end
		sizet end = ++pos;
		for (;; ++end)
			if (end = str.find_first_of('"', end); end == string::npos || str[end - 1] != '\\')
				break;
		if (end >= str.length())
			break;

		// remove escapes and add to quote list
		string quote(str.data() + pos, end - pos);
		for (sizet i = quote.find_first_of('\\'); i < quote.length(); i = quote.find_first_of('\\', i + 1))
			if (quote[i + 1] == '"')
				quote.erase(i, 1);
		words.push_back(std::move(quote));
		pos = end + 1;
	}
	return words;
}

vector<string_view> getWords(string_view str) {
	vector<string_view> words;
	sizet p = 0;
	for (; p < str.length() && isSpace(str[p]); ++p);
	while (p < str.length()) {
		sizet i = p;
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

uint32 ceilPow2(uint32 val) {
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return val + 1;
}
