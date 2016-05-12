#pragma once

#include "utils/types.h"

enum EDirFilter : byte {
	FILTER_FILE = 1,
	FILTER_DIR  = 2,
	FILTER_LINK = 4
};
EDirFilter operator|(EDirFilter a, EDirFilter b);

class Filer {
public:
	static byte CheckDirectories(const GeneralSettings& sets);
	static bool ReadTextFile(string file, vector<string>& lines, bool printMessage=true);
	static bool WriteTextFile(string file, const vector<string>& lines);
	static vector<fs::path> ListDir(fs::path dir, EDirFilter filter, const vector<string>& extFilter={});
	static vector<string> GetPicsFromDir(fs::path dir);

	static map<string, string> GetTextures();
	static map<string, string> GetSounds();
	static map<string, string> GetLines(string filename);

	static Playlist LoadPlaylist(string name);
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
	static bool findFont(string font, string *dir=nullptr);
};
