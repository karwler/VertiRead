#include "utils.h"
#ifdef _WIN32
#include <windows.h>
#endif

vector<string> getAvailibleRenderers() {
	vector<string> renderers((sizet(SDL_GetNumRenderDrivers())));
	for (sizet i = 0; i < renderers.size(); i++)
		renderers[i] = getRendererName(int(i));
	return renderers;
}

Rect Rect::crop(const Rect& rect) {
	Rect isct;
	if (!SDL_IntersectRect(this, &rect, &isct))
		return *this = Rect(0);

	vec2i te = end(), ie = isct.end();
	Rect crop;
	crop.x = isct.x > x ? isct.x - x : 0;
	crop.y = isct.y > y ? isct.y - y : 0;
	crop.w = ie.x < te.x ? te.x - ie.x + crop.x : 0;
	crop.h = ie.y < te.y ? te.y - ie.y + crop.y : 0;
	*this = isct;
	return crop;
}

Thread::Thread(int (*func)(void*), void* data) :
	data(data)
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

static inline int natCompareRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		bool nad = notDigit(*a), nbd = notDigit(*b);
		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (*a < *b) {
			if (!bias)
				bias = -1;
		} else if (*a > *b) {
			if (!bias)
				bias = 1;
		} else if (!(*a || *b))
			return bias;
	}
}

static inline int natCompareLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		bool nad = notDigit(*a), nbd = notDigit(*b);
		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (*a < *b)
			return -1;
		if (*a > *b)
			return 1;
	}
}

int strnatcmp(const char* a, const char* b) {
	for (;; a++, b++) {
		char ca = *a, cb = *b;
		for (; isSpace(ca); ca = *++a);
		for (; isSpace(cb); cb = *++b);

		if (isDigit(ca) && isDigit(cb)) {
			if (ca == '0' || cb == '0') {
				if (int result = natCompareLeft(a, b))
					return result;
			} else if (int result = natCompareRight(a, b))
				return result;
		}
		if (!(ca || cb))
			return 0;
		if (ca < cb)
			return -1;
		if (ca > cb)
			return 1;
	}
}

static bool pathCompareLoop(const string& as, const string& bs, sizet& ai, sizet& bi) {
	do {
		// comparee names of next entry
		sizet an = as.find_first_of(dsep, ai);
		sizet bn = bs.find_first_of(dsep, bi);
		if (as.compare(ai, an - ai, bs, bi, bn - bi))
			return false;
		ai = an;
		bi = bn;

		// skip directory separators
		ai = as.find_first_not_of(dsep, ai);
		bi = bs.find_first_not_of(dsep, bi);
	} while (ai < as.length() && bi < bs.length());
	return true;	// one has reached it's end so don't forget to check later which one (paths are equal if both have ended)
}

bool pathCmp(const string& as, const string& bs) {
	sizet ai = 0, bi = 0;	// check if both paths have reached their ends simultaneously
	return pathCompareLoop(as, bs, ai, bi) ? ai >= as.length() && bi >= bs.length() : false;
}

bool isSubpath(const string& path, string parent) {
	if (std::all_of(parent.begin(), parent.end(), [](char c) -> bool { return c == dsep; }))	// always true if parent is root
		return true;

	sizet ai = 0, bi = 0;	// parent has to have reached it's end while path was still matching
	return pathCompareLoop(path, parent, ai, bi) ? bi >= parent.length() : false;
}

string parentPath(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return dseps;
#endif
	sizet end = path.find_last_not_of(dsep);	// skip initial separators
	if (end == string::npos)					// if the entire path is separators, return root
		return dseps;

	sizet pos = path.find_last_of(dsep, end);	// skip to separators between parent and child
	if (pos == string::npos)					// if there are none, just cut off child
		return path.substr(0, end + 1);

	pos = path.find_last_not_of(dsep, pos);		// skip separators to get to the parent entry
	return pos == string::npos ? dseps : path.substr(0, pos + 1);	// cut off child
}

string getChild(const string& path, const string& parent) {
	if (std::all_of(parent.begin(), parent.end(), [](char c) -> bool { return c == dsep; }))
		return path;

	sizet ai = 0, bi = 0;
	return pathCompareLoop(path, parent, ai, bi) && bi >= parent.length() ? path.substr(ai) : emptyStr;
}

string filename(const string& path) {
	if (path.empty() || path == dseps)
		return emptyStr;

	sizet end = path.back() == dsep ? path.length() - 1 : path.length();
	sizet pos = path.find_last_of(dsep, end - 1);
	return pos == string::npos ? path.substr(0, end) : path.substr(pos + 1, end-pos - 1);
}

string strEnclose(string str) {
	for (string::iterator it = str.begin(); it != str.end(); it++)
		if (*it == '"')
			str.insert(it++, '\\');
	return '"' + str + '"';
}

vector<string> strUnenclose(const string& str) {
	vector<string> words;
	for (sizet pos = 0;;) {
		// find next start
		pos = str.find_first_of('"', pos) + 1;
		if (!pos)
			break;

		// find start's end
		sizet end = pos;
		for (;; end++) {
			end = str.find_first_of('"', end);
			if (end == string::npos || str[end-1] != '\\')
				break;
		}
		if (end >= str.length())
			break;

		// remove escapes and add to quote list
		string quote = str.substr(pos, end - pos);
		for (sizet i = quote.find_first_of('\\'); i < quote.length(); i = quote.find_first_of('\\', i + 1))
			if (quote[i+1] == '"')
				quote.erase(i, 1);
		words.push_back(quote);
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
string wtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return emptyStr;
	len--;
	
	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

string wtos(const wstring& src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return emptyStr;

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const char* src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	if (len <= 1)
		return L"";
	len--;

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src, -1, dst.data(), len);
	return dst;
}

wstring stow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return L"";

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
	return dst;
}
#endif
