#pragma once

#include "utils/settings.h"
#include <archive.h>
#include <archive_entry.h>
#include <functional>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

enum FileType : uint8 {
	FTYPE_REG = 0x01,
	FTYPE_DIR = 0x02,
	FTYPE_LNK = 0x04,
	FTYPE_BLK = 0x08,
	FTYPE_CHR = 0x10,
	FTYPE_FIF = 0x20,
	FTYPE_SOC = 0x40,
	FTYPE_UNK = 0x80,

	FTYPE_NON = 0x00,
	FTYPE_STD = 0x03,	// REG | DIR
	FTYPE_ANY = 0xFF
};

inline constexpr FileType operator~(FileType a) {
	return FileType(~uint8(a));
}

inline constexpr FileType operator&(FileType a, FileType b) {
	return FileType(uint8(a) & uint8(b));
}

inline constexpr FileType operator&=(FileType& a, FileType b) {
	return a = FileType(uint8(a) & uint8(b));
}

inline constexpr FileType operator^(FileType a, FileType b) {
	return FileType(uint8(a) ^ uint8(b));
}

inline constexpr FileType operator^=(FileType& a, FileType b) {
	return a = FileType(uint8(a) ^ uint8(b));
}

inline constexpr FileType operator|(FileType a, FileType b) {
	return FileType(uint8(a) | uint8(b));
}

inline constexpr FileType operator|=(FileType& a, FileType b) {
	return a = FileType(uint8(a) | uint8(b));
}

/* For interpreting lines in ini files:
   The first equal sign to be read splits the line into property and value, therefore titles can't contain equal signs.
   A key must be enclosed by brackets positioned before the equal sign.
   Any space characters before and after the property string as well as between a key's closing bracket and the eqal sign are ignored.
   The same applies to titles and keys, but not to values. */
class IniLine {
public:
	enum class Type : uint8 {
		empty,
		prpVal,		// property, value, no key, no title
		prpKeyVal,	// property, key, value, no title
		title		// title, no everything else
	};
private:
	Type type;
	string prp;	// property, aka. the thing before the equal sign/brackets
	string key;	// the thing between the brackets (empty if there are no brackets)
	string val;	// value, aka. the thing after the equal sign

public:
	IniLine();
	IniLine(const string& line);

	Type getType() const;
	const string& getPrp() const;
	const string& getKey() const;
	const string& getVal() const;

	void setVal(const string& property, const string& value);
	void setVal(const string& property, const string& vkey, const string& value);
	void setTitle(const string& title);
	Type setLine(const string& str);

	static string get(const string& title);
	static string get(const string& prp, const string& val);
	static string get(const string& prp, const string& key, const string& val);
};

inline IniLine::IniLine() :
	type(Type::empty)
{}

inline IniLine::IniLine(const string& line) {
	setLine(line);
}

inline IniLine::Type IniLine::getType() const {
	return type;
}

inline const string& IniLine::getPrp() const {
	return prp;
}

inline const string& IniLine::getKey() const {
	return key;
}

inline const string& IniLine::getVal() const {
	return val;
}

inline string IniLine::get(const string& title) {
	return '[' + title + "]\n";
}

inline string IniLine::get(const string& prp, const string& val) {
	return prp + '=' + val + '\n';
}

inline string IniLine::get(const string& prp, const string& key, const string& val) {
	return prp + '[' + key + "]=" + val + '\n';
}

// handles all filesystem interactions
class FileSys {
public:
	static const array<SDL_Color, sizet(Color::texture)+1> defaultColors;
	static const array<string, defaultColors.size()> colorNames;

#ifdef _WIN32
	static constexpr char dirTexs[] = "textures\\";
#else
	static constexpr char dirTexs[] = "textures/";
#endif
	static constexpr char extIni[] = ".ini";
private:
	static constexpr char defaultFrMode[] = "rb";
	static constexpr char defaultFwMode[] = "wb";
	static constexpr sizet archiveReadBlockSize = 10240;
#ifdef _WIN32
	static const array<string, 22> takenFilenames;
	static constexpr sizet drivesMax = 26;
	static constexpr sizet fnameMax = 255;
#else
	static constexpr sizet fnameMax = NAME_MAX;
#endif
	static constexpr char fileThemes[] = "themes.ini";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileBindings[] = "bindings.ini";
	static constexpr char fileBooks[] = "books.dat";

	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordFullscreen[] = "fullscreen";
	static constexpr char iniKeywordResolution[] = "resolution";
	static constexpr char iniKeywordDirection[] = "direction";
	static constexpr char iniKeywordZoom[] = "zoom";
	static constexpr char iniKeywordSpacing[] = "spacing";
	static constexpr char iniKeywordPictureLimit[] = "picture_limit";
	static constexpr char iniKeywordFont[] = "font";
	static constexpr char iniKeywordTheme[] = "theme";
	static constexpr char iniKeywordShowHidden[] = "show_hidden";
	static constexpr char iniKeywordLibrary[] = "library";
	static constexpr char iniKeywordRenderer[] = "renderer";
	static constexpr char iniKeywordScrollSpeed[] = "scroll_speed";
	static constexpr char iniKeywordDeadzone[] = "deadzone";

	static constexpr char keyKey[] = "K_";
	static constexpr char keyButton[] = "B_";
	static constexpr char keyHat[] = "H_";
	static constexpr char keySep[] = "_";
	static constexpr char keyAxisPos[] = "A_+";
	static constexpr char keyAxisNeg[] = "A_-";
	static constexpr char keyGButton[] = "G_";
	static constexpr char keyGAxisPos[] = "X_+";
	static constexpr char keyGAxisNeg[] = "X_-";

	string dirSets;	// settings directory
#ifdef _WIN32		// os's font directories
	array<string, 2> dirFonts;
#else
	array<string, 3> dirFonts;
#endif
public:
	FileSys();

	vector<string> getAvailibleThemes();
	array<SDL_Color, defaultColors.size()> loadColors(const string& theme);	// updates settings' colors according to settings' theme
	bool getLastPage(const string& book, string& drc, string& fname);
	bool saveLastPage(const string& book, const string& drc, const string& fname);
	Settings* loadSettings();
	bool saveSettings(const Settings* sets);
	array<Binding, Binding::names.size()> getBindings();
	bool saveBindings(const array<Binding, Binding::names.size()>& bindings);
	string findFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path

	static vector<string> listDir(const string& drc, FileType filter = FTYPE_STD, bool showHidden = true, bool readLinks = true);
	static int iterateDirRec(const string& drc, const std::function<int (string)>& call, FileType filter = FTYPE_STD, bool readLinks = true, bool followLinks = false);
	static pair<vector<string>, vector<string>> listDirSep(const string& drc, FileType filter = FTYPE_REG, bool showHidden = true, bool readLinks = true);	// first is list of files, second is list of directories

	static string validateFilename(string file);
	static bool createDir(const string& path);
	static FileType fileType(const string& file, bool readLink = true);
	static bool isPicture(const string& file);
	static bool isFont(const string& file);
	static bool isArchive(const string& file);
	static bool isPictureArchive(const string& file);
	static bool isArchivePicture(const string& file, const string& pname);
	
	static archive* openArchive(const string& file);
	static vector<string> listArchive(const string& file);
	static mapFiles listArchivePictures(const string& file, vector<string>& names);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);

	static int moveContentThreaded(void* data);	// moves files from one directory to another (data points to a thread and the thread's data is a pair of strings; src, dst)
#ifdef _WIN32
	static string wgetenv(const string& name);
#endif
	const string& getDirSets() const;

private:
	static vector<string> readFileLines(const string& file, bool printMessage = true);
	static string readTextFile(const string& file, bool printMessage = true);
	static bool writeTextFile(const string& file, const string& text);
	static bool writeTextFile(const string& file, const vector<string>& lines);
	static SDL_Color readColor(const string& line);

	static int setWorkingDir();
#ifdef _WIN32
	static vector<char> listDrives();
	static bool atrcmp(DWORD attrs, FileType filter);
#else
	static FileType stmtoft(const string& file, int (*statfunc)(const char*, struct stat*));
	static bool dtycmp(const string& drc, const dirent* entry, FileType filter, bool readLink);
#endif
};

inline const string& FileSys::getDirSets() const {
	return dirSets;
}

inline bool FileSys::createDir(const string& path) {
#ifdef _WIN32
	return CreateDirectoryW(stow(path).c_str(), 0);
#else
	return !mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}
#ifdef _WIN32
inline bool FileSys::atrcmp(DWORD attrs, FileType filter) {
	return filter & (attrs & FILE_ATTRIBUTE_DIRECTORY ? FTYPE_DIR : FTYPE_REG);
}
#else
inline FileType FileSys::fileType(const string& file, bool readLink) {
	return stmtoft(file, readLink ? stat : lstat);
}
#endif
