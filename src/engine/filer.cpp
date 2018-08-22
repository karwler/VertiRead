#include "world.h"
#include <SDL2/SDL_image.h>
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <iostream>
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

IniLine::IniLine(const string& line) {
	setLine(line);
}

string IniLine::line() const {
	if (type == Type::argVal)
		return line(arg, val);
	if (type == Type::argKeyVal)
		return line(arg, key, val);
	if (type == Type::title)
		return line(arg);
	return "";
}

string IniLine::line(const string& title) {
	return '[' + title + ']';
}

string IniLine::line(const string& arg, const string& val) {
	return arg + '=' + val;
}

string IniLine::line(const string& arg, const string& key, const string& val) {
	return arg + '[' + key + "]=" + val;
}

void IniLine::setVal(const string& property, const string& value) {
	type = Type::argVal;
	arg = property;
	key.clear();
	val = value;
}

void IniLine::setVal(const string& property, const string& vkey, const string& value) {
	type = Type::argKeyVal;
	arg = property;
	key = vkey;
	val = value;
}

void IniLine::setTitle(const string& title) {
	type = Type::title;
	arg = title;
	key.clear();
	val.clear();
}

IniLine::Type IniLine::setLine(const string& str) {
	if (str.empty()) {
		clear();
		return type;
	}

	// check if title
	if (str[0] == '[' && str.back() == ']') {
		arg = str.substr(1, str.length()-2);
		key.clear();
		val.clear();
		return type = Type::title;
	}

	// find position of the '=' to split line into argument and value
	sizt i0 = str.find_first_of('=');
	if (i0 == string::npos) {
		clear();
		return type;
	}
	val = str.substr(i0+1);

	// get arg and key if availible
	sizt i1 = str.find_first_of('[');
	sizt i2 = str.find_first_of(']', i1);
	if (i1 < i2 && i2 < i0) {	// if '[' preceeds ']' and both preceed '='
		arg = str.substr(0, i1);
		key = str.substr(i1+1, i2-i1-1);
		return type = Type::argKeyVal;
	}
	arg = str.substr(0, i0);
	key.clear();
	return type = Type::argVal;
}

void IniLine::clear() {
	type = Type::empty;
	arg.clear();
	val.clear();
	key.clear();
}

// FILER

const string Filer::dirExec = appendDsep(Filer::getExecDir());
#ifdef _WIN32
const vector<string> Filer::dirFonts = {Filer::dirExec, wgetenv("SystemDrive") + "\\Windows\\Fonts\\"};
const string Filer::dirSets = wgetenv("AppData") + "\\" + Default::titleDefault + "\\";
#else
const vector<string> Filer::dirFonts = {Filer::dirExec, "/usr/share/fonts/", string(std::getenv("HOME")) + "/.fonts/"};
const string Filer::dirSets = string(std::getenv("HOME")) + "/." + Default::titleExtra + "/";
#endif
const string Filer::dirLangs = Filer::dirExec + Default::dirLanguages + dsep;
const string Filer::dirTexs = Filer::dirExec + Default::dirTextures + dsep;

vector<string> Filer::getAvailibleThemes() {
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines))
		return {};

	vector<string> themes;
	for (string& line : lines) {
		IniLine il(line);
		if (il.getType() == IniLine::Type::title)
			themes.push_back(il.getArg());
	}
	return themes;
}

vector<SDL_Color> Filer::getColors(const string& theme) {
	vector<SDL_Color> colors = Default::colors;
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines))
		return colors;

	// find title equal to theme
	IniLine il;
	sizt i = 0;
	while (i < lines.size()) {
		if (il.setLine(lines[i]) == IniLine::Type::title && il.getArg() == theme)
			break;
		i++;
	}

	// read colors until the end of the file or another title
	while (++i < lines.size()) {
		if (il.setLine(lines[i]) == IniLine::Type::title)
			break;
		if (il.getType() != IniLine::Type::argVal)
			continue;

		sizt cid = strToEnum<sizt>(Default::colorNames, il.getArg());
		if (cid < colors.size()) {
			vector<string> elems = getWords(il.getVal());
			for (uint8 i = 0; i < elems.size() && i < 4; i++)
				reinterpret_cast<uint8*>(&colors[cid])[i] = stoi(elems[i]);
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
		IniLine il(line);
		if (il.getType() == IniLine::Type::argVal)
			translation.insert(make_pair(il.getArg(), il.getVal()));
	}
	return translation;
}

string Filer::getLastPage(const string& book) {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileBooks, lines, false))
		return "";

	for (string& line : lines) {
		IniLine il(line);
		if (il.getType() == IniLine::Type::argVal && il.getArg() == book)
			return il.getVal();
	}
	return "";
}

void Filer::saveLastPage(const string& file) {
	vector<string> lines;
	readTextFile(dirSets + Default::fileBooks, lines, false);

	sizt id = lines.size();
	string book = getBook(file);
	for (sizt i = 0; i < lines.size(); i++) {
		IniLine il(lines[i]);
		if (il.getType() == IniLine::Type::argVal && il.getArg() == book) {
			id = i;
			break;
		}
	}
	if (id == lines.size())
		lines.push_back(IniLine::line(book, file));
	else
		lines[id] = IniLine::line(book, file);
	writeTextFile(dirSets + Default::fileBooks, lines);
}

Settings Filer::getSettings() {
	Settings sets;
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileSettings, lines, false))
		return sets;

	for (string& line : lines) {
		IniLine il(line);
		if (il.getType() != IniLine::Type::argVal)
			continue;

		if (il.getArg() == Default::iniKeywordMaximized)
			sets.maximized = stob(il.getVal());
		else if (il.getArg() == Default::iniKeywordFullscreen)
			sets.fullscreen = stob(il.getVal());
		else if (il.getArg() == Default::iniKeywordResolution)
			sets.setResolution(il.getVal());
		else if (il.getArg() == Default::iniKeywordDirection)
			sets.direction.set(il.getVal());
		else if (il.getArg() == Default::iniKeywordZoom)
			sets.zoom = stof(il.getVal());
		else if (il.getArg() == Default::iniKeywordSpacing)
			sets.spacing = stoi(il.getVal());
		else if (il.getArg() == Default::iniKeywordFont)
			sets.setFont(il.getVal());
		else if (il.getArg() == Default::iniKeywordLanguage)
			sets.setLang(il.getVal());
		else if (il.getArg() == Default::iniKeywordTheme)
			sets.setTheme(il.getVal());
		else if (il.getArg() == Default::iniKeywordLibrary)
			sets.setDirLib(il.getVal());
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
		IniLine::line(Default::iniKeywordMaximized, btos(sets.maximized)),
		IniLine::line(Default::iniKeywordFullscreen, btos(sets.fullscreen)),
		IniLine::line(Default::iniKeywordResolution, sets.getResolutionString()),
		IniLine::line(Default::iniKeywordDirection, sets.direction.toString()),
		IniLine::line(Default::iniKeywordZoom, ntos(sets.zoom)),
		IniLine::line(Default::iniKeywordSpacing, ntos(sets.spacing)),
		IniLine::line(Default::iniKeywordFont, sets.getFont()),
		IniLine::line(Default::iniKeywordLanguage, sets.getLang()),
		IniLine::line(Default::iniKeywordTheme, sets.getTheme()),
		IniLine::line(Default::iniKeywordLibrary, sets.getDirLib()),
		IniLine::line(Default::iniKeywordRenderer, sets.renderer),
		IniLine::line(Default::iniKeywordScrollSpeed, sets.getScrollSpeedString()),
		IniLine::line(Default::iniKeywordDeadzone, ntos(sets.getDeadzone()))
	};
	writeTextFile(dirSets + Default::fileSettings, lines);
}

vector<Binding> Filer::getBindings() {
	vector<Binding> bindings(static_cast<sizt>(Binding::Type::numBindings));
	for (sizt i = 0; i < bindings.size(); i++)
		bindings[i].setDefaultSelf(static_cast<Binding::Type>(i));
	
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileBindings, lines, false))
		return bindings;

	for (string& line : lines) {
		IniLine il(line);
		if (il.getType() != IniLine::Type::argVal || il.getVal().length() < 3)
			continue;

		sizt bid = strToEnum<sizt>(Default::bindingNames, il.getArg());
		if (bid >= bindings.size())
			continue;

		switch (toupper(il.getVal()[0])) {
		case 'K':	// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(il.getVal().substr(2).c_str()));
			break;
		case 'B':	// joystick button
			bindings[bid].setJbutton(stoi(il.getVal().substr(2)));
			break;
		case 'H':	// joystick hat
			for (sizt i = 2; i < il.getVal().length(); i++)
				if (il.getVal()[i] < '0' || il.getVal()[i] > '9') {
					bindings[bid].setJhat(stoi(il.getVal().substr(2, i-2)), jtStrToHat(il.getVal().substr(i+1)));
					break;
				}
			break;
		case 'A':	// joystick axis
			bindings[bid].setJaxis(stoi(il.getVal().substr(3)), (il.getVal()[2] != '-'));
			break;
		case 'G': {	// gamepad button
			SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Default::gbuttonNames, il.getVal().substr(2));
			if (cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break; }
		case 'X': {	// gamepad axis
			SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Default::gaxisNames, il.getVal().substr(3));
			if (cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (il.getVal()[2] != '-'));
		} }
	}
	return bindings;
}

void Filer::saveBindings(const vector<Binding>& bindings) {
	vector<string> lines;
	for (sizt i = 0; i < bindings.size(); i++) {
		string name = enumToStr(Default::bindingNames, i);
		if (bindings[i].keyAssigned())
			lines.push_back(IniLine::line(name, "K_" + string(SDL_GetScancodeName(bindings[i].getKey()))));

		if (bindings[i].jbuttonAssigned())
			lines.push_back(IniLine::line(name, "B_" + ntos(bindings[i].getJctID())));
		else if (bindings[i].jhatAssigned())
			lines.push_back(IniLine::line(name, "H_" + ntos(bindings[i].getJctID()) + "_" + jtHatToStr(bindings[i].getJhatVal())));
		else if (bindings[i].jaxisAssigned())
			lines.push_back(IniLine::line(name, string(bindings[i].jposAxisAssigned() ? "A_+" : "A_-") + ntos(bindings[i].getJctID())));

		if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine::line(name, "G_" + enumToStr(Default::gbuttonNames, bindings[i].getGbutton())));
		else if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine::line(name, string(bindings[i].gposAxisAssigned() ? "X_+" : "X_-") + enumToStr(Default::gaxisNames, bindings[i].getGaxis())));
	}
	writeTextFile(dirSets + Default::fileBindings, lines);
}

bool Filer::readTextFile(const string& file, vector<string>& lines, bool printMessage) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		if (printMessage)
			std::cerr << "Couldn't open file " << file << std::endl;
		return false;
	}
	lines.clear();

	for (string line; readLine(ifs, line);)
		if (line.size())		// skip empty lines
			lines.push_back(line);
	return true;
}

bool Filer::writeTextFile(const string& file, const vector<string>& lines) {
	std::ofstream ofs(file.c_str());
	if (!ofs.good()) {
		std::cerr << "Couldn't write file " << file << std::endl;
		return false;
	}
	for (const string& line : lines)
		ofs << line << std::endl;
	return true;
}

bool Filer::createDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), 0775);
#endif
}

vector<string> Filer::listDir(const string& drc, FileType filter) {
	vector<string> entries;
#ifdef _WIN32
	if (drc == "\\") {	// if in "root" directory, get drive letters and present them as directories
		vector<char> letters = listDrives();
		entries.resize(letters.size());
		for (sizt i = 0; i < entries.size(); i++)
			entries[i] = letters[i] + string(":");
		return entries;
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(appendDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))	// ignore . and ..
			if ((!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (filter & FTYPE_FILE)) || ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (filter & FTYPE_DIR)))
				entries.push_back(wtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return entries;

	while (dirent* data = readdir(directory))
		if (strcmp(data->d_name, ".") && strcmp(data->d_name, ".."))	// ignore . and ..
			if ((data->d_type != DT_DIR && (filter & FTYPE_FILE)) || (data->d_type == DT_DIR && (filter & FTYPE_DIR)))
				entries.push_back(data->d_name);
	closedir(directory);
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}

vector<string> Filer::listDirRecursively(string drc) {
	drc = appendDsep(drc);
	vector<string> entries;
#ifdef _WIN32
	if (drc == "\\") {	// if in "root" directory, get drive letters and present them as directories
		for (char c : listDrives()) {
			vector<string> newEs = listDirRecursively(c + string(":"));
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		}
		return entries;
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(drc + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (!(wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")))	// ignore . and ..
			continue;

		string name = wtos(data.cFileName);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	// append subdirectoy's files to entries
			vector<string> newEs = listDirRecursively(drc + name);
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		} else
			entries.push_back(drc + name);
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return entries;

	while (dirent* data = readdir(directory)) {
		if (!(strcmp(data->d_name, ".") && strcmp(data->d_name, "..")))	// ignore . and ..
			continue;

		if (data->d_type == DT_DIR) {	// append subdirectoy's files to entries
			vector<string> newEs = listDirRecursively(drc + data->d_name);
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		} else
			entries.push_back(drc + data->d_name);
	}
	closedir(directory);
#endif
	return entries;
}

pair<vector<string>, vector<string>> Filer::listDirSeparate(const string& drc) {
	vector<string> files, dirs;
#ifdef _WIN32
	if (drc == "\\") {	// if in "root" directory, get drive letters and present them as directories
		vector<char> letters = listDrives();
		dirs.resize(letters.size());
		for (sizt i = 0; i < dirs.size(); i++)
			dirs[i] = letters[i] + string(":");
		return make_pair(files, dirs);
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(appendDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return make_pair(files, dirs);

	do {
		if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..")) {	// ignore . and ..
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				dirs.push_back(wtos(data.cFileName));
			else
				files.push_back(wtos(data.cFileName));
		}
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return make_pair(files, dirs);

	while (dirent* data = readdir(directory))
		if (strcmp(data->d_name, ".") && strcmp(data->d_name, "..")) {	// ignore . and ..
			if (data->d_type == DT_DIR)
				dirs.push_back(data->d_name);
			else
				files.push_back(data->d_name);
		}
	closedir(directory);
#endif
	std::sort(files.begin(), files.end(), strnatless);
	std::sort(dirs.begin(), dirs.end(), strnatless);
	return make_pair(files, dirs);
}

FileType Filer::fileType(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path)) {
		vector<char> letters = Filer::listDrives();
		return inRange(path[0], letters[0], letters.back()) ? FTYPE_DIR : FTYPE_NONE;
	}
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

bool Filer::isPicture(const string& file) {
	if (SDL_Surface* img = IMG_Load(file.c_str())) {
		SDL_FreeSurface(img);
		return true;
	}
	return false;
}

bool Filer::isArchive(const string& file) {
	struct archive* arch = archive_read_new();
	archive_read_support_format_all(arch);
	archive_read_support_compression_all(arch);
	if (archive_read_open_filename(arch, file.c_str(), Default::archiveReadBlockSize))
		return false;
	
	archive_read_free(arch);
	return true;
}

bool Filer::isFont(const string& file) {
	if (TTF_Font* fnt = TTF_OpenFont(file.c_str(), Default::fontTestHeight)) {
		TTF_CloseFont(fnt);
		return true;
	}
	return false;
}
#ifdef _WIN32
vector<char> Filer::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();
	
	for (char i = 0; i < 26; i++)
		if (drives & (1 << i))
			letters.push_back('A'+i);
	return letters;
}

string Filer::wgetenv(const string& name) {
	wchar* buffer = new wchar[Default::envBufferSize];
	DWORD res = GetEnvironmentVariableW(stow(name).c_str(), buffer, Default::envBufferSize);
	if (!res)
		buffer[0] = L'\0';
	else if (res >= Default::envBufferSize) {
		delete[] buffer;
		buffer = new wchar[res];
		res = GetEnvironmentVariableW(stow(name).c_str(), buffer, res);
		if (!res)
			buffer[0] = L'\0';
	}
	string val = wtos(buffer);
	delete[] buffer;
	return val;
}
#endif
string Filer::getExecDir() {
	string path;
#ifdef _WIN32
	wchar buffer[MAX_PATH];
	return GetModuleFileNameW(GetModuleHandleW(nullptr), buffer, MAX_PATH) ? parentPath(wtos(buffer)) : getWorkingDir();
#else
	char buffer[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
	if (len < 0)
		return getWorkingDir();
	buffer[len] = '\0';
	return parentPath(buffer);
#endif
}

string Filer::getWorkingDir() {
#ifdef _WIN32
	wchar buffer[MAX_PATH];
	return GetCurrentDirectoryW(MAX_PATH, buffer) ? wtos(buffer) : "";
#else
	char buffer[PATH_MAX];
	return getcwd(buffer, sizeof(buffer)) ? buffer : "";
#endif
}

string Filer::findFont(const string& font) {	
	if (isAbsolute(font) && isFont(font))	// check if font refers to a file
		return font;

	for (const string& drc : dirFonts)	// check font directories
		for (string& it : listDirRecursively(drc))
			if (!strcicmp(hasExt(it) ? delExt(filename(it)) : filename(it), font) && isFont(it))
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
