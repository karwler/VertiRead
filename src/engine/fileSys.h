#pragma once

#include "utils/settings.h"
#include <archive.h>
#include <archive_entry.h>
#include <fstream>

/* For interpreting lines in ini files:
   The first equal sign to be read splits the line into property and value, therefore titles can't contain equal signs.
   A key must be enclosed by brackets positioned before the equal sign.
   Any space characters before and after the property string as well as between a key's closing bracket and the equal sign are ignored.
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

	template <class... T> static void writeTitle(std::ofstream& ss, T&&... title);
	template <class P, class... T> static void writeVal(std::ofstream& ss, P&& prp, T&&... val);
	template <class P, class K, class... T> static void writeKeyVal(std::ofstream& ss, P&& prp, K&& key, T&&... val);
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

template <class... T>
void IniLine::writeTitle(std::ofstream& ss, T&&... title) {
	((ss << '[') << ... << std::forward<T>(title)) << "]\n";
}

template <class P, class... T>
void IniLine::writeVal(std::ofstream& ss, P&& prp, T&&... val) {
	((ss << std::forward<P>(prp) << '=') << ... << std::forward<T>(val)) << '\n';
}

template <class P, class K, class... T>
void IniLine::writeKeyVal(std::ofstream& ss, P&& prp, K&& key, T&&... val) {
	((ss << std::forward<P>(prp) << '[' << std::forward<K>(key) << "]=") << ... << std::forward<T>(val)) << '\n';
}

// handles all filesystem interactions
class FileSys {
public:
	static constexpr array<SDL_Color, sizet(Color::texture)+1> defaultColors = {
		SDL_Color{ 10, 10, 10, 255 },		// background
		SDL_Color{ 90, 90, 90, 255 },		// normal
		SDL_Color{ 60, 60, 60, 255 },		// dark
		SDL_Color{ 120, 120, 120, 255 },	// light
		SDL_Color{ 105, 105, 105, 255 },	// select
		SDL_Color{ 75, 75, 75, 255 },		// tooltip
		SDL_Color{ 210, 210, 210, 255 },	// text
		SDL_Color{ 210, 210, 210, 255 }		// texture
	};
	static constexpr array<const char*, defaultColors.size()> colorNames = {
		"background",
		"normal",
		"dark",
		"light",
		"select",
		"tooltip",
		"text",
		"texture"
	};

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
	static constexpr array<const char*, 22> takenFilenames = {
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
	};
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

	fs::path dirSets;	// settings directory
#ifdef _WIN32		// os's font directories
	array<fs::path, 2> dirFonts;
#else
	array<fs::path, 3> dirFonts;
#endif
public:
	FileSys();

	vector<string> getAvailableThemes() const;
	array<SDL_Color, defaultColors.size()> loadColors(const string& theme) const;	// updates settings' colors according to settings' theme
	bool getLastPage(const string& book, string& drc, string& fname) const;
	bool saveLastPage(const string& book, const string& drc, const string& fname) const;
	Settings* loadSettings() const;
	void saveSettings(const Settings* sets) const;
	array<Binding, Binding::names.size()> getBindings() const;
	void saveBindings(const array<Binding, Binding::names.size()>& bindings) const;
	fs::path findFont(const string& font) const;	// on success returns absolute path to font file, otherwise returns empty path

	static vector<fs::path> listDir(const fs::path& drc, bool files = true, bool dirs = true, bool showHidden = true);
	static pair<vector<fs::path>, vector<fs::path>> listDirSep(const fs::path& drc, bool showHidden = true);	// first is list of files, second is list of directories

	static fs::path validateFilename(const fs::path& file);
	static bool isPicture(const fs::path& file);
	static bool isFont(const fs::path& file);
	static bool isArchive(const fs::path& file);
	static bool isPictureArchive(const fs::path& file);
	static bool isArchivePicture(const fs::path& file, const string& pname);

	static archive* openArchive(const fs::path& file);
	static vector<string> listArchive(const fs::path& file);
	static mapFiles listArchivePictures(const fs::path& file, vector<string>& names);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);

	static int moveContentThreaded(void* data);	// moves files from one directory to another (data points to a thread and the thread's data is a pair of strings; src, dst)
	const fs::path& getDirSets() const;

private:
	static vector<string> readFileLines(const fs::path& file, bool printMessage = true);
	static string readTextFile(const fs::path& file, bool printMessage = true);
	static bool writeTextFile(const fs::path& file, const vector<string>& lines);
	static SDL_Color readColor(const string& line);

	static int setWorkingDir();
#ifdef _WIN32
	static vector<fs::path> listDrives();
#endif
};

inline const fs::path& FileSys::getDirSets() const {
	return dirSets;
}
