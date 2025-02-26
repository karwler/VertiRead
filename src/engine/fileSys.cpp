#include "fileSys.h"
#include "optional/fontconfig.h"
#include "prog/types.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef WITH_SDL3
#include <SDL3/SDL_filesystem.h>
#else
#include <SDL_filesystem.h>
#endif
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#endif
#include <filesystem>
#include <map>
#ifndef WITH_SDL3
#include <mutex>
#endif
#include <regex>
#include <span>
namespace fs = std::filesystem;

#ifdef _WIN32
#define makeNative(s) sstow(s)
#define fromNative(s) swtos(s)
#else
#define makeNative(s) (s)
#define fromNative(s) (s)
#endif

namespace {

#ifndef WITH_SDL3
std::mutex logLock;
#endif

struct QasciiViewCiLess {
	bool operator()(string_view a, string_view b) const noexcept {
		if (int diff = a.length() - b.length())
			return diff < 0;
		for (size_t i = 0; i < a.length(); ++i)
			if (int diff = toupper(a[i]) - toupper(b[i]))
				return diff < 0;
		return false;
	}
};

#ifdef _WIN32
class RegistryIterator {
private:
	HKEY key = nullptr;
	DWORD i = 0;
	DWORD type;
	DWORD nlen, dlen;
	wchar_t name[MAX_PATH];
	wchar_t data[MAX_PATH];

public:
	RegistryIterator(HKEY root, const wchar_t* path) noexcept { RegOpenKeyExW(root, path, 0, KEY_READ, &key); }
	~RegistryIterator() noexcept { RegCloseKey(key); }

	bool next() noexcept;
	operator bool() const noexcept { return key; }
	DWORD getType() const noexcept { return type; }
	wstring_view getString() const { return wstring_view(data, dlen / sizeof(wchar_t) - 1); }
};

bool RegistryIterator::next() noexcept {
	nlen = std::size(name);
	dlen = sizeof(data);
	return RegEnumValueW(key, i++, name, &nlen, nullptr, &type, reinterpret_cast<BYTE*>(data), &dlen) == ERROR_SUCCESS;
}
#endif

#ifdef CAN_FONTCFG
class Fontconfig {
private:
	FcConfig* config;

public:
	Fontconfig();
	~Fontconfig() { fcConfigDestroy(config); }

	nstring search(const char* font);
	void list(char32_t first, char32_t last, vector<nstring>& fonts);
	void list(char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
};

Fontconfig::Fontconfig() {
	if (config = fcInitLoadConfigAndFonts(); !config)
		throw std::runtime_error("Failed to init fontconfig");
}

nstring Fontconfig::search(const char* font) {
	nstring found;
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(font))) {
		if (fcConfigSubstitute(config, pattern, FcMatchPattern)) {
			fcDefaultSubstitute(pattern);
			if (FcResult res; FcPattern* fmpat = fcFontMatch(config, pattern, &res)) {
				if (FcChar8* file; fcPatternGetString(fmpat, FC_FILE, 0, &file) == FcResultMatch)
					found = makeNative(reinterpret_cast<const char*>(file));
				fcPatternDestroy(fmpat);
			}
		}
		fcPatternDestroy(pattern);
	}
	return found;
}

void Fontconfig::list(char32_t first, char32_t last, vector<nstring>& fonts) {
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(fmt::format(":charset={:X} {:X}", uint32(first), uint32(last)).data()))) {
		if (FcObjectSet* objects = fcObjectSetBuild(FC_FILE, FC_CHARSET, nullptr)) {
			if (FcFontSet* flist = fcFontList(config, pattern, objects)) {
				for (int i = 0; i < flist->nfont; ++i)
					if (FcChar8* file; fcPatternGetString(flist->fonts[i], FC_FILE, 0, &file) == FcResultMatch)
						fonts.emplace_back(makeNative(reinterpret_cast<const char*>(file)));
				fcFontSetDestroy(flist);
			}
			fcObjectSetDestroy(objects);
		}
		fcPatternDestroy(pattern);
	}
}

void Fontconfig::list(char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(fmt::format(":charset={:X} {:X}", uint32(first), uint32(last)).data()))) {
		if (FcObjectSet* objects = fcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_CHARSET, nullptr)) {
			if (FcFontSet* flist = fcFontList(config, pattern, objects)) {
				FcChar8* family;
				FcChar8* style;
				FcChar8* file;
				for (int i = 0; i < flist->nfont; ++i)
					if (fcPatternGetString(flist->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch && fcPatternGetString(flist->fonts[i], FC_FILE, 0, &file) == FcResultMatch) {
						if (fcPatternGetString(flist->fonts[i], FC_STYLE, 0, &style) == FcResultMatch)
							fonts.emplace_back(fmt::format("{} {}", reinterpret_cast<char*>(family), reinterpret_cast<char*>(style)), reinterpret_cast<char*>(file));
						else
							fonts.emplace_back(reinterpret_cast<char*>(family), reinterpret_cast<char*>(file));
					}
				fcFontSetDestroy(flist);
			}
			fcObjectSetDestroy(objects);
		}
		fcPatternDestroy(pattern);
	}
}
#endif

struct IniLine {
	enum class Type : uint8 {
		empty,
		prpVal,
		prpKeyVal,
		title
	};

	string_view prp;
	string_view key;
	string_view val;

	Type setLine(string_view str) noexcept;

	template <class T> static void writeVal(SDL_RWops* ofh, string_view prp, const T& val);
	template <class K, class T> static void writeKeyVal(SDL_RWops* ofh, string_view prp, const K& key, const T& val);
};

IniLine::Type IniLine::setLine(string_view str) noexcept {
	size_t i0 = str.find_first_of('=');
	size_t i1 = str.find_first_of('[');
	size_t i2 = str.find_first_of(']', i1);
	if (i0 != string::npos) {
		val = str.substr(i0 + 1);
		if (i2 < i0) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return Type::prpKeyVal;
		}
		prp = trim(str.substr(0, i0));
		key = string_view();
		return Type::prpVal;
	}
	if (i2 != string::npos) {
		prp = trim(str.substr(i1 + 1, i2 - i1 - 1));
		key = string_view();
		val = string_view();
		return Type::title;
	}
	prp = string_view();
	val = string_view();
	key = string_view();
	return Type::empty;
}

template <class T>
void IniLine::writeVal(SDL_RWops* ofh, string_view prp, const T& val) {
	string line = fmt::format("{}={}" LINEND, prp, val);
	SDL_RWwrite(ofh, line.data(), sizeof(char), line.length());
}

template <class K, class T>
void IniLine::writeKeyVal(SDL_RWops* ofh, string_view prp, const K& key, const T& val) {
	string line = fmt::format("{}[{}]={}" LINEND, prp, key, val);
	SDL_RWwrite(ofh, line.data(), sizeof(char), line.length());
}

struct CsvText {
	enum class Code : uint8 {
		end,
		field,
		last
	};

	string field;
	const char* text;
	const char* lineStart;
	const char* lineEnd;
	bool nextLine = true;

	CsvText(const char* str) noexcept;

	template <bool fill = true> Code readField();

	static string makeLine(std::span<const string> fields);
};

CsvText::CsvText(const char* str) noexcept :
	text(str + strspn(str, "\r\n"))
{}

template <bool fill>
CsvText::Code CsvText::readField() {
	char ch = *text;
	if (!ch)
		return Code::end;
	if (nextLine)
		lineStart = text;

	if (ch != '"') {
		size_t elen = strcspn(text, ",\r\n");
		if constexpr (fill)
			field.assign(text, elen);
		text += elen;
	} else {
		const char* end;
		if constexpr (fill) {
			field.clear();
			text = readQuoteString(text + 1, field);
			text += strcspn(text, ",\r\n");
		} else {
			for (end = strchr(++text, '"'); end && end[1] == '"'; end = strchr(end + 2, '"'));
			text = end ? end + strcspn(end + 1, ",\r\n") : text + strlen(text);
		}
	}

	if (nextLine = *text != ','; !nextLine) {
		++text;
		return Code::field;
	}
	lineEnd = text;
	text += strspn(text, "\r\n");
	return Code::last;
}

string CsvText::makeLine(std::span<const string> fields) {
	string line;
	if (!fields.empty()) {
		for (string_view fld : fields) {
			if (size_t e = fld.find_first_of(",\"\r\n"); e == string_view::npos)
				line += fld;
			else {
				size_t p = 0;
				line += '"';
				if (fld[e] == '"') {
					line.append(fld.data(), e);
					line += '"';
					p = e++;
				}
				while ((e = fld.find('"', e)) != string_view::npos) {
					line.append(fld.data() + p, e - p);
					line += '"';
					p = e++;
				}
				line.append(fld.data() + p, fld.length() - p);
				line += '"';
			}
			line += ',';
		}
		line.pop_back();
	}
	return line;
}

}

FileSys::ListFontFamiliesData::ListFontFamiliesData(string&& dir, string&& selected, char32_t from, char32_t to) noexcept :
	cdir(std::move(dir)),
	desired(std::move(selected)),
	first(from),
	last(to)
{}

FileSys::MoveContentData::MoveContentData(string&& sdir, string&& ddir) noexcept :
	src(std::move(sdir)),
	dst(std::move(ddir))
{}

FileSys::FileSys() {
	// set up file/directory path constants
	string dirBase;
#ifdef WITH_SDL3
	if (const char* path = SDL_GetBasePath())
#ifdef _WIN32
		dirBase = path;
#else
		dirBase = parentPath(path);
#endif
#else
	if (uptr<char[], SdlFreePtr> path(SDL_GetBasePath()); path)
#ifdef _WIN32
		dirBase = path.get();
#else
		dirBase = parentPath(path.get());
#endif
#endif
	if (dirBase.empty())
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get base directory");

#ifdef _WIN32
	dirSets = swtos(_wgetenv(L"AppData")) / "VertiRead";
	dirConfs = std::move(dirBase);
#else
	dirSets = joinPaths(string_view(Settings::homeDir()), ".local/share/vertiread"sv);
	dirConfs = dirBase / "share/vertiread";
#endif

	std::error_code ec;
	if (fs::create_directories(makeNative(dirSets), ec); ec)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create settings directory: %s", ec.message().data());
	try {
		std::regex rgx(R"r(log_[\d-]+\.txt)r", std::regex::icase | std::regex::optimize);
		for (const fs::directory_entry& it : fs::directory_iterator(makeNative(dirSets), fs::directory_options::skip_permission_denied))
			if (string name = fromNative(it.path().filename().native()); std::regex_match(name, rgx) && it.is_regular_file(ec))
				if (fs::remove(it.path(), ec); ec)
					SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to remove old log file '%s': %s", name.data(), ec.message().data());
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}

	if (!Settings::hasFlag(Settings::flagLog)) {
		tm tim = currentDateTime();
		if (logFile = SDL_RWFromFile((dirSets / fmt::format("log_{}-{:02}-{:02}.txt", tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday)).data(), "wb"); logFile)
			SDL_LogSetOutputFunction(logWrite, logFile);
		else
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create log file: %s", SDL_GetError());
	}
	if (!isDirectory(dirIcons()))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find icons directory: %s", ec.message().data());
	if (!isRegular(dirConfs / fileThemes))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find themes file: %s", ec.message().data());

#ifdef CAN_FONTCFG
	try {
		if (symFontconfig())
			fontconfig = new Fontconfig;
	} catch (const std::runtime_error& err) {
		closeFontconfig();
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
#endif
}

FileSys::~FileSys() {
#ifdef CAN_FONTCFG
	delete static_cast<Fontconfig*>(fontconfig);
	closeFontconfig();
#endif
	if (logFile) {
		SDL_LogSetOutputFunction(nullptr, nullptr);
		SDL_RWclose(logFile);
	}
}

vector<string> FileSys::getAvailableThemes() const {
	vector<string> themes;
	IniLine il;
	const string* locations[2] = { &dirSets, &dirConfs };
	for (size_t i = 0; i < std::size(locations) && themes.empty(); ++i) {
		string text = readTextFile((*locations[i] / fileThemes).data());
		for (string_view tx = text; !tx.empty();)
			if (il.setLine(readNextLine(tx)) == IniLine::Type::title)
				themes.emplace_back(il.prp);
	}
	return !themes.empty() ? themes : vector<string>{ "default" };
}

array<vec4, Settings::defaultColors.size()> FileSys::loadColors(string_view theme) const {
	array<vec4, Settings::defaultColors.size()> colors = Settings::defaultColors;
	string text = readTextFile((dirSets / fileThemes).data());
	if (text.empty())
		text = readTextFile((dirConfs / fileThemes).data());

	IniLine il;	// find title equal to theme and read colors until the end of the file or another title
	string_view tx = text;
	while (!tx.empty() && (il.setLine(readNextLine(tx)) != IniLine::Type::title || il.prp != theme));
	for (IniLine::Type type; !tx.empty() && (type = il.setLine(readNextLine(tx))) != IniLine::Type::title;)
		if (type == IniLine::Type::prpVal)
			if (size_t cid = strToEnum<size_t>(Settings::colorNames, il.prp); cid < colors.size())
				colors[cid] = toVec<vec4>(il.val);
	return colors;
}

stvector<string, Settings::maxPageElements> FileSys::getLastPage(string_view book) const {
	string text = readTextFile((dirSets / fileBooks).data());
	CsvText csv = text.data();
	for (CsvText::Code cc; (cc = csv.readField()) != CsvText::Code::end;)
		if (cc == CsvText::Code::field) {
			if (csv.field == book) {
				stvector<string, Settings::maxPageElements> paths;
				while (paths.size() < paths.max_size() && (cc = csv.readField()) != CsvText::Code::end) {
					paths.push_back(std::move(csv.field));
					if (cc == CsvText::Code::last)
						break;
				}
				if (!paths.empty())
					return paths;
			} else
				while (csv.readField<false>() == CsvText::Code::field);
		}
	return stvector<string, Settings::maxPageElements>();
}

void FileSys::saveLastPage(const stvector<string, Settings::maxPageElements>& paths) const {
	string file = dirSets / fileBooks;
	string text = readTextFile(file.data());
	CsvText csv = text.data();
	CsvText::Code cc;
	while ((cc = csv.readField()) != CsvText::Code::end) {
		if (csv.field == paths[0]) {
			for (; cc == CsvText::Code::field; cc = csv.readField<false>());
			break;
		}
		for (; cc == CsvText::Code::field; cc = csv.readField<false>());
	}

	if (uptr<SDL_RWops> ofh(SDL_RWFromFile(file.data(), cc == CsvText::Code::end ? "ab" : "wb")); ofh) {
		string line = CsvText::makeLine(std::span(paths));
		if (cc == CsvText::Code::end) {
			if (!text.empty() && text.back() != '\n' && text.back() != '\r')
				SDL_RWwrite(ofh.get(), LINEND, sizeof(char), strlen(LINEND));
			SDL_RWwrite(ofh.get(), line.data(), sizeof(char), line.length());
			SDL_RWwrite(ofh.get(), LINEND, sizeof(char), strlen(LINEND));
		} else {
			text.replace(csv.lineStart - text.data(), csv.lineEnd - csv.lineStart, line);
			SDL_RWwrite(ofh.get(), text.data(), sizeof(char), text.length());
		}
	} else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to write books file '%s': %s", file.data(), SDL_GetError());
}

uptr<Settings> FileSys::loadSettings() const {
	std::map<string_view, void (*)(const FileSys*, Settings*, string_view), QasciiViewCiLess> assignPrpVal = {
		{ iniKeywordCompression, [](const FileSys*, Settings* s, string_view v) { s->compression = strToEnum(Settings::compressionNames, trim(v), Settings::defaultCompression); } },
		{ iniKeywordDeadzone, [](const FileSys*, Settings* s, string_view v) { s->setDeadzone(toNum<uint16>(v)); } },
		{ iniKeywordDevice, [](const FileSys*, Settings* s, string_view v) { s->device = toVec<u32vec2>(v, 0, 0x10); } },
		{ iniKeywordDirection, [](const FileSys*, Settings* s, string_view v) { s->direction = strToEnum(Direction::names, trim(v), Settings::defaultDirection); } },
		{ iniKeywordFont, [](const FileSys* f, Settings* s, string_view v) { s->font = isFont(f->findFont(string(v)).data()) ? v : Settings::defaultFont; } },	// will get sanitized in DrawSys if necessary
		{ iniKeywordFontMono, [](const FileSys*, Settings* s, string_view v) { s->monoFont = toBool(trim(v)); } },
		{ iniKeywordGamma, [](const FileSys*, Settings* s, string_view v) { s->setGamma(v); } },
		{ iniKeywordLibrary, [](const FileSys*, Settings* s, string_view v) { s->dirLib = v; } },
		{ iniKeywordMaximized, [](const FileSys*, Settings* s, string_view v) { s->maximized = toBool(trim(v)); } },
		{ iniKeywordMaxPictureRes, [](const FileSys*, Settings* s, string_view v) { s->maxPicRes = std::max(toNum<uint>(v), Settings::minPicRes); } },
		{ iniKeywordPictureLimit, [](const FileSys*, Settings* s, string_view v) { s->picLim.set(v); } },
		{ iniKeywordPreview, [](const FileSys*, Settings* s, string_view v) { s->preview = strToEnum<Settings::Preview>(Settings::previewNames, trim(v), Settings::defaultPreview); } },
		{ iniKeywordRenderer, [](const FileSys*, Settings* s, string_view v) { s->renderer = Settings::getRenderer(v); } },
		{ iniKeywordResolution, [](const FileSys*, Settings* s, string_view v) { s->resolution = toVec<ivec2>(v); } },
		{ iniKeywordScreen, [](const FileSys*, Settings* s, string_view v) { s->screen = strToEnum(Settings::screenModeNames, trim(v), Settings::defaultScreenMode); } },
		{ iniKeywordScrollSpeed, [](const FileSys*, Settings* s, string_view v) { s->scrollSpeed = toVec<vec2>(v); } },
		{ iniKeywordShowHidden, [](const FileSys*, Settings* s, string_view v) { s->showHidden = toBool(trim(v)); } },
		{ iniKeywordSpacing, [](const FileSys*, Settings* s, string_view v) { s->spacing = toNum<ushort>(v); } },
		{ iniKeywordTheme, [](const FileSys* f, Settings* s, string_view v) { s->setTheme(v, f->getAvailableThemes()); } },
		{ iniKeywordTooltips, [](const FileSys*, Settings* s, string_view v) { s->tooltips = toBool(trim(v)); } },
		{ iniKeywordVSync, [](const FileSys*, Settings* s, string_view v) { s->vsync = toBool(trim(v)); } },
		{ iniKeywordZoom, [](const FileSys*, Settings* s, string_view v) { s->setZoom(v); } }
	};
	uptr<Settings> sets = std::make_unique<Settings>(getAvailableThemes());
	IniLine il;
	string text = readTextFile((dirSets / fileSettings).data());
	for (string_view tx = text; tx.length();) {
		switch (il.setLine(readNextLine(tx))) {
		using enum IniLine::Type;
		case prpVal:
			if (auto ait = assignPrpVal.find(il.prp); ait != assignPrpVal.end())
				ait->second(this, sets.get(), il.val);
			break;
		case prpKeyVal:
			if (strciequal(il.prp, iniKeywordDisplay))
				sets->displays.emplace_back(toVec<ivec4>(il.val), toNum<int>(il.key));
		}
	}
	sets->unionDisplays();
	sets->setRenderer();
	return sets;
}

void FileSys::saveSettings(const Settings* sets) const {
	string file = dirSets / fileSettings;
	if (uptr<SDL_RWops> ofh(SDL_RWFromFile(file.data(), "wb")); ofh) {
		IniLine::writeVal(ofh.get(), iniKeywordCompression, Settings::compressionNames[eint(sets->compression)]);
		IniLine::writeVal(ofh.get(), iniKeywordDeadzone, sets->getDeadzone());
		IniLine::writeVal(ofh.get(), iniKeywordDevice, toStr<0x10>(sets->device));
		IniLine::writeVal(ofh.get(), iniKeywordDirection, Direction::names[uint8(sets->direction)]);
		for (const Settings::Display& it : sets->displays)
			IniLine::writeKeyVal(ofh.get(), iniKeywordDisplay, it.did, toStr(it.rect.asVec()));
		IniLine::writeVal(ofh.get(), iniKeywordFont, sets->font);
		IniLine::writeVal(ofh.get(), iniKeywordFontMono, toStr(sets->monoFont));
		IniLine::writeVal(ofh.get(), iniKeywordGamma, fmt::format("{} {}", Settings::gammaNames[eint(sets->gammaType)], uint(sets->gammaValue)));
		IniLine::writeVal(ofh.get(), iniKeywordLibrary, sets->dirLib);
		IniLine::writeVal(ofh.get(), iniKeywordMaximized, toStr(sets->maximized));
		IniLine::writeVal(ofh.get(), iniKeywordMaxPictureRes, sets->maxPicRes);
		IniLine::writeVal(ofh.get(), iniKeywordPictureLimit, fmt::format("{} {} {}", PicLim::names[eint(sets->picLim.type)], sets->picLim.count, PicLim::memoryString(sets->picLim.size)));
		IniLine::writeVal(ofh.get(), iniKeywordPreview, Settings::previewNames[eint(sets->preview)]);
		IniLine::writeVal(ofh.get(), iniKeywordRenderer, Settings::rendererNames[eint(sets->renderer)]);
		IniLine::writeVal(ofh.get(), iniKeywordResolution, toStr(sets->resolution));
		IniLine::writeVal(ofh.get(), iniKeywordScreen, Settings::screenModeNames[eint(sets->screen)]);
		IniLine::writeVal(ofh.get(), iniKeywordScrollSpeed, toStr(sets->scrollSpeed));
		IniLine::writeVal(ofh.get(), iniKeywordShowHidden, toStr(sets->showHidden));
		IniLine::writeVal(ofh.get(), iniKeywordSpacing, sets->spacing);
		IniLine::writeVal(ofh.get(), iniKeywordTheme, sets->getTheme());
		IniLine::writeVal(ofh.get(), iniKeywordTooltips, toStr(sets->tooltips));
		IniLine::writeVal(ofh.get(), iniKeywordVSync, toStr(sets->vsync));
		IniLine::writeVal(ofh.get(), iniKeywordZoom, fmt::format("{} {}", Settings::zoomNames[eint(sets->zoomType)], int(sets->zoom)));
	} else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to write settings file '%s': %s", file.data(), SDL_GetError());
}

array<Binding, Binding::names.size()> FileSys::loadBindings() const {
	array<Binding, Binding::names.size()> bindings;
	for (size_t i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));

	IniLine il;
	string text = readTextFile((dirSets / fileBindings).data());
	for (string_view tx = text; tx.length();) {
		if (il.setLine(readNextLine(tx)) != IniLine::Type::prpVal)
			continue;
		size_t bid = strToEnum<size_t>(Binding::names, il.prp);
		if (bid >= bindings.size())
			continue;
		string_view bdsc = trim(il.val);
		if (bdsc.length() < 3)
			continue;

		switch (toupper(bdsc[0])) {
		case 'K':	// keyboard key
			bindings[bid].setKey(SDL_GetKeyFromName(string(bdsc).data() + 2));
			break;
		case 'B':	// joystick button
			bindings[bid].setJbutton(toNum<uint8>(bdsc.substr(2)));
			break;
		case 'H':	// joystick hat
			if (size_t id = std::find_if(bdsc.begin() + 2, bdsc.end(), [](char c) -> bool { return !isdigit(c); }) - bdsc.begin(); id < bdsc.length())
				bindings[bid].setJhat(toNum<uint8>(bdsc.substr(2, id - 2)), Binding::hatNameToValue(bdsc.substr(id + 1)));
			break;
		case 'A':	// joystick axis
			bindings[bid].setJaxis(toNum<uint8>(bdsc.substr(3)), bdsc[2] != '-');
			break;
		case 'G':	// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Binding::gbuttonNames, bdsc.substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case 'X':	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Binding::gaxisNames, bdsc.substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (bdsc[2] != '-'));
			break;
		default:
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid binding identifier: %c", bdsc[0]);
		}
	}
	return bindings;
}

void FileSys::saveBindings(const array<Binding, Binding::names.size()>& bindings) const {
	string file = dirSets / fileBindings;
	if (uptr<SDL_RWops> ofh(SDL_RWFromFile(file.data(), "wb")); ofh) {
		for (size_t i = 0; i < bindings.size(); ++i) {
			if (bindings[i].keyAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("K_{}", SDL_GetKeyName(bindings[i].getKey())));

			if (bindings[i].jbuttonAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("B_{}", uint(bindings[i].getJctID())));
			else if (bindings[i].jhatAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("H_{}_{}", uint(bindings[i].getJctID()), Binding::hatValueToName(bindings[i].getJhatVal())));
			else if (bindings[i].jaxisAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("A_{}{}", bindings[i].jposAxisAssigned() ? '+' : '-', uint(bindings[i].getJctID())));

			if (bindings[i].gbuttonAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("G_{}", Binding::gbuttonNames[eint(bindings[i].getGbutton())]));
			else if (bindings[i].gbuttonAssigned())
				IniLine::writeVal(ofh.get(), Binding::names[i], fmt::format("X_{}{}", bindings[i].gposAxisAssigned() ? '+' : '-', Binding::gaxisNames[eint(bindings[i].getGaxis())]));
		}
	} else
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to write bindings file '%s': %s", file.data(), SDL_GetError());
}

string FileSys::currentDirectory() {
#ifdef _WIN32
	wchar_t buf[MAX_PATH];
	DWORD len = GetCurrentDirectory(MAX_PATH, buf);
	return len && len < MAX_PATH ? swtos(wstring_view(buf, len)) : string();
#else
	char buf[PATH_MAX];
	return getcwd(buf, PATH_MAX) ? buf : string();
#endif
}

bool FileSys::isRegular(const string& path) noexcept {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(sstow(path).data());
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
	return hasModeFlags(path.data(), S_IFREG);
#endif
}

bool FileSys::isDirectory(const string& path) noexcept {
#ifdef _WIN32
	return isDirectory(sstow(path).data());
#else
	return hasModeFlags(path.data(), S_IFDIR);
#endif
}

#ifdef _WIN32
bool FileSys::isDirectory(const wchar_t* path) noexcept {
	DWORD attr = GetFileAttributesW(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

#else

bool FileSys::hasModeFlags(const char* path, mode_t flags) noexcept {
	struct stat ps;
	return !stat(path, &ps) && (ps.st_mode & flags);
}
#endif

Data FileSys::readBinaryFile(const nchar* path) noexcept {
	Data data;
#ifdef _WIN32
	if (HANDLE fh = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr); fh != INVALID_HANDLE_VALUE) {
		if (LARGE_INTEGER siz; GetFileSizeEx(fh, &siz)) {
			try {
				data.resize(siz.QuadPart);
				if (DWORD len; ReadFile(fh, data.data(), data.size(), &len, nullptr)) {
					if (len < data.size())
						data.resize(len);
				} else
					data.clear();
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				data.clear();
			}
		}
		CloseHandle(fh);
	}
#else
	if (int fd = open(path, O_RDONLY); fd != -1) {
		if (struct stat ps; !fstat(fd, &ps)) {
			try {
				data.resize(ps.st_size);
				if (ssize_t len = read(fd, data.data(), data.size()); len < ps.st_size)
					data.resize(std::max(len, ssize_t(0)));
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				data.clear();
			}
		}
		close(fd);
	}
#endif
	return data;
}

#if defined(EXT_DIRECT3D_SHADERS) || defined(EXT_VULKAN_SHADERS)
pair<uptr<uint32[]>, size_t> FileSys::readShaderFile(const char* name) const {
	uptr<SDL_RWops> ifh(SDL_RWFromFile((dirConfs / "shaders" / name).data(), "rb"));
	if (!ifh)
		throw std::runtime_error(fmt::format("Failed to read shader: {}", SDL_GetError()));
	int64 siz = SDL_RWsize(ifh.get());
	if (siz <= 0)
		throw std::runtime_error(fmt::format("Failed to read shader: {}", SDL_GetError()));
	uptr<uint32[]> data = std::make_unique_for_overwrite<uint32[]>(siz / sizeof(uint32));
	siz = SDL_RWread(ifh.get(), data.get(), 1, siz - siz % sizeof(uint32));
	if (siz -= siz % sizeof(uint32); !siz)
		throw std::runtime_error(fmt::format("Failed to read shader: {}", SDL_GetError()));
	return pair(std::move(data), siz);
}
#endif

string FileSys::readTextFile(const char* path) noexcept {
	string text;
	if (uptr<SDL_RWops> ifh(SDL_RWFromFile(path, "rb")); ifh)
		if (int64 siz = SDL_RWsize(ifh.get()); siz > 0) {
			try {
				text.resize(siz);
				if (size_t len = SDL_RWread(ifh.get(), text.data(), sizeof(char), siz); len < text.length())
					text.resize(len);
				if (text.starts_with("\xEF\xBB\xBF"))
					text.erase(0, 3);
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				text.clear();
			}
		}
	return text;
}

string_view FileSys::readNextLine(string_view& text) noexcept {
	auto pos = rng::find_if(text, [](char ch) -> bool { return ch != '\n' && ch != '\r'; });
	auto end = std::find_if(pos, text.end(), [](char ch) -> bool { return ch == '\n' || ch == '\r'; });
	text = string_view(end, text.end());
	return string_view(pos, end);
}

bool FileSys::isFont(const nchar* file) noexcept {
	bool ret = false;
#ifdef _WIN32
	if (HANDLE fh = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr); fh != INVALID_HANDLE_VALUE) {
		char sig[5];
		DWORD len;
		ret = ReadFile(fh, sig, sizeof(sig), &len, nullptr) && len == sizeof(sig) && (!memcmp(sig, "\0\1\0\0\0", 5) || !memcmp(sig, "OTTO", 4) || !memcmp(sig, "\1fcp", 4));
		CloseHandle(fh);
	}
#else
	if (int fd = open(file, O_RDONLY)) {
		char sig[5];
		ret = read(fd, sig, sizeof(sig)) == sizeof(sig) && (!memcmp(sig, "\0\1\0\0\0", 5) || !memcmp(sig, "OTTO", 4) || !memcmp(sig, "\1fcp", 4));
		close(fd);
	}
#endif
	return ret;
}

void FileSys::moveContentThread(std::stop_token stoken, uptr<MoveContentData> md) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		fs::path src = makeNative(md->src), dst = makeNative(md->dst);
		fs::create_directories(dst);
		vector<fs::path> entries;
		for (const fs::directory_entry& it : fs::directory_iterator(src, fs::directory_options::skip_permission_denied))
			entries.push_back(it.path().filename());
		for (uintptr_t i = 0, lim = entries.size(); i < lim; ++i) {
			if (stoken.stop_requested()) {
				rc = ResultCode::stop;
				break;
			}
			pushEvent(SDL_USEREVENT_THREAD_MOVE, ThreadEvent::progress, std::bit_cast<void*>(i), std::bit_cast<void*>(lim));
			fs::rename(src / entries[i], dst / entries[i]);	// TODO: why is this in a loop instead of moving the parent directory?
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_MOVE, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)));
}

nstring FileSys::findFont(const string& font) const {
	if (isFont(font))
		return makeNative(font);
	if (nstring path = searchFontDirectory(makeNative(font), makeNative(dirConfs)); !path.empty())
		return path;
#ifdef CAN_FONTCFG
	if (fontconfig)
		if (nstring path = static_cast<Fontconfig*>(fontconfig)->search(font.data()); !path.empty())
			return path;
#endif
#ifdef _WIN32
	if (nstring path = searchFontRegistry(makeNative(font)); !path.empty())
		return path;
#endif
	if (nstring path = searchFontDirectory(makeNative(font), localFontDir()); !path.empty())
		return path;
	return searchFontDirectory(makeNative(font), systemFontDir());
}

nstring FileSys::searchFontDirectory(const nstring& font, const nstring& drc) {
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec))
		if (it.is_regular_file(ec))
			if (fs::path fname = it.path().filename(); (strciequal(fname.native(), font) || strciequal(fname.stem().native(), font)) && isFont(it.path().c_str()))
				return it.path();
	return nstring();
}

#ifdef _WIN32
nstring FileSys::searchFontRegistry(const nstring& font) {
	for (HKEY root : { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE })
		for (RegistryIterator rit(root, fontsKey); rit.next();)
			if (rit.getType() == REG_SZ) {
				wstring_view fname = rit.getString();
				if (wstring_view freduce = filename(fname); strciequal(freduce, font) || strciequal(delExtension(freduce), font))
					return root == HKEY_CURRENT_USER ? nstring(fname) : systemFontDir() / fname;
			}
	return nstring();
}
#endif

vector<nstring> FileSys::listFontFiles(FT_Library lib, char32_t first, char32_t last) const {
	vector<nstring> fonts;
	listFontFilesInDirectory(lib, makeNative(dirConfs), first, last, fonts);
#ifdef CAN_FONTCFG
	if (fontconfig) {
		static_cast<Fontconfig*>(fontconfig)->list(first, last, fonts);
		return fonts;
	}
#endif
#if defined(_WIN32) && !defined(__MINGW32__)
	listFontFilesInRegistry<HKEY_CURRENT_USER>(lib, first, last, fonts);
	listFontFilesInRegistry<HKEY_LOCAL_MACHINE>(lib, first, last, fonts);
#else
	listFontFilesInDirectory(lib, localFontDir(), first, last, fonts);
	listFontFilesInDirectory(lib, systemFontDir(), first, last, fonts);
#endif
	return fonts;
}

void FileSys::listFontFilesInDirectory(FT_Library lib, const nstring& drc, char32_t first, char32_t last, vector<nstring>& fonts) {
	Data fdata;
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec))
		if (it.is_regular_file(ec))
			if (FT_Face face = openFace(lib, it.path().c_str(), first, last, fdata); face) {
				FT_Done_Face(face);
				fonts.push_back(it.path());
			}
}

#if defined(_WIN32) && !defined(__MINGW32__)
template <HKEY root>
void FileSys::listFontFilesInRegistry(FT_Library lib, char32_t first, char32_t last, vector<nstring>& fonts) {
	Data fdata;
	wstring gfpath;
	if constexpr (root == HKEY_LOCAL_MACHINE)
		gfpath = systemFontDir();
	if (RegistryIterator rit(root, fontsKey); rit) {
		while (rit.next())
			if (rit.getType() == REG_SZ) {
				wstring fpath;
				if constexpr (root == HKEY_CURRENT_USER)
					fpath = rit.getString();
				else
					fpath = gfpath / rit.getString();
				if (FT_Face face = openFace(lib, fpath.data(), first, last, fdata); face) {
					FT_Done_Face(face);
					fonts.push_back(fpath);
				}
			}
	} else
		listFontFilesInDirectory(lib, root == HKEY_CURRENT_USER ? localFontDir() : gfpath, first, last, fonts);
}
#endif

void FileSys::listFontFamiliesThread(std::stop_token stoken, uptr<ListFontFamiliesData> ld) noexcept {
	try {
		FT_Library fl;
		if (FT_Error err = FT_Init_FreeType(&fl))
			throw std::runtime_error(FT_Error_String(err));
		uptr<std::remove_pointer_t<FT_Library>, decltype(&FT_Done_FreeType)> lib(fl, FT_Done_FreeType);
		vector<pair<Cstring, Cstring>> fonts;
		listFontFamiliesInDirectorySubthread(stoken, lib.get(), makeNative(ld->cdir), ld->first, ld->last, fonts);

		string desiredPath;
		if (!isFont(ld->desired)) {
			auto it = rng::find_if(fonts, [&ld](const pair<Cstring, Cstring>& fp) -> bool {
				string_view fname = filename(fp.second.data());
				return strciequal(fname, ld->desired) || strciequal(delExtension(fname), ld->desired);
			});
			if (it != fonts.end())
				desiredPath = it->second.data();
		}

		bool skip = false;
#ifdef CAN_FONTCFG
		if (!stoken.stop_requested() && symFontconfig()) {
			try {
				Fontconfig().list(ld->first, ld->last, fonts);
				skip = true;
			} catch (const std::runtime_error& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			}
		}
#endif
		if (!skip) {
#if defined(_WIN32) && !defined(__MINGW32__)
			listFontFamiliesInRegistrySubthread<HKEY_CURRENT_USER>(stoken, lib.get(), ld->first, ld->last, fonts);
			listFontFamiliesInRegistrySubthread<HKEY_LOCAL_MACHINE>(stoken, lib.get(), ld->first, ld->last, fonts);
#else
			listFontFamiliesInDirectorySubthread(stoken, lib.get(), localFontDir(), ld->first, ld->last, fonts);
			listFontFamiliesInDirectorySubthread(stoken, lib.get(), systemFontDir(), ld->first, ld->last, fonts);
#endif
		}
		rng::sort(fonts, [](const pair<Cstring, Cstring>& a, const pair<Cstring, Cstring>& b) -> bool {
			int cmp = strcmp(a.first.data(), b.first.data());
			return cmp < 0 || (!cmp && strcmp(a.second.data(), b.second.data()) < 0);
		});

		uptr<FontListResult> flr = std::make_unique<FontListResult>(fonts.size());
		if (!desiredPath.empty())
			flr->select = rng::find_if(fonts, [&desiredPath](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), desiredPath.data()); }) - fonts.begin();
		else if (auto it = rng::find_if(fonts, [&ld](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), ld->desired.data()); }); it != fonts.end())
			flr->select = it - fonts.begin();
		else {
			fonts.emplace(fonts.begin(), ld->desired, Cstring());
			flr->select = 0;
		}
		for (size_t i = 0; i < fonts.size(); ++i) {
			flr->families[i] = std::move(fonts[i].first);
			flr->files[i] = std::move(fonts[i].second);
		}
		pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0, flr.release());
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0);
	}
}

void FileSys::listFontFamiliesInDirectorySubthread(const std::stop_token& stoken, FT_Library lib, const nstring& drc, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	Data fdata;
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec)) {
		if (stoken.stop_requested())
			break;
		if (it.is_regular_file(ec))
			if (FT_Face face = openFace(lib, it.path().c_str(), first, last, fdata)) {
				fonts.emplace_back(fmt::format("{} {}", face->family_name, face->style_name), fromNative(it.path().native()));
				FT_Done_Face(face);
			}
	}
}

#if defined(_WIN32) && !defined(__MINGW32__)
template <HKEY root>
void FileSys::listFontFamiliesInRegistrySubthread(const std::stop_token& stoken, FT_Library lib, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	Data fdata;
	wstring gfpath;
	if constexpr (root == HKEY_LOCAL_MACHINE)
		gfpath = systemFontDir();
	if (RegistryIterator rit(root, fontsKey); rit) {
		while (!stoken.stop_requested() && rit.next())
			if (rit.getType() == REG_SZ) {
				wstring fpath;
				if constexpr (root == HKEY_CURRENT_USER)
					fpath = rit.getString();
				else
					fpath = gfpath / rit.getString();
				if (FT_Face face = openFace(lib, fpath.data(), first, last, fdata)) {
					fonts.emplace_back(fmt::format("{} {}", face->family_name, face->style_name), swtos(fpath));
					FT_Done_Face(face);
				}
			}
	} else
		listFontFamiliesInDirectorySubthread(stoken, lib, root == HKEY_CURRENT_USER ? localFontDir() : gfpath, first, last, fonts);
}
#endif

FT_Face FileSys::openFace(FT_Library lib, const nchar* file, char32_t first, char32_t last, Data& fdata) {
	fdata = readBinaryFile(file);
	if (FT_Face face; !FT_New_Memory_Face(lib, fdata.data(), fdata.size(), 0, &face)) {
		char32_t ch;
		for (ch = first; FT_Get_Char_Index(face, ch) && ch <= last; ++ch);
		if (ch > last)
			return face;
	}
	return nullptr;
}

string FileSys::sanitizeFontPath(const nstring& path) const {
#ifdef _WIN32
	wstring confDir = sstow(dirConfs);
	return path.starts_with(confDir) ? swtos(filename(delExtension(path))) : swtos(path);
#else
	return path.starts_with(dirConfs) ? string(filename(delExtension(path))) : path;
#endif
}

void SDLCALL FileSys::logWrite(void* userdata, int, SDL_LogPriority priority, const char* message) noexcept {
	auto fh = static_cast<SDL_RWops*>(userdata);
	const char* sprio;
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
		sprio = "VERBOSE";
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		sprio = "DEBUG";
		break;
	case SDL_LOG_PRIORITY_INFO:
		sprio = "INFO";
		break;
	case SDL_LOG_PRIORITY_WARN:
		sprio = "WARN";
		break;
	case SDL_LOG_PRIORITY_ERROR:
		sprio = "ERROR";
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		sprio = "CRITICAL";
		break;
	default:
		sprio = "";
	}
	tm tim = currentDateTime();
	string line = fmt::format("{:02}:{:02}:{:02} {}: {}" LINEND, tim.tm_hour, tim.tm_min, tim.tm_sec, sprio, message);
#ifndef WITH_SDL3
	std::lock_guard lockg(logLock);
#endif
	SDL_RWwrite(fh, line.data(), sizeof(char), line.length());
}
