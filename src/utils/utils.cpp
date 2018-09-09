#include "engine/world.h"
#include <locale>
#include <codecvt>
#ifndef _WIN32
#include <strings.h>
#endif

int strcicmp(const string& a, const string& b) {
#ifdef _WIN32
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

static int natCompareRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		if (!isDigit(*a)  &&  !isDigit(*b))
			return bias;
		else if (!isDigit(*a))
			return -1;
		else if (!isDigit(*b))
			return +1;
		else if (*a < *b) {
			if (!bias)
				bias = -1;
		} else if (*a > *b) {
			if (!bias)
				bias = +1;
		} else if (!*a  &&  !*b)
			return bias;
	}
	return 0;
}

static int natCompareLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		if (!isDigit(*a)  &&  !isDigit(*b))
			return 0;
		else if (!isDigit(*a))
			return -1;
		else if (!isDigit(*b))
			return +1;
		else if (*a < *b)
			return -1;
		else if (*a > *b)
			return +1;
	}
	return 0;
}

int strnatcmp(const char* a, const char* b) {
	for (;; a++, b++) {
		char ca = *a;
		char cb = *b;
		while (isSpace(ca))
			ca = *++a;
		while (isSpace(cb))
			cb = *++b;

		if (isDigit(ca) && isDigit(cb)) {
			if (ca == '0' || cb == '0') {
				if (int result = natCompareLeft(a, b))
					return result;
			} else {
				if (int result = natCompareRight(a, b))
					return result;
			}
		}

		if (!ca && !cb)
			return 0;
		if (ca < cb)
			return -1;
		if (ca > cb)
			return +1;
	}
}

string trim(const string& str) {
	sizt pos = 0;
	while (isSpace(str[pos]) && pos < str.length())
		pos++;
	if (pos == str.length())
		return "";

	sizt end = str.length();
	while (isSpace(str[--end]) && end < str.length());
	return str.substr(pos, end - pos + 1);
}

string ltrim(const string& str) {
	sizt pos = 0;
	while (isSpace(str[pos]) && pos < str.length())
		pos++;
	return str.substr(pos);
}

string rtrim(const string& str) {
	if (str.empty())
		return "";

	sizt end = str.length();
	while (isSpace(str[--end]) && end < str.length());
	return str.substr(0, end + 1);
}

static bool pathCompareLoop(const string& as, const string& bs, sizt& ai, sizt& bi) {
	do {
		// comparee names of next entry
		sizt an = as.find_first_of(dsep, ai);
		sizt bn = bs.find_first_of(dsep, bi);
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
	sizt ai = 0, bi = 0;
	if (pathCompareLoop(as, bs, ai, bi))
		return ai >= as.length() && bi >= bs.length();	// check if both paths have reached their ends simultaneously
	return false;
}

bool isSubpath(const string& path, string parent) {
	if (strchk(parent, [](char c) -> bool { return c == dsep; }))	// always true if parent is root
		return true;

	sizt ai = 0, bi = 0;
	if (pathCompareLoop(path, parent, ai, bi))
		return bi >= parent.length();	// parent has to have reached it's end while path was still matching
	return false;
}

string parentPath(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path))
		return dseps;
#endif
	sizt end = path.find_last_not_of(dsep);		// skip initial separators
	if (end == string::npos)					// if the entire path is separators, return root
		return dseps;

	sizt pos = path.find_last_of(dsep, end);	// skip to separators between parent and child
	if (pos == string::npos)					// if there are none, just cut off child
		return path.substr(0, end + 1);

	pos = path.find_last_not_of(dsep, pos);		// skip separators to get to the parent entry
	return pos == string::npos ? dseps : path.substr(0, pos + 1);	// cut off child
}

string childPath(const string& parent, const string& child) {
#ifdef _WIN32
	return strchk(parent, [](char c) -> bool { return c == dsep; }) && isDriveLetter(child) ? child : appendDsep(parent) + child;
#else
	return appendDsep(parent) + child;
#endif
}

string getChild(const string& path, const string& parent) {
	if (strchk(parent, [](char c) -> bool { return c == dsep; }))
		return path;

	sizt ai = 0, bi = 0;
	if (pathCompareLoop(path, parent, ai, bi) && bi >= parent.length())
		return path.substr(ai);
	return "";
}

string filename(const string& path) {
	if (path.empty() || path == dseps)
		return "";

	sizt end = path.back() == dsep ? path.length()-1 : path.length();
	sizt pos = path.find_last_of(dsep, end-1);
	return pos == string::npos ? path.substr(0, end) : path.substr(pos+1, end-pos-1);
}

string getExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return path.substr(i+1);
		if (path[i] == dsep)
			return "";
	}
	return "";
}

bool hasExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return true;
		if (path[i] == dsep)
			return false;
	}
	return false;
}

string delExt(const string& path) {
	for (sizt i = path.length()-1; i < path.length(); i--) {
		if (path[i] == '.')
			return path.substr(0, i);
		if (path[i] == dsep)
			return path;
	}
	return path;
}

string appendDsep(const string& path) {
	return path.size() && path.back() == dsep ? path : path + dsep;
}
#ifdef _WIN32
bool isDriveLetter(const string& path) {
	if (path.length() >= 2 && isDriveLetter(path[0]) && path[1] == ':') {
		for (sizt i = 2; i < path.length(); i++)
			if (path[i] != dsep)
				return false;
		return true;
	}
	return false;
}
#endif
bool strchk(const string& str, bool (*cmp)(char)) {
	for (sizt i = 0; i < str.length(); i++)
		if (!cmp(str[i]))
			return false;
	return true;
}

bool strchk(const string& str, sizt pos, sizt len, bool (*cmp)(char)) {
	bringUnder(len, str.length());
	while (pos < len)
		if (!cmp(str[pos++]))
			return false;
	return true;
}

vector<string> getWords(const string& line) {
	sizt i = 0;
	while (isSpace(line[i]))
		i++;

	sizt start = i;
	vector<string> words;
	while (i < line.length()) {
		if (isSpace(line[i])) {
			words.push_back(line.substr(start, i - start));
			while (isSpace(line[++i]) && i < line.length());
			start = i;
		} else
			i++;
	}
	if (i > start)
		words.push_back(line.substr(start, i - start));
	return words;
}

string strEnclose(string str) {
	for (string::iterator it = str.begin(); it != str.end(); it++)
		if (*it == '"')
			str.insert(it++, '\\');
	return '"' + str + '"';
}

vector<string> strUnenclose(const string& str) {
	vector<string> words;
	for (sizt pos = 0;;) {
		// find next start
		pos = str.find_first_of('"', pos);
		if (pos < str.length())
			pos++;
		else
			break;

		// find start's end
		sizt end = pos;
		while (end < str.length()) {
			end = str.find_first_of('"', end);
			if (str[end-1] == '\\')
				end++;
			else
				break;
		}

		// remove escapes and add to quote list
		if (end < str.length()) {
			string quote = str.substr(pos, end - pos);
			for (sizt i = 0; i < quote.length(); i++)
				if (quote[i] == '\\' && quote[i+1] == '"')
					quote.erase(i, 1);
			words.push_back(quote);
			pos = end + 1;
		} else
			break;
	}
	return words;
}

archive* openArchive(const string& file) {
	archive* arch = archive_read_new();
	archive_read_support_filter_all(arch);
	archive_read_support_format_all(arch);

	if (archive_read_open_filename(arch, file.c_str(), Default::archiveReadBlockSize)) {
		archive_read_free(arch);
		return nullptr;
	}
	return arch;
}

SDL_RWops* readArchiveEntry(archive* arch, archive_entry* entry) {
	la_int64_t bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	void* buffer = new uint8[bsiz];
	la_ssize_t size = archive_read_data(arch, buffer, bsiz);
	return size < 0 ? nullptr : SDL_RWFromMem(buffer, size);
}

SDL_Rect cropRect(SDL_Rect& rect, const SDL_Rect& frame) {
	if (rect.w <= 0 || rect.h <= 0 || frame.w <= 0 || frame.h <= 0)	// idfk
		return rect = {0, 0, 0, 0};

	// ends of each rect and frame
	vec2i rend = rectEnd(rect);
	vec2i fend = rectEnd(frame);
	if (rect.x > fend.x || rect.y > fend.y || rend.x < frame.x || rend.y < frame.y)	// if rect is out of frame
		return rect = {0, 0, 0, 0};

	// crop rect if it's boundaries are out of frame
	SDL_Rect crop = {0, 0, 0, 0};
	if (rect.x < frame.x) {	// left
		crop.x = frame.x - rect.x;
		rect.x = frame.x;
		rect.w -= crop.x;
	}
	if (rend.x > fend.x) {	// right
		crop.w = rend.x - fend.x;
		rect.w -= crop.w;
	}
	if (rect.y < frame.y) {	// top
		crop.y = frame.y - rect.y;
		rect.y = frame.y;
		rect.h -= crop.y;
	}
	if (rend.y > fend.y) {	// bottom
		crop.h = rend.y - fend.y;
		rect.h -= crop.h;
	}
	// get full width and height of crop
	crop.w += crop.x;
	crop.h += crop.y;
	return crop;
}

SDL_Rect overlapRect(SDL_Rect rect, const SDL_Rect& frame) {
	if (rect.w <= 0 || rect.h <= 0 || frame.w <= 0 || frame.h <= 0)		// idfk
		return {0, 0, 0, 0};

	// ends of both rects
	vec2i rend = rectEnd(rect);
	vec2i fend = rectEnd(frame);
	if (rect.x > fend.x || rect.y > fend.y || rend.x < frame.x || rend.y < frame.y)	// if they don't overlap
		return {0, 0, 0, 0};

	// crop rect if it's boundaries are out of frame
	if (rect.x < frame.x) {	// left
		rect.w -= frame.x - rect.x;
		rect.x = frame.x;
	}
	if (rend.x > fend.x)	// right
		rect.w -= rend.x - fend.x;
	if (rect.y < frame.y) {	// top
		rect.h -= frame.y - rect.y;
		rect.y = frame.y;
	}
	if (rend.y > fend.y)	// bottom
		rect.h -= rend.y - fend.y;
	return rect;
}

string wtos(const wstring& wstr) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().to_bytes(wstr);
}

wstring stow(const string& str) {
	return std::wstring_convert<std::codecvt_utf8<wchar>, wchar>().from_bytes(str);
}

string jtHatToStr(uint8 jhat) {
	return Default::hatNames.count(jhat) ? Default::hatNames.at(jhat) : "invalid";
}

uint8 jtStrToHat(const string& str) {
	for (const pair<uint8, string>& it : Default::hatNames)
		if (!strcicmp(it.second, str))
			return it.first;
	return 0x10;
}
