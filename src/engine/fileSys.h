#pragma once

#include "utils/settings.h"
#include "utils/stvector.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#ifdef WITH_SDL3
#include <SDL3/SDL_log.h>
#else
#include <SDL_log.h>
#endif
#include <stop_token>

struct FT_FaceRec_;
struct FT_LibraryRec_;

// handles all filesystem interactions
class FileSys {
public:
	struct ListFontFamiliesData {
		string cdir;
		string desired;
		char32_t first, last;

		ListFontFamiliesData(string&& dir, string&& selected, char32_t from, char32_t to) noexcept;
	};

	struct MoveContentData {
		string src, dst;

		MoveContentData(string&& sdir, string&& ddir) noexcept;
	};
private:
#ifdef _WIN32
	static constexpr wchar_t fontsKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
#endif
	static constexpr char fileThemes[] = "themes.ini";
	static constexpr char fileSettings[] = "settings.ini";
	static constexpr char fileBindings[] = "bindings.ini";
	static constexpr char fileBooks[] = "books.csv";

	static constexpr char iniKeywordCompression[] = "compression";
	static constexpr char iniKeywordDeadzone[] = "deadzone";
	static constexpr char iniKeywordDevice[] = "device";
	static constexpr char iniKeywordDirection[] = "direction";
	static constexpr char iniKeywordDisplay[] = "display";
	static constexpr char iniKeywordFont[] = "font";
	static constexpr char iniKeywordFontMono[] = "font_mono";
	static constexpr char iniKeywordGamma[] = "gamma";
	static constexpr char iniKeywordLibrary[] = "library";
	static constexpr char iniKeywordMaximized[] = "maximized";
	static constexpr char iniKeywordMaxPictureRes[] = "max_picture_res";
	static constexpr char iniKeywordPictureLimit[] = "picture_limit";
	static constexpr char iniKeywordPreview[] = "preview";
	static constexpr char iniKeywordRenderer[] = "renderer";
	static constexpr char iniKeywordResolution[] = "resolution";
	static constexpr char iniKeywordScreen[] = "screen";
	static constexpr char iniKeywordScrollSpeed[] = "scroll_speed";
	static constexpr char iniKeywordShowHidden[] = "show_hidden";
	static constexpr char iniKeywordSpacing[] = "spacing";
	static constexpr char iniKeywordTheme[] = "theme";
	static constexpr char iniKeywordTooltips[] = "tooltips";
	static constexpr char iniKeywordVSync[] = "vsync";
	static constexpr char iniKeywordZoom[] = "zoom";

	string dirSets;	// settings directory
	string dirConfs;	// internal config directory
	SDL_RWops* logFile;
#ifdef CAN_FONTCFG
	void* fontconfig = nullptr;	// is class Fontconfig
#endif
public:
	FileSys();
	~FileSys();

	vector<string> getAvailableThemes() const;
	array<vec4, Settings::defaultColors.size()> loadColors(string_view theme) const;	// updates settings' colors according to settings' theme
	stvector<string, Settings::maxPageElements> getLastPage(string_view book) const;
	void saveLastPage(const stvector<string, Settings::maxPageElements>& paths) const;
	uptr<Settings> loadSettings() const;
	void saveSettings(const Settings* sets) const;
	array<Binding, Binding::names.size()> loadBindings() const;
	void saveBindings(const array<Binding, Binding::names.size()>& bindings) const;
	nstring findFont(const string& font) const;	// on success returns absolute path to font file, otherwise returns empty path
	vector<nstring> listFontFiles(FT_LibraryRec_* lib, char32_t first, char32_t last) const;
	static void listFontFamiliesThread(std::stop_token stoken, uptr<ListFontFamiliesData> ld) noexcept;
	static bool isFont(const string& path) noexcept;
	static void moveContentThread(std::stop_token stoken, uptr<MoveContentData> md) noexcept;

	const string& getDirSets() const noexcept { return dirSets; }
	const string& getDirConfs() const noexcept { return dirConfs; }
	string dirIcons() const { return dirConfs / "icons"; }
	string sanitizeFontPath(const nstring& path) const;

	static string currentDirectory();
	static bool isRegular(const string& path) noexcept;
	static bool isDirectory(const string& path) noexcept;
#ifdef _WIN32
	static bool isDirectory(const wchar_t* path) noexcept;
#else
	static bool hasModeFlags(const char* path, mode_t flags) noexcept;
#endif
	static Data readBinaryFile(const string& path) noexcept;
	static Data readBinaryFile(const nchar* path) noexcept;
#if defined(EXT_DIRECT3D_SHADERS) || defined(EXT_VULKAN_SHADERS)
	pair<uptr<uint32[]>, size_t> readShaderFile(const char* name) const;
#endif
private:
	static string readTextFile(const char* path) noexcept;
	static string_view readNextLine(string_view& text) noexcept;

	static bool isFont(const nchar* path) noexcept;
	static nstring searchFontDirectory(const nstring& font, const nstring& drc);
	static void listFontFilesInDirectory(FT_LibraryRec_* lib, const nstring& drc, char32_t first, char32_t last, vector<nstring>& fonts);
	static void listFontFamiliesInDirectorySubthread(const std::stop_token& stoken, FT_LibraryRec_* lib, const nstring& drc, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
	static FT_FaceRec_* openFace(FT_LibraryRec_* lib, const nchar* file, char32_t first, char32_t last, Data& fdata);
#ifdef _WIN32
	static nstring searchFontRegistry(const nstring& font);
#ifndef __MINGW32__
	template <HKEY root> static void listFontFilesInRegistry(FT_LibraryRec_* lib, char32_t first, char32_t last, vector<nstring>& fonts);
	template <HKEY root> static void listFontFamiliesInRegistrySubthread(const std::stop_token& stoken, FT_LibraryRec_* lib, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
#endif
#endif
	static nstring localFontDir();
	static nstring systemFontDir();
	static bool toBool(string_view str) noexcept;
	static void SDLCALL logWrite(void* userdata, int category, SDL_LogPriority priority, const char* message) noexcept;
};

inline bool FileSys::isFont(const string& path) noexcept {
#ifdef _WIN32
	return isFont(sstow(path).data());
#else
	return isFont(path.data());
#endif
}

inline Data FileSys::readBinaryFile(const string& path) noexcept {
#ifdef _WIN32
	return readBinaryFile(sstow(path).data());
#else
	return readBinaryFile(path.data());
#endif
}

inline nstring FileSys::localFontDir() {
#ifdef _WIN32
	return joinPaths(wstring_view(_wgetenv(L"LocalAppdata")), L"Microsoft\\Windows\\Fonts"sv);
#else
	return joinPaths(string_view(Settings::homeDir()), ".fonts"sv);
#endif
}

inline nstring FileSys::systemFontDir() {
#ifdef _WIN32
	return joinPaths(wstring_view(_wgetenv(L"SystemDrive")), L"Windows\\Fonts"sv);
#else
	return "/usr/share/fonts";
#endif
}

inline bool FileSys::toBool(string_view str) noexcept {
	return strciequal(str, "true") || strciequal(str, "on") || strciequal(str, "y") || rng::any_of(str, [](char c) -> bool { return c >= '1' && c <= '9'; });
}
