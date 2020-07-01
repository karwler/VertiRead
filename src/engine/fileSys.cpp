#include "windowSys.h"
#include "utils/compare.h"
#include <queue>
#ifdef _WIN32
#include <windows.h>
#endif

// INI LINE

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
	sizet i0 = str.find_first_of('=');
	sizet i1 = str.find_first_of('[');
	sizet i2 = str.find_first_of(']', i1);

	if (i0 != string::npos) {
		val = str.substr(i0 + 1);
		if (i2 < i0) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return type = Type::prpKeyVal;
		}
		prp = trim(str.substr(0, i0));
		key.clear();
		return type = Type::prpVal;
	}
	if (i2 != string::npos) {
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

// FILE SYS

FileSys::FileSys() {
	// set up file/directory path constants
	if (setWorkingDir())
		std::cerr << "failed to set working directory" << std::endl;
#ifdef _WIN32
	dirFonts = { fs::path(L"."), fs::path(_wgetenv(L"SystemDrive")) / L"Windows\\Fonts" };
	dirSets = fs::path(_wgetenv(L"AppData")) / L"VertiRead";
#else
	dirFonts = { fs::u8path("."), fs::u8path(getenv("HOME")) / ".fonts", fs::u8path("/usr/share/fonts/") };
	dirSets = fs::u8path(getenv("HOME")) / ".vertiread";
#endif
	// check if all (more or less) necessary files and directories exist
	try {
		if (!fs::is_directory(dirSets) && !fs::create_directories(dirSets))
			std::cerr << "failed to create settings directory" << std::endl;
	} catch (...) {
		std::cerr << "invalid settings path" << std::endl;
	}
	try {
		if (!fs::is_regular_file(fileThemes))
			std::cerr << "failed to find themes file" << std::endl;
	} catch (...) {
		std::cerr << "invalid themes path" << std::endl;
	}
	try {
		if (!fs::is_directory(dirTexs))
			std::cerr << "failed to find texture directory" << std::endl;
	} catch (...) {
		std::cerr << "invalid texture path" << std::endl;
	}
}

vector<string> FileSys::getAvailableThemes() const {
	vector<string> themes;
	for (IniLine il : readFileLines(fileThemes))
		if (il.getType() == IniLine::Type::title)
			themes.push_back(il.getPrp());
	return !themes.empty() ? themes : vector<string>{ "default" };
}

array<SDL_Color, FileSys::defaultColors.size()> FileSys::loadColors(const string& theme) const {
	array<SDL_Color, defaultColors.size()> colors = defaultColors;
	vector<string> lines = readFileLines(fileThemes);

	IniLine il;	// find title equal to theme and read colors until the end of the file or another title
	vector<string>::iterator it = std::find_if(lines.begin(), lines.end(), [&il, theme](const string& ln) -> bool { return il.setLine(ln) == IniLine::Type::title && il.getPrp() == theme; });
	if (it == lines.end())
		return colors;

	while (++it != lines.end()) {
		if (il.setLine(*it) == IniLine::Type::title)
			break;
		if (il.getType() == IniLine::Type::prpVal)
			if (sizet cid = strToEnum<sizet>(colorNames, il.getPrp()); cid < colors.size())
				colors[cid] = readColor(il.getVal());
	}
	return colors;
}

bool FileSys::getLastPage(const string& book, string& drc, string& fname) const {
	for (const string& line : readFileLines(dirSets / fileBooks, false))
		if (vector<string> words = strUnenclose(line); words.size() >= 2 && words[0] == book) {
			drc = words[1];
			fname = words.size() >= 3 ? words[2] : string();
			return true;
		}
	return false;
}

bool FileSys::saveLastPage(const string& book, const string& drc, const string& fname) const {
	fs::path path = dirSets / fileBooks;
	vector<string> lines = readFileLines(path, false);
	vector<string>::iterator li = std::find_if(lines.begin(), lines.end(), [book](const string& it) -> bool { vector<string> words = strUnenclose(it); return words.size() >= 2 && words[0] == book; });

	if (string ilin = strEnclose(book) + ' ' + strEnclose(drc) + ' ' + strEnclose(fname); li != lines.end())
		*li = std::move(ilin);
	else
		lines.push_back(std::move(ilin));
	return writeTextFile(path, lines);
}

Settings* FileSys::loadSettings() const {
	Settings* sets = new Settings(dirSets, getAvailableThemes());
	for (IniLine il : readFileLines(dirSets / fileSettings, false)) {
		if (il.getType() != IniLine::Type::prpVal)
			continue;

		if (il.getPrp() == iniKeywordMaximized)
			sets->maximized = stob(il.getVal());
		else if (il.getPrp() == iniKeywordFullscreen)
			sets->fullscreen = stob(il.getVal());
		else if (il.getPrp() == iniKeywordResolution)
			sets->resolution = stoiv<ivec2>(il.getVal().c_str(), strtoul);
		else if (il.getPrp() == iniKeywordDirection)
			sets->direction = strToEnum<Direction::Dir>(Direction::names, il.getVal());
		else if (il.getPrp() == iniKeywordZoom)
			sets->zoom = sstof(il.getVal());
		else if (il.getPrp() == iniKeywordSpacing)
			sets->spacing = int(sstoul(il.getVal()));
		else if (il.getPrp() == iniKeywordPictureLimit)
			sets->picLim.set(il.getVal());
		else if (il.getPrp() == iniKeywordFont)
			sets->font = FileSys::isFont(findFont(il.getVal())) ? il.getVal() : Settings::defaultFont;
		else if (il.getPrp() == iniKeywordTheme)
			sets->setTheme(il.getVal(), getAvailableThemes());
		else if (il.getPrp() == iniKeywordShowHidden)
			sets->showHidden = stob(il.getVal());
		else if (il.getPrp() == iniKeywordLibrary)
			sets->setDirLib(fs::u8path(il.getVal()), dirSets);
		else if (il.getPrp() == iniKeywordRenderer)
			sets->renderer = il.getVal();
		else if (il.getPrp() == iniKeywordScrollSpeed)
			sets->scrollSpeed = stofv<vec2>(il.getVal().c_str());
		else if (il.getPrp() == iniKeywordDeadzone)
			sets->setDeadzone(int(sstol(il.getVal())));
	}
	return sets;
}

void FileSys::saveSettings(const Settings* sets) const {
	fs::path path = dirSets / fileSettings;
	std::ofstream ofh(path, std::ios::binary);
	if (!ofh.good()) {
		std::cerr << "failed to write settings file " << path << std::endl;
		return;
	}
	IniLine::writeVal(ofh, iniKeywordMaximized, btos(sets->maximized));
	IniLine::writeVal(ofh, iniKeywordFullscreen, btos(sets->fullscreen));
	IniLine::writeVal(ofh, iniKeywordResolution, sets->resolution.x, ' ', sets->resolution.y);
	IniLine::writeVal(ofh, iniKeywordZoom, sets->zoom);
	IniLine::writeVal(ofh, iniKeywordPictureLimit, PicLim::names[uint8(sets->picLim.type)], ' ', sets->picLim.getCount(), ' ', memoryString(sets->picLim.getSize()));
	IniLine::writeVal(ofh, iniKeywordSpacing, sets->spacing);
	IniLine::writeVal(ofh, iniKeywordDirection, Direction::names[uint8(sets->direction)]);
	IniLine::writeVal(ofh, iniKeywordFont, sets->font);
	IniLine::writeVal(ofh, iniKeywordTheme, sets->getTheme());
	IniLine::writeVal(ofh, iniKeywordShowHidden, btos(sets->showHidden));
	IniLine::writeVal(ofh, iniKeywordLibrary, sets->getDirLib().u8string());
	IniLine::writeVal(ofh, iniKeywordRenderer, sets->renderer);
	IniLine::writeVal(ofh, iniKeywordScrollSpeed, sets->scrollSpeed.x, ' ', sets->scrollSpeed.y);
	IniLine::writeVal(ofh, iniKeywordDeadzone, sets->getDeadzone());
}

array<Binding, Binding::names.size()> FileSys::getBindings() const {
	array<Binding, Binding::names.size()> bindings;
	for (sizet i = 0; i < bindings.size(); i++)
		bindings[i].reset(Binding::Type(i));

	for (IniLine il : readFileLines(dirSets / fileBindings, false)) {
		if (il.getType() != IniLine::Type::prpVal || il.getVal().length() < 3)
			continue;
		sizet bid = strToEnum<sizet>(Binding::names, il.getPrp());
		if (bid >= bindings.size())
			continue;

		switch (toupper(il.getVal()[0])) {
		case keyKey[0]:			// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(il.getVal().substr(2).c_str()));
			break;
		case keyButton[0]:		// joystick button
			bindings[bid].setJbutton(uint8(sstoul(il.getVal().substr(2))));
			break;
		case keyHat[0]:			// joystick hat
			if (sizet id = sizet(std::find_if(il.getVal().begin() + 2, il.getVal().end(), [](char c) -> bool { return !isdigit(c); }) - il.getVal().begin()); id < il.getVal().size())
				bindings[bid].setJhat(uint8(sstoul(il.getVal().substr(2, id-2))), strToVal(KeyGetter::hatNames, il.getVal().substr(id+1)));
			break;
		case keyAxisPos[0]:		// joystick axis
			bindings[bid].setJaxis(uint8(sstoul(il.getVal().substr(3))), il.getVal()[2] != keyAxisNeg[2]);
			break;
		case keyGButton[0]:		// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(KeyGetter::gbuttonNames, il.getVal().substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case keyGAxisPos[0]:	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(KeyGetter::gaxisNames, il.getVal().substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (il.getVal()[2] != keyGAxisNeg[2]));
		}
	}
	return bindings;
}

void FileSys::saveBindings(const array<Binding, Binding::names.size()>& bindings) const {
	fs::path path = dirSets / fileBindings;
	std::ofstream ofh(path, std::ios::binary);
	if (!ofh.good()) {
		std::cerr << "failed to write bindings file " << path << std::endl;
		return;
	}
	for (sizet i = 0; i < bindings.size(); i++) {
		const char* name = Binding::names[i];
		if (bindings[i].keyAssigned())
			IniLine::writeVal(ofh, name, keyKey, SDL_GetScancodeName(bindings[i].getKey()));

		if (bindings[i].jbuttonAssigned())
			IniLine::writeVal(ofh, name, keyButton, uint(bindings[i].getJctID()));
		else if (bindings[i].jhatAssigned())
			IniLine::writeVal(ofh, name, keyHat, uint(bindings[i].getJctID()), keySep, KeyGetter::hatNames.at(bindings[i].getJhatVal()));
		else if (bindings[i].jaxisAssigned())
			IniLine::writeVal(ofh, name, (bindings[i].jposAxisAssigned() ? keyAxisPos : keyAxisNeg), uint(bindings[i].getJctID()));

		if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofh, name, keyGButton, KeyGetter::gbuttonNames[uint8(bindings[i].getGbutton())]);
		else if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofh, name, (bindings[i].gposAxisAssigned() ? keyGAxisPos : keyGAxisNeg), KeyGetter::gaxisNames[uint8(bindings[i].getGaxis())]);
	}
}

vector<string> FileSys::readFileLines(const fs::path& file, bool printMessage) {
	vector<string> lines;
	constexpr char nl[] = "\n\r";
	string text = readTextFile(file, printMessage);
	for (sizet e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); sizet len = e - p)
			lines.push_back(text.substr(p, len));
	return lines;
}

string FileSys::readTextFile(const fs::path& file, bool printMessage) {
	string text;
	try {
		std::ifstream ifh(file, std::ios::binary | std::ios::ate);
		if (!ifh.good())
			throw 0;
		std::streampos len = ifh.tellg();
		if (len == -1)
			throw 0;
		ifh.seekg(0);

		text.resize(len);
		if (ifh.read(text.data(), text.length()); sizet(ifh.gcount()) < text.length())
			text.resize(ifh.gcount());
	} catch (...) {
		if (printMessage)
			std::cerr << "failed to open file " << file << std::endl;
		return string();
	}
	return text;
}

bool FileSys::writeTextFile(const fs::path& file, const vector<string>& lines) {
	try {
		std::ofstream ofh(file, std::ios::binary);
		if (!ofh.good())
			throw 0;

		for (const string& it : lines) {
			ofh.write(it.c_str(), it.length());
			ofh.put('\n');
		}
	} catch (...) {
		std::cerr << "failed to write file " << file << std::endl;
		return false;
	}
	return true;
}

SDL_Color FileSys::readColor(const string& line) {
	SDL_Color color = { 0, 0, 0, 255 };
	const char* pos = line.c_str();
	for (; isSpace(*pos); pos++);

	if (*pos == '#') {
		while (*++pos == '#');
		char* end;
		if (uint32 num = uint32(strtoul(pos, &end, 0x10)); end != pos) {
			if (uint8 mov = (8 - uint8(end - pos)) * 4)
				num = (num << mov) + UINT8_MAX;
			memcpy(&color, &num, sizeof(uint32));
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

vector<fs::path> FileSys::listDir(const fs::path &drc, bool files, bool dirs, bool showHidden) {
#ifdef _WIN32
	if (drc.empty())	// if in "root" directory, get drive letters and present them as directories
		return dirs ? listDrives() : vector<fs::path>();
#endif
	vector<fs::path> entries;
	try {
		for (const fs::directory_entry& it : fs::directory_iterator(drc, fs::directory_options::skip_permission_denied)) {
#ifdef _WIN32
			if (DWORD attr = GetFileAttributesW(it.path().c_str()); attr != INVALID_FILE_ATTRIBUTES && (showHidden || !(attr & FILE_ATTRIBUTE_HIDDEN)) && (it.status().type() == fs::file_type::directory ? dirs : files))
				entries.push_back(it.path().filename());
#else
			if (fs::path name = it.path().filename(); (showHidden || name.c_str()[0] != '.') && (it.status().type() == fs::file_type::directory ? dirs : files))
				entries.push_back(std::move(name));
#endif
		}
		std::sort(entries.begin(), entries.end(), StrNatCmp());
	} catch (...) {}
	return entries;
}

pair<vector<fs::path>, vector<fs::path>> FileSys::listDirSep(const fs::path& drc, bool showHidden) {
#ifdef _WIN32
	if (drc.empty())	// if in "root" directory, get drive letters and present them as directories
		return pair(vector<fs::path>(), listDrives());
#endif
	vector<fs::path> files, dirs;
	try {
		for (const fs::directory_entry& it : fs::directory_iterator(drc, fs::directory_options::skip_permission_denied)) {
#ifdef _WIN32
			if (DWORD attr = GetFileAttributesW(it.path().c_str()); attr != INVALID_FILE_ATTRIBUTES && (showHidden || !(attr & FILE_ATTRIBUTE_HIDDEN)))
				(it.status().type() == fs::file_type::directory ? dirs : files).push_back(it.path().filename());
#else
			if (fs::path name = it.path().filename(); showHidden || name.c_str()[0] != '.')
				(it.status().type() == fs::file_type::directory ? dirs : files).push_back(std::move(name));
#endif
		}
		std::sort(files.begin(), files.end(), StrNatCmp());
		std::sort(dirs.begin(), dirs.end(), StrNatCmp());
	} catch (...) {}
	return pair(std::move(files), std::move(dirs));
}

fs::path FileSys::validateFilename(const fs::path& file) {
	string str = file.u8string();
#ifdef _WIN32
	for (const char* it : takenFilenames)
		if (sizet len = strlen(it); !strncicmp(str, it, len))
			str.erase(0, len);
	str.erase(std::remove_if(str.begin(), str.end(), [](char c) -> bool { return c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*' || uchar(c) < ' '; }), str.end());

	if (str[0] == ' ')
		str.erase(0, str.find_first_not_of(' '));
	while (str.back() == ' ' || str.back() == '.')
		str.erase(str.find_last_not_of(str.back() == ' ' ? ' ' : '.'));
#else
	str.erase(std::remove_if(str.begin(), str.end(), isDsep), str.end());
#endif
	if (str.length() > fnameMax)
		str.resize(fnameMax);
	return fs::u8path(str);
}

bool FileSys::isPicture(const fs::path& file) {
	if (SDL_Surface* img = IMG_Load(file.u8string().c_str())) {
		SDL_FreeSurface(img);
		return true;
	}
	return false;
}

bool FileSys::isFont(const fs::path& file) {
	if (TTF_Font* fnt = TTF_OpenFont(file.u8string().c_str(), FontSet::fontTestHeight)) {
		TTF_CloseFont(fnt);
		return true;
	}
	return false;
}

bool FileSys::isArchive(const fs::path& file) {
	if (archive* arch = openArchive(file)) {
		archive_read_free(arch);
		return true;
	}
	return false;
}

bool FileSys::isPictureArchive(const fs::path& file) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (SDL_Surface* pic = loadArchivePicture(arch, entry)) {
				SDL_FreeSurface(pic);
				archive_read_free(arch);
				return true;
			}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isArchivePicture(const fs::path& file, const string& pname) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (SDL_Surface* img = loadArchivePicture(arch, entry)) {
					SDL_FreeSurface(img);
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

archive* FileSys::openArchive(const fs::path& file) {
	archive* arch = archive_read_new();
	archive_read_support_filter_all(arch);
	archive_read_support_format_all(arch);

#ifdef _WIN32
	if (archive_read_open_filename_w(arch, file.c_str(), archiveReadBlockSize)) {
#else
	if (archive_read_open_filename(arch, file.c_str(), archiveReadBlockSize)) {
#endif
		archive_read_free(arch);
		return nullptr;
	}
	return arch;
}

vector<string> FileSys::listArchive(const fs::path& file) {
	vector<string> entries;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			entries.emplace_back(archive_entry_pathname_utf8(entry));

		archive_read_free(arch);
		std::sort(entries.begin(), entries.end(), StrNatCmp());
	}
	return entries;
}

mapFiles FileSys::listArchivePictures(const fs::path& file, vector<string>& names) {
	mapFiles files;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);) {
			if (SDL_Surface* img = loadArchivePicture(arch, entry)) {
				string pname = archive_entry_pathname_utf8(entry);
				files.emplace(pname, pair(SIZE_MAX, uptrt(img->w) * uptrt(img->h) * img->format->BytesPerPixel));
				names.push_back(std::move(pname));
				SDL_FreeSurface(img);
			}
		}
		archive_read_free(arch);

		std::sort(names.begin(), names.end(), StrNatCmp());
		for (sizet i = 0; i < names.size(); i++)
			files[names[i]].first = i;
	}
	return files;
}

SDL_Surface* FileSys::loadArchivePicture(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return nullptr;

	uint8* buffer = new uint8[sizet(bsiz)];
	int64 size = archive_read_data(arch, buffer, sizet(bsiz));
	SDL_Surface* pic = size > 0 ? IMG_Load_RW(SDL_RWFromMem(buffer, int(size)), SDL_TRUE) : nullptr;
	delete[] buffer;
	return pic;
}

int FileSys::moveContentThreaded(void* data) {
	Thread* proc = static_cast<Thread*>(data);
	pairPath* locs = static_cast<pairPath*>(proc->data);
	vector<fs::path> files = listDir(locs->first);

	for (uptrt i = 0, lim = files.size(); i < lim; i++) {
		if (!proc->getRun())
			break;

		pushEvent(UserCode::moveProgress, reinterpret_cast<void*>(i), reinterpret_cast<void*>(lim));
		fs::path path = locs->first / files[i];
#ifdef _WIN32
		if (_wrename(path.c_str(), (locs->second / files[i]).c_str()))
#else
		if (rename(path.c_str(), (locs->second / files[i]).c_str()))
#endif
			std::cerr << "failed no move " << path << std::endl;
	}
	pushEvent(UserCode::moveFinished);
	delete locs;
	return 0;
}

int FileSys::setWorkingDir() {
	char* path = SDL_GetBasePath();
	if (!path)
		return 1;
#ifdef _WIN32
	int err = _wchdir(cstow(path).c_str());
#else
	int err = chdir(path);
#endif
	SDL_free(path);
	return err;
}

fs::path FileSys::findFont(const string& font) const {
	if (fs::path path = fs::u8path(font); isFont(path))	// check if font refers to a file
		return path;

	for (const fs::path& drc : dirFonts) {	// check font directories
		try {
			for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied))
				if (!strcicmp(it.path().stem().u8string(), font) && isFont(it.path()))
					return it.path();
		} catch (...) {}
	}
	return fs::path();	// nothing found
}

#ifdef _WIN32
vector<fs::path> FileSys::listDrives() {
	vector<fs::path> letters;
	DWORD drives = GetLogicalDrives();
	for (char i = 0; i < drivesMax; i++)
		if (drives & (1 << i))
			letters.emplace_back(wstring{ wchar('A' + i), ':', '\\' });
	return letters;
}
#endif
