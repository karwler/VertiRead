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

static inline int natCmpLetter(int a, int b) {
	if (a != b) {
		int au = toupper(a), bu = toupper(b);
		return au != bu ? au - bu : b - a;
	}
	return 0;
}

static inline int natCmpLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = natCmpLetter(*a, *b))
			return dif;
	}
}

static inline int natCmpRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (!(*a || *b))
			return bias;
		if (int dif = natCmpLetter(*a, *b); dif && !bias)
			bias = dif;
	}
}

int strnatcmp(const char* a, const char* b) {
	for (;; a++, b++) {
		char ca = *a, cb = *b;
		for (; isSpace(ca); ca = *++a);
		for (; isSpace(cb); cb = *++b);

		if (isdigit(ca) && isdigit(cb))
			if (int dif = ca == '0' || cb == '0' ? natCmpLeft(a, b) : natCmpRight(a, b))
				return dif;
		if (!(ca || cb))
			return 0;
		if (int dif = natCmpLetter(*a, *b))
			return dif;
	}
}

static bool pathCompareLoop(const string& as, const string& bs, string::const_iterator& ai, string::const_iterator& bi) {
	do {
		// comparee names of next entry
		string::const_iterator an = std::find_if(ai, as.end(), isDsep);
		string::const_iterator bn = std::find_if(bi, bs.end(), isDsep);
		if (!std::equal(ai, an, bi, bn))
			return false;

		// skip directory separators
		ai = std::find_if(an, as.end(), notDsep);
		bi = std::find_if(bn, bs.end(), notDsep);
	} while (ai != as.end() && bi != bs.end());
	return true;	// one has reached it's end so don't forget to check later which one (paths are equal if both have ended)
}

bool pathCmp(const string& as, const string& bs) {
	string::const_iterator ai = as.begin(), bi = bs.begin();	// check if both paths have reached their ends simultaneously
	return pathCompareLoop(as, bs, ai, bi) && ai == as.end() && bi == bs.end();
}

bool isSubpath(const string& path, const string& parent) {
	if (std::all_of(parent.begin(), parent.end(), isDsep))	// always true if parent is root
		return true;

	string::const_iterator ai = path.begin(), bi = parent.begin();	// parent has to have reached it's end while path was still matching
	return pathCompareLoop(path, parent, ai, bi) && bi == parent.end();
}

string parentPath(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return dseps;
#endif
	string::const_reverse_iterator end = std::find_if(path.rbegin(), path.rend(), notDsep);	// skip initial separators
	if (end == path.rend())					// if the entire path is separators, return root
		return dseps;

	string::const_reverse_iterator pos = std::find_if(end, path.rend(), isDsep);	// skip to separators between parent and child
	if (pos == path.rend())					// if there are none, just cut off child
		return string(path.begin(), end.base());

	pos = std::find_if(pos, path.rend(), notDsep);	// skip separators to get to the parent entry
	return pos != path.rend() ? string(path.begin(), pos.base()) : dseps;	// cut off child
}

string getChild(const string& path, const string& parent) {
	if (std::all_of(parent.begin(), parent.end(), isDsep))
		return path;

	string::const_iterator ai = path.begin(), bi = parent.begin();
	return pathCompareLoop(path, parent, ai, bi) && bi == parent.end() ? string(ai, path.end()) : "";
}

string filename(const string& path) {
	if (path.empty())
		return path;
	if (std::all_of(path.begin(), path.end(), isDsep))
		return dseps;

	string::const_reverse_iterator end = notDsep(path.back()) ? path.rbegin() : std::find_if(path.rbegin() + 1, path.rend(), notDsep);
	string::const_reverse_iterator pos = std::find_if(end, path.rend(), isDsep);
	return pos != path.rend() ? string(pos.base(), end.base()) : string(path.begin(), end.base());
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
		return "";
	len--;

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

string swtos(const wstring& src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0, nullptr, nullptr);
	if (len <= 0)
		return "";

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring cstow(const char* src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	if (len <= 1)
		return L"";
	len--;

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src, -1, dst.data(), len);
	return dst;
}

wstring sstow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return L"";

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
	return dst;
}
#endif
