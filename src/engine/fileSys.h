#pragma once

#include "prog/types.h"
#include "utils/settings.h"
#include <atomic>
#include <fstream>

struct IniLine {
	enum class Type : uint8 {
		empty,
		prpVal,
		prpKeyVal,
		title
	};

	Type type = Type::empty;
	string_view prp;
	string_view key;
	string_view val;

	Type setLine(string_view str);

	template <class... T> static void writeTitle(std::ofstream& ofs, T&&... title);
	template <class P, class... T> static void writeVal(std::ofstream& ofs, P&& prp, T&&... val);
	template <class P, class K, class... T> static void writeKeyVal(std::ofstream& ofs, P&& prp, K&& key, T&&... val);
};

template <class... T>
void IniLine::writeTitle(std::ofstream& ofs, T&&... title) {
	((ofs << '[') << ... << std::forward<T>(title)) << ']' << LINEND;
}

template <class P, class... T>
void IniLine::writeVal(std::ofstream& ofs, P&& prp, T&&... val) {
	((ofs << std::forward<P>(prp) << '=') << ... << std::forward<T>(val)) << LINEND;
}

template <class P, class K, class... T>
void IniLine::writeKeyVal(std::ofstream& ofs, P&& prp, K&& key, T&&... val) {
	((ofs << std::forward<P>(prp) << '[' << std::forward<K>(key) << "]=") << ... << std::forward<T>(val)) << LINEND;
}

// handles all filesystem interactions
class FileSys {
private:
#ifdef _WIN32
	static constexpr array<string_view, 22> takenFilenames = {
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
	};
	static constexpr char drivesMax = 26;
#endif
	static constexpr size_t archiveReadBlockSize = 10240;
	static constexpr char fileThemes[] = "themes.ini";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileBindings[] = "bindings.ini";
	static constexpr char fileBooks[] = "books.dat";

	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordDisplay[] = "display";
	static constexpr char iniKeywordResolution[] = "resolution";
	static constexpr char iniKeywordRenderer[] = "renderer";
	static constexpr char iniKeywordDevice[] = "device";
	static constexpr char iniKeywordCompression[] = "compression";
	static constexpr char iniKeywordVSync[] = "vsync";
	static constexpr char iniKeywordGpuSelecting[] = "gpu_selecting";
	static constexpr char iniKeywordDirection[] = "direction";
	static constexpr char iniKeywordZoom[] = "zoom";
	static constexpr char iniKeywordSpacing[] = "spacing";
	static constexpr char iniKeywordPictureLimit[] = "picture_limit";
	static constexpr char iniKeywordMaxPictureRes[] = "max_picture_res";
	static constexpr char iniKeywordFont[] = "font";
	static constexpr char iniKeywordHinting[] = "hinting";
	static constexpr char iniKeywordTheme[] = "theme";
	static constexpr char iniKeywordPreview[] = "preview";
	static constexpr char iniKeywordShowHidden[] = "show_hidden";
	static constexpr char iniKeywordTooltips[] = "tooltips";
	static constexpr char iniKeywordLibrary[] = "library";
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

	fs::path dirBase;	// application base directory
	fs::path dirSets;	// settings directory
	fs::path dirConfs;	// internal config directory
	std::ofstream logFile;
#ifndef _WIN32
	void* fontconfig = nullptr;
#endif
public:
	FileSys(const uset<string>& cmdFlags);
	~FileSys();

	vector<string> getAvailableThemes() const;
	array<vec4, Settings::defaultColors.size()> loadColors(string_view theme) const;	// updates settings' colors according to settings' theme
	tuple<bool, string, string> getLastPage(string_view book) const;
	void saveLastPage(string_view book, string_view drc, string_view fname) const;
	Settings* loadSettings() const;
	void saveSettings(const Settings* sets) const;
	array<Binding, Binding::names.size()> loadBindings() const;
	void saveBindings(const array<Binding, Binding::names.size()>& bindings) const;
	fs::path findFont(const fs::path& font) const;	// on success returns absolute path to font file, otherwise returns empty path
	vector<string> listFonts() const;

	static vector<string> listDir(const char* drc, bool files = true, bool dirs = true, bool showHidden = true);
	static pair<vector<string>, vector<string>> listDirSep(const char* drc, bool showHidden = true);	// first is list of files, second is list of directories

	static vector<cbyte> readBinFile(const fs::path& file);
	static bool isPicture(const char* file);
	static bool isFont(const fs::path& file);
	static bool isArchive(const char* file);
	static bool isPictureArchive(const char* file);
	static bool isArchivePicture(const char* file, string_view pname);

	static archive* openArchive(const char* file);
	static vector<string> listArchiveFiles(const char* file);
	static void makeArchiveTreeThread(std::atomic<ThreadType>& mode, BrowserResultAsync ra, uintptr_t maxRes);
	static SDL_Surface* loadArchivePicture(const char* file, string_view pname);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);

	static void moveContentThread(std::atomic<ThreadType>& mode, fs::path src, fs::path dst);
	const fs::path& getDirSets() const;
	fs::path dirIcons() const;

private:
	static string_view readNextLine(string_view& text);
	static string readTextFile(const fs::path& file);
	template <Integer C, std::endian bo> static string processTextFile(std::ifstream& ifs, std::streampos offs);
	template <Integer C, std::endian bo> static C readChar(std::ifstream& ifs, std::streampos& len);
	static void writeChar8(string& str, char32_t ch);
	template <Class T> static void readFileContent(std::ifstream& ifs, T& data, std::streampos len);

	static bool isPicture(SDL_RWops* ifh, string_view ext);
	static bool isPicture(archive* arch, archive_entry* entry);
	static pair<uptr<cbyte[]>, int64> readArchiveEntry(archive* arch, archive_entry* entry);
	static string searchFontDirs(const fs::path& font, std::initializer_list<fs::path> dirs);
#ifdef _WIN32
	static vector<string> listDrives();
#endif
	static void SDLCALL logWrite(void* userdata, int category, SDL_LogPriority priority, const char* message);
};

inline const fs::path& FileSys::getDirSets() const {
	return dirSets;
}

inline fs::path FileSys::dirIcons() const {
	return dirConfs / "icons";
}

inline bool FileSys::isPicture(const char* file) {
	return isPicture(SDL_RWFromFile(file, "rb"), fileExtension(file));
}
