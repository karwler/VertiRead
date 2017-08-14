#include "world.h"
#include <fstream>
#include <streambuf>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
	namespace uni {		// necessary to prevent conflicts
	#include <unistd.h>
	}
#include <dirent.h>
#include <sys/types.h>
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
		bool isTitle;
		string arg, val, key;
		if (splitIniLine(line, arg, val, key, isTitle))
			if (!isTitle && !contains(themes, arg))
				themes.push_back(arg);
	}
	return themes;
}

void Filer::getColors(map<EColor, vec4c>& colors, const string& theme) {
	vector<string> lines;
	if (!readTextFile(dirExec + Default::fileThemes, lines), false)
		return;

	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == theme) {
			EColor clr = EColor(stoi(key));
			if (colors.count(clr) == 0)
				colors.insert(make_pair(clr, VideoSettings::getDefaultColor(clr)));

			vector<string> elems = getWords(val, ' ');
			if (elems.size() > 0) colors[clr].x = stoi(elems[0]);
			if (elems.size() > 1) colors[clr].y = stoi(elems[1]);
			if (elems.size() > 2) colors[clr].z = stoi(elems[2]);
			if (elems.size() > 3) colors[clr].a = stoi(elems[3]);
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
		bool isTitle;
		string arg, val, key;
		if (splitIniLine(line, arg, val, key, isTitle) && !isTitle)
			translation.insert(make_pair(arg, val));
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

map<string, Texture> Filer::getTextures() {
	map<string, Texture> texes;
	for (string& it : listDir(dirTexs, FTYPE_FILE)) {
		string path = dirTexs + it;
		if (SDL_Surface* surf = IMG_Load(path.c_str()))	// add only valid textures
			texes.insert(make_pair(delExt(it), Texture(path, surf)));
	}
	return texes;
}

vector<string> Filer::getPics(const string& dir) {
	if (fileType(dir) != FTYPE_DIR)
		return {};

	vector<string> pics = listDir(dir, FTYPE_FILE);
	for (string& it : pics)
		it = dir + it;
	return pics;
}

Playlist Filer::getPlaylist(const string& name) {
	vector<string> lines;
	if (!readTextFile(World::library()->getSettings().playlistPath() + name + ".ini", lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == Default::iniKeywordBook)
			plist.books.push_back(val);
		else if (arg == Default::iniKeywordSong)
			plist.songs.push_back(val);	// AudioSys will check if the song is playable cause we want all lines for the playlist editor
	}
	return plist;
}

void Filer::savePlaylist(const Playlist& plist) {
	vector<string> lines;
	for (const string& name : plist.books)
		lines.push_back(Default::iniKeywordSong + string("=") + name);
	for (const string& file : plist.songs)
		lines.push_back(Default::iniKeywordSong + string("=") + file);

	writeTextFile(World::library()->getSettings().playlistPath() + plist.name + ".ini", lines);
}

GeneralSettings Filer::getGeneralSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileGeneralSettings, lines, false))
		return GeneralSettings();

	GeneralSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == Default::iniKeywordLanguage)
			sets.setLang(val);
		else if (arg == Default::iniKeywordLibrary)
			sets.setDirLib(val);
		else if (arg == Default::iniKeywordPlaylists)
			sets.setDirPlist(val);
	}
	return sets;
}

void Filer::saveSettings(const GeneralSettings& sets) {
	vector<string> lines = {
		Default::iniKeywordLanguage + string("=") + sets.getLang(),
		Default::iniKeywordLibrary + string("=") + sets.getDirLib(),
		Default::iniKeywordPlaylists + string("=") + sets.getDirPlist()
	};
	writeTextFile(dirSets + Default::fileGeneralSettings, lines);
}

VideoSettings Filer::getVideoSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileVideoSettings, lines, false))
		return VideoSettings();

	VideoSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == Default::iniKeywordFont)
			sets.setFont(val);
		else if (arg == Default::iniKeywordRenderer)
			sets.renderer = val;
		else if (arg == Default::iniKeywordMaximized)
			sets.maximized = stob(val);
		else if (arg == Default::iniKeywordFullscreen)
			sets.fullscreen = stob(val);
		else if (arg == Default::iniKeywordResolution) {
			vector<string> elems = getWords(val, ' ');
			if (elems.size() > 0) sets.resolution.x = stoi(elems[0]);
			if (elems.size() > 1) sets.resolution.y = stoi(elems[1]);
		}
		else if (arg == Default::iniKeywordTheme)
			sets.theme = val;
	}
	getColors(sets.colors, sets.theme);
	return sets;
}

void Filer::saveSettings(const VideoSettings& sets) {
	vector<string> lines = {
		Default::iniKeywordFont + string("=") + sets.getFont(),
		Default::iniKeywordRenderer + string("=") + sets.renderer,
		Default::iniKeywordMaximized + string("=") + btos(sets.maximized),
		Default::iniKeywordFullscreen + string("=") + btos(sets.fullscreen),
		Default::iniKeywordResolution + string("=") + to_string(sets.resolution.x) + ' ' + to_string(sets.resolution.y),
		Default::iniKeywordTheme + string("=") + sets.theme
	};
	writeTextFile(dirSets + Default::fileVideoSettings, lines);
}

AudioSettings Filer::getAudioSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileAudioSettings, lines, false))
		return AudioSettings();

	AudioSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == Default::iniKeywordVolMusic)
			sets.musicVolume = stoi(val);
		else if (arg == Default::iniKeywordVolSound)
			sets.soundVolume = stoi(val);
		else if (arg == Default::iniKeywordSongDelay)
			sets.songDelay = stof(val);
	}
	return sets;
}

void Filer::saveSettings(const AudioSettings& sets) {
	vector<string> lines = {
		Default::iniKeywordVolMusic + string("=") + to_string(sets.musicVolume),
		Default::iniKeywordVolSound + string("=") + to_string(sets.soundVolume),
		Default::iniKeywordSongDelay + string("=") + to_string(sets.songDelay)
	};
	writeTextFile(dirSets + Default::fileAudioSettings, lines);
}

ControlsSettings Filer::getControlsSettings() {
	vector<string> lines;
	if (!readTextFile(dirSets + Default::fileControlsSettings, lines, false))
		return ControlsSettings();

	ControlsSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, arg, val, key, isTitle) || isTitle)
			continue;

		if (arg == Default::iniKeywordScrollSpeed) {
			vector<string> elems = getWords(val, ' ');
			if (elems.size() > 0)
				sets.scrollSpeed.x = stof(elems[0]);
			if (elems.size() > 1)
				sets.scrollSpeed.y = stof(elems[1]);
		}
		else if (arg == Default::iniKeywordDeadzone)
			sets.deadzone = stoi(val);
		else if (arg == Default::iniKeywordShortcut && sets.shortcuts.count(key) != 0) {		// shortcuts have to already contain a variable for this key
			Shortcut* sc = sets.shortcuts[key];
			switch (toupper(val[0])) {
			case 'K':	// keyboard key
				sc->setKey(SDL_GetScancodeFromName(val.substr(2).c_str()));
				break;
			case 'B':	// joystick button
				sc->setJbutton(stoi(val.substr(2)));
				break;
			case 'H':	// joystick hat
				for (size_t i=2; i<val.size(); i++)
					if (val[i] < '0' || val[i] > '9') {
						sc->setJhat(stoi(val.substr(2, i-2)), jtStrToHat(val.substr(i+1)));
						break;
					}
				break;
			case 'A':	// joystick axis
				sc->setJaxis(stoi(val.substr(3)), (val[2] != '-'));
				break;
			case 'G':	// gamepad button
				sc->gbutton(gpStrToButton(val.substr(2)));
				break;
			case 'X':	// gamepad axis
				sc->setGaxis(gpStrToAxis(val.substr(3)), (val[2] != '-'));
			}
		}
	}
	return sets;
}

void Filer::saveSettings(const ControlsSettings& sets) {
	vector<string> lines = {
		Default::iniKeywordShortcut + string("=") + to_string(sets.scrollSpeed.x) + " " + to_string(sets.scrollSpeed.y),
		Default::iniKeywordDeadzone + string("=") + to_string(sets.deadzone)
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
			lines.push_back(Default::iniKeywordShortcut + string("[") + it.first + "]=" + val);
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
	return std::remove(path.c_str());
}

bool Filer::rename(const string& path, const string& newPath) {
	return std::rename(path.c_str(), newPath.c_str());
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
	ulong attrib = GetFileAttributesW(stow(path).c_str());
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
	int len = uni::readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
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
