#include "filer.h"
#include "world.h"
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

// FILE TYPE

EFileType operator~(EFileType a) {
	return static_cast<EFileType>(~static_cast<uint8>(a));
}
EFileType operator&(EFileType a, EFileType b) {
	return static_cast<EFileType>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EFileType operator&=(EFileType& a, EFileType b) {
	return a = static_cast<EFileType>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EFileType operator^(EFileType a, EFileType b) {
	return static_cast<EFileType>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EFileType operator^=(EFileType& a, EFileType b) {
	return a = static_cast<EFileType>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EFileType operator|(EFileType a, EFileType b) {
	return static_cast<EFileType>(static_cast<uint8>(a) | static_cast<uint8>(b));
}
EFileType operator|=(EFileType& a, EFileType b) {
	return a = static_cast<EFileType>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// FILER

const string Filer::dirExec = Filer::getDirExec();

#ifdef _WIN32
const string Filer::dirSets = string(std::getenv("AppData")) + "\\"+Default::titleDefault+"\\";
#else
const string Filer::dirSets = string(std::getenv("HOME")) + "/."+Default::titleExtra+"/";
#endif

const string Filer::dirLangs = Filer::dirExec + Default::dirLanguages + dsep;
const string Filer::dirSnds = Filer::dirExec + Default::dirSounds + dsep;
const string Filer::dirTexs = Filer::dirExec + Default::dirTextures + dsep;

#ifdef _WIN32
const vector<string> Filer::dirFonts = {string(std::getenv("SystemDrive")) + "\\Windows\\Fonts\\"};
#else
const vector<string> Filer::dirFonts = {"/usr/share/fonts/"};
#endif

void Filer::checkDirectories(const GeneralSettings& sets) {
	if (!fileExists(dirSets))
		mkDir(dirSets);
	if (!fileExists(sets.libraryPath()))
		mkDir(sets.libraryPath());
	if (!fileExists(sets.playlistPath()))
		mkDir(sets.playlistPath());
	if (!fileExists(dirExec + Default::fileThemes))
		cerr << "couldn't find themes.ini" << endl;
	if (!fileExists(dirLangs))
		cerr << "couldn't find translation directory" << endl;
	if (!fileExists(dirSnds))
		cerr << "couldn't find sound directory" << endl;
	if (!fileExists(dirTexs))
		cerr << "couldn't find texture directory" << endl;
}

vector<string> Filer::getAvailibleThemes() {
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines))
		return {};

	vector<string> themes;
	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.type != IniLine::Type::title && !contains(themes, il.arg))
			themes.push_back(il.arg);
	}
	return themes;
}

void Filer::getColors(map<EColor, SDL_Color>& colors, const string& theme) {
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines), false)
		return;

	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == theme) {
			EColor clr = EColor(stoi(il.key));
			if (colors.count(clr) == 0)
				colors.insert(make_pair(clr, VideoSettings::getDefaultColor(clr)));
			SDL_Color& color = colors[clr];

			vector<string> elems = getWords(il.val, ' ');
			if (elems.size() > 0) color.r = stoi(elems[0]);
			if (elems.size() > 1) color.g = stoi(elems[1]);
			if (elems.size() > 2) color.b = stoi(elems[2]);
			if (elems.size() > 3) color.a = stoi(elems[3]);
		}
	}
}

vector<string> Filer::getAvailibleLanguages() {
	vector<string> files = {Default::language};
	if (!fileExists(dirLangs))
		return {};

	for (string& it : listDir(dirLangs, FTYPE_FILE, {".ini"}))
		files.push_back(delExt(it));
	return files;
}

map<string, string> Filer::getLines(const string& language) {
	vector<string> lines;
	if (!readTextFile(dirLangs + language + ".ini", lines, false))
		return map<string, string>();

	map<string, string> translation;
	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.type != IniLine::Type::title)
			translation.insert(make_pair(il.arg, il.val));
	}
	return translation;
}

map<string, Mix_Chunk*> Filer::getSounds() {
	map<string, Mix_Chunk*> sounds;
	for (string& it : listDir(dirSnds, FTYPE_FILE))
		if (Mix_Chunk* cue = Mix_LoadWAV(string(dirSnds+it).c_str()))	// add only valid sound files
			sounds.insert(make_pair(delExt(it), cue));
	return sounds;
}

Playlist Filer::getPlaylist(const string& name) {
	vector<string> lines;
	if (!readTextFile(World::library()->getSettings().playlistPath() + name + ".ini", lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == Default::iniKeywordBook)
			plist.books.push_back(il.val);
		else if (il.arg == Default::iniKeywordSong)
			plist.songs.push_back(il.val);	// AudioSys will check if the song is playable cause we want all lines for the playlist editor
	}
	return plist;
}

void Filer::savePlaylist(const Playlist& plist) {
	vector<string> lines;
	for (const string& name : plist.books)
		lines.push_back(IniLine(Default::iniKeywordSong, name).line());
	for (const string& file : plist.songs)
		lines.push_back(IniLine(Default::iniKeywordSong, file).line());

	writeTextFile(World::library()->getSettings().playlistPath() + plist.name + ".ini", lines);
}

string Filer::getLastPage(const string& book) {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileLastPages, lines, false))
		return "";

	for (string& line : lines) {
		IniLine il;
		if (il.setLine(line) && il.type != IniLine::Type::title && il.arg == book)
			return il.val;
	}
	return "";
}

void Filer::saveLastPage(const string& file) {
	vector<string> lines;
	readTextFile(dirSets + Default::fileLastPages, lines, false);

	// find line to replace or push back
	size_t id = lines.size();
	string book = getBook(file);
	for (size_t i=0; i!=lines.size(); i++) {
		IniLine il;
		if (il.setLine(lines[i]) && il.type != IniLine::Type::title && il.arg == book) {
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

GeneralSettings Filer::getGeneralSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileGeneralSettings, lines, false))
		return GeneralSettings();

	GeneralSettings sets;
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == Default::iniKeywordLanguage)
			sets.setLang(il.val);
		else if (il.arg == Default::iniKeywordLibrary)
			sets.setDirLib(il.val);
		else if (il.arg == Default::iniKeywordPlaylists)
			sets.setDirPlist(il.val);
	}
	return sets;
}

void Filer::saveSettings(const GeneralSettings& sets) {
	vector<string> lines = {
		IniLine(Default::iniKeywordLanguage, sets.getLang()).line(),
		IniLine(Default::iniKeywordLibrary, sets.getDirLib()).line(),
		IniLine(Default::iniKeywordPlaylists, sets.getDirPlist()).line()
	};
	writeTextFile(dirSets + Default::fileGeneralSettings, lines);
}

VideoSettings Filer::getVideoSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileVideoSettings, lines, false))
		return VideoSettings();

	VideoSettings sets;
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == Default::iniKeywordFont)
			sets.setFont(il.val);
		else if (il.arg == Default::iniKeywordRenderer)
			sets.renderer = il.val;
		else if (il.arg == Default::iniKeywordMaximized)
			sets.maximized = stob(il.val);
		else if (il.arg == Default::iniKeywordFullscreen)
			sets.fullscreen = stob(il.val);
		else if (il.arg == Default::iniKeywordResolution) {
			vector<string> elems = getWords(il.val, ' ');
			if (elems.size() > 0) sets.resolution.x = stoi(elems[0]);
			if (elems.size() > 1) sets.resolution.y = stoi(elems[1]);
		} else if (il.arg == Default::iniKeywordTheme)
			sets.theme = il.val;
	}
	getColors(sets.colors, sets.theme);
	return sets;
}

void Filer::saveSettings(const VideoSettings& sets) {
	vector<string> lines = {
		IniLine(Default::iniKeywordFont, sets.getFont()).line(),
		IniLine(Default::iniKeywordRenderer, sets.renderer).line(),
		IniLine(Default::iniKeywordMaximized, btos(sets.maximized)).line(),
		IniLine(Default::iniKeywordFullscreen, btos(sets.fullscreen)).line(),
		IniLine(Default::iniKeywordResolution, to_string(sets.resolution.x) + ' ' + to_string(sets.resolution.y)).line(),
		IniLine(Default::iniKeywordTheme, sets.theme).line()
	};
	writeTextFile(dirSets + Default::fileVideoSettings, lines);
}

AudioSettings Filer::getAudioSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileAudioSettings, lines, false))
		return AudioSettings();

	AudioSettings sets;
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == Default::iniKeywordVolMusic)
			sets.musicVolume = stoi(il.val);
		else if (il.arg == Default::iniKeywordVolSound)
			sets.soundVolume = stoi(il.val);
		else if (il.arg == Default::iniKeywordSongDelay)
			sets.songDelay = stof(il.val);
	}
	return sets;
}

void Filer::saveSettings(const AudioSettings& sets) {
	vector<string> lines = {
		IniLine(Default::iniKeywordVolMusic, to_string(sets.musicVolume)).line(),
		IniLine(Default::iniKeywordVolSound, to_string(sets.soundVolume)).line(),
		IniLine(Default::iniKeywordSongDelay, to_string(sets.songDelay)).line()
	};
	writeTextFile(dirSets + Default::fileAudioSettings, lines);
}

ControlsSettings Filer::getControlsSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileControlsSettings, lines, false))
		return ControlsSettings();

	ControlsSettings sets;
	for (string& line : lines) {
		IniLine il;
		if (!il.setLine(line) || il.type == IniLine::Type::title)
			continue;

		if (il.arg == Default::iniKeywordScrollSpeed) {
			vector<string> elems = getWords(il.val, ' ');
			if (elems.size() > 0)
				sets.scrollSpeed.x = stof(elems[0]);
			if (elems.size() > 1)
				sets.scrollSpeed.y = stof(elems[1]);
		} else if (il.arg == Default::iniKeywordDeadzone)
			sets.deadzone = stoi(il.val);
		else if (il.arg == Default::iniKeywordShortcut && sets.shortcuts.count(il.key) != 0) {		// shortcuts have to already contain a variable for this key
			Shortcut* sc = sets.shortcuts[il.key];
			switch (toupper(il.val[0])) {
			case 'K':	// keyboard key
				sc->setKey(SDL_GetScancodeFromName(il.val.substr(2).c_str()));
				break;
			case 'B':	// joystick button
				sc->setJbutton(stoi(il.val.substr(2)));
				break;
			case 'H':	// joystick hat
				for (size_t i=2; i<il.val.size(); i++)
					if (il.val[i] < '0' || il.val[i] > '9') {
						sc->setJhat(stoi(il.val.substr(2, i-2)), jtStrToHat(il.val.substr(i+1)));
						break;
					}
				break;
			case 'A':	// joystick axis
				sc->setJaxis(stoi(il.val.substr(3)), (il.val[2] != '-'));
				break;
			case 'G':	// gamepad button
				sc->gbutton(gpStrToButton(il.val.substr(2)));
				break;
			case 'X':	// gamepad axis
				sc->setGaxis(gpStrToAxis(il.val.substr(3)), (il.val[2] != '-'));
			}
		}
	}
	return sets;
}

void Filer::saveSettings(const ControlsSettings& sets) {
	vector<string> lines = {
		IniLine(Default::iniKeywordShortcut, to_string(sets.scrollSpeed.x) + " " + to_string(sets.scrollSpeed.y)).line(),
		IniLine(Default::iniKeywordDeadzone, to_string(sets.deadzone)).line()
	};
	for (const pair<string, Shortcut*>& it : sets.shortcuts) {
		vector<string> values;
		if (it.second->keyAssigned())
			values.push_back("K_" + string(SDL_GetScancodeName(it.second->getKey())));

		if (it.second->jbuttonAssigned())
			values.push_back("B_" + to_string(it.second->getJctID()));
		else if (it.second->jhatAssigned())
			values.push_back("H_" + to_string(it.second->getJctID()) + "_" + jtHatToStr(it.second->getJhatVal()));
		else if (it.second->jaxisAssigned())
			values.push_back(string(it.second->jposAxisAssigned() ? "A_+" : "A_-") + to_string(it.second->getJctID()));

		if (it.second->gbuttonAssigned())
			values.push_back("G_" + gpButtonToStr(it.second->getGctID()));
		else if (it.second->gbuttonAssigned())
			values.push_back(string(it.second->gposAxisAssigned() ? "X_+" : "X_-") + gpAxisToStr(it.second->getGctID()));

		for (string& val : values)
			lines.push_back(IniLine(Default::iniKeywordShortcut, it.first, val).line());
	}
	writeTextFile(dirSets + Default::fileControlsSettings, lines);
}

bool Filer::readTextFile(const string& file, string& data) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		cerr << "couldn't opem file " << file << endl;
		return false;
	}
	data = string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return true;
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

bool Filer::remove(const string& path) {
	return remove(path.c_str());
}

bool Filer::rename(const string& path, const string& newPath) {
	return rename(path.c_str(), newPath.c_str());
}

vector<string> Filer::listDir(const string& dir, EFileType filter, const vector<string>& extFilter) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(dir+"*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return {};
	do {
		if (data.cFileName == wstring(L".") || data.cFileName == wstring(L".."))
			continue;
		
		string name = wtos(data.cFileName);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (filter & FTYPE_DIR)
				entries.push_back(name);
		} else if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			if (filter & FTYPE_LINK)
				entries.push_back(name);
		} else if (filter & FTYPE_FILE) {
			if (extFilter.empty())
				entries.push_back(name);
			else
				for (const string& ext : extFilter)
					if (hasExt(name, ext)) {
						entries.push_back(name);
						break;
					}
		}
	} while (FindNextFileW(hFind, &data) != 0);
	FindClose(hFind);
#else
	DIR* directory = opendir(dir.c_str());
	if (directory) {
		dirent* data = readdir(directory);
		while (data) {
			if (data->d_name == string(".") || data->d_name == string("..")) {
				data = readdir(directory);
				continue;
			}

			if (data->d_type == DT_DIR) {
				if (filter & FTYPE_DIR)
					entries.push_back(data->d_name);
			} else if (data->d_type == DT_LNK) {
				if (filter & FTYPE_LINK)
					entries.push_back(data->d_name);
			} else if (filter & FTYPE_FILE) {
				if (extFilter.empty())
					entries.push_back(data->d_name);
				else
					for (const string& ext : extFilter)
						if (hasExt(data->d_name, ext)) {
							entries.push_back(data->d_name);
							break;
						}
			}
			data = readdir(directory);
		}
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end());
	return entries;
}

vector<string> Filer::listDirRecursively(const string& dir, size_t offs) {
	if (offs == 0)
		offs = dir.length();
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(dir+"*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return {};
	do {
		if (data.cFileName == wstring(L".") || data.cFileName == wstring(L".."))
			continue;

		string name = wtos(data.cFileName);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			vector<string> newEs = listDirRecursively(dir+name+dsep, offs);
			std::sort(entries.begin(), entries.end());
			entries.insert(entries.end(), newEs.begin(), newEs.end());
		} else
			entries.push_back(dir.substr(offs) + name);
	} while (FindNextFileW(hFind, &data) != 0);
	FindClose(hFind);
#else
	DIR* directory = opendir(dir.c_str());
	if (directory) {
		dirent* data = readdir(directory);
		while (data) {
			if (data->d_name == string(".") || data->d_name == string("..")) {
				data = readdir(directory);
				continue;
			}

			if (data->d_type == DT_DIR) {
				vector<string> newEs = listDirRecursively(dir+data->d_name+dsep, offs);
				std::sort(entries.begin(), entries.end());
				entries.insert(entries.end(), newEs.begin(), newEs.end());
			} else
				entries.push_back(dir.substr(offs) + data->d_name);

			data = readdir(directory);
		}
		closedir(directory);
	}
#endif
	return entries;
}

EFileType Filer::fileType(const string& path) {
#ifdef _WIN32
	DWORD attrib = GetFileAttributesW(stow(path).c_str());
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
		return FTYPE_DIR;
	if (attrib & FILE_ATTRIBUTE_SPARSE_FILE)
		return FTYPE_LINK;
#else
	struct stat ps;
	stat(path.c_str(), &ps);
	if (S_ISDIR(ps.st_mode))
		return FTYPE_DIR;
	if (S_ISLNK(ps.st_mode))
		return FTYPE_LINK;
#endif
	return FTYPE_FILE;
}

bool Filer::fileExists(const string& path) {
#ifdef _WIN32
	return GetFileAttributesW(stow(path).c_str()) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat ps;
	return stat(path.c_str(), &ps) == 0;
#endif
}

#ifdef _WIN32
vector<char> Filer::listDrives() {
	vector<char> letters;
	DWORD drives = GetLogicalDrives();
	
	for (char i=0; i!=26; i++)
		if (drives & (1 << i))
			letters.push_back('A'+i);
	return letters;
}
#endif

string Filer::getDirExec() {
	string path;
#ifdef _WIN32
	wchar buffer[Default::dirExecMaxBufferLength];
	GetModuleFileNameW(0, buffer, Default::dirExecMaxBufferLength);
	path = wtos(buffer);
#else
	char buffer[Default::dirExecMaxBufferLength];
	int len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
	buffer[len] = '\0';
	path = buffer;
#endif
	return parentPath(path);
}

string Filer::findFont(const string& font) {
	if (isAbsolute(font)) {	// check fontpath first
		if (fileType(font) == FTYPE_FILE)
			return font;
		return checkDirForFont(filename(font), parentPath(font));
	}

	for (const string& dir : dirFonts) {	// check global font directories
		string file = checkDirForFont(font, dir);
		if (!file.empty())
			return file;
	}
	return "";
}

string Filer::checkDirForFont(const string& font, const string& dir) {
	for (string& it : listDirRecursively(dir)) {
		string file = findChar(font, '.') ? filename(it) : delExt(filename(it));
		if (strcmpCI(file, font))
			return dir + it;
	}
	return "";
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
