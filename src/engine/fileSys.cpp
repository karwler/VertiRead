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

IniLine::IniLine(const string& line) {
	setLine(line);
}

string IniLine::line() const {
	if (type == Type::prpVal)
		return line(prp, val);
	if (type == Type::prpKeyVal)
		return line(prp, key, val);
	if (type == Type::title)
		return line(prp);
	return "";
}

string IniLine::line(const string& title) {
	return '[' + title + ']';
}

string IniLine::line(const string& prp, const string& val) {
	return prp + '=' + val;
}

string IniLine::line(const string& prp, const string& key, const string& val) {
	return prp + '[' + key + "]=" + val;
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

	if (i0 == string::npos) {
		if (i2 != string::npos) {
			prp = trim(str.substr(i1 + 1, i2 - i1 - 1));
			key.clear();
			val.clear();
			return type = Type::title;
		}
	} else {
		val = str.substr(i0 + 1);
		if (i2 < i0 && strchk(str, i2 + 1, i0 - i2 - 1, isSpace)) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return type = Type::prpKeyVal;
		} else {
			prp = trim(str.substr(0, i0));
			key.clear();
			return type = Type::prpVal;
		}
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
	dirFonts = {dirExec, "/usr/share/fonts/", string(std::getenv("HOME")) + "/.fonts/"};
	dirSets = string(std::getenv("HOME")) + "/." + Default::titleExtra + dseps;
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
	for (string& line : readTextFile(dirExec + Default::fileThemes)) {
		IniLine il(line);
		if (il.getType() == IniLine::Type::title)
			themes.push_back(il.getPrp());
	}
	return themes;
}

vector<SDL_Color> FileSys::getColors(const string& theme) {
	vector<SDL_Color> colors = Default::colors;
	vector<string> lines = readTextFile(dirExec + Default::fileThemes);

	// find title equal to theme
	IniLine il;
	sizt i = 0;
	while (i < lines.size()) {
		if (il.setLine(lines[i]) == IniLine::Type::title && il.getPrp() == theme)
			break;
		i++;
	}

	// read colors until the end of the file or another title
	while (++i < lines.size()) {
		if (il.setLine(lines[i]) == IniLine::Type::title)
			break;
		if (il.getType() != IniLine::Type::prpVal)
			continue;

		sizt cid = strToEnum<sizt>(Default::colorNames, il.getPrp());
		if (cid < colors.size()) {
			vector<string> elems = getWords(il.getVal());
			for (uint8 i = 0; i < elems.size() && i < 4; i++)
				reinterpret_cast<uint8*>(&colors[cid])[i] = stoi(elems[i]);
		}
	}
	return colors;
}

vector<string> FileSys::getAvailibleLanguages() {
	vector<string> files = {Default::language};
	if (fileType(dirLangs) != FTYPE_DIR)
		return {};

	for (string& it : listDir(dirLangs, FTYPE_FILE))
		files.push_back(delExt(it));
	return files;
}

umap<string, string> FileSys::getTranslations(const string& language) {
	umap<string, string> translation;
	for (string& line : readTextFile(dirLangs + language + ".ini", false)) {
		IniLine il(line);
		if (il.getType() == IniLine::Type::prpVal)
			translation.insert(make_pair(il.getPrp(), il.getVal()));
	}
	return translation;
}

bool FileSys::getLastPage(const string& book, string& drc, string& fname) {
	for (string& line : readTextFile(dirSets + Default::fileBooks, false)) {
		vector<string> words = strUnenclose(line);
		if (words.size() >= 2 && words[0] == book) {
			drc = words[1];
			fname = words.size() >= 3 ? words[2] : "";
			return true;
		}
	}
	return false;
}

bool FileSys::saveLastPage(const string& book, const string& drc, const string& fname) {
	vector<string> lines = readTextFile(dirSets + Default::fileBooks, false);
	sizt id = 0;
	for (; id < lines.size(); id++) {
		vector<string> words = strUnenclose(lines[id]);
		if (words.size() >= 2 && words[0] == book)
			break;
	}

	string ilin = strEnclose(book) + ' ' + strEnclose(drc) + ' ' + strEnclose(fname);
	if (id < lines.size())
		lines[id] = ilin;
	else
		lines.push_back(ilin);
	return writeTextFile(dirSets + Default::fileBooks, lines);
}

Settings* FileSys::getSettings() {
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
			sets->setResolution(il.getVal());
		else if (il.getPrp() == Default::iniKeywordDirection)
			sets->direction.set(il.getVal());
		else if (il.getPrp() == Default::iniKeywordZoom)
			sets->zoom = stof(il.getVal());
		else if (il.getPrp() == Default::iniKeywordSpacing)
			sets->spacing = stoi(il.getVal());
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
			sets->setScrollSpeed(il.getVal());
		else if (il.getPrp() == Default::iniKeywordDeadzone)
			sets->setDeadzone(stoi(il.getVal()));
	}
	return sets;
}

bool FileSys::saveSettings(const Settings* sets) {
	vector<string> lines = {
		IniLine::line(Default::iniKeywordMaximized, btos(sets->maximized)),
		IniLine::line(Default::iniKeywordFullscreen, btos(sets->fullscreen)),
		IniLine::line(Default::iniKeywordResolution, sets->getResolutionString()),
		IniLine::line(Default::iniKeywordDirection, sets->direction.toString()),
		IniLine::line(Default::iniKeywordZoom, ntos(sets->zoom)),
		IniLine::line(Default::iniKeywordSpacing, ntos(sets->spacing)),
		IniLine::line(Default::iniKeywordFont, sets->getFont()),
		IniLine::line(Default::iniKeywordLanguage, sets->getLang()),
		IniLine::line(Default::iniKeywordTheme, sets->getTheme()),
		IniLine::line(Default::iniKeywordLibrary, sets->getDirLib()),
		IniLine::line(Default::iniKeywordRenderer, sets->renderer),
		IniLine::line(Default::iniKeywordScrollSpeed, sets->getScrollSpeedString()),
		IniLine::line(Default::iniKeywordDeadzone, ntos(sets->getDeadzone()))
	};
	return writeTextFile(dirSets + Default::fileSettings, lines);
}

vector<Binding> FileSys::getBindings() {
	vector<Binding> bindings(static_cast<sizt>(Binding::Type::numBindings));
	for (sizt i = 0; i < bindings.size(); i++)
		bindings[i].setDefaultSelf(static_cast<Binding::Type>(i));
	
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

bool FileSys::saveBindings(const vector<Binding>& bindings) {
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
	return writeTextFile(dirSets + Default::fileBindings, lines);
}

vector<string> FileSys::readTextFile(const string& file, bool printMessage) {
	vector<string> lines;
	std::ifstream ifs(file.c_str());
	if (ifs.good()) {
		for (string line; readLine(ifs, line);)
			if (line.size())		// skip empty lines
				lines.push_back(line);
	} else if (printMessage)
		std::cerr << "Couldn't open file " << file << std::endl;
	return lines;
}

bool FileSys::writeTextFile(const string& file, const vector<string>& lines) {
	std::ofstream ofs(file.c_str());
	if (ofs.good()) {
		for (const string& line : lines)
			ofs << line << std::endl;
		return true;	
	}
	std::cerr << "Couldn't write file " << file << std::endl;
	return false;
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
		archive_entry* entry;
		while (!archive_read_next_header(arch, &entry))
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
vector<char> FileSys::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();
	
	for (char i = 0; i < 26; i++)
		if (drives & (1 << i))
			letters.push_back('A'+i);
	return letters;
}

string FileSys::wgetenv(const string& name) {
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

std::istream& FileSys::readLine(std::istream& ifs, string& str) {
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
