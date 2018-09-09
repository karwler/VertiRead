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
	/* The first equal sign to be read splits the line into property and value, therefore titles can't contain equal signs.
	   A key must be enclosed by brackets immediatly positioned before the equal sign.
	   If there are any characters other than spaces between a closing bracket and the equal sign, the brackets count as part of the property name.
	   Any space characters before and after the property string are ignored.
	   The same applies to titles and keys, but not to values. */
	enum class Type : uint8 {
		empty,
		prpVal,		// property, value, no key, no title
		prpKeyVal,	// property, key, value, no title
		title		// title, no everything else
	};

	IniLine();
	IniLine(const string& line);

	Type getType() const { return type; }
	const string& getPrp() const { return prp; }
	const string& getKey() const { return key; }
	const string& getVal() const { return val; }

	string line() const;	// get the actual INI line from prp, key and val
	static string line(const string& title);
	static string line(const string& prp, const string& val);
	static string line(const string& prp, const string& key, const string& val);

	void setVal(const string& property, const string& value);
	void setVal(const string& property, const string& vkey, const string& value);
	void setTitle(const string& title);
	Type setLine(const string& str);

private:
	Type type;
	string prp;	// property, aka. the thing before the equal sign/brackets
	string key;	// the thing between the brackets (empty if there are no brackets)
	string val;	// value, aka. the thing after the equal sign
};

// handles all filesystem interactions
class FileSys {
public:
	FileSys();

	vector<string> getAvailibleThemes();
	vector<SDL_Color> getColors(const string& theme);	// updates settings' colors according to settings' theme
	vector<string> getAvailibleLanguages();
	umap<string, string> getTranslations(const string& language);

	bool getLastPage(const string& book, string& drc, string& fname);
	bool saveLastPage(const string& book, const string& drc, const string& fname);
	Settings* getSettings();
	bool saveSettings(const Settings* sets);
	vector<Binding> getBindings();
	bool saveBindings(const vector<Binding>& sets);
	string findFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path

	static bool createDir(const string& path);
	static vector<string> listDir(const string& drc, FileType filter=FTYPE_ANY);
	static vector<string> listDirRecursively(string drc);
	static pair<vector<string>, vector<string>> listDirSeparate(const string& drc);	// first is list of files, second is list of directories
	static FileType fileType(const string& path);
	static bool isPicture(const string& file);
	static bool isArchive(const string& file);
	static bool isFont(const string& file);
	static string wgetenv(const string& name);

	const string& getDirExec() const { return dirExec; }
	const string& getDirSets() const { return dirSets; }
	const string& getDirLangs() const { return dirLangs; }
	const string& getDirTexs() const { return dirTexs; }

private:
	static vector<string> readTextFile(const string& file, bool printMessage = true);
	static bool writeTextFile(const string& file, const vector<string>& lines);
#ifdef _WIN32
	static vector<char> listDrives();	// get list of driver letters under windows
#endif
	static string getExecDir();
	static string getWorkingDir();
	static std::istream& readLine(std::istream& ifs, string& str);

	string dirExec;	// directory in which the executable should currently be
	string dirSets;	// settings directory
	string dirLangs;	// language files directory
	string dirTexs;	// textures directory
	vector<string> dirFonts;	// os's font directories
};
