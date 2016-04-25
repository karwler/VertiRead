#include "world.h"

EDirFilter operator|(EDirFilter a, EDirFilter b) {
	return static_cast<EDirFilter>(static_cast<byte>(a) | static_cast<byte>(b));
}

byte Filer::CheckDirectories() {
	byte retval = 0;
	if (!fs::exists(dirLib()))
		fs::create_directory(dirLib());
	if (!fs::exists(dirPlist()))
		fs::create_directory(dirPlist());
	if (!fs::exists(dirSets()))
		fs::create_directory(dirSets());
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

bool Filer::ReadTextFile(string file, vector<string>& lines, bool printMessage) {
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

bool Filer::WriteTextFile(string file, const vector<string>& lines) {
	std::ofstream ofs(file.c_str());
	if (!ofs.good()) {
		cerr << "couldn't write file " << file << endl;
		return false;
	}
	for (const string& line : lines)
		ofs << line << endl;
	return true;
}

vector<fs::path> Filer::ListDir(fs::path dir, EDirFilter filter, const vector<string>& extFilter) {
	vector<fs::path> names;
	if (!fs::is_directory(dir))
		return names;
	for (fs::directory_iterator it(dir); it != fs::directory_iterator(); it++) {
		if (filter == 0)
			names.push_back(it->path());
		else if (filter & FILTER_FILE && fs::is_regular_file(it->path())) {
			if (extFilter.empty())
				names.push_back(it->path());
			else for (const string& ext : extFilter)
				if (it->path().extension() == ext) {
					names.push_back(it->path());
					break;
				}
		}
		else if (filter & FILTER_DIR && fs::is_directory(it->path()))
			names.push_back(it->path());
		else if (filter & FILTER_LINK && fs::is_symlink(it->path()))
			names.push_back(it->path());
	}
	sort(names.begin(), names.end());
	return names;
}

vector<string> Filer::GetPicsFromDir(fs::path dir) {
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

Playlist Filer::LoadPlaylist(string name) {
	vector<string> lines;
	if (!ReadTextFile(dirPlist() + name, lines))
		return Playlist();

	Playlist plist(name);
	for (string& line : lines) {
		string arg, val;
		splitIniLine(line, &arg, &val);
		if (arg == "books") {
			vector<string> elems = getWords(val);
			for (string& it : elems)
				plist.books.push_back(it);
		}
		else
			plist.songs.push_back(line);
	}
	return plist;
}

void Filer::SavePlaylist(const Playlist& plist) {
	vector<string> lines = {
		"books="
	};
	for (const string& name : plist.books)
		lines[0] += name;

	for (const fs::path& file : plist.songs)
		lines.push_back(file.string());

	WriteTextFile(dirPlist() + plist.name, lines);
}

GeneralSettings Filer::LoadGeneralSettings() {
	vector<string> lines;
	if (!ReadTextFile(dirSets() + "general.ini", lines, false))
		return GeneralSettings();

	GeneralSettings sets;
	for (string& line : lines) {
		string arg, val;
		splitIniLine(line, &arg, &val);
		// load settings
	}
	return sets;
}

void Filer::SaveSettings(const GeneralSettings& sets) {
	vector<string> lines;
	// save settings
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
		if (arg == "vsync")
			sets.vsync = stob(val);
		else if (arg == "font")
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
		"vsync=" + btos(sets.vsync),
		"font=" + sets.font,
		"renderer=" + sets.renderer,
		"maximized=" + btos(sets.maximized),
		"fullscreen=" + btos(sets.fullscreen),
		"resolution=" + to_string(sets.resolution.x) + ' ' + to_string(sets.resolution.y)
	};
	for (const pair<EColor, vec4b>& it : sets.colors)
		lines.push_back("color["+to_string(int(it.first))+"]=" + to_string(it.second.x) + ' ' + to_string(it.second.y) + ' ' + to_string(it.second.z) + ' ' + to_string(it.second.a));
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

string Filer::execDir() {
	const int MAX_LEN = 4096;
	fs::path path;
#ifdef _WIN32
	TCHAR buffer[MAX_LEN];
	GetModuleFileName (NULL, buffer, MAX_LEN);
	for (uint i = 0; buffer[i] != 0; i++)
		path += buffer[i];
#else
	char buffer[MAX_LEN];
	int len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
	if (len < MAX_LEN) {
		buffer[len] = '\0';
		path = buffer;
	}
	else
		path = fs::initial_path().string() + "/" + World::args[0];
#endif
	return path.parent_path().string() + dsep;
}

string Filer::dirLib() {
	return execDir() + "library" + dsep;
}

string Filer::dirPlist() {
	return execDir() + "playlists" + dsep;
}

string Filer::dirSets() {
	return execDir() + "settings" + dsep;
}

string Filer::dirSnds() {
	return execDir() + "data"+dsep+"sounds"+dsep;
}

string Filer::dirTexs() {
	return execDir() + "data"+dsep+"textures"+dsep;
}

string Filer::dirFonts() {
#ifdef _WIN32
	return string(getenv("SystemDrive")) + "\\Windows\\Fonts\\";
#else
	return "/usr/share/fonts/truetype/msttcorefonts/";
#endif
}
