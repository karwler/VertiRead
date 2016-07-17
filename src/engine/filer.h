#pragma once

#include "utils/types.h"

enum EDirFilter : uint8 {
	FILTER_FILE = 0x1,
	FILTER_DIR  = 0x2,
	FILTER_LINK = 0x4,
	FILTER_ALL  = 0xFF
};
EDirFilter operator~(EDirFilter a);
EDirFilter operator&(EDirFilter a, EDirFilter b);
EDirFilter operator&=(EDirFilter& a, EDirFilter b);
EDirFilter operator^(EDirFilter a, EDirFilter b);
EDirFilter operator^=(EDirFilter& a, EDirFilter b);
EDirFilter operator|(EDirFilter a, EDirFilter b);
EDirFilter operator|=(EDirFilter& a, EDirFilter b);

class Filer {
public:
	static uint8 CheckDirectories(const GeneralSettings& sets);
	static bool ReadTextFile(const string& file, vector<string>& lines, bool printMessage=true);
	static bool WriteTextFile(const string& file, const vector<string>& lines);

	static vector<fs::path> ListDir(const fs::path& dir, EDirFilter filter=FILTER_ALL, const vector<string>& extFilter={});
	static vector<fs::path> ListDirRecursively(const fs::path& dir, EDirFilter filter=FILTER_ALL, const vector<string>& extFilter={});

	static vector<string> GetAvailibleThemes();
	static void GetColors(map<EColor, vec4b>& colors, const string& theme);
	static vector<string> GetAvailibleLanguages();
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

#ifdef _WIN32
	static bool isDriveLetter(const string& path);
	static vector<char> ListDrives();
#endif
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

	static vector<fs::path> dirFonts();
	static fs::path FindFont(const fs::path& font);	// on success returns absolute path to font file, otherwise returns empty path
private:
	static fs::path CheckDirForFont(const fs::path& font, const fs::path& dir);	// returns same as FindFont
};
