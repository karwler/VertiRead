#pragma once

#include "utils/types.h"

class Filer {
public:
	static byte CheckDirectories(const GeneralSettings& sets);
	static bool ReadTextFile(const string& file, vector<string>& lines, bool printMessage=true);
	static bool WriteTextFile(const string& file, const vector<string>& lines);
	static vector<fs::path> ListDir(const fs::path& dir, EDirFilter filter=FILTER_ALL, const vector<string>& extFilter={});

	static map<string, string> GetLines(const string& language);
	static map<string, Mix_Chunk*> GetSounds();
	static map<string, Texture> GetTextures();
	static vector<string> GetPics(const fs::path& dir);

	static Playlist LoadPlaylist(const string& name);
	static void SavePlaylist(const Playlist& plist);

	static GeneralSettings LoadGeneralSettings();
	static void SaveSettings(const GeneralSettings& sets);
	static VideoSettings LoadVideoSettings();
	static void SaveSettings(const VideoSettings& sets);
	static AudioSettings LoadAudioSettings();
	static void SaveSettings(const AudioSettings& sets);
	static ControlsSettings LoadControlsSettings();
	static void SaveSettings(const ControlsSettings& sets);

#ifdef __APPLE__
	static string execDir(bool raw=false);
#else
	static string execDir();
#endif
	static string dirSets();
	static string dirData();
	static string dirLangs();
	static string dirSnds();
	static string dirTexs();

	static vector<string> dirFonts();
	static bool findFont(const string& font, string* dir=nullptr);
};
