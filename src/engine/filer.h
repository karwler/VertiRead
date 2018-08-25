#pragma once

#include "utils/utils.h"

enum FileType : uint8 {
	FTYPE_NONE = 0x0,
	FTYPE_FILE = 0x1,
	FTYPE_DIR  = 0x2,
	FTYPE_ANY  = 0xFF
};
inline FileType operator~(FileType a) { return static_cast<FileType>(~static_cast<uint8>(a)); }
inline FileType operator&(FileType a, FileType b) { return static_cast<FileType>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline FileType operator&=(FileType& a, FileType b) { return a = static_cast<FileType>(static_cast<uint8>(a) & static_cast<uint8>(b)); }
inline FileType operator^(FileType a, FileType b) { return static_cast<FileType>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline FileType operator^=(FileType& a, FileType b) { return a = static_cast<FileType>(static_cast<uint8>(a) ^ static_cast<uint8>(b)); }
inline FileType operator|(FileType a, FileType b) { return static_cast<FileType>(static_cast<uint8>(a) | static_cast<uint8>(b)); }
inline FileType operator|=(FileType& a, FileType b) { return a = static_cast<FileType>(static_cast<uint8>(a) | static_cast<uint8>(b)); }

// for interpreting lines in ini files
class IniLine {
public:
	enum class Type : uint8 {
		empty,
		argVal,		// argument, value, no key, no title
		argKeyVal,	// argument, key, value, no title
		title		// title, no everything else
	};

	IniLine();
	IniLine(const string& line);

	Type getType() const { return type; }
	const string& getArg() const { return arg; }
	const string& getKey() const { return key; }
	const string& getVal() const { return val; }

	string line() const;	// get the actual INI line from arg, key and val
	static string line(const string& title);
	static string line(const string& arg, const string& val);
	static string line(const string& arg, const string& key, const string& val);

	void setVal(const string& property, const string& value);
	void setVal(const string& property, const string& vkey, const string& value);
	void setTitle(const string& title);
	Type setLine(string str);
	void clear();

private:
	Type type;
	string arg;	// argument, aka. the thing before the equal sign/brackets
	string key;	// the thing between the brackets (empty if there are no brackets)
	string val;	// value, aka. the thing after the equal sign
};

// handles all filesystem interactions
class Filer {
public:
	static vector<string> getAvailibleThemes();
	static vector<SDL_Color> getColors(const string& theme);	// updates settings' colors according to settings' theme
	static vector<string> getAvailibleLanguages();
	static umap<string, string> getTranslations(const string& language);

	static bool getLastPage(const string& book, string& drc, string& fname);
	static bool saveLastPage(const string& book, const string& drc, const string& fname);
	static Settings getSettings();
	static bool saveSettings(const Settings& sets);
	static vector<Binding> getBindings();
	static bool saveBindings(const vector<Binding>& sets);

	static bool readTextFile(const string& file, vector<string>& lines, bool printMessage=true);
	static bool writeTextFile(const string& file, const vector<string>& lines);
	static bool createDir(const string& path);
	static vector<string> listDir(const string& drc, FileType filter=FTYPE_ANY);
	static vector<string> listDirRecursively(string drc);
	static pair<vector<string>, vector<string>> listDirSeparate(const string& drc);	// first is list of files, second is list of directories
	static FileType fileType(const string& path);
	static bool isPicture(const string& file);
	static bool isArchive(const string& file);
	static bool isFont(const string& file);

#ifdef _WIN32
	static vector<char> listDrives();	// get list of driver letters under windows
	static string wgetenv(const string& name);
#endif
	static string getExecDir();
	static string getWorkingDir();
	static string findFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path
	
	static const string dirExec;	// directory in which the executable should currently be
	static const string dirSets;	// settings directory
	static const string dirLangs;	// language files directory
	static const string dirTexs;	// textures directory
	static const vector<string> dirFonts;	// os's font directories

private:
	static std::istream& readLine(std::istream& ifs, string& str);
};
