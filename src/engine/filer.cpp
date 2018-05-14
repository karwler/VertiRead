#include "world.h"
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

// INI LINE

IniLine::IniLine() :
	type(Type::empty)
{}

IniLine::IniLine(const string& ARG, const string& VAL) :
	type(Type::argVal),
	arg(ARG),
	val(VAL)
{}

IniLine::IniLine(const string& ARG, const string& KEY, const string& VAL) :
	type(Type::argKeyVal),
	arg(ARG),
	key(KEY),
	val(VAL)
{}

IniLine::IniLine(const string& TIT) :
	type(Type::title),
	arg(TIT)
{}

string IniLine::line() const {
	if (type == Type::argVal)
		return arg + '=' + val;
	if (type == Type::argKeyVal)
		return arg + '[' + key + "]=" + val;
	if (type == Type::title)
		return '[' + arg + ']';
	return "";
}

void IniLine::setVal(const string& ARG, const string& VAL) {
	type = Type::argVal;
	arg = ARG;
	key.clear();
	val = VAL;
}

void IniLine::setVal(const string& ARG, const string& KEY, const string& VAL) {
	type = Type::argKeyVal;
	arg = ARG;
	key = KEY;
	val = VAL;
}

void IniLine::setTitle(const string& TIT) {
	type = Type::title;
	arg = TIT;
	key.clear();
	val.clear();
}

bool IniLine::setLine(const string& str) {
	// clear line in case the function will return false
	clear();
	if (str.empty())
		return false;

	// check if title
	if (str[0] == '[' && str.back() == ']') {
		arg = str.substr(1, str.length()-2);
		type = Type::title;
		return true;
	}

	// find position of the '=' to split line into argument and value
	sizt i0 = str.find_first_of('=');
	if (i0 == string::npos)
		return false;
	val = str.substr(i0+1);

	// get arg and key if availible
	sizt i1 = str.find_first_of('[');
	sizt i2 = str.find_first_of(']', i1);
	if (i1 < i2 && i2 < i0) {	// if '[' preceeds ']' and both preceed '='
		arg = str.substr(0, i1);
		key = str.substr(i1+1, i2-i1-1);
		type = Type::argKeyVal;
	} else {
		arg = str.substr(0, i0);
		type = Type::argVal;
	}
	return true;
}

void IniLine::clear() {
	type = Type::empty;
	arg.clear();
	val.clear();
	key.clear();
}

// FILER

const string Filer::dirExec = Filer::getDirExec();

#ifdef _WIN32
const string Filer::dirSets = string(std::getenv("AppData")) + "\\"+Default::titleDefault+"\\";
#else
const string Filer::dirSets = string(std::getenv("HOME")) + "/."+Default::titleExtra+"/";
#endif

const string Filer::dirLangs = Filer::dirExec + Default::dirLanguages + dsep;
const string Filer::dirTexs = Filer::dirExec + Default::dirTextures + dsep;

#ifdef _WIN32
const vector<string> Filer::dirFonts = {string(std::getenv("SystemDrive")) + "\\Windows\\Fonts\\"};
#else
const vector<string> Filer::dirFonts = {"/usr/share/fonts/"};
#endif

vector<string> Filer::getAvailibleThemes() {
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines))
		return {};

	vector<string> themes;
	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.getType() == IniLine::Type::title)
			themes.push_back(il.getArg());
	}
	return themes;
}

vector<SDL_Color> Filer::getColors(const string& theme) {
	vector<SDL_Color> colors = {
		Default::colorBackground,
		Default::colorNormal,
		Default::colorDark,
		Default::colorLight,
		Default::colorText
	};
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines), false)
		return colors;

	// find title equal to theme
	IniLine il;
	sizt i = 0;
	while (i < lines.size()) {
		il.setLine(lines[i]);
		if (il.getType() == IniLine::Type::title && il.getArg() == theme)
			break;
		i++;
	}

	// read colors until the end of the file or another title
	while (++i < lines.size()) {
		il.setLine(lines[i]);
		if (il.getType() == IniLine::Type::title)
			break;

		sizt cid = static_cast<sizt>(strToColor(il.getArg()));
		if (cid < colors.size()) {
			vector<vec2t> elems = getWords(il.getVal());
			if (elems.size() > 0)
				colors[cid].r = stoi(il.getVal().substr(elems[0].l, elems[0].u));
			if (elems.size() > 1)
				colors[cid].g = stoi(il.getVal().substr(elems[1].l, elems[1].u));
			if (elems.size() > 2)
				colors[cid].b = stoi(il.getVal().substr(elems[2].l, elems[2].u));
			if (elems.size() > 3)
				colors[cid].a = stoi(il.getVal().substr(elems[3].l, elems[3].u));
		}
	}
	return colors;
}

vector<string> Filer::getAvailibleLanguages() {
	vector<string> files = {Default::language};
	if (fileType(dirLangs) != FTYPE_DIR)
		return {};

	for (string& it : listDir(dirLangs, FTYPE_FILE))
		files.push_back(delExt(it));
	return files;
}

umap<string, string> Filer::getTranslations(const string& language) {
	vector<string> lines;
	if (!readTextFile(dirLangs + language + ".ini", lines, false))
		return umap<string, string>();

	umap<string, string> translation;
	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.getType() != IniLine::Type::title)
			translation.insert(make_pair(il.getArg(), il.getVal()));
	}
	return translation;
}

Playlist Filer::getPlaylist(const string& name) {
	vector<string> lines;
	if (!readTextFile(appendDsep(World::winSys()->sets.getDirPlist()) + name + ".ini", lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.getType() == IniLine::Type::title)
			continue;

		if (il.getArg() == Default::iniKeywordBook)
			plist.books.insert(il.getVal());
		else if (il.getArg() == Default::iniKeywordSong)
			plist.songs.push_back(il.getVal());	// AudioSys will check if the song is playable cause we want all lines for the playlist editor
	}
	return plist;
}

void Filer::savePlaylist(const Playlist& plist) {
	vector<string> lines;
	for (const string& name : plist.books)
		lines.push_back(IniLine(Default::iniKeywordBook, name).line());
	for (const string& file : plist.songs)
		lines.push_back(IniLine(Default::iniKeywordSong, file).line());
	writeTextFile(appendDsep(World::winSys()->sets.getDirPlist()) + plist.name + ".ini", lines);
}

string Filer::getLastPage(const string& book) {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileLastPages, lines, false))
		return "";

	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.getType() != IniLine::Type::title && il.getArg() == book)
			return il.getVal();
	}
	return "";
}

void Filer::saveLastPage(const string& file) {
	vector<string> lines;
	readTextFile(dirSets + Default::fileLastPages, lines, false);

	// find line to replace or push back
	sizt id = lines.size();
	string book = getBook(file);
	for (sizt i=0; i!=lines.size(); i++) {
		IniLine il;
		if (il.setLine(lines[i]) && il.getType() != IniLine::Type::title && il.getArg() == book) {
			id = i;
			break;
		}
	}
	if (id == lines.size())
		lines.push_back(IniLine(book, file).line());
	else
		lines[id] = IniLine(book, file).line();

	writeTextFile(dirSets + Default::fileLastPages, lines);
}

Settings Filer::getSettings() {
	Settings sets;
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileSettings, lines, false))
		return sets;

	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.getType() == IniLine::Type::title)
			continue;

		if (il.getArg() == Default::iniKeywordMaximized)
			sets.maximized = stob(il.getVal());
		else if (il.getArg() == Default::iniKeywordFullscreen)
			sets.fullscreen = stob(il.getVal());
		else if (il.getArg() == Default::iniKeywordResolution)
			sets.setResolution(il.getVal());
		else if (il.getArg() == Default::iniKeywordFont)
			sets.setFont(il.getVal());
		else if (il.getArg() == Default::iniKeywordLanguage)
			sets.setLang(il.getVal());
		else if (il.getArg() == Default::iniKeywordTheme)
			sets.setTheme(il.getVal());
		else if (il.getArg() == Default::iniKeywordLibrary)
			sets.setDirLib(il.getVal());
		else if (il.getArg() == Default::iniKeywordPlaylists)
			sets.setDirPlist(il.getVal());
		else if (il.getArg() == Default::iniKeywordVolume)
			sets.setVolume(stoi(il.getVal()));
		else if (il.getArg() == Default::iniKeywordRenderer)
			sets.renderer = il.getVal();
		else if (il.getArg() == Default::iniKeywordScrollSpeed)
			sets.setScrollSpeed(il.getVal());
		else if (il.getArg() == Default::iniKeywordDeadzone)
			sets.setDeadzone(stoi(il.getVal()));
	}
	return sets;
}

void Filer::saveSettings(const Settings& sets) {
	vector<string> lines = {
		IniLine(Default::iniKeywordMaximized, btos(sets.maximized)).line(),
		IniLine(Default::iniKeywordFullscreen, btos(sets.fullscreen)).line(),
		IniLine(Default::iniKeywordResolution, sets.getResolutionString()).line(),
		IniLine(Default::iniKeywordFont, sets.getFont()).line(),
		IniLine(Default::iniKeywordLanguage, sets.getLang()).line(),
		IniLine(Default::iniKeywordTheme, sets.getTheme()).line(),
		IniLine(Default::iniKeywordLibrary, sets.getDirLib()).line(),
		IniLine(Default::iniKeywordPlaylists, sets.getDirPlist()).line(),
		IniLine(Default::iniKeywordVolume, ntos(sets.getVolume())).line(),
		IniLine(Default::iniKeywordRenderer, sets.renderer).line(),
		IniLine(Default::iniKeywordScrollSpeed, sets.getScrollSpeedString()).line(),
		IniLine(Default::iniKeywordDeadzone, ntos(sets.getDeadzone())).line()
	};
	writeTextFile(dirSets + Default::fileSettings, lines);
}

vector<Binding> Filer::getBindings() {
	vector<Binding> bindings(static_cast<sizt>(Binding::Type::numBindings));
	for (sizt i=0; i<bindings.size(); i++)
		bindings[i].setDefaultSelf(static_cast<Binding::Type>(i));

	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileBindings, lines, false))
		return bindings;

	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.getType() == IniLine::Type::title || il.getVal().size() < 3)
			continue;

		sizt bid = static_cast<sizt>(strToBindingType(il.getArg()));
		switch (toupper(il.getVal()[0])) {
		case 'K':	// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(il.getVal().substr(2).c_str()));
			break;
		case 'B':	// joystick button
			bindings[bid].setJbutton(stoi(il.getVal().substr(2)));
			break;
		case 'H':	// joystick hat
			for (sizt i=2; i<il.getVal().size(); i++)
				if (il.getVal()[i] < '0' || il.getVal()[i] > '9') {
					bindings[bid].setJhat(stoi(il.getVal().substr(2, i-2)), jtStrToHat(il.getVal().substr(i+1)));
					break;
				}
			break;
		case 'A':	// joystick axis
			bindings[bid].setJaxis(stoi(il.getVal().substr(3)), (il.getVal()[2] != '-'));
			break;
		case 'G':	// gamepad button
			bindings[bid].setGbutton(gpStrToButton(il.getVal().substr(2)));
			break;
		case 'X':	// gamepad axis
			bindings[bid].setGaxis(gpStrToAxis(il.getVal().substr(3)), (il.getVal()[2] != '-'));
		}
	}
	return bindings;
}

void Filer::saveBindings(const vector<Binding>& bindings) {
	vector<string> lines;
	for (sizt i=0; i<bindings.size(); i++) {
		string name = bindingTypeToStr(static_cast<Binding::Type>(i));
		if (bindings[i].keyAssigned())
			lines.push_back(IniLine(name, "K_" + string(SDL_GetScancodeName(bindings[i].getKey()))).line());

		if (bindings[i].jbuttonAssigned())
			lines.push_back(IniLine(name, "B_" + ntos(bindings[i].getJctID())).line());
		else if (bindings[i].jhatAssigned())
			lines.push_back(IniLine(name, "H_" + ntos(bindings[i].getJctID()) + "_" + jtHatToStr(bindings[i].getJhatVal())).line());
		else if (bindings[i].jaxisAssigned())
			lines.push_back(IniLine(name, string(bindings[i].jposAxisAssigned() ? "A_+" : "A_-") + ntos(bindings[i].getJctID())).line());

		if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine(name, "G_" + gpButtonToStr(bindings[i].getGctID())).line());
		else if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine(name, string(bindings[i].gposAxisAssigned() ? "X_+" : "X_-") + gpAxisToStr(bindings[i].getGctID())).line());
	}
	writeTextFile(dirSets + Default::fileBindings, lines);
}

bool Filer::readTextFile(const string& file, vector<string>& lines, bool printMessage) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		if (printMessage)
			cerr << "couldn't open file " << file << endl;
		return false;
	}
	lines.clear();

	for (string line; readLine(ifs, line);)
		if (!line.empty())		// skip empty lines
			lines.push_back(line);
	return true;
}

bool Filer::writeTextFile(const string& file, const vector<string>& lines) {
	std::ofstream ofs(file.c_str());
	if (!ofs.good()) {
		cerr << "couldn't write file " << file << endl;
		return false;
	}
	for (const string& line : lines)
		ofs << line << endl;
	return true;
}

bool Filer::mkDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

vector<string> Filer::listDir(const string& dir, FileType filter) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(dir+"*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))	// ignore . and ..
			if ((!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (filter & FTYPE_FILE)) || ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (filter & FTYPE_DIR)))
				entries.push_back(wtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(dir.c_str());
	if (!directory)
		return entries;

	while (dirent* data = readdir(directory))
		if (strcmp(data->d_name, ".") && strcmp(data->d_name, ".."))	// ignore . and ..
			if ((data->d_type != DT_DIR && (filter & FTYPE_FILE)) || (data->d_type == DT_DIR && (filter & FTYPE_DIR)))
				entries.push_back(data->d_name);
	closedir(directory);
#endif
	std::sort(entries.begin(), entries.end());
	return entries;
}

vector<string> Filer::listDirRecursively(string dir) {
	dir = appendDsep(dir);
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(dir+"*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (!(wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")))	// ignore . and ..
			continue;

		string name = wtos(data.cFileName);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	// append subdirectoy's files to entries
			vector<string> newEs = listDirRecursively(dir + name);
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		} else
			entries.push_back(dir + name);
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(dir.c_str());
	if (!directory)
		return entries;

	while (dirent* data = readdir(directory)) {
		if (!(strcmp(data->d_name, ".") && strcmp(data->d_name, "..")))	// ignore . and ..
			continue;

		if (data->d_type == DT_DIR) {	// append subdirectoy's files to entries
			vector<string> newEs = listDirRecursively(dir + data->d_name);
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		} else
			entries.push_back(dir + data->d_name);
	}
	closedir(directory);
#endif
	return entries;
}

FileType Filer::fileType(const string& path) {
#ifdef _WIN32
	DWORD attrib = GetFileAttributesW(stow(path).c_str());
	if (attrib == INVALID_FILE_ATTRIBUTES)
		return FTYPE_NONE;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
		return FTYPE_DIR;
#else
	struct stat ps;
	if (stat(path.c_str(), &ps))
		return FTYPE_NONE;
	if (S_ISDIR(ps.st_mode))
		return FTYPE_DIR;
#endif
	return FTYPE_FILE;
}

bool Filer::rename(const string& path, const string& newPath) {
	return !std::rename(path.c_str(), newPath.c_str());
}

bool Filer::remove(const string& path) {
	if (fileType(path) == FTYPE_DIR)
		for (string& it : listDir(path))
			remove(appendDsep(path) + it);
	return !std::remove(path.c_str());
}

#ifdef _WIN32
vector<char> Filer::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();
	
	for (char i=0; i<26; i++)
		if (drives & (1 << i))
			letters.push_back('A'+i);
	return letters;
}
#endif

string Filer::getDirExec() {
	string path;
#ifdef _WIN32
	wchar buffer[MAX_PATH];
	GetModuleFileNameW(0, buffer, MAX_PATH);
	path = wtos(buffer);
#else
	char buffer[PATH_MAX];
	int len = readlink("/proc/self/exe", buffer, PATH_MAX-1);
	buffer[len] = '\0';
	path = buffer;
#endif
	return parentPath(path);
}

string Filer::findFont(const string& font) {	
	if (isAbsolute(font) && fileType(font) == FTYPE_FILE)	// check if font refers to a file
		return font;

	for (const string& dir : dirFonts)	// check font directories
		for (string& it : listDirRecursively(dir))
			if (strcmpCI(hasExt(it) ? delExt(filename(it)) : filename(it), font))
				return it;
	return "";	// nothing found
}

std::istream& Filer::readLine(std::istream& ifs, string& str) {
	str.clear();
	std::istream::sentry se(ifs, true);
	std::streambuf* sbf = ifs.rdbuf();

	while (true) {
		int c = sbf->sbumpc();
		switch (c) {
		case '\n':
			return ifs;
		case '\r':
			if (sbf->sgetc() == '\n')
				sbf->sbumpc();
			return ifs;
		case EOF:
			if (str.empty())
				ifs.setstate(std::ios::eofbit);
			return ifs;
		default:
			str += char(c);
		}
	}
}
