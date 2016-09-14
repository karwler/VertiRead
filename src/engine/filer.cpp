#include "world.h"
#include <fstream>
#include <streambuf>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
	#ifdef __APPLE__
	#include <mach-o/dyld.h>
	#else
	#include <unistd.h>
	#endif
#endif

// DIR FILTER

EDirFilter operator~(EDirFilter a) {
	return static_cast<EDirFilter>(~static_cast<uint8>(a));
}
EDirFilter operator&(EDirFilter a, EDirFilter b) {
	return static_cast<EDirFilter>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EDirFilter operator&=(EDirFilter& a, EDirFilter b) {
	return a = static_cast<EDirFilter>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
EDirFilter operator^(EDirFilter a, EDirFilter b) {
	return static_cast<EDirFilter>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EDirFilter operator^=(EDirFilter& a, EDirFilter b) {
	return a = static_cast<EDirFilter>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
EDirFilter operator|(EDirFilter a, EDirFilter b) {
	return static_cast<EDirFilter>(static_cast<uint8>(a) | static_cast<uint8>(b));
}
EDirFilter operator|=(EDirFilter& a, EDirFilter b) {
	return a = static_cast<EDirFilter>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// FILER

const string Filer::dirExec = Filer::GetDirExec();

#ifdef __APPLE__
const string Filer::dirData = GetDirExec(true) + "../Resources/";
#else
const string Filer::dirData = dirExec;
#endif

#ifdef _WIN32
const string Filer::dirSets = string(getenv("AppData")) + "\\VertiRead\\";
#elif __APPLE__
const string Filer::dirSets = string(getenv("HOME")) + "/Library/Application Support/VertiRead/";
#else
const string Filer::dirSets = string(getenv("HOME")) + "/.vertiread/";
#endif

const string Filer::dirLangs = Filer::dirData + "languages"+dsep;
const string Filer::dirSnds = Filer::dirData + "sounds"+dsep;
const string Filer::dirTexs = Filer::dirData + "textures"+dsep;

uint8 Filer::CheckDirectories(const GeneralSettings& sets) {
	uint8 retval = 0;
	if (!Exists(dirSets))
		MkDir(dirSets);
	if (!Exists(sets.LibraryPath()))
		MkDir(sets.LibraryPath());
	if (!Exists(sets.PlaylistPath()))
		MkDir(sets.PlaylistPath());
	if (!Exists(dirData + "themes.ini")) {
		cerr << "couldn't find themes.ini" << endl;
		retval |= 1;
	}
	if (!Exists(dirLangs)) {
		cerr << "couldn't find translation directory" << endl;
		retval |= 2;
	}
	if (!Exists(dirSnds)) {
		cerr << "couldn't find sound directory" << endl;
		retval |= 4;
	}
	if (!Exists(dirTexs)) {
		cerr << "couldn't find texture directory" << endl;
		retval |= 8;
	}
	return retval;
}

vector<string> Filer::GetAvailibleThemes() {
	vector<string> lines;
	if (!ReadTextFile(dirData + "themes.ini", lines))
		return {};

	vector<string> themes;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (splitIniLine(line, &arg, &val, &key, &isTitle))
			if (!isTitle && !contains(themes, arg))
				themes.push_back(arg);
	}
	std::sort(themes.begin(), themes.end());
	return themes;
}

void Filer::GetColors(map<EColor, vec4b>& colors, const string& theme) {
	vector<string> lines;
	if (!ReadTextFile(dirData + "themes.ini", lines), false)
		return;

	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, &arg, &val, &key, &isTitle) || isTitle)
			continue;

		if (arg == theme) {
			EColor clr = EColor(stoi(key));
			if (colors.count(clr) == 0)
				colors.insert(make_pair(clr, VideoSettings::GetDefaultColor(clr)));

			vector<string> elems = getWords(val, ' ', ',');
			if (elems.size() > 0) colors[clr].x = stoi(elems[0]);
			if (elems.size() > 1) colors[clr].y = stoi(elems[1]);
			if (elems.size() > 2) colors[clr].z = stoi(elems[2]);
			if (elems.size() > 3) colors[clr].a = stoi(elems[3]);
		}
	}
}

vector<string> Filer::GetAvailibleLanguages() {
	vector<string> files = { "english" };
	if (!Exists(dirLangs))
		return files;

	for (string& it : ListDir(dirLangs, FILTER_FILE, {"ini"}))
		files.push_back(delExt(it));
	sort(files.begin(), files.end());
	return files;
}

map<string, string> Filer::GetLines(const string& language) {
	vector<string> lines;
	if (!ReadTextFile(dirLangs + language + ".ini", lines, false))
		return map<string, string>();

	map<string, string> translation;
	for (string& line : lines) {
		bool isTitle;
		string arg, val;
		if (splitIniLine(line, &arg, &val, nullptr, &isTitle) && !isTitle)
			translation.insert(make_pair(arg, val));
	}
	return translation;
}

map<string, Mix_Chunk*> Filer::GetSounds() {
	map<string, Mix_Chunk*> sounds;
	for (string& it : ListDir(dirSnds, FILTER_FILE))
		if (Mix_Chunk* cue = Mix_LoadWAV(string(dirLangs+it).c_str()))				// add only valid sound files
			sounds.insert(make_pair(delExt(it), cue));
	return sounds;
}

map<string, Texture> Filer::GetTextures() {
	map<string, Texture> texes;
	for (string& it : ListDir(dirTexs, FILTER_FILE)) {
		string path = dirTexs + it;
		if (SDL_Surface* surf = IMG_Load(path.c_str()))				// add only valid textures
			texes.insert(make_pair(delExt(it), Texture(path, surf)));
	}
	return texes;
}

vector<string> Filer::GetPics(const string& dir) {
	vector<string> pics;
	if (FileType(dir) != EFileType::dir)
		return pics;

	pics = ListDir(dir, FILTER_FILE);
	for (string& it : pics)
		it = dir + it;
	return pics;
}

Playlist Filer::LoadPlaylist(const string& name) {
	vector<string> lines;
	if (!ReadTextFile(World::scene()->Settings().PlaylistPath() + name, lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		bool isTitle;
		string arg, val;
		if (!splitIniLine(line, &arg, &val, nullptr, &isTitle) || isTitle)
			continue;

		if (arg == "book")
			plist.books.push_back(val);
		else if (arg == "song")
			plist.songs.push_back(line);
	}
	return plist;
}

void Filer::SavePlaylist(const Playlist& plist) {
	vector<string> lines;
	for (const string& name : plist.books)
		lines.push_back("book=" + name);
	for (const string& file : plist.songs)
		lines.push_back("song=" + file);

	WriteTextFile(World::scene()->Settings().PlaylistPath() + plist.name, lines);
}

GeneralSettings Filer::LoadGeneralSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets + "general.ini", lines, false))
		return GeneralSettings();

	GeneralSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val;
		if (!splitIniLine(line, &arg, &val, nullptr, &isTitle) || isTitle)
			continue;

		if (arg == "language")
			sets.Lang(val);
		else if (arg == "library")
			sets.DirLib(val);
		else if (arg == "playlists")
			sets.DirPlist(val);
	}
	return sets;
}

void Filer::SaveSettings(const GeneralSettings& sets) {
	vector<string> lines = {
		"language=" + sets.Lang(),
		"library=" + sets.DirLib(),
		"playlists=" + sets.DirPlist()
	};
	WriteTextFile(dirSets + "general.ini", lines);
}

VideoSettings Filer::LoadVideoSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets + "video.ini", lines, false))
		return VideoSettings();

	VideoSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, &arg, &val, &key, &isTitle) || isTitle)
			continue;

		if (arg == "font")
			sets.SetFont(val);
		else if (arg == "renderer")
			sets.renderer = val;
		else if (arg == "maximized")
			sets.maximized = stob(val);
		else if (arg == "fullscreen")
			sets.fullscreen = stob(val);
		else if (arg == "resolution") {
			vector<string> elems = getWords(val, ' ', ',');
			if (elems.size() > 0) sets.resolution.x = stoi(elems[0]);
			if (elems.size() > 1) sets.resolution.y = stoi(elems[1]);
		}
		else if (arg == "theme")
			sets.theme = val;
	}
	GetColors(sets.colors, sets.theme);
	return sets;
}

void Filer::SaveSettings(const VideoSettings& sets) {
	vector<string> lines = {
		"font=" + sets.Font(),
		"renderer=" + sets.renderer,
		"maximized=" + btos(sets.maximized),
		"fullscreen=" + btos(sets.fullscreen),
		"resolution=" + to_string(sets.resolution.x) + ' ' + to_string(sets.resolution.y),
		"theme=" + sets.theme
	};
	WriteTextFile(dirSets + "video.ini", lines);
}

AudioSettings Filer::LoadAudioSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets + "audio.ini", lines, false))
		return AudioSettings();

	AudioSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val;
		if (!splitIniLine(line, &arg, &val, nullptr, &isTitle) || isTitle)
			continue;

		if (arg == "music_vol")
			sets.musicVolume = stoi(val);
		else if (arg == "interface_vol")
			sets.soundVolume = stoi(val);
		else if (arg == "song_delay")
			sets.songDelay = stof(val);
	}
	return sets;
}

void Filer::SaveSettings(const AudioSettings& sets) {
	vector<string> lines = {
		"music_vol=" + to_string(sets.musicVolume),
		"interface_vol=" + to_string(sets.soundVolume),
		"song_delay=" + to_string(sets.songDelay)
	};
	WriteTextFile(dirSets + "audio.ini", lines);
}

ControlsSettings Filer::LoadControlsSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets + "controls.ini", lines, false))
		return ControlsSettings();

	ControlsSettings sets;
	for (string& line : lines) {
		bool isTitle;
		string arg, val, key;
		if (!splitIniLine(line, &arg, &val, &key, &isTitle) || isTitle)
			continue;

		if (arg == "scroll_speed") {
			vector<string> elems = getWords(val, ' ', ',');
			if (elems.size() > 0) sets.scrollSpeed.x = stof(elems[0]);
			if (elems.size() > 1) sets.scrollSpeed.y = stof(elems[1]);
		}
		else if (arg == "shortcut" && sets.shortcuts.count(key) != 0) {		// shortcuts have to already contain a variable for this key
			Shortcut* sc = sets.shortcuts[key];
			switch (toupper(val[0])) {
			case 'K':	// keyboard key
				sc->Key(SDL_GetScancodeFromName(val.substr(2).c_str()));
				break;
			case 'B':	// controller button
				sc->JButton(stoi(val.substr(2)));
				break;
			case 'H':	// controller hat
				sc->JHat(stoi(val.substr(2)));
				break;
			case 'A':	// controller axis
				sc->JAxis(stoi(val.substr(3)), (val[2] != '-'));
			}
		}
	}
	return sets;
}

void Filer::SaveSettings(const ControlsSettings& sets) {
	vector<string> lines = {
		"scroll_speed=" + to_string(sets.scrollSpeed.x) + " " + to_string(sets.scrollSpeed.y)
	};
	for (const pair<string, Shortcut*>& it : sets.shortcuts) {
		vector<string> values;
		if (it.second->KeyAssigned())
			values.push_back("K_" + string(SDL_GetScancodeName(it.second->Key())));
		if (it.second->JButtonAssigned())
			values.push_back("B_" + to_string(it.second->JCtr()));
		else if (it.second->JHatAssigned())
			values.push_back("H_" + to_string(it.second->JCtr()));
		else if (it.second->JAxisAssigned()) {
			string val = it.second->JPosAxisAssigned() ? "A_+" : "A_-";
			values.push_back(val + to_string(it.second->JCtr()));
		}

		for (string& val : values)
			lines.push_back("shortcut[" + it.first + "]=" + val);
	}
	WriteTextFile(dirSets + "controls.ini", lines);
}

bool Filer::ReadTextFile(const string& file, string& data) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		cerr << "couldn't opem file " << file << endl;
		return false;
	}
	data = string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	return true;
}

bool Filer::ReadTextFile(const string& file, vector<string>& lines, bool printMessage) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		if (printMessage)
			cerr << "couldn't open file " << file << endl;
		return false;
	}
	lines.clear();
	for (string line; ReadLine(ifs, line);)
		lines.push_back(line);
	return true;
}

bool Filer::WriteTextFile(const string& file, const vector<string>& lines) {
	std::ofstream ofs(file.c_str());
	if (!ofs.good()) {
		cerr << "couldn't write file " << file << endl;
		return false;
	}
	for (const string& line : lines)
		ofs << line << endl;
	return true;
}

bool Filer::MkDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryA(path.c_str(), NULL);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}

bool Filer::Remove(const string& path) {
	return std::remove(path.c_str());
}

bool Filer::Rename(const string& path, const string& newPath) {
	return std::rename(path.c_str(), newPath.c_str());
}

vector<string> Filer::ListDir(const string& dir, EDirFilter filter, const vector<string>& extFilter) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAA data;
	HANDLE hFile = FindFirstFileA(string(dir+"*").c_str(), &data);
	if (hFile != INVALID_HANDLE_VALUE)
		while (FindNextFileA(hFile, &data) != 0 || GetLastError() != ERROR_NO_MORE_FILES) {
			if (data.cFileName == string(".") || data.cFileName == string(".."))
				continue;
			
			if ((filter & FILTER_FILE) && ((data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) || data.dwFileAttributes == FILE_ATTRIBUTE_NORMAL)) {
				if (extFilter.empty())
					entries.push_back(data.cFileName);
				else for (const string& ext : extFilter)
					if (data.cFileName == ext) {
						entries.push_back(data.cFileName);
						break;
					}
			}
			else if ((filter & FILTER_DIR) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				entries.push_back(data.cFileName);
			else if ((filter & FILTER_LINK) && (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
				entries.push_back(data.cFileName);
		}
#else
	DIR* directory = opendir(directory.c_str());
	if (directory) {
		dirent* data = readdir(directory);
		while (data) {
			if (data.cFileName == string(".") || data.cFileName == string("..")) {
				data = readdir(directory);
				continue;
			}
			if ((filter & FILTER_FILE) && data->d_type == DT_REG) {
				if (extFilter.empty())
					entries.push_back(data.cFileName);
				else for (const string& ext : extFilter)
					if (data.cFileName == ext) {
						entries.push_back(data.cFileName);
						break;
					}
			}
			else if ((filter & FILTER_DIR) && data->d_type == DT_DIR)
				entries.push_back(data.cFileName);
			else if ((filter & FILTER_LINK) && data->d_type == DT_LNK)
				entries.push_back(data.cFileName);

			data = readdir(directory);
		}
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end());
	return entries;
}

vector<string> Filer::ListDirRecursively(const string& dir) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAA data;
	HANDLE hFile = FindFirstFileA(string(dir+"*").c_str(), &data);
	if (hFile != INVALID_HANDLE_VALUE)
		while (FindNextFileA(hFile, &data) != 0 || GetLastError() != ERROR_NO_MORE_FILES) {
			if (data.cFileName == string(".") || data.cFileName == string(".."))
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				vector<string> newEs = ListDirRecursively(dir+data.cFileName+dsep);
				std::sort(newEs.begin(), newEs.end());
				for (string& it : newEs)
					it = dir + it;
				entries.insert(entries.end(), newEs.begin(), newEs.end());
			}
			else
				entries.push_back(data.cFileName);
		}
#else
	DIR* directory = opendir(directory.c_str());
	if (directory) {
		dirent* data = readdir(directory);
		while (data) {
			if (data.cFileName == string(".") || data.cFileName == string("..")) {
				data = readdir(directory);
				continue;
			}
			if (data->d_type == DT_DIR) {
				vector<string> newEs = ListDirRecursively(dir+data->d_name+dsep);
				std::sort(newEs.begin(), newEs.end());
				for (string& it : newEs)
					it = dir + it;
				entries.insert(entries.end(), newEs.begin(), newEs.end());
			}
			else
				entries.push_back(data->d_name);

			data = readdir(directory);
		}
		closedir(directory);
	}
#endif
	return entries;
}

EFileType Filer::FileType(const string& path) {
#ifdef _WIN32
	ulong attrib = GetFileAttributesA(path.c_str());
	if ((attrib & FILE_ATTRIBUTE_ARCHIVE) || attrib == FILE_ATTRIBUTE_NORMAL)
		return EFileType::reg;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
		return EFileType::dir;
	if (attrib & FILE_ATTRIBUTE_SPARSE_FILE)
		return EFileType::link;
#else
	struct stat ps;
	stat(path.c_str(), &ps);
	if (S_ISREG(ps.st_mode))
		return EFileType::reg;
	if (S_ISDIR(ps.st_mode))
		return EFileType::dir;
	if (S_ISLNK(ps.st_mode))
		return EFileType::link;
#endif
	return EFileType::other;
}
bool Filer::Exists(const string& path) {
#ifdef _WIN32
	return (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES);
#else
	struct stat ps;
	return (stat(path.c_str(), &ps) == 0);
#endif
}

#ifdef _WIN32
vector<char> Filer::ListDrives() {
	vector<char> letters;
	ulong drives = GetLogicalDrives();

	for (char i=0; i!=26; i++)
		if (drives & (1 << i))
			letters.push_back('A'+i);
	return letters;
}
#endif

#ifdef __APPLE__
string Filer::GetDirExec(bool raw) {
#else
string Filer::GetDirExec() {
#endif
	const int MAX_LEN = 4096;
	string path;
	
#ifdef _WIN32
	char buffer[MAX_LEN];
	GetModuleFileNameA(NULL, buffer, MAX_LEN);
	path = buffer;
#elif __APPLE__
	char buffer[MAX_LEN];
	uint size = sizeof(buffer);
	_NSGetExecutablePath(buffer, &size);
	path = buffer;

	if (!raw) {
		string test = path.string();
		size_t pos = 0;
		if (findString(test, ".app/", &pos))	// if running in a package
			for (size_t i=pos; i!=0; i--)
				if (test[i] == dsep)
					return test.substr(0, i+1);
	}
#else
	char buffer[MAX_LEN];
	int len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
	buffer[len] = '\0';
	path = buffer;
#endif
	return parentPath(path);
}

vector<string> Filer::dirFonts() {
#ifdef _WIN32
	return {string(getenv("SystemDrive")) + "\\Windows\\Fonts\\"};
#elif __APPLE__
	return {string(getenv("HOME"))+"/Library/Fonts/", "/Library/Fonts/", "/System/Library/Fonts/", "/Network/Library/Fonts/"};
#else
	return { "/usr/share/fonts/" };
#endif
}

string Filer::FindFont(const string& font) {
	if (isAbsolute(font)) {	// check fontpath first
		if (FileType(font) == EFileType::reg)
			return font;
		return CheckDirForFont(filename(font), parentPath(font));
	}

	for (string& dir : dirFonts()) {	// check global font directories
		string file = CheckDirForFont(font, dir);
		if (!file.empty())
			return file;
	}
	return "";
}

string Filer::CheckDirForFont(const string& font, const string& dir) {
	for (string& it : ListDirRecursively(dir)) {
		string file = findChar(font, '.') ? filename(it) : delExt(filename(it));
		if (strcmpCI(file, font))
			return dir + it;
	}
	return "";
}

std::istream& Filer::ReadLine(std::istream& ifs, string& str) {
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
