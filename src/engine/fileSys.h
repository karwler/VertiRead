#pragma once

#include "utils/settings.h"
#include <fstream>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

struct FT_FaceRec_;
struct FT_LibraryRec_;

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
	static constexpr wchar_t fontsKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
#endif
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
#ifdef CAN_FONTCFG
	void* fontconfig = nullptr;	// is class Fontconfig
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
	vector<fs::path> listFontFiles(FT_LibraryRec_* lib, char32_t first, char32_t last) const;
	static void listFontFamiliesThread(std::stop_token stoken, fs::path cdir, string selected, char32_t first, char32_t last);
	static bool isFont(const fs::path& file);
	static void moveContentThread(std::stop_token stoken, fs::path src, fs::path dst);

	const fs::path& getDirSets() const { return dirSets; }
	const fs::path& getDirConfs() const { return dirConfs; }
	fs::path dirIcons() const { return dirConfs / "icons"; }
	string sanitizeFontPath(const fs::path& path) const;

private:
	static string_view readNextLine(string_view& text);
	static string readTextFile(const fs::path& file);
	template <Integer C, std::endian bo> static string processTextFile(std::ifstream& ifs, std::streampos offs);
	template <Integer C, std::endian bo> static C readChar(std::ifstream& ifs, std::streampos& len);
	static void writeChar8(string& str, char32_t ch);

	static fs::path searchFontDirectory(const fs::path& font, const fs::path& drc);
	static void listFontFilesInDirectory(FT_LibraryRec_* lib, const fs::path& drc, char32_t first, char32_t last, vector<fs::path>& fonts);
	static void listFontFamiliesInDirectoryThread(std::stop_token stoken, FT_LibraryRec_* lib, const fs::path& drc, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
	static FT_FaceRec_* openFace(FT_LibraryRec_* lib, const fs::path& file, char32_t first, char32_t last, vector<byte_t>& fdata);
#ifdef _WIN32
	static fs::path searchFontRegistry(const fs::path& font);
#ifndef __MINGW32__
	template <HKEY root> static void listFontFilesInRegistry(FT_LibraryRec_* lib, char32_t first, char32_t last, vector<fs::path>& fonts);
	template <HKEY root> static void listFontFamiliesInRegistryThread(std::stop_token stoken, FT_LibraryRec_* lib, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
#endif
#endif
	static fs::path localFontDir();
	static fs::path systemFontDir();
	static void SDLCALL logWrite(void* userdata, int category, SDL_LogPriority priority, const char* message);
};

inline string FileSys::sanitizeFontPath(const fs::path& path) const {
	return fromPath(path.native().starts_with(dirConfs.native()) ? path.stem() : path);
}

inline fs::path FileSys::localFontDir() {
#ifdef _WIN32
	return fs::path(_wgetenv(L"LocalAppdata")) / L"Microsoft\\Windows\\Fonts";
#else
	return fs::path(getenv("HOME")) / ".fonts";
#endif
}

inline fs::path FileSys::systemFontDir() {
#ifdef _WIN32
	return fs::path(_wgetenv(L"SystemDrive")) / L"Windows\\Fonts";
#else
	return "/usr/share/fonts";
#endif
}
