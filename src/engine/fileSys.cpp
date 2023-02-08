#include "fileSys.h"
#include "drawSys.h"
#include "prog/browser.h"
#include "utils/compare.h"
#include <archive.h>
#include <archive_entry.h>
#include <queue>
#include <regex>
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fontconfig/fontconfig.h>
#endif

// INI LINE

void IniLine::setVal(string_view property, string_view value) {
	type = Type::prpVal;
	prp = property;
	key.clear();
	val = value;
}

void IniLine::setVal(string_view property, string_view vkey, string_view value) {
	type = Type::prpKeyVal;
	prp = property;
	key = vkey;
	val = value;
}

void IniLine::setTitle(string_view title) {
	type = Type::title;
	prp = title;
	key.clear();
	val.clear();
}

IniLine::Type IniLine::setLine(string_view str) {
	size_t i0 = str.find_first_of('=');
	size_t i1 = str.find_first_of('[');
	size_t i2 = str.find_first_of(']', i1);

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

vector<IniLine> IniLine::readLines(string_view text) {
	vector<IniLine> lines;
	constexpr char nl[] = "\n\r";
	for (size_t e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); size_t len = e - p)
			lines.emplace_back(string_view(text.data() + p, len));
	return lines;
}

// FILE SYS

FileSys::FileSys() {
	// set up file/directory path constants
	if (char* path = SDL_GetBasePath()) {
#ifdef _WIN32
		dirBase = fs::u8path(path);
#else
		dirBase = fs::u8path(path).parent_path().parent_path();	// one extra because of the appended directory separator
#endif
		SDL_free(path);
	}
	if (dirBase.empty())
		logError("Failed to get base directory");

#ifdef _WIN32
	dirSets = fs::path(_wgetenv(L"AppData")) / L"VertiRead";
	dirConfs = dirBase;
#else
	dirSets = fs::u8path(getenv("HOME")) / ".local/share/vertiread";
	dirConfs = dirBase / "share/vertiread";
#endif

	std::error_code ec;
	try {
		std::regex rgx(R"r(log_[\d-]+\.txt)r", std::regex::icase | std::regex::optimize);
		for (const fs::directory_entry& it : fs::directory_iterator(dirSets, fs::directory_options::skip_permission_denied))
			if (string name = it.path().filename().u8string(); std::regex_match(name, rgx) && it.is_regular_file(ec))
				if (fs::remove(it.path(), ec); ec)
					logError("Failed to remove old log file '", name, "': ", ec.message());
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}

	fs::path logPath = dirSets / ("log_" + tmToDateStr(currentDateTime()) + ".txt");
	if (logFile.open(logPath); logFile.good())
		SDL_LogSetOutputFunction(logWrite, &logFile);
	else
		logError("Failed to create log file: ", logPath);

	// check if all (more or less) necessary files and directories exist
	if (fs::create_directories(dirSets, ec))
		logError("Failed to create settings directory: ", ec.message());
	if (!fs::is_directory(dirIcons(), ec) || ec)
		logError("Failed to find icons directory: ", ec.message());
	if (!fs::is_regular_file(dirConfs / fileThemes, ec) || ec)
		logError("Failed to find themes file: ", ec.message());
}

FileSys::~FileSys() {
	SDL_LogSetOutputFunction(nullptr, nullptr);
	logFile.close();
}

vector<string> FileSys::getAvailableThemes() const {
	vector<string> themes;
	vector<IniLine> lines = IniLine::readLines(readTextFile(dirSets / fileThemes, false));
	if (lines.empty())
		lines = IniLine::readLines(readTextFile(dirConfs / fileThemes));

	for (IniLine& il : lines)
		if (il.getType() == IniLine::Type::title)
			themes.push_back(std::move(il.getPrp()));
	return !themes.empty() ? themes : vector<string>{ "default" };
}

array<vec4, Settings::defaultColors.size()> FileSys::loadColors(string_view theme) const {
	array<vec4, Settings::defaultColors.size()> colors = Settings::defaultColors;
	vector<string> lines = readFileLines(dirSets / fileThemes, false);
	if (lines.empty())
		lines = readFileLines(dirConfs / fileThemes);

	IniLine il;	// find title equal to theme and read colors until the end of the file or another title
	vector<string>::iterator it = std::find_if(lines.begin(), lines.end(), [&il, theme](const string& ln) -> bool { return il.setLine(ln) == IniLine::Type::title && il.getPrp() == theme; });
	if (it == lines.end())
		return colors;

	while (++it != lines.end()) {
		if (il.setLine(*it) == IniLine::Type::title)
			break;
		if (il.getType() == IniLine::Type::prpVal)
			if (size_t cid = strToEnum<size_t>(Settings::colorNames, il.getPrp()); cid < colors.size())
				colors[cid] = toVec<vec4>(il.getVal());
	}
	return colors;
}

tuple<bool, string, string> FileSys::getLastPage(string_view book) const {
	for (const string& line : readFileLines(dirSets / fileBooks, false))
		if (vector<string> words = strUnenclose(line); words.size() >= 2 && words[0] == book)
			return tuple(true, std::move(words[1]), words.size() >= 3 ? std::move(words[2]) : string());
	return tuple(false, string(), string());
}

bool FileSys::saveLastPage(string_view book, string_view drc, string_view fname) const {
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
	for (IniLine& il : IniLine::readLines(readTextFile(dirSets / fileSettings, false))) {
		switch (il.getType()) {
		case IniLine::Type::prpVal:
			if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordMaximized))
				sets->maximized = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordScreen))
				sets->screen = strToEnum(Settings::screenModeNames, il.getVal(), Settings::defaultScreenMode);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordResolution))
				sets->resolution = toVec<ivec2>(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordRenderer))
				sets->renderer = strToEnum(Settings::rendererNames, il.getVal(), Settings::defaultRenderer);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordDevice))
				sets->device = toVec<u32vec2>(il.getVal(), 0, 0x10);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordCompression))
				sets->compression = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordVSync))
				sets->vsync = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordGpuSelecting))
				sets->gpuSelecting = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordDirection))
				sets->direction = strToEnum(Direction::names, il.getVal(), Settings::defaultDirection);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordZoom))
				sets->zoom = toNum<float>(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordSpacing))
				sets->spacing = toNum<ushort>(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordPictureLimit))
				sets->picLim.set(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordMaxPictureRes))
				sets->maxPicRes = std::max(toNum<uint>(il.getVal()), Settings::minPicRes);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordFont))
				sets->font = FileSys::isFont(findFont(il.getVal())) ? il.getVal() : Settings::defaultFont;
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordTheme))
				sets->setTheme(il.getVal(), getAvailableThemes());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordPreview))
				sets->preview = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordShowHidden))
				sets->showHidden = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordTooltips))
				sets->tooltips = toBool(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordLibrary))
				sets->setDirLib(fs::u8path(il.getVal()), dirSets);
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordScrollSpeed))
				sets->scrollSpeed = toVec<vec2>(il.getVal());
			else if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordDeadzone))
				sets->setDeadzone(toNum<uint>(il.getVal()));
			break;
		case IniLine::Type::prpKeyVal:
			if (!SDL_strcasecmp(il.getPrp().c_str(), iniKeywordDisplay))
				sets->displays[toNum<int>(il.getKey())] = toVec<ivec4>(il.getVal());
		}
	}
	sets->unionDisplays();
	return sets;
}

void FileSys::saveSettings(const Settings* sets) const {
	fs::path path = dirSets / fileSettings;
	std::ofstream ofh(path, std::ios::binary);
	if (!ofh.good()) {
		logError("Failed to write settings file ", path);
		return;
	}
	IniLine::writeVal(ofh, iniKeywordMaximized, toStr(sets->maximized));
	IniLine::writeVal(ofh, iniKeywordScreen, Settings::screenModeNames[size_t(sets->screen)]);
	for (const auto& [id, rect] : sets->displays)
		IniLine::writeKeyVal(ofh, iniKeywordDisplay, id, toStr(rect.toVec()));
	IniLine::writeVal(ofh, iniKeywordResolution, sets->resolution.x, ' ', sets->resolution.y);
	IniLine::writeVal(ofh, iniKeywordRenderer, Settings::rendererNames[size_t(sets->renderer)]);
	IniLine::writeVal(ofh, iniKeywordDevice, toStr<0x10>(sets->device));
	IniLine::writeVal(ofh, iniKeywordCompression, toStr(sets->compression));
	IniLine::writeVal(ofh, iniKeywordVSync, toStr(sets->vsync));
	IniLine::writeVal(ofh, iniKeywordGpuSelecting, toStr(sets->gpuSelecting));
	IniLine::writeVal(ofh, iniKeywordZoom, sets->zoom);
	IniLine::writeVal(ofh, iniKeywordPictureLimit, PicLim::names[uint8(sets->picLim.type)], ' ', sets->picLim.getCount(), ' ', PicLim::memoryString(sets->picLim.getSize()));
	IniLine::writeVal(ofh, iniKeywordMaxPictureRes, toStr(sets->maxPicRes));
	IniLine::writeVal(ofh, iniKeywordSpacing, sets->spacing);
	IniLine::writeVal(ofh, iniKeywordDirection, Direction::names[uint8(sets->direction)]);
	IniLine::writeVal(ofh, iniKeywordFont, sets->font);
	IniLine::writeVal(ofh, iniKeywordTheme, sets->getTheme());
	IniLine::writeVal(ofh, iniKeywordPreview, toStr(sets->preview));
	IniLine::writeVal(ofh, iniKeywordShowHidden, toStr(sets->showHidden));
	IniLine::writeVal(ofh, iniKeywordTooltips, toStr(sets->tooltips));
	IniLine::writeVal(ofh, iniKeywordLibrary, sets->getDirLib().u8string());
	IniLine::writeVal(ofh, iniKeywordScrollSpeed, sets->scrollSpeed.x, ' ', sets->scrollSpeed.y);
	IniLine::writeVal(ofh, iniKeywordDeadzone, sets->getDeadzone());
}

array<Binding, Binding::names.size()> FileSys::getBindings() const {
	array<Binding, Binding::names.size()> bindings;
	for (size_t i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));

	for (const IniLine& il : IniLine::readLines(readTextFile(dirSets / fileBindings, false))) {
		if (il.getType() != IniLine::Type::prpVal || il.getVal().length() < 3)
			continue;
		size_t bid = strToEnum<size_t>(Binding::names, il.getPrp());
		if (bid >= bindings.size())
			continue;

		switch (toupper(il.getVal()[0])) {
		case keyKey[0]:			// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(il.getVal().c_str() + 2));
			break;
		case keyButton[0]:		// joystick button
			bindings[bid].setJbutton(toNum<uint8>(string_view(il.getVal()).substr(2)));
			break;
		case keyHat[0]:			// joystick hat
			if (size_t id = std::find_if(il.getVal().begin() + 2, il.getVal().end(), [](char c) -> bool { return !isdigit(c); }) - il.getVal().begin(); id < il.getVal().size())
				bindings[bid].setJhat(toNum<uint8>(string_view(il.getVal()).substr(2, id - 2)), strToVal(Binding::hatNames, string_view(il.getVal()).substr(id + 1)));
			break;
		case keyAxisPos[0]:		// joystick axis
			bindings[bid].setJaxis(toNum<uint8>(string_view(il.getVal()).substr(3)), il.getVal()[2] != keyAxisNeg[2]);
			break;
		case keyGButton[0]:		// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Binding::gbuttonNames, string_view(il.getVal()).substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case keyGAxisPos[0]:	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Binding::gaxisNames, string_view(il.getVal()).substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (il.getVal()[2] != keyGAxisNeg[2]));
			break;
		default:
			throw std::runtime_error("Invalid binding identifier: "s + il.getVal()[0]);
		}
	}
	return bindings;
}

void FileSys::saveBindings(const array<Binding, Binding::names.size()>& bindings) const {
	fs::path path = dirSets / fileBindings;
	std::ofstream ofh(path, std::ios::binary);
	if (!ofh.good()) {
		logError("Failed to write bindings file ", path);
		return;
	}
	for (size_t i = 0; i < bindings.size(); ++i) {
		if (bindings[i].keyAssigned())
			IniLine::writeVal(ofh, Binding::names[i], keyKey, SDL_GetScancodeName(bindings[i].getKey()));

		if (bindings[i].jbuttonAssigned())
			IniLine::writeVal(ofh, Binding::names[i], keyButton, uint(bindings[i].getJctID()));
		else if (bindings[i].jhatAssigned())
			IniLine::writeVal(ofh, Binding::names[i], keyHat, uint(bindings[i].getJctID()), keySep, Binding::hatNames.at(bindings[i].getJhatVal()));
		else if (bindings[i].jaxisAssigned())
			IniLine::writeVal(ofh, Binding::names[i], (bindings[i].jposAxisAssigned() ? keyAxisPos : keyAxisNeg), uint(bindings[i].getJctID()));

		if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofh, Binding::names[i], keyGButton, Binding::gbuttonNames[uint8(bindings[i].getGbutton())]);
		else if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofh, Binding::names[i], (bindings[i].gposAxisAssigned() ? keyGAxisPos : keyGAxisNeg), Binding::gaxisNames[uint8(bindings[i].getGaxis())]);
	}
}

vector<string> FileSys::readFileLines(const fs::path& file, bool printMessage) {
	vector<string> lines;
	constexpr char nl[] = "\n\r";
	string text = readTextFile(file, printMessage);
	for (size_t e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); size_t len = e - p)
			lines.push_back(text.substr(p, len));
	return lines;
}

string FileSys::readTextFile(const fs::path& file, bool printMessage) {
	if (std::ifstream ifh(file, std::ios::binary | std::ios::ate); ifh.good())
		if (std::streampos len = ifh.tellg(); len != -1) {
			ifh.seekg(0);

			string text;
			text.resize(len);
			if (ifh.read(text.data(), text.length()); size_t(ifh.gcount()) < text.length())
				text.resize(ifh.gcount());
			return text;
		}
	if (printMessage)
		logError("Failed to open file ", file);
	return string();
}

bool FileSys::writeTextFile(const fs::path& file, const vector<string>& lines) {
	std::ofstream ofh(file, std::ios::binary);
	if (!ofh.good()) {
		logError("Failed to write file ", file);
		return false;
	}

	for (const string& it : lines) {
		ofh.write(it.c_str(), it.length());
		ofh.write(linend.data(), linend.length());
	}
	return true;
}

vector<fs::path> FileSys::listDir(const fs::path &drc, bool files, bool dirs, bool showHidden) {
#ifdef _WIN32
	if (drc.empty())	// if in "root" directory, get drive letters and present them as directories
		return dirs ? listDrives() : vector<fs::path>();
#endif
	vector<fs::path> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((drc / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dirs : files))
				entries.emplace_back(data.cFileName);
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc.c_str())) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && (showHidden || entry->d_name[0] != '.') && ((entry->d_type != DT_LNK ? entry->d_type == DT_DIR : fs::is_directory(drc / entry->d_name)) ? dirs : files))
				entries.emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end(), StrNatCmp());
	return entries;
}

pair<vector<fs::path>, vector<fs::path>> FileSys::listDirSep(const fs::path& drc, bool showHidden) {
#ifdef _WIN32
	if (drc.empty())	// if in "root" directory, get drive letters and present them as directories
		return pair(vector<fs::path>(), listDrives());
#endif
	vector<fs::path> files, dirs;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((drc / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)))
				(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dirs : files).emplace_back(data.cFileName);
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc.c_str())) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && (showHidden || entry->d_name[0] != '.'))
				((entry->d_type != DT_LNK ? entry->d_type == DT_DIR : fs::is_directory(drc / entry->d_name)) ? dirs : files).emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	std::sort(files.begin(), files.end(), StrNatCmp());
	std::sort(dirs.begin(), dirs.end(), StrNatCmp());
	return pair(std::move(files), std::move(dirs));
}

fs::path FileSys::validateFilename(const fs::path& file) {
	string str = file.u8string();
#ifdef _WIN32
	for (string_view it : takenFilenames)
		if (!SDL_strncasecmp(str.c_str(), it.data(), it.length()))
			str.erase(0, it.length());
	str.erase(std::remove_if(str.begin(), str.end(), [](char c) -> bool { return c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*' || uchar(c) < ' '; }), str.end());

	if (str[0] == ' ')
		str.erase(0, str.find_first_not_of(' '));
	while (str.back() == ' ' || str.back() == '.')
		str.erase(str.find_last_not_of(str.back() == ' ' ? ' ' : '.'));
#else
	str.erase(std::remove_if(str.begin(), str.end(), isDsep), str.end());
#endif
	if (str.length() > 255)
		str.resize(255);
	return fs::u8path(str);
}

bool FileSys::isPicture(SDL_RWops* ifh, string_view ext) {
	constexpr int (SDLCALL* const magics[])(SDL_RWops*) = {
		IMG_isJPG,
		IMG_isPNG,
		IMG_isBMP,
		IMG_isGIF,
		IMG_isTIF,
		IMG_isWEBP,
		IMG_isCUR,
		IMG_isICO,
		IMG_isLBM,
		IMG_isPCX,
		IMG_isPNM,
		IMG_isSVG,
		IMG_isXCF,
		IMG_isXPM,
		IMG_isXV,
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		IMG_isAVIF,
		IMG_isJXL,
		IMG_isQOI
#endif
	};
	if (ifh) {
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
			if (test(ifh)) {
				SDL_RWclose(ifh);
				return true;
			}

		if (!SDL_strncasecmp(ext.data(), "TGA", 3) && !ext[3])
			if (SDL_Surface* img = IMG_LoadTGA_RW(ifh)) {
				SDL_FreeSurface(img);
				SDL_RWclose(ifh);
				return true;
			}
		SDL_RWclose(ifh);
	}
	return false;
}

bool FileSys::isFont(const fs::path& file) {
	if (std::ifstream ifs(file); ifs.good()) {
		array<char, 5> sig;
		ifs.read(sig.data(), sig.size());
		return ifs.gcount() == std::streamsize(sig.size()) && (!memcmp(sig.data(), "\x00\x01\x00\x00\x00", 5) || !memcmp(sig.data(), "OTTO", 4));
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
			if (isPicture(arch, entry)) {
				archive_read_free(arch);
				return true;
			}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isArchivePicture(const fs::path& file, string_view pname) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (isPicture(arch, entry)) {
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isPicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 && isPicture(SDL_RWFromConstMem(buffer.get(), bsiz), fileExtension(archive_entry_pathname_utf8(entry)));
}

archive* FileSys::openArchive(const fs::path& file) {
	archive* arch = archive_read_new();
	if (arch) {
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
	}
	return arch;
}

pair<uptr<uint8[]>, int64> FileSys::readArchiveEntry(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0 || archive_entry_filetype(entry) != AE_IFREG)
		return pair(nullptr, 0);

	uptr<uint8[]> buffer = std::make_unique<uint8[]>(bsiz);
	bsiz = archive_read_data(arch, buffer.get(), bsiz);
	return pair(std::move(buffer), bsiz);
}

vector<string> FileSys::listArchiveFiles(const fs::path& file) {
	vector<string> entries;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_filetype(entry) == AE_IFREG)
				entries.emplace_back(archive_entry_pathname_utf8(entry));

		archive_read_free(arch);
		std::sort(entries.begin(), entries.end(), StrNatCmp());
	}
	return entries;
}

void FileSys::makeArchiveTreeThread(std::atomic<ThreadType>& mode, BrowserResultAsync ra, uintptr_t maxRes) {
	archive* arch = openArchive(ra.curDir);
	if (!arch) {
		pushEvent(SDL_USEREVENT_ARCHIVE_FINISHED);
		return;
	}

	for (archive_entry* entry; !archive_read_next_header(arch, &entry);) {
		if (mode != ThreadType::archive) {
			archive_read_free(arch);
			return;
		}
		pushEvent(SDL_USEREVENT_ARCHIVE_PROGRESS, reinterpret_cast<void*>(archive_entry_ino(entry)));

		ArchiveDir* node = &ra.arch;
		for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
			if (const char* next = strchr(path, '/')) {
				vector<ArchiveDir>::iterator dit = std::find_if(node->dirs.begin(), node->dirs.end(), [path, next](const ArchiveDir& it) -> bool { return std::equal(it.name.begin(), it.name.end(), path, next); });
				node = dit != node->dirs.end() ? &*dit : &node->dirs.emplace_back(node, string(path, next));
				path = next + 1;
			} else {
				SDL_Surface* img = loadArchivePicture(arch, entry);
				node->files.emplace_back(path, img ? std::min(uintptr_t(img->w), maxRes) * std::min(uintptr_t(img->h), maxRes) * 4 : 0);
				SDL_FreeSurface(img);
				break;
			}
		}
	}
	archive_read_free(arch);
	ra.arch.sort();
	pushEvent(SDL_USEREVENT_ARCHIVE_FINISHED, new BrowserResultAsync(std::move(ra)));
}

SDL_Surface* FileSys::loadArchivePicture(const fs::path& file, string_view pname) {
	SDL_Surface* img = nullptr;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (img = loadArchivePicture(arch, entry); img)
					break;
		archive_read_free(arch);
	}
	return img;
}

SDL_Surface* FileSys::loadArchivePicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 ? IMG_Load_RW(SDL_RWFromMem(buffer.get(), bsiz), SDL_TRUE) : nullptr;
}

void FileSys::moveContentThread(std::atomic<ThreadType>& mode, fs::path src, fs::path dst) {
	uptr<string> errors = std::make_unique<string>();
	std::error_code ec;
	if (fs::create_directories(dst, ec); !ec) {
		vector<fs::path> files = listDir(src);
		for (uintptr_t i = 0, lim = files.size(); i < lim; ++i) {
			if (mode != ThreadType::move)
				break;

			pushEvent(SDL_USEREVENT_MOVE_PROGRESS, reinterpret_cast<void*>(i), reinterpret_cast<void*>(lim));
			if (fs::rename(src / files[i], dst / files[i], ec); ec)
				*errors += ec.message() + '\n';
		}
	} else
		*errors += ec.message() + '\n';
	if (!errors->empty())
		errors->pop_back();
	pushEvent(SDL_USEREVENT_MOVE_FINISHED, errors.release());
}

fs::path FileSys::findFont(const string& font) const {
	if (fs::path path = fs::u8path(font); isFont(path))
		return path;
#ifdef _WIN32
	return searchFontDirs(font, { dirConfs, fs::path(_wgetenv(L"LocalAppdata")) / L"\\Microsoft\\Windows\\Fonts", fs::path(_wgetenv(L"SystemDrive")) / L"\\Windows\\Fonts"});
#else
	if (fs::path path = searchFontDirs(font, { dirConfs }); !path.empty())
		return path;

	if (void* lib = dlopen("libfontconfig.so", RTLD_NOW)) {
		fs::path found;
		FcConfig* (*InitLoadConfigAndFonts)() = reinterpret_cast<decltype(InitLoadConfigAndFonts)>(dlsym(lib, "FcInitLoadConfigAndFonts"));
		FcPattern* (*NameParse)(const FcChar8*) = reinterpret_cast<decltype(NameParse)>(dlsym(lib, "FcNameParse"));
		FcBool (*ConfigSubstitute)(FcConfig*, FcPattern*, FcMatchKind) = reinterpret_cast<decltype(ConfigSubstitute)>(dlsym(lib, "FcConfigSubstitute"));
		void (*DefaultSubstitute)(FcPattern*) = reinterpret_cast<decltype(DefaultSubstitute)>(dlsym(lib, "FcDefaultSubstitute"));
		FcPattern* (*FontMatch)(FcConfig*, FcPattern*, FcResult*) = reinterpret_cast<decltype(FontMatch)>(dlsym(lib, "FcFontMatch"));
		FcResult (*PatternGetString)(FcPattern*, const char*, int, FcChar8**) = reinterpret_cast<decltype(PatternGetString)>(dlsym(lib, "FcPatternGetString"));
		void (*PatternDestroy)(FcPattern*) = reinterpret_cast<decltype(PatternDestroy)>(dlsym(lib, "FcPatternDestroy"));
		void (*ConfigDestroy)(FcConfig*) = reinterpret_cast<decltype(ConfigDestroy)>(dlsym(lib, "FcConfigDestroy"));
		if (InitLoadConfigAndFonts && NameParse && ConfigSubstitute && DefaultSubstitute && FontMatch && PatternGetString && PatternDestroy && ConfigDestroy) {
			if (FcConfig* config = InitLoadConfigAndFonts()) {
				if (FcPattern* pat = NameParse(reinterpret_cast<const FcChar8*>(font.c_str()))) {
					if (ConfigSubstitute(config, pat, FcMatchPattern)) {
						DefaultSubstitute(pat);
						if (FcResult res; FcPattern* fmpat = FontMatch(config, pat, &res)) {
							if (FcChar8* file = nullptr; PatternGetString(fmpat, FC_FILE, 0, &file) == FcResultMatch)
								if (fs::path path = fs::u8path(reinterpret_cast<char*>(file)); isFont(path))
									found = std::move(path);
							PatternDestroy(fmpat);
						}
					}
					PatternDestroy(pat);
				}
				ConfigDestroy(config);
			}
		}
		dlclose(lib);
		if (!found.empty())
			return found;
	}
	return searchFontDirs(font, { fs::u8path(getenv("HOME")) / ".fonts", fs::u8path("/usr/share/fonts") });
#endif
}

fs::path FileSys::searchFontDirs(const string& font, std::initializer_list<fs::path> dirs) {
	for (const fs::path& drc : dirs) {
		try {
			for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied))
				if (!SDL_strcasecmp(it.path().stem().u8string().c_str(), font.c_str()) && isFont(it.path()))
					return it.path();
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
	}
	return fs::path();
}

#ifdef _WIN32
vector<fs::path> FileSys::listDrives() {
	vector<fs::path> letters;
	DWORD drives = GetLogicalDrives();
	for (char i = 0; i < drivesMax; ++i)
		if (drives & (1 << i))
			letters.emplace_back(wstring{ wchar('A' + i), ':', '\\' });
	return letters;
}
#endif

void SDLCALL FileSys::logWrite(void* userdata, int, SDL_LogPriority priority, const char* message) {
	std::ofstream& ofs = *static_cast<std::ofstream*>(userdata);
	ofs << tmToTimeStr(currentDateTime()) << ' ';
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
		ofs << "VERBOSE: ";
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		ofs << "DEBUG: ";
		break;
	case SDL_LOG_PRIORITY_INFO:
		ofs << "INFO: ";
		break;
	case SDL_LOG_PRIORITY_WARN:
		ofs << "WARN: ";
		break;
	case SDL_LOG_PRIORITY_ERROR:
		ofs << "ERROR: ";
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		ofs << "CRITICAL: ";
	}
	ofs << message << std::endl;
}
