#pragma once

#include "utils/settings.h"

enum EFileType : uint8 {
	file,
	dir,
	link
};

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

	static vector<string> GetAvailibleThemes();
	static void GetColors(map<EColor, vec4c>& colors, const string& theme);
	static vector<string> GetAvailibleLanguages();
	static map<string, string> GetLines(const string& language);
	static map<string, Mix_Chunk*> GetSounds();
	static map<string, Texture> GetTextures();
	static vector<string> GetPics(const string& dir);

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

	static bool ReadTextFile(const string& file, string& data);
	static bool ReadTextFile(const string& file, vector<string>& lines, bool printMessage=true);
	static bool WriteTextFile(const string& file, const vector<string>& lines);
	static bool MkDir(const string& path);
	static bool Remove(const string& path);
	static bool Rename(const string& path, const string& newPath);
	static vector<string> ListDir(const string& dir, EDirFilter filter=FILTER_ALL, const vector<string>& extFilter={});
	static vector<string> ListDirRecursively(const string& dir, size_t offs=0);
	static EFileType FileType(const string& path);
	static bool Exists(const string& path);

#ifdef _WIN32
	static vector<char> ListDrives();
#endif
	static string GetDirExec();
	static vector<string> dirFonts();
	static string FindFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path
	
	static const string dirExec;
	static const string dirSets;
	static const string dirLangs;
	static const string dirSnds;
	static const string dirTexs;

private:
	static string CheckDirForFont(const string& font, const string& dir);	// returns same as FindFont
	static std::istream& ReadLine(std::istream& ifs, string& str);
};
