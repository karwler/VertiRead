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

bool Rect::overlap(const Rect& rect, vec2i& sback, vec2i& rback) const {
	if (w <= 0 || h <= 0 || rect.w <= 0 || rect.h <= 0)	// idfk
		return false;

	sback = back();
	rback = rect.back();
	return x < rback.x && y < rback.y && sback.x >= rect.x && sback.y >= rect.y;
}

Rect Rect::crop(const Rect& frame) {
	vec2i rback, fback;
	if (!overlap(frame, rback, fback))
		return *this = Rect(0);

	Rect crop(0);	// crop rect if it's boundaries are out of frame
	if (x < frame.x) {	// left
		crop.x = frame.x - x;
		x = frame.x;
		w -= crop.x;
	}
	if (rback.x > fback.x) {	// right
		crop.w = rback.x - fback.x;
		w -= crop.w;
	}
	if (y < frame.y) {	// top
		crop.y = frame.y - y;
		y = frame.y;
		h -= crop.y;
	}
	if (rback.y > fback.y) {	// bottom
		crop.h = rback.y - fback.y;
		h -= crop.h;
	}
	crop.size() += crop.pos();	// get full width and height of crop
	return crop;
}

Rect Rect::getOverlap(const Rect& frame) const {
	Rect rect = *this;
	vec2i rback, fback;
	if (!rect.overlap(frame, rback, fback))
		return Rect(0);

	// crop rect if it's boundaries are out of frame
	if (rect.x < frame.x) {	// left
		rect.w -= frame.x - rect.x;
		rect.x = frame.x;
	}
	if (rback.x > fback.x)	// right
		rect.w -= rback.x - fback.x;
	if (rect.y < frame.y) {	// top
		rect.h -= frame.y - rect.y;
		rect.y = frame.y;
	}
	if (rback.y > fback.y)	// bottom
		rect.h -= rback.y - fback.y;
	return rect;
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
		if (!ca && !cb)
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
	if (sizet ai = 0, bi = 0; pathCompareLoop(as, bs, ai, bi))
		return ai >= as.length() && bi >= bs.length();	// check if both paths have reached their ends simultaneously
	return false;
}

bool isSubpath(const string& path, string parent) {
	if (std::all_of(parent.begin(), parent.end(), [](char c) -> bool { return c == dsep; }))	// always true if parent is root
		return true;

	if (sizet ai = 0, bi = 0; pathCompareLoop(path, parent, ai, bi))
		return bi >= parent.length();	// parent has to have reached it's end while path was still matching
	return false;
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

	if (sizet ai = 0, bi = 0; pathCompareLoop(path, parent, ai, bi) && bi >= parent.length())
		return path.substr(ai);
	return emptyStr;
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
