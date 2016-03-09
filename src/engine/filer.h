#pragma once

#include "utils/types.h"

#ifdef _WIN32
const string dsep = "\\";
#else
const string dsep = "/";
#endif

enum EDirFilter : byte {
	FILTER_FILE = 0x1,
	FILTER_DIR = 0x2,
	FILTER_LINK = 0x4
};
EDirFilter operator|(EDirFilter a, EDirFilter b);

class Filer {
public:
	static int CheckDirectories();
	static bool ReadTextFile(string file, vector<string>& lines);
	static bool WriteTextFile(string file, const vector<string>& lines);
	static vector<fs::path> ListDir(fs::path dir, EDirFilter filter, vector<string> extFilter=vector<string>());

	static Playlist LoadPlaylist(string name);
	static void SavePlaylist(Playlist plist);

	static GeneralSettings LoadGeneralSettings();
	static void SaveSettings(GeneralSettings sets);
	static VideoSettings LoadVideoSettings();
	static void SaveSettings(VideoSettings sets);
	static AudioSettings LoadAudioSettings();
	static void SaveSettings(AudioSettings sets);
	static ControlsSettings LoadControlsSettings();
	static void SaveSettings(ControlsSettings sets);

	static string execDir();
	static string dirLib();
	static string dirPlist();
	static string dirSets();
	static string dirSnds();
	static string dirTexs();
};
