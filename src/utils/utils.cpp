#include "engine/world.h"
#ifdef _WIN32
#include <windows.h>
#endif

vector<string> getAvailibleRenderers() {
	vector<string> renderers((sizt(SDL_GetNumRenderDrivers())));
	for (sizt i = 0; i < renderers.size(); i++)
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
		return *this = Rect();

	Rect crop;	// crop rect if it's boundaries are out of frame
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
	vec2i rend, fend;
	if (!rect.overlap(frame, rend, fend))
		return Rect();

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

static int natCompareRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		if (bool nad = !isDigit(*a), nbd = !isDigit(*b); nad && nbd)
			return bias;
		else if (nad)
			return -1;
		else if (nbd)
			return 1;
		else if (*a < *b) {
			if (!bias)
				bias = -1;
		} else if (*a > *b) {
			if (!bias)
				bias = 1;
		} else if (!(*a || *b))
			return bias;
	}
}

static int natCompareLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		if (bool nad = !isDigit(*a), nbd = !isDigit(*b); nad && nbd)
			return 0;
		else if (nad)
			return -1;
		else if (nbd)
			return 1;
		else if (*a < *b)
			return -1;
		else if (*a > *b)
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

string trim(const string& str) {
	sizt pos = 0;
	for (; isSpace(str[pos]); pos++);
	if (pos == str.length())
		return "";

	sizt end = str.length();
	while (--end < str.length() && isSpace(str[end]));
	return str.substr(pos, end - pos + 1);
}

string trimZero(const string& str) {
	sizt id = str.find_last_not_of('0');
	return str.substr(0, str[id] == '.' ? id : id + 1);
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
	if (sizt ai = 0, bi = 0; pathCompareLoop(as, bs, ai, bi))
		return ai >= as.length() && bi >= bs.length();	// check if both paths have reached their ends simultaneously
	return false;
}

bool isSubpath(const string& path, string parent) {
	if (strchk(parent, [](char c) -> bool { return c == dsep; }))	// always true if parent is root
		return true;

	if (sizt ai = 0, bi = 0; pathCompareLoop(path, parent, ai, bi))
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

	if (sizt ai = 0, bi = 0; pathCompareLoop(path, parent, ai, bi) && bi >= parent.length())
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

SDL_Color getColor(const string& line) {
	SDL_Color color = {0, 0, 0, 255};
	const char* pos = line.c_str();
	for (; isSpace(*pos); pos++);

	if (*pos == '#') {
		while (*++pos == '#');
		char* end;
		if (uint32 num = uint32(strtoul(pos, &end, 16)); end != pos) {
			if (uint32 mov = 32 - uint32(end - pos) * 4)
				num = (num << mov) + 255;
			*reinterpret_cast<uint32*>(&color) = num;
		}
	} else for (uint i = 0; i < 4 && *pos;) {
		char* end;
		if (uint8 num = uint8(strtoul(pos, &end, 0)); end != pos) {
			reinterpret_cast<uint8*>(&color)[i++] = num;
			for (pos = end; isSpace(*pos); pos++);
		} else
			pos++;
	}
	return color;
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
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	uint8* buffer = new uint8[sizt(bsiz)];
	int64 size = archive_read_data(arch, buffer, sizt(bsiz));
	return size < 0 ? nullptr : SDL_RWFromMem(buffer, int(size));
}
#ifdef _WIN32
string wtos(const wstring& src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length() + 1), nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return "";

	string dst(len - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), int(src.length() + 1), dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length() + 1), nullptr, 0);
	if (len <= 1)
		return L"";

	wstring dst(len - 1, '\0');
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length() + 1), dst.data(), len);
	return dst;
}
#endif
uint8 jtStrToHat(const string& str) {
	for (const pair<uint8, string>& it : Default::hatNames)
		if (!strcicmp(it.second, str))
			return it.first;
	return 0;
}
