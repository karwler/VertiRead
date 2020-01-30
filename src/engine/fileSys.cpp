#include "world.h"
#include <queue>

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
	dirFonts = { "./", appDsep(SDL_getenv("SystemDrive")) + "Windows\\Fonts\\" };
	dirSets = appDsep(SDL_getenv("AppData")) + WindowSys::title + dseps;
#else
	dirFonts = { "./", "/usr/share/fonts/", appDsep(SDL_getenv("HOME")) + ".fonts/" };
	dirSets = appDsep(SDL_getenv("HOME")) + ".vertiread/";
#endif
	// check if all (more or less) necessary files and directories exist
	if (fileType(dirSets) != FTYPE_DIR && !createDir(dirSets))
		std::cerr << "failed to create settings directory" << std::endl;
	if (fileType(fileThemes) != FTYPE_REG)
		std::cerr << "failed to find themes file" << std::endl;
	if (fileType(dirTexs) != FTYPE_DIR)
		std::cerr << "failed to find texture directory" << std::endl;
}

vector<string> FileSys::getAvailibleThemes() {
	vector<string> themes;
	for (IniLine il : readFileLines(fileThemes))
		if (il.getType() == IniLine::Type::title)
			themes.push_back(il.getPrp());
	return themes;
}

array<SDL_Color, FileSys::defaultColors.size()> FileSys::loadColors(const string& theme) {
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

bool FileSys::getLastPage(const string& book, string& drc, string& fname) {
	for (const string& line : readFileLines(dirSets + fileBooks, false))
		if (vector<string> words = strUnenclose(line); words.size() >= 2 && words[0] == book) {
			drc = words[1];
			fname = words.size() >= 3 ? words[2] : "";
			return true;
		}
	return false;
}

bool FileSys::saveLastPage(const string& book, const string& drc, const string& fname) {
	vector<string> lines = readFileLines(dirSets + fileBooks, false);
	vector<string>::iterator li = std::find_if(lines.begin(), lines.end(), [book](const string& it) -> bool { vector<string> words = strUnenclose(it); return words.size() >= 2 && words[0] == book; });

	if (string ilin = strEnclose(book) + ' ' + strEnclose(drc) + ' ' + strEnclose(fname); li != lines.end())
		*li = std::move(ilin);
	else
		lines.push_back(std::move(ilin));
	return writeTextFile(dirSets + fileBooks, lines);
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings;
	for (IniLine il : readFileLines(dirSets + fileSettings, false)) {
		if (il.getType() != IniLine::Type::prpVal)
			continue;

		if (il.getPrp() == iniKeywordMaximized)
			sets->maximized = stob(il.getVal());
		else if (il.getPrp() == iniKeywordFullscreen)
			sets->fullscreen = stob(il.getVal());
		else if (il.getPrp() == iniKeywordResolution)
			sets->resolution.set(il.getVal(), strtoul, 0);
		else if (il.getPrp() == iniKeywordDirection)
			sets->direction = strToEnum<Direction::Dir>(Direction::names, il.getVal());
		else if (il.getPrp() == iniKeywordZoom)
			sets->zoom = sstof(il.getVal());
		else if (il.getPrp() == iniKeywordSpacing)
			sets->spacing = int(sstoul(il.getVal()));
		else if (il.getPrp() == iniKeywordPictureLimit)
			sets->picLim.set(il.getVal());
		else if (il.getPrp() == iniKeywordFont)
			sets->setFont(il.getVal());
		else if (il.getPrp() == iniKeywordTheme)
			sets->setTheme(il.getVal());
		else if (il.getPrp() == iniKeywordShowHidden)
			sets->showHidden = stob(il.getVal());
		else if (il.getPrp() == iniKeywordLibrary)
			sets->setDirLib(il.getVal());
		else if (il.getPrp() == iniKeywordRenderer)
			sets->renderer = il.getVal();
		else if (il.getPrp() == iniKeywordScrollSpeed)
			sets->scrollSpeed.set(il.getVal(), strtof);
		else if (il.getPrp() == iniKeywordDeadzone)
			sets->setDeadzone(int(sstol(il.getVal())));
	}
	return sets;
}

void FileSys::saveSettings(const Settings* sets) {
	string path = dirSets + fileSettings;
	std::ofstream ofh(path);
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
	IniLine::writeVal(ofh, iniKeywordFont, sets->getFont());
	IniLine::writeVal(ofh, iniKeywordTheme, sets->getTheme());
	IniLine::writeVal(ofh, iniKeywordShowHidden, btos(sets->showHidden));
	IniLine::writeVal(ofh, iniKeywordLibrary, sets->getDirLib());
	IniLine::writeVal(ofh, iniKeywordRenderer, sets->renderer);
	IniLine::writeVal(ofh, iniKeywordScrollSpeed, sets->scrollSpeed.x, ' ', sets->scrollSpeed.y);
	IniLine::writeVal(ofh, iniKeywordDeadzone, sets->getDeadzone());
}

array<Binding, Binding::names.size()> FileSys::getBindings() {
	array<Binding, Binding::names.size()> bindings;
	for (sizet i = 0; i < bindings.size(); i++)
		bindings[i].reset(Binding::Type(i));
	
	for (IniLine il : readFileLines(dirSets + fileBindings, false)) {
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

void FileSys::saveBindings(const array<Binding, Binding::names.size()>& bindings) {
	string path = dirSets + fileBindings;
	std::ofstream ofh(path);
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

vector<string> FileSys::readFileLines(const string& file, bool printMessage) {
	vector<string> lines;
	constexpr char nl[] = "\n\r";
	string text = readTextFile(file, printMessage);
	for (sizet e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); sizet len = e - p)
			lines.push_back(text.substr(p, len));
	return lines;
}

string FileSys::readTextFile(const string& file, bool printMessage) {
	string text;
	try {
		SDL_RWops* ifh = SDL_RWFromFile(file.c_str(), defaultFrMode);
		if (!ifh)
			throw ifh;
		int64 len = SDL_RWsize(ifh);
		if (len == -1)
			throw ifh;

		text.resize(sizet(len));
		if (sizet read = SDL_RWread(ifh, text.data(), sizeof(*text.data()), text.length()); read < text.length())
			text.resize(read);
		SDL_RWclose(ifh);
	} catch (SDL_RWops* ifh) {
		if (ifh)
			SDL_RWclose(ifh);
		if (printMessage)
			std::cerr << "failed to open file " << file << std::endl;
	}
	return text;
}

bool FileSys::writeTextFile(const string& file, const vector<string>& lines) {
	if (FILE* ofh = fopen(file.c_str(), defaultFwMode)) {
		for (const string& it : lines) {
			fwrite(it.c_str(), sizeof(char), it.length(), ofh);
			fputc('\n', ofh);
		}
		fclose(ofh);
	} else {
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

vector<string> FileSys::listDir(string drc, FileType filter, bool showHidden, bool readLinks) {
	drc = appDsep(drc);
	vector<string> entries;
#ifdef _WIN32
	if (std::all_of(drc.begin(), drc.end(), isDsep)) {	// if in "root" directory, get drive letters and present them as directories
		if (!(filter & FTYPE_DIR))
			return entries;

		vector<char> dcv = listDrives();
		entries.resize(dcv.size());
		for (sizet i = 0; i < dcv.size(); i++)
			entries.push_back({ dcv[i], ':' });
		return entries;
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(sstow(appDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (!isDotName(data.cFileName) && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) && atrcmp(data.dwFileAttributes, filter))
			entries.push_back(cwtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return entries;

	while (dirent* entry = readdir(directory))
		if (!isDotName(entry->d_name) && (showHidden || entry->d_name[0] != '.') && dtycmp(drc, entry, filter, readLinks))
			entries.emplace_back(entry->d_name);
	closedir(directory);
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}

int FileSys::iterateDirRec(const string& drc, const std::function<int (const string&)>& call, FileType filter, bool readLinks, bool followLinks) {
	int ret = 0;
#ifdef _WIN32
	std::queue<wstring> dirs;
	if (std::all_of(drc.begin(), drc.end(), isDsep)) {	// if in "root" directory, get drive letters and present them as directories
		for (char dc : listDrives()) {
			if (filter & FTYPE_DIR && (ret = call({ dc, ':' })))
				return ret;
			dirs.push({ wchar(dc), ':', wchar(dsep) });
		}
	} else
		dirs.push(sstow(appDsep(drc)));

	do {
		WIN32_FIND_DATAW data;
		if (HANDLE hFind = FindFirstFileW(wstring(dirs.front() + L'*').c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
			do {
				if (isDotName(data.cFileName))
					continue;

				if (atrcmp(data.dwFileAttributes, filter) && (ret = call(swtos(dirs.front() + data.cFileName)))) {
					FindClose(hFind);
					return ret;
				}
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					dirs.push(appDsep(dirs.front() + data.cFileName));
			} while (FindNextFileW(hFind, &data));
			FindClose(hFind);
		}
#else
	std::queue<string> dirs;
	dirs.push(appDsep(drc));
	do {
		if (DIR* directory = opendir(dirs.front().c_str())) {
			while (dirent* entry = readdir(directory)) {
				if (isDotName(entry->d_name))
					continue;

				if (dtycmp(dirs.front(), entry, filter, readLinks) && (ret = call(dirs.front() + entry->d_name))) {
					closedir(directory);
					return ret;
				}
				if (dtycmp(dirs.front(), entry, FTYPE_DIR, followLinks))
					dirs.push(appDsep(dirs.front() + entry->d_name));
			}
			closedir(directory);
		}
#endif
		dirs.pop();
	} while (!dirs.empty());
	return ret;
}

pair<vector<string>, vector<string>> FileSys::listDirSep(string drc, FileType filter, bool showHidden, bool readLinks) {
	drc = appDsep(drc);
	vector<string> files, dirs;
#ifdef _WIN32
	if (std::all_of(drc.begin(), drc.end(), isDsep)) {	// if in "root" directory, get drive letters and present them as directories
		vector<char> letters = listDrives();
		dirs.resize(letters.size());
		for (sizet i = 0; i < dirs.size(); i++)
			dirs[i] = { letters[i], ':' };
		return pair(std::move(filter & FTYPE_DIR ? dirs : files), std::move(dirs));
	}

	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(sstow(appDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return pair(std::move(files), std::move(dirs));

	do {
		if (!isDotName(data.cFileName) && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))) {
			if (atrcmp(data.dwFileAttributes, filter))
				files.push_back(cwtos(data.cFileName));
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				dirs.push_back(cwtos(data.cFileName));
		}
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return pair(std::move(files), std::move(dirs));

	while (dirent* entry = readdir(directory))
		if (!isDotName(entry->d_name) && (showHidden || entry->d_name[0] != '.')) {
			if (dtycmp(drc, entry, filter, readLinks))
				files.emplace_back(entry->d_name);
			if (dtycmp(drc, entry, FTYPE_DIR, readLinks))
				dirs.emplace_back(entry->d_name);
		}
	closedir(directory);
#endif
	std::sort(files.begin(), files.end(), strnatless);
	std::sort(dirs.begin(), dirs.end(), strnatless);
	return pair(std::move(files), std::move(dirs));
}

string FileSys::validateFilename(string file) {
	if (isDotName(file))
		return "";
#ifdef _WIN32
	for (const char* it : takenFilenames)
		if (sizet len = strlen(it); !strncicmp(file, it, len))
			file.erase(0, len);
	file.erase(std::remove_if(file.begin(), file.end(), [](char c) -> bool { return c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*' || uchar(c) < ' '; }), file.end());

	if (file[0] == ' ')
		file.erase(0, file.find_first_not_of(' '));
	while (file.back() == ' ' || file.back() == '.')
		file.erase(file.find_last_not_of(file.back() == ' ' ? ' ' : '.'));
#else
	file.erase(std::remove_if(file.begin(), file.end(), isDsep), file.end());
#endif
	if (file.length() > fnameMax)
		file.resize(fnameMax);
	return file;
}

bool FileSys::isPicture(const string& file) {
	if (SDL_Surface* img = IMG_Load(file.c_str())) {
		SDL_FreeSurface(img);
		return true;
	}
	return false;
}

bool FileSys::isFont(const string& file) {
	if (TTF_Font* fnt = TTF_OpenFont(file.c_str(), FontSet::fontTestHeight)) {
		TTF_CloseFont(fnt);
		return true;
	}
	return false;
}

bool FileSys::isArchive(const string& file) {
	if (archive* arch = openArchive(file)) {
		archive_read_free(arch);
		return true;
	}
	return false;
}

bool FileSys::isPictureArchive(const string& file) {
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

bool FileSys::isArchivePicture(const string& file, const string& pname) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname(entry) == pname)
				if (SDL_Surface* img = loadArchivePicture(arch, entry)) {
					SDL_FreeSurface(img);
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

archive* FileSys::openArchive(const string& file) {
	archive* arch = archive_read_new();
	archive_read_support_filter_all(arch);
	archive_read_support_format_all(arch);

	if (archive_read_open_filename(arch, file.c_str(), archiveReadBlockSize)) {
		archive_read_free(arch);
		return nullptr;
	}
	return arch;
}

vector<string> FileSys::listArchive(const string& file) {
	vector<string> entries;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (const char* ename = archive_entry_pathname(entry); *ename)
				entries.emplace_back(ename);

		archive_read_free(arch);
		std::sort(entries.begin(), entries.end(), strnatless);
	}
	return entries;
}

mapFiles FileSys::listArchivePictures(const string& file, vector<string>& names) {
	mapFiles files;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (const char* ename = archive_entry_pathname(entry); SDL_Surface* img = loadArchivePicture(arch, entry)) {
				files.emplace(ename, pair(SIZE_MAX, uptrt(img->w) * uptrt(img->h) * img->format->BytesPerPixel));
				names.emplace_back(ename);
				SDL_FreeSurface(img);
			}
		archive_read_free(arch);

		std::sort(names.begin(), names.end(), strnatless);
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
	pairStr locs = proc->pop<pairStr>();
	vector<string> files = listDir(locs.first, FTYPE_ANY, true, false);

	for (uptrt i = 0, lim = files.size(); i < lim; i++) {
		if (!proc->getRun())
			break;

		World::winSys()->pushEvent(UserCode::moveProgress, reinterpret_cast<void*>(i), reinterpret_cast<void*>(lim));
		if (string path = childPath(locs.first, files[i]); rename(path.c_str(), childPath(locs.second, files[i]).c_str()))
			std::cerr << "failed no move " << path << std::endl;
	}
	World::winSys()->pushEvent(UserCode::moveFinished);
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

string FileSys::findFont(const string& font) {
	if (isFont(font))	// check if font refers to a file
		return font;

	for (const string& drc : dirFonts)	// check font directories
		if (string path; iterateDirRec(drc, [&font, &path](const string& file) -> int {
				if (!strcicmp(hasExt(file) ? delExt(filename(file)) : filename(file), font) && isFont(file)) {
					path = file;
					return 1;
				}
				return 0;
			}, FTYPE_REG, false, false))
			return path;
	return "";	// nothing found
}
#ifdef _WIN32
vector<char> FileSys::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();

	for (char i = 0; i < drivesMax; i++)
		if (drives & (1 << i))
			letters.push_back('A' + i);
	return letters;
}

FileType FileSys::fileType(const string& file, bool readLink) {
	if (std::all_of(file.begin(), file.end(), isDsep))
		return FTYPE_DIR;
	if (isDriveLetter(file)) {
		vector<char> letters = FileSys::listDrives();
		return std::find(letters.begin(), letters.end(), file[0]) != letters.end() ? FTYPE_DIR : FTYPE_NON;
	}

	DWORD attrib = GetFileAttributesW(sstow(file).c_str());
	if (attrib == INVALID_FILE_ATTRIBUTES)
		return FTYPE_NON;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
		return FTYPE_DIR;
	return FTYPE_REG;
}
#else
FileType FileSys::stmtoft(const string& file, int (*statfunc)(const char*, struct stat*)) {
	struct stat ps;
	if (statfunc(file.c_str(), &ps))
		return FTYPE_NON;

	switch (ps.st_mode & S_IFMT) {
	case S_IFDIR:
		return FTYPE_DIR;
	case S_IFCHR:
		return FTYPE_CHR;
	case S_IFBLK:
		return FTYPE_BLK;
	case S_IFREG:
		return FTYPE_REG;
	case S_IFIFO:
		return FTYPE_FIF;
	case S_IFLNK:
		return FTYPE_LNK;
	case S_IFSOCK:
		return FTYPE_SOC;
	}
	return FTYPE_UNK;
}

bool FileSys::dtycmp(const string& drc, const dirent* entry, FileType filter, bool readLink) {
	switch (entry->d_type) {
	case DT_FIFO:
		return filter & FTYPE_FIF;
	case DT_CHR:
		return filter & FTYPE_CHR;
	case DT_DIR:
		return filter & FTYPE_DIR;
	case DT_BLK:
		return filter & FTYPE_BLK;
	case DT_REG:
		return filter & FTYPE_REG;
	case DT_LNK:
		return filter & (readLink ? stmtoft(drc + entry->d_name, stat) : FTYPE_LNK);
	case DT_SOCK:
		return filter & FTYPE_SOC;
	case DT_UNKNOWN:
		return filter & stmtoft(drc + entry->d_name, readLink ? stat : lstat);
	}
	return false;
}
#endif
