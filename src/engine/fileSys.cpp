#include "world.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

// INI LINE

string IniLine::line() const {
	if (type == Type::prpVal)
		return line(prp, val);
	if (type == Type::prpKeyVal)
		return line(prp, key, val);
	if (type == Type::title)
		return line(prp);
	return "";
}

void IniLine::setVal(const string& property, const string& value) {
	type = Type::prpVal;
	prp = property;
	key.clear();
	val = value;
}

void IniLine::setVal(const string& property, const string& vkey, const string& value) {
	type = Type::prpKeyVal;
	prp = property;
	key = vkey;
	val = value;
}

void IniLine::setTitle(const string& title) {
	type = Type::title;
	prp = title;
	key.clear();
	val.clear();
}

IniLine::Type IniLine::setLine(const string& str) {
	sizt i0 = str.find_first_of('=');
	sizt i1 = str.find_first_of('[');
	sizt i2 = str.find_first_of(']', i1);

	if (i0 != string::npos) {
		val = str.substr(i0 + 1);
		if (i2 < i0) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return type = Type::prpKeyVal;
		} else {
			prp = trim(str.substr(0, i0));
			key.clear();
			return type = Type::prpVal;
		}
	} else if (i2 != string::npos) {
		prp = trim(str.substr(i1 + 1, i2 - i1 - 1));
		key.clear();
		val.clear();
		return type = Type::title;
	}
	prp.clear();
	val.clear();
	key.clear();
	return type = Type::empty;
}

// FILER

FileSys::FileSys() {
	// set up file/directory path constants
	dirExec = appendDsep(getExecDir());
#ifdef _WIN32
	dirFonts = {dirExec, wgetenv("SystemDrive") + "\\Windows\\Fonts\\"};
	dirSets = wgetenv("AppData") + dseps + Default::titleDefault + dseps;
#else
	dirFonts = {dirExec, "/usr/share/fonts/", string(getenv("HOME")) + "/.fonts/"};
	dirSets = string(getenv("HOME")) + "/." + Default::titleExtra + dseps;
#endif
	dirLangs = dirExec + Default::dirLanguages + dsep;
	dirTexs = dirExec + Default::dirTextures + dsep;

	// check if all (more or less) necessary files and directories exist
	if (fileType(dirSets) != FTYPE_DIR)
		if (!createDir(dirSets))
			std::cerr << "Couldn't create settings directory." << std::endl;
	if (fileType(dirExec + Default::fileThemes) != FTYPE_FILE)
		std::cerr << "Couldn't find themes file." << std::endl;
	if (fileType(dirLangs) != FTYPE_DIR)
		std::cerr << "Couldn't find language directory." << std::endl;
	if (fileType(dirTexs) != FTYPE_DIR)
		std::cerr << "Couldn't find texture directory." << std::endl;
}

vector<string> FileSys::getAvailibleThemes() {
	vector<string> themes;
	for (string& line : readTextFile(dirExec + Default::fileThemes))
		if (IniLine il(line); il.getType() == IniLine::Type::title)
			themes.push_back(il.getPrp());
	return themes;
}

vector<SDL_Color> FileSys::loadColors(const string& theme) {
	vector<SDL_Color> colors = Default::colors;
	vector<string> lines = readTextFile(dirExec + Default::fileThemes);

	// find title equal to theme
	IniLine il;
	sizt i = 0;
	for (; i < lines.size(); i++)
		if (il.setLine(lines[i]) == IniLine::Type::title && il.getPrp() == theme)
			break;

	// read colors until the end of the file or another title
	while (++i < lines.size()) {
		if (il.setLine(lines[i]) == IniLine::Type::title)
			break;
		if (il.getType() == IniLine::Type::prpVal)
			if (sizt cid = strToEnum<sizt>(Default::colorNames, il.getPrp()); cid < colors.size())
				colors[cid] = getColor(il.getVal());
	}
	return colors;
}

vector<string> FileSys::getAvailibleLanguages() {
	if (fileType(dirLangs) != FTYPE_DIR)
		return {};

	vector<string> files = {Default::language};
	for (string& it : listDir(dirLangs, FTYPE_FILE))
		files.push_back(delExt(it));
	return files;
}

umap<string, string> FileSys::loadTranslations(const string& language) {
	umap<string, string> translation;
	for (string& line : readTextFile(dirLangs + language + ".ini", false))
		if (IniLine il(line); il.getType() == IniLine::Type::prpVal)
			translation.insert(std::make_pair(il.getPrp(), il.getVal()));
	return translation;
}

bool FileSys::getLastPage(const string& book, string& drc, string& fname) {
	for (string& line : readTextFile(dirSets + Default::fileBooks, false))
		if (vector<string> words = strUnenclose(line); words.size() >= 2 && words[0] == book) {
			drc = words[1];
			fname = words.size() >= 3 ? words[2] : "";
			return true;
		}
	return false;
}

bool FileSys::saveLastPage(const string& book, const string& drc, const string& fname) {
	vector<string> lines = readTextFile(dirSets + Default::fileBooks, false);
	sizt id = 0;
	for (; id < lines.size(); id++)
		if (vector<string> words = strUnenclose(lines[id]); words.size() >= 2 && words[0] == book)
			break;

	if (string ilin = strEnclose(book) + ' ' + strEnclose(drc) + ' ' + strEnclose(fname); id < lines.size())
		lines[id] = ilin;
	else
		lines.push_back(ilin);
	return writeTextFile(dirSets + Default::fileBooks, lines);
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (string& line : readTextFile(dirSets + Default::fileSettings, false)) {
		IniLine il(line);
		if (il.getType() != IniLine::Type::prpVal)
			continue;

		if (il.getPrp() == Default::iniKeywordMaximized)
			sets->maximized = stob(il.getVal());
		else if (il.getPrp() == Default::iniKeywordFullscreen)
			sets->fullscreen = stob(il.getVal());
		else if (il.getPrp() == Default::iniKeywordResolution)
			sets->resolution.set(il.getVal(), strtoul, 0);
		else if (il.getPrp() == Default::iniKeywordDirection)
			sets->direction.set(il.getVal());
		else if (il.getPrp() == Default::iniKeywordZoom)
			sets->zoom = sstof(il.getVal());
		else if (il.getPrp() == Default::iniKeywordSpacing)
			sets->spacing = int(sstoul(il.getVal()));
		else if (il.getPrp() == Default::iniKeywordFont)
			sets->setFont(il.getVal());
		else if (il.getPrp() == Default::iniKeywordLanguage)
			sets->setLang(il.getVal());
		else if (il.getPrp() == Default::iniKeywordTheme)
			sets->setTheme(il.getVal());
		else if (il.getPrp() == Default::iniKeywordLibrary)
			sets->setDirLib(il.getVal());
		else if (il.getPrp() == Default::iniKeywordRenderer)
			sets->renderer = il.getVal();
		else if (il.getPrp() == Default::iniKeywordScrollSpeed)
			sets->scrollSpeed.set(il.getVal(), strtof);
		else if (il.getPrp() == Default::iniKeywordDeadzone)
			sets->setDeadzone(int(sstoul(il.getVal())));
	}
	return sets;
}

bool FileSys::saveSettings(const Settings* sets) {
	vector<string> lines = {
		IniLine::line(Default::iniKeywordMaximized, btos(sets->maximized)),
		IniLine::line(Default::iniKeywordFullscreen, btos(sets->fullscreen)),
		IniLine::line(Default::iniKeywordResolution, sets->getResolutionString()),
		IniLine::line(Default::iniKeywordDirection, sets->direction.toString()),
		IniLine::line(Default::iniKeywordZoom, trimZero(to_string(sets->zoom))),
		IniLine::line(Default::iniKeywordSpacing, to_string(sets->spacing)),
		IniLine::line(Default::iniKeywordFont, sets->getFont()),
		IniLine::line(Default::iniKeywordLanguage, sets->getLang()),
		IniLine::line(Default::iniKeywordTheme, sets->getTheme()),
		IniLine::line(Default::iniKeywordLibrary, sets->getDirLib()),
		IniLine::line(Default::iniKeywordRenderer, sets->renderer),
		IniLine::line(Default::iniKeywordScrollSpeed, sets->getScrollSpeedString()),
		IniLine::line(Default::iniKeywordDeadzone, to_string(sets->getDeadzone()))
	};
	return writeTextFile(dirSets + Default::fileSettings, lines);
}

vector<Binding> FileSys::getBindings() {
	vector<Binding> bindings((sizt(Binding::Type::refresh)) + 1);
	for (sizt i = 0; i < bindings.size(); i++)
		bindings[i].setDefaultSelf(Binding::Type(i));
	
	for (string& line : readTextFile(dirSets + Default::fileBindings, false)) {
		IniLine il(line);
		if (il.getType() != IniLine::Type::prpVal || il.getVal().length() < 3)
			continue;

		sizt bid = strToEnum<sizt>(Default::bindingNames, il.getPrp());
		if (bid >= bindings.size())
			continue;

		switch (toupper(il.getVal()[0])) {
		case 'K':	// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(il.getVal().substr(2).c_str()));
			break;
		case 'B':	// joystick button
			bindings[bid].setJbutton(uint8(sstoul(il.getVal().substr(2))));
			break;
		case 'H':	// joystick hat
			for (sizt i = 2; i < il.getVal().length(); i++)
				if (il.getVal()[i] < '0' || il.getVal()[i] > '9') {
					bindings[bid].setJhat(uint8(sstoul(il.getVal().substr(2, i-2))), uint8(jtStrToHat(il.getVal().substr(i+1))));
					break;
				}
			break;
		case 'A':	// joystick axis
			bindings[bid].setJaxis(uint8(sstoul(il.getVal().substr(3))), il.getVal()[2] != '-');
			break;
		case 'G':	// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Default::gbuttonNames, il.getVal().substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case 'X':	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Default::gaxisNames, il.getVal().substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (il.getVal()[2] != '-'));
		}
	}
	return bindings;
}

bool FileSys::saveBindings(const vector<Binding>& bindings) {
	vector<string> lines;
	for (sizt i = 0; i < bindings.size(); i++) {
		string name = enumToStr(Default::bindingNames, i);
		if (bindings[i].keyAssigned())
			lines.push_back(IniLine::line(name, "K_" + string(SDL_GetScancodeName(bindings[i].getKey()))));

		if (bindings[i].jbuttonAssigned())
			lines.push_back(IniLine::line(name, "B_" + to_string(bindings[i].getJctID())));
		else if (bindings[i].jhatAssigned())
			lines.push_back(IniLine::line(name, "H_" + to_string(bindings[i].getJctID()) + "_" + jtHatToStr(bindings[i].getJhatVal())));
		else if (bindings[i].jaxisAssigned())
			lines.push_back(IniLine::line(name, string(bindings[i].jposAxisAssigned() ? "A_+" : "A_-") + to_string(bindings[i].getJctID())));

		if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine::line(name, "G_" + enumToStr(Default::gbuttonNames, bindings[i].getGbutton())));
		else if (bindings[i].gbuttonAssigned())
			lines.push_back(IniLine::line(name, string(bindings[i].gposAxisAssigned() ? "X_+" : "X_-") + enumToStr(Default::gaxisNames, bindings[i].getGaxis())));
	}
	return writeTextFile(dirSets + Default::fileBindings, lines);
}

vector<string> FileSys::readTextFile(const string& file, bool printMessage) {
	FILE* ifh = fopen(file.c_str(), "rb");
	if (!ifh) {
		if (printMessage)
			std::cerr << "Couldn't open file " << file << std::endl;
		return {};
	}

	vector<string> lines(1);
	for (int c = fgetc(ifh); c != EOF; c = fgetc(ifh)) {
		if (c != '\n' && c != '\r')
			lines.back() += char(c);
		else if (lines.back().size())
			lines.push_back("");
	}
	fclose(ifh);
	if (lines.back().empty())
		lines.pop_back();
	return lines;
}

bool FileSys::writeTextFile(const string& file, const vector<string>& lines) {
	FILE* ofh = fopen(file.c_str(), "wb");
	if (!ofh) {
		std::cerr << "Couldn't write file " << file << std::endl;
		return false;
	}

	for (const string& it : lines)
		fputs((it + '\n').c_str(), ofh);
	fclose(ofh);
	return true;
}

bool FileSys::createDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

vector<string> FileSys::listDir(const string& drc, FileType filter) {
	vector<string> entries;
#ifdef _WIN32
	if (drc == dseps) {	// if in "root" directory, get drive letters and present them as directories
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

vector<string> FileSys::listDirRecursively(string drc) {
	drc = appendDsep(drc);
	vector<string> entries;
#ifdef _WIN32
	if (drc == dseps) {	// if in "root" directory, get drive letters and present them as directories
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

pair<vector<string>, vector<string>> FileSys::listDirSeparate(const string& drc) {
	vector<string> files, dirs;
#ifdef _WIN32
	if (drc == dseps) {	// if in "root" directory, get drive letters and present them as directories
		vector<char> letters = listDrives();
		dirs.resize(letters.size());
		for (sizt i = 0; i < dirs.size(); i++)
			dirs[i] = letters[i] + string(":");
		return std::make_pair(files, dirs);
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(appendDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return std::make_pair(files, dirs);

	do {
		if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))	// ignore . and ..
			(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dirs : files).push_back(wtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return make_pair(files, dirs);

	while (dirent* data = readdir(directory))
		if (strcmp(data->d_name, ".") && strcmp(data->d_name, ".."))	// ignore . and ..
			(data->d_type == DT_DIR ? dirs : files).push_back(data->d_name);
	closedir(directory);
#endif
	std::sort(files.begin(), files.end(), strnatless);
	std::sort(dirs.begin(), dirs.end(), strnatless);
	return std::make_pair(files, dirs);
}

FileType FileSys::fileType(const string& path) {
#ifdef _WIN32
	if (isDriveLetter(path)) {
		vector<char> letters = FileSys::listDrives();
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

bool FileSys::isPicture(const string& file) {
	if (SDL_Surface* img = IMG_Load(file.c_str())) {
		SDL_FreeSurface(img);
		return true;
	}
	return false;
}

bool FileSys::isArchive(const string& file) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (SDL_RWops* io = readArchiveEntry(arch, entry))
				if (SDL_Surface* surf = IMG_Load_RW(io, SDL_TRUE)) {
					SDL_FreeSurface(surf);
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isFont(const string& file) {
	if (TTF_Font* fnt = TTF_OpenFont(file.c_str(), Default::fontTestHeight)) {
		TTF_CloseFont(fnt);
		return true;
	}
	return false;
}
#ifdef _WIN32
string FileSys::wgetenv(const string& name) {
	wstring var = stow(name);
	DWORD len = GetEnvironmentVariableW(var.c_str(), nullptr, 0);
	if (len <= 1)
		return "";

	wstring str(len - 1, '\0');
	GetEnvironmentVariableW(var.c_str(), str.data(), len);
	return wtos(str);
}

vector<char> FileSys::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();

	for (char i = 0; i < 26; i++)
		if (drives & (1 << i))
			letters.push_back('A' + i);
	return letters;
}
#endif
string FileSys::getExecDir() {
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

string FileSys::getWorkingDir() {
#ifdef _WIN32
	wchar buffer[MAX_PATH];
	return GetCurrentDirectoryW(MAX_PATH, buffer) ? wtos(buffer) : "";
#else
	char buffer[PATH_MAX];
	return getcwd(buffer, sizeof(buffer)) ? buffer : "";
#endif
}

string FileSys::findFont(const string& font) {	
	if (isFont(font))	// check if font refers to a file
		return font;

	for (const string& drc : dirFonts)	// check font directories
		for (string& it : listDirRecursively(drc))
			if (!strcicmp(hasExt(it) ? delExt(filename(it)) : filename(it), font) && isFont(it))
				return it;
	return "";	// nothing found
}
