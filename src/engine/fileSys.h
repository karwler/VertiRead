#pragma once

#include "utils/settings.h"

enum FileType : uint8 {
	FTYPE_NONE = 0x0,
	FTYPE_FILE = 0x1,
	FTYPE_DIR  = 0x2,
	FTYPE_ANY  = 0xFF
};

inline FileType operator~(FileType a) {
	return FileType(~uint8(a));
}

inline FileType operator&(FileType a, FileType b) {
	return FileType(uint8(a) & uint8(b));
}

inline FileType operator&=(FileType& a, FileType b) {
	return a = FileType(uint8(a) & uint8(b));
}

inline FileType operator^(FileType a, FileType b) {
	return FileType(uint8(a) ^ uint8(b));
}

inline FileType operator^=(FileType& a, FileType b) {
	return a = FileType(uint8(a) ^ uint8(b));
}

inline FileType operator|(FileType a, FileType b) {
	return FileType(uint8(a) | uint8(b));
}

inline FileType operator|=(FileType& a, FileType b) {
	return a = FileType(uint8(a) | uint8(b));
}

// for interpreting lines in ini files
class IniLine {
public:
	/* The first equal sign to be read splits the line into property and value, therefore titles can't contain equal signs.
	   A key must be enclosed by brackets positioned before the equal sign.
	   Any space characters before and after the property string as well as between a key's closing bracket and the eqal sign are ignored.
	   The same applies to titles and keys, but not to values. */
	enum class Type : uint8 {
		empty,
		prpVal,		// property, value, no key, no title
		prpKeyVal,	// property, key, value, no title
		title		// title, no everything else
	};

	IniLine();
	IniLine(const string& line);

	Type getType() const;
	const string& getPrp() const;
	const string& getKey() const;
	const string& getVal() const;

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

inline string IniLine::line(const string& title) {
	return '[' + title + ']';
}

inline string IniLine::line(const string& prp, const string& val) {
	return prp + '=' + val;
}

inline string IniLine::line(const string& prp, const string& key, const string& val) {
	return prp + '[' + key + "]=" + val;
}

// handles all filesystem interactions
class FileSys {
public:
	FileSys();

	vector<string> getAvailibleThemes();
	vector<SDL_Color> loadColors(const string& theme);	// updates settings' colors according to settings' theme
	vector<string> getAvailibleLanguages();
	umap<string, string> loadTranslations(const string& language);

	bool getLastPage(const string& book, string& drc, string& fname);
	bool saveLastPage(const string& book, const string& drc, const string& fname);
	Settings* loadSettings();
	bool saveSettings(const Settings* sets);
	vector<Binding> getBindings();
	bool saveBindings(const vector<Binding>& sets);
	string findFont(const string& font);	// on success returns absolute path to font file, otherwise returns empty path

	static bool createDir(const string& path);
	static vector<string> listDir(const string& drc, FileType filter = FTYPE_ANY);
	static vector<string> listDirRecursively(string drc);
	static pair<vector<string>, vector<string>> listDirSeparate(const string& drc);	// first is list of files, second is list of directories
	static FileType fileType(const string& path);
	static bool isPicture(const string& file);
	static bool isArchive(const string& file);
	static bool isFont(const string& file);
#ifdef _WIN32
	static string wgetenv(const string& name);
#endif
	const string& getDirExec() const;
	const string& getDirSets() const;
	const string& getDirLangs() const;
	const string& getDirTexs() const;

private:
	static vector<string> readTextFile(const string& file, bool printMessage = true);
	static bool writeTextFile(const string& file, const vector<string>& lines);
#ifdef _WIN32
	static vector<char> listDrives();	// get list of driver letters under windows
#endif
	static string getExecDir();
	static string getWorkingDir();

	string dirExec;	// directory in which the executable should currently be
	string dirSets;	// settings directory
	string dirLangs;	// language files directory
	string dirTexs;	// textures directory
	vector<string> dirFonts;	// os's font directories
};

inline const string& FileSys::getDirExec() const {
	return dirExec;
}

inline const string& FileSys::getDirSets() const {
	return dirSets;
}

inline const string& FileSys::getDirLangs() const {
	return dirLangs;
}

inline const string& FileSys::getDirTexs() const {
	return dirTexs;
}
