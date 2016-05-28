#include "world.h"

EDirFilter operator|(EDirFilter a, EDirFilter b) {
	return static_cast<EDirFilter>(static_cast<byte>(a) | static_cast<byte>(b));
}

byte Filer::CheckDirectories(const GeneralSettings& sets) {
	byte retval = 0;
	if (!fs::exists(dirSets()))
		fs::create_directories(dirSets());
	if (!fs::exists(sets.libraryParh()))
		fs::create_directories(sets.libraryParh());
	if (!fs::exists(sets.playlistParh()))
		fs::create_directories(sets.playlistParh());
	if (!fs::exists(dirSnds())) {
		cerr << "couldn't find sound directory" << endl;
		retval = 1;
	}
	if (!fs::exists(dirTexs())) {
		cerr << "couldn't find texture directory" << endl;
		retval = 2;
	}
	return retval;
}

bool Filer::ReadTextFile(const string& file, vector<string>& lines, bool printMessage) {
	std::ifstream ifs(file.c_str());
	if (!ifs.good()) {
		if (printMessage)
			cerr << "couldn't read file " << file << endl;
		return false;
	}
	lines.clear();
	for (string line; getline(ifs, line);)
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

vector<fs::path> Filer::ListDir(const fs::path& dir, EDirFilter filter, const vector<string>& extFilter) {
	vector<fs::path> names;
	if (!fs::is_directory(dir))
		return names;

	for (fs::directory_iterator it(dir); it != fs::directory_iterator(); it++) {
		if (filter == 0)
			names.push_back(it->path());
		else if ((filter & FILTER_FILE) && fs::is_regular_file(it->path())) {
			if (extFilter.empty())
				names.push_back(it->path());
			else for (const string& ext : extFilter)
				if (it->path().extension() == ext) {
					names.push_back(it->path());
					break;
				}
		}
		else if ((filter & FILTER_DIR) && fs::is_directory(it->path()))
			names.push_back(it->path());
		else if ((filter & FILTER_LINK) && fs::is_symlink(it->path()))
			names.push_back(it->path());
	}
	sort(names.begin(), names.end());
	return names;
}

vector<string> Filer::GetPicsFromDir(const fs::path& dir) {
	vector<string> pics;
	if (!fs::is_directory(dir))
		return pics;

	for (fs::directory_iterator it(dir); it != fs::directory_iterator(); it++)
		if (fs::is_regular_file(it->path()))
			pics.push_back(it->path().string());
	sort(pics.begin(), pics.end());
	return pics;
}

map<string, string> Filer::GetTextures() {
	map<string, string> paths;
	for (fs::directory_iterator it(dirTexs()); it != fs::directory_iterator(); it++)
		paths.insert(make_pair(removeExtension(it->path().filename()).string(), it->path().string()));
	return paths;
}

map<string, string> Filer::GetSounds() {
	map<string, string> paths;
	for (fs::directory_iterator it(dirSnds()); it != fs::directory_iterator(); it++)
		paths.insert(make_pair(removeExtension(it->path().filename()).string(), it->path().string()));
	return paths;
}

Playlist Filer::LoadPlaylist(const string& name) {
	vector<string> lines;
	if (!ReadTextFile(World::scene()->Settings().playlistParh() + name, lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		string arg, val;
		splitIniLine(line, &arg, &val);
		if (arg == "book")
			plist.books.push_back(val);
		else
			plist.songs.push_back(line);
	}
	return plist;
}

void Filer::SavePlaylist(const Playlist& plist) {
	vector<string> lines;
	for (const string& name : plist.books)
		lines.push_back("book=" + name);
	for (const fs::path& file : plist.songs)
		lines.push_back(file.string());

	WriteTextFile(World::scene()->Settings().playlistParh() + plist.name, lines);
}

GeneralSettings Filer::LoadGeneralSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets() + "general.ini", lines, false))
		return GeneralSettings();

	GeneralSettings sets;
	for (string& line : lines) {
		string arg, val;
		splitIniLine(line, &arg, &val);
		if (arg == "library")
			sets.dirLib = val;
		else if (arg == "playlists")
			sets.dirPlist = val;
	}
	return sets;
}

void Filer::SaveSettings(const GeneralSettings& sets) {
	vector<string> lines = {
		"library=" + sets.dirLib,
		"playlists=" + sets.dirPlist
	};
	WriteTextFile(dirSets() + "general.ini", lines);
}

VideoSettings Filer::LoadVideoSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets() + "video.ini", lines, false))
		return VideoSettings();

	VideoSettings sets;
	for (string& line : lines) {
		string arg, val, key;
		splitIniLine(line, &arg, &val, &key);
		if (arg == "font")
			sets.font = val;
		else if (arg == "renderer")
			sets.renderer = val;
		else if (arg == "maximized")
			sets.maximized = stob(val);
		else if (arg == "fullscreen")
			sets.fullscreen = stob(val);
		else if (arg == "resolution") {
			vector<string> elems = getWords(val);
			if (elems.size() > 0) sets.resolution.x = stoi(elems[0]);
			if (elems.size() > 1) sets.resolution.y = stoi(elems[1]);
		}
		else if (arg == "color") {
			vector<string> elems = getWords(val);
			if (elems.size() > 0) sets.colors[EColor(stoi(key))].x = stoi(elems[0]);
			if (elems.size() > 1) sets.colors[EColor(stoi(key))].y = stoi(elems[1]);
			if (elems.size() > 2) sets.colors[EColor(stoi(key))].z = stoi(elems[2]);
			if (elems.size() > 3) sets.colors[EColor(stoi(key))].a = stoi(elems[3]);
		}
	}
	return sets;
}

void Filer::SaveSettings(const VideoSettings& sets) {
	vector<string> lines = {
		"font=" + sets.font,
		"renderer=" + sets.renderer,
		"maximized=" + btos(sets.maximized),
		"fullscreen=" + btos(sets.fullscreen),
		"resolution=" + to_string(sets.resolution.x) + ' ' + to_string(sets.resolution.y)
	};
	for (const pair<EColor, vec4b>& it : sets.colors)
		lines.push_back("color["+to_string(ushort(it.first))+"]=" + to_string(it.second.x) + ' ' + to_string(it.second.y) + ' ' + to_string(it.second.z) + ' ' + to_string(it.second.a));
	WriteTextFile(dirSets() + "video.ini", lines);
}

AudioSettings Filer::LoadAudioSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets() + "audio.ini", lines, false))
		return AudioSettings();

	AudioSettings sets;
	for (string& line : lines) {
		string arg, val;
		splitIniLine(line, &arg, &val);
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
	WriteTextFile(dirSets() + "audio.ini", lines);
}

ControlsSettings Filer::LoadControlsSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets() + "controls.ini", lines, false))
		return ControlsSettings();

	ControlsSettings sets;
	for (string& line : lines) {
		string arg, val, key;
		splitIniLine(line, &arg, &val, &key);
		if (arg == "scroll_speed") {
			vector<string> elems = getWords(val);
			if (elems.size() > 0) sets.scrollSpeed.x = stof(elems[0]);
			if (elems.size() > 1) sets.scrollSpeed.y = stof(elems[1]);
		}
		else if (arg == "shortcut")
			sets.shortcuts[key].key = SDL_GetScancodeFromName(val.c_str());
		else if (arg == "holder")
			sets.holders[key] = SDL_GetScancodeFromName(val.c_str());
	}
	return sets;
}

void Filer::SaveSettings(const ControlsSettings& sets) {
	vector<string> lines = {
		"scroll_speed=" + to_string(sets.scrollSpeed.x) + " " + to_string(sets.scrollSpeed.y)
	};
	for (const pair<string, Shortcut>& it : sets.shortcuts)
		lines.push_back("shortcut[" + it.first + "]=" + SDL_GetScancodeName(it.second.key));
	for (const pair<string, SDL_Scancode>& it : sets.holders)
		lines.push_back("holder[" + it.first + "]=" + SDL_GetScancodeName(it.second));
	WriteTextFile(dirSets() + "controls.ini", lines);
}

#ifdef __APPLE__
string Filer::execDir(bool raw) {
#else
string Filer::execDir() {
#endif
	const int MAX_LEN = 4096;
	fs::path path;

#ifdef _WIN32
	char buffer[MAX_LEN];
	if (GetModuleFileName(NULL, buffer, MAX_LEN))
		path = buffer;
	else {
		fs::path dir = fs::path(World::args[0]).parent_path();
		path = dir.is_absolute() ? dir : fs::initial_path().string() + "\\" + dir.string();
	}
#elif __APPLE__
	char buffer[MAX_LEN];
	uint size = sizeof(buffer);
	if (!_NSGetExecutablePath(buffer, &size))
		path = buffer;
	else
		path = fs::initial_path().string() + "/" + fs::path(World::args[0]).parent_path().string();

	if (!raw) {
		string test = path.string();
		int pos = findString(test, ".app/");	// if running in a package
		for (int i=pos; i>=0; i--)
			if (test[i] == '/')
				return test.substr(0, i+1);
	}
#else
	char buffer[MAX_LEN];
	int len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
	if (len < MAX_LEN) {
		buffer[len] = '\0';
		path = buffer;
	}
	else
		path = fs::initial_path().string() + "/" + fs::path(World::args[0]).parent_path().string();

#endif
	return path.parent_path().string() + dsep;
}

string Filer::dirSets() {
#ifdef _WIN32
	return string(getenv("AppData")) + "\\VertiRead\\";
#elif __APPLE__
	return string(getenv("HOME")) + "/Library/Application Support/VertiRead/";
#else
	return string(getenv("HOME")) + "/.vertiread/";
#endif
}

string Filer::dirData() {
#ifdef __APPLE__
	return execDir(true) + "../Resources/data/";
#else
	return execDir() + "data"+dsep;
#endif
}

string Filer::dirSnds() {
	return dirData() + "sounds"+dsep;
}

string Filer::dirTexs() {
	return dirData() + "textures"+dsep;
}

vector<string> Filer::dirFonts() {
#ifdef _WIN32
	return {string(getenv("SystemDrive")) + "\\Windows\\Fonts\\"};
#elif __APPLE__
	return {string(getenv("HOME"))+"/Library/Fonts/", "/Library/Fonts/", "/System/Library/Fonts/", "/Network/Library/Fonts/"};
#else
	return {"/usr/share/fonts/", "/usr/share/fonts/truetype/msttcorefonts/"};
#endif
}

bool Filer::findFont(const string& font, string* dir) {
	if (fs::path(font).is_absolute()) {	// check fontpath first
		if (!fs::is_regular_file(font))
			return false;

		if (dir)
			*dir = fs::path(font).parent_path().string() +dsep;
		return true;
	}

	vector<string> paths = dirFonts();	// check global font directories
	for (string& pt : paths)
		if (fs::is_regular_file(pt + font)) {
			if (dir)
				*dir = pt;
			return true;
		}
	return false;
}
