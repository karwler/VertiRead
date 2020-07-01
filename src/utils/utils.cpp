#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

void pushEvent(UserCode code, void* data1, void* data2) {
	SDL_Event event;
	event.user = { SDL_USEREVENT, SDL_GetTicks(), 0, int32(code), data1, data2 };
	SDL_PushEvent(&event);
}

Rect Rect::crop(const Rect& rect) {
	Rect isct;
	if (!SDL_IntersectRect(this, &rect, &isct))
		return *this = Rect(0);

	ivec2 te = end(), ie = isct.end();
	Rect crop;
	crop.x = isct.x > x ? isct.x - x : 0;
	crop.y = isct.y > y ? isct.y - y : 0;
	crop.w = ie.x < te.x ? te.x - ie.x + crop.x : 0;
	crop.h = ie.y < te.y ? te.y - ie.y + crop.y : 0;
	*this = isct;
	return crop;
}

Thread::Thread(int (*func)(void*), void* pdata) :
	data(pdata)
{
	start(func);
}

bool Thread::start(int (*func)(void*)) {
	run = true;
	proc = SDL_CreateThread(func, "", this);
	if (!proc)
		run = false;
	return run;
}

int Thread::finish() {
	run = false;
	int res;
	SDL_WaitThread(proc, &res);
	proc = nullptr;
	return res;
}

bool isDriveLetter(const fs::path& path) {
	const fs::path::value_type* p = path.c_str();
	if (isalpha(p[0]) && p[1] == ':') {
		for (p += 2; isDsep(*p); p++);
		return !*p;
	}
	return false;
}

fs::path parentPath(const fs::path& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return topDir;
#endif
	const fs::path::value_type* p = path.c_str();
	if (sizet len = std::char_traits<fs::path::value_type>::length(p); len && isDsep(p[--len])) {
		while (len && isDsep(p[--len]));
		if (len)
			return fs::path(p, p + len).parent_path();
	}
	return path.parent_path();
}

string strEnclose(string str) {
	for (sizet i = 0; i < str.length(); i++)
		if (str[i] == '"')
			str.insert(str.begin() + i++, '\\');
	return '"' + str + '"';
}

vector<string> strUnenclose(const string& str) {
	vector<string> words;
	for (sizet pos = 0;;) {
		// find next start
		if (pos = str.find_first_of('"', pos); pos == string::npos)
			break;

		// find start's end
		sizet end = ++pos;
		for (;; end++)
			if (end = str.find_first_of('"', end); end == string::npos || str[end-1] != '\\')
				break;
		if (end >= str.length())
			break;

		// remove escapes and add to quote list
		string quote = str.substr(pos, end - pos);
		for (sizet i = quote.find_first_of('\\'); i < quote.length(); i = quote.find_first_of('\\', i + 1))
			if (quote[i+1] == '"')
				quote.erase(i, 1);
		words.push_back(std::move(quote));
		pos = end + 1;
	}
	return words;
}

vector<string> getWords(const string& str) {
	sizet i;
	for (i = 0; isSpace(str[i]); i++);

	vector<string> words;
	for (sizet start; str[i];) {
		for (start = i; notSpace(str[i]); i++);
		words.emplace_back(str, start, i - start);
		for (; isSpace(str[i]); i++);
	}
	return words;
}

#ifdef _WIN32
string cwtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return string();
	len--;

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

string swtos(const wstring& src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return string();

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring cstow(const char* src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	if (len <= 1)
		return wstring();
	len--;

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src, -1, dst.data(), len);
	return dst;
}

wstring sstow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return wstring();

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
	return dst;
}
#endif
