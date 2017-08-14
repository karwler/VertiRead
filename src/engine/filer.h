#pragma once

#include "utils/settings.h"

enum EFileType : uint8 {
	FTYPE_FILE = 0x1,
	FTYPE_DIR  = 0x2,
	FTYPE_LINK = 0x4
};
EFileType operator~(EFileType a);
EFileType operator&(EFileType a, EFileType b);
EFileType operator&=(EFileType& a, EFileType b);
EFileType operator^(EFileType a, EFileType b);
EFileType operator^=(EFileType& a, EFileType b);
EFileType operator|(EFileType a, EFileType b);
EFileType operator|=(EFileType& a, EFileType b);

// handles all filesystem interactions
class Filer {
public:
	static void checkDirectories(const GeneralSettings& sets);	// check if all (more or less) necessary files and directories exist

	static vector<string> getAvailibleThemes();
	static void getColors(map<EColor, vec4c>& colors, const string& theme);	// get theme's colors
	static vector<string> getAvailibleLanguages();
	static map<string, string> getLines(const string& language);	// get translations from language (-file)
	static map<string, Mix_Chunk*> getSounds();
	static map<string, Texture> getTextures();
	static vector<string> getPics(const string& dir);	// get pictures for ReaderBox instance

	static Playlist getPlaylist(const string& name);
	static void savePlaylist(const Playlist& plist);

	static GeneralSettings getGeneralSettings();
	static void saveSettings(const GeneralSettings& sets);
	static VideoSettings getVideoSettings();
	static void saveSettings(const VideoSettings& sets);
	static AudioSettings getAudioSettings();
	static void saveSettings(const AudioSettings& sets);
	static ControlsSettings getControlsSettings();
	static void saveSettings(const ControlsSettings& sets);

	static bool readTextFile(const string& file, string& data);
	static bool readTextFile(const string& file, vector<string>& lines, bool printMessage=true);
	static bool writeTextFile(const string& file, const vector<string>& lines);
	static bool mkDir(const string& path);
	static bool remove(const string& path);
	static bool rename(const string& path, const string& newPath);
	static vector<string> listDir(const string& dir, EFileType filter=FTYPE_FILE | FTYPE_DIR | FTYPE_LINK, const vector<string>& extFilter={});
	static vector<string> listDirRecursively(const string& dir, size_t offs=0);
	static EFileType fileType(const string& path);
	static bool fileExists(const string& path);		// can be used for directories

#ifdef _WIN32
	static vector<char> listDrives();	// get list of driver letters under windows
#endif
	static string getDirExec();		// set dirExec
	static string findFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path
	
	static const string dirExec;	// directory in which the executable should currently be
	static const string dirSets;	// settings directory
	static const string dirLangs;	// language files directory
	static const string dirSnds;	// sounds directory
	static const string dirTexs;	// textures directory
	static const vector<string> dirFonts;	// os's font directories

private:
	static string checkDirForFont(const string& font, const string& dir);	// necessary for FindFont()
	static std::istream& readLine(std::istream& ifs, string& str);
};
