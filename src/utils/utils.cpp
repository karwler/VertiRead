#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

void pushEvent(UserEvent code, void* data1, void* data2) {
	SDL_Event event{};
	event.user.type = code;
	event.user.timestamp = SDL_GetTicks();
	event.user.data1 = data1;
	event.user.data2 = data2;
	if (int rc = SDL_PushEvent(&event); rc <= 0)
		throw std::runtime_error(rc ? SDL_GetError() : "Event queue full");
}

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
	if (size_t len = std::char_traits<fs::path::value_type>::length(p); len && isDsep(p[--len])) {
		while (len && isDsep(p[--len]));
		if (len)
			return fs::path(p, p + len).parent_path();
	}
	return path.parent_path();
}

string_view filename(string_view path) {
	string_view::reverse_iterator end = std::find_if_not(path.rbegin(), path.rend(), isDsep);
	string_view::iterator pos = std::find_if(end, path.rend(), isDsep).base();
	return string_view(&*pos, end.base() - pos);
}

string_view fileExtension(string_view path) {
	string_view::reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() && *it == '.' && it + 1 != path.rend() && !isDsep(it[1]) ? string_view(&*it.base(), path.end() - it.base()) : string_view();
}

string_view trim(string_view str) {
	string_view::iterator pos = std::find_if(str.begin(), str.end(), notSpace);
	return string_view(str.data() + (pos - str.begin()), std::find_if(str.rbegin(), std::make_reverse_iterator(pos), notSpace).base() - pos);
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

uint32 ceilPow2(uint32 val) {
	--val;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	return val + 1;
}
