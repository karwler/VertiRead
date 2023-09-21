#include "fileSys.h"
#include "utils/compare.h"
#include <archive.h>
#include <archive_entry.h>
#include <regex>
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fontconfig/fontconfig.h>

namespace {

class FontconfigLocal {
private:
	void* lib;
	FcConfig* config;

	FcPattern* (*fcPatternCreate)();
	void (*fcPatternDestroy)(FcPattern* p);
	FcResult (*fcPatternGetString)(FcPattern* p, const char* object, int n, FcChar8**);
	void (*fcDefaultSubstitute)(FcPattern* pattern);
	FcPattern* (*fcNameParse)(const FcChar8* name);
	void (*fcFontSetDestroy)(FcFontSet* p);
	void (*fcObjectSetDestroy)(FcObjectSet* os);
	FcObjectSet* (*fcObjectSetBuild)(const char* first, ...);
	void (*fcConfigDestroy)(FcConfig* config);
	FcBool (*fcConfigSubstitute)(FcConfig* config, FcPattern* p, FcMatchKind kind);
	FcPattern* (*fcFontMatch)(FcConfig* config, FcPattern* p, FcResult* result);
	FcFontSet* (*fcFontList)(FcConfig* config, FcPattern* p, FcObjectSet* os);

public:
	FontconfigLocal();
	~FontconfigLocal();

	string search(const char* font);
	vector<string> list();
};

FontconfigLocal::FontconfigLocal() :
	lib(dlopen("libfontconfig.so", RTLD_NOW))
{
	if (!lib)
		throw std::runtime_error(coalesce(static_cast<const char*>(dlerror()), "Failed to open fontconfig"));

	FcConfig* (*fcInitLoadConfigAndFonts)() = reinterpret_cast<decltype(fcInitLoadConfigAndFonts)>(dlsym(lib, "FcInitLoadConfigAndFonts"));
	fcPatternCreate = reinterpret_cast<decltype(fcPatternCreate)>(dlsym(lib, "FcPatternCreate"));
	fcPatternDestroy = reinterpret_cast<decltype(fcPatternDestroy)>(dlsym(lib, "FcPatternDestroy"));
	fcPatternGetString = reinterpret_cast<decltype(fcPatternGetString)>(dlsym(lib, "FcPatternGetString"));
	fcDefaultSubstitute = reinterpret_cast<decltype(fcDefaultSubstitute)>(dlsym(lib, "FcDefaultSubstitute"));
	fcNameParse = reinterpret_cast<decltype(fcNameParse)>(dlsym(lib, "FcNameParse"));
	fcFontSetDestroy = reinterpret_cast<decltype(fcFontSetDestroy)>(dlsym(lib, "FcFontSetDestroy"));
	fcObjectSetDestroy = reinterpret_cast<decltype(fcObjectSetDestroy)>(dlsym(lib, "FcObjectSetDestroy"));
	fcObjectSetBuild = reinterpret_cast<decltype(fcObjectSetBuild)>(dlsym(lib, "FcObjectSetBuild"));
	fcConfigDestroy = reinterpret_cast<decltype(fcConfigDestroy)>(dlsym(lib, "FcConfigDestroy"));
	fcConfigSubstitute = reinterpret_cast<decltype(fcConfigSubstitute)>(dlsym(lib, "FcConfigSubstitute"));
	fcFontMatch = reinterpret_cast<decltype(fcFontMatch)>(dlsym(lib, "FcFontMatch"));
	fcFontList = reinterpret_cast<decltype(fcFontList)>(dlsym(lib, "FcFontList"));
	try {
		if (!(fcInitLoadConfigAndFonts && fcPatternCreate && fcPatternDestroy && fcPatternGetString && fcDefaultSubstitute && fcNameParse && fcFontSetDestroy && fcObjectSetDestroy && fcObjectSetBuild && fcConfigDestroy && fcConfigSubstitute && fcFontMatch && fcFontList))
			throw std::runtime_error("Failed to find fontconfig functions");
		if (config = fcInitLoadConfigAndFonts(); !config)
			throw std::runtime_error("Failed to init fontconfig");
	} catch (const std::runtime_error&) {
		dlclose(lib);
		throw;
	}
}

FontconfigLocal::~FontconfigLocal() {
	fcConfigDestroy(config);
	dlclose(lib);
}

string FontconfigLocal::search(const char* font) {
	string found;
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(font))) {
		if (fcConfigSubstitute(config, pattern, FcMatchPattern)) {
			fcDefaultSubstitute(pattern);
			if (FcResult res; FcPattern* fmpat = fcFontMatch(config, pattern, &res)) {
				if (FcChar8* file; fcPatternGetString(fmpat, FC_FILE, 0, &file) == FcResultMatch)
					found = reinterpret_cast<const char*>(file);
				fcPatternDestroy(fmpat);
			}
		}
		fcPatternDestroy(pattern);
	}
	return found;
}

vector<string> FontconfigLocal::list() {
	vector<string> files;
	if (FcPattern* pattern = fcPatternCreate()) {
		if (FcObjectSet* objects = fcObjectSetBuild(FC_FAMILY, nullptr)) {
			if (FcFontSet* fonts = fcFontList(config, pattern, objects)) {
				for (int i = 0; i < fonts->nfont; ++i)
					if (FcChar8* name; fcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &name) == FcResultMatch)
						files.emplace_back(reinterpret_cast<const char*>(name));
				fcFontSetDestroy(fonts);
			}
			fcObjectSetDestroy(objects);
		}
		fcPatternDestroy(pattern);
	}
	return files;
}

}
#endif

// INI LINE

IniLine::Type IniLine::setLine(string_view str) {
	size_t i0 = str.find_first_of('=');
	size_t i1 = str.find_first_of('[');
	size_t i2 = str.find_first_of(']', i1);
	if (i0 != string::npos) {
		val = str.substr(i0 + 1);
		if (i2 < i0) {
			prp = trim(str.substr(0, i1));
			key = trim(str.substr(i1 + 1, i2 - i1 - 1));
			return type = Type::prpKeyVal;
		}
		prp = trim(str.substr(0, i0));
		key = string_view();
		return type = Type::prpVal;
	}
	if (i2 != string::npos) {
		prp = trim(str.substr(i1 + 1, i2 - i1 - 1));
		key = string_view();
		val = string_view();
		return type = Type::title;
	}
	prp = string_view();
	val = string_view();
	key = string_view();
	return type = Type::empty;
}

// FILE SYS

FileSys::FileSys(const uset<string>& cmdFlags) {
	// set up file/directory path constants
	if (char* path = SDL_GetBasePath()) {
#ifdef _WIN32
		dirBase = toPath(path);
#else
		dirBase = parentPath(path);
#endif
		SDL_free(path);
	}
	if (dirBase.empty())
		logError("Failed to get base directory");

#ifdef _WIN32
	dirSets = fs::path(_wgetenv(L"AppData")) / L"VertiRead";
	dirConfs = dirBase;
#else
	dirSets = fs::path(getenv("HOME")) / ".local/share/vertiread";
	dirConfs = dirBase / "share/vertiread";
#endif

	std::error_code ec;
	try {
		std::regex rgx(R"r(log_[\d-]+\.txt)r", std::regex::icase | std::regex::optimize);
		for (const fs::directory_entry& it : fs::directory_iterator(dirSets, fs::directory_options::skip_permission_denied))
			if (string name = fromPath(it.path().filename()); std::regex_match(name, rgx) && it.is_regular_file(ec))
				if (fs::remove(it.path(), ec); ec)
					logError("Failed to remove old log file '", name, "': ", ec.message());
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}

	if (!cmdFlags.contains(Settings::flagLog)) {
		fs::path logPath = dirSets / std::format("log_{}.txt", tmToDateStr(currentDateTime()));
		if (logFile.open(logPath, std::ios::binary); logFile.good())
			SDL_LogSetOutputFunction(logWrite, &logFile);
		else
			logError("Failed to create log file: ", logPath);
	}

	// check if all (more or less) necessary files and directories exist
	if (fs::create_directories(dirSets, ec))
		logError("Failed to create settings directory: ", ec.message());
	if (!fs::is_directory(dirIcons(), ec) || ec)
		logError("Failed to find icons directory: ", ec.message());
	if (!fs::is_regular_file(dirConfs / fileThemes, ec) || ec)
		logError("Failed to find themes file: ", ec.message());

#ifndef _WIN32
	try {
		fontconfig = new FontconfigLocal;
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
#endif
}

FileSys::~FileSys() {
#ifndef _WIN32
	delete static_cast<FontconfigLocal*>(fontconfig);
#endif
	SDL_LogSetOutputFunction(nullptr, nullptr);
}

vector<string> FileSys::getAvailableThemes() const {
	vector<string> themes;
	IniLine il;
	array<fs::path, 2> locations = { dirSets, dirConfs };
	for (size_t i = 0; i < locations.size() && themes.empty(); ++i) {
		string text = readTextFile(locations[i] / fileThemes);
		for (string_view tx = text; tx.length();)
			if (il.setLine(readNextLine(tx)) == IniLine::Type::title)
				themes.emplace_back(il.prp);
	}
	return !themes.empty() ? themes : vector<string>{ "default" };
}

array<vec4, Settings::defaultColors.size()> FileSys::loadColors(string_view theme) const {
	array<vec4, Settings::defaultColors.size()> colors = Settings::defaultColors;
	string text = readTextFile(dirSets / fileThemes);
	if (text.empty())
		text = readTextFile(dirConfs / fileThemes);

	IniLine il;	// find title equal to theme and read colors until the end of the file or another title
	string_view tx = text;
	while (!tx.empty())
		if (il.setLine(readNextLine(tx)) == IniLine::Type::title && il.prp == theme)
			break;

	while (!tx.empty()) {
		if (il.setLine(readNextLine(tx)) == IniLine::Type::title)
			break;
		if (il.type == IniLine::Type::prpVal)
			if (size_t cid = strToEnum<size_t>(Settings::colorNames, il.prp); cid < colors.size())
				colors[cid] = toVec<vec4>(il.val);
	}
	return colors;
}

tuple<bool, string, string> FileSys::getLastPage(string_view book) const {
	string text = readTextFile(dirSets / fileBooks);
	for (string_view tx = text; !tx.empty();)
		if (vector<string> words = strUnenclose(readNextLine(tx)); words.size() >= 2 && words[0] == book)
			return tuple(true, std::move(words[1]), words.size() >= 3 ? std::move(words[2]) : string());
	return tuple(false, string(), string());
}

void FileSys::saveLastPage(string_view book, string_view drc, string_view fname) const {
	fs::path file = dirSets / fileBooks;
	string text = readTextFile(file);
	string_view line;
	for (string_view tx = text; !tx.empty();) {
		line = readNextLine(tx);
		if (vector<string> words = strUnenclose(line); words.size() >= 2 && words[0] == book)
			break;
	}
	if (string ilin = std::format("{} {} {}", strEnclose(book), strEnclose(drc), strEnclose(fname)); line.empty())
		text += ilin + LINEND;
	else
		text.replace(line.data() - text.c_str(), line.length(), ilin);

	if (std::ofstream ofs(file, std::ios::binary); ofs.good())
		ofs.write(text.c_str(), text.length());
	else
		logError("Failed to write books file: ", file);
}

Settings* FileSys::loadSettings() const {
	Settings* sets = new Settings(dirSets, getAvailableThemes());
	IniLine il;
	string text = readTextFile(dirSets / fileSettings);
	for (string_view tx = text; tx.length();) {
		switch (il.setLine(readNextLine(tx))) {
		using enum IniLine::Type;
		case prpVal:
			if (strciequal(il.prp, iniKeywordMaximized))
				sets->maximized = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordScreen))
				sets->screen = strToEnum(Settings::screenModeNames, il.val, Settings::defaultScreenMode);
			else if (strciequal(il.prp, iniKeywordResolution))
				sets->resolution = toVec<ivec2>(il.val);
			else if (strciequal(il.prp, iniKeywordRenderer))
				sets->renderer = strToEnum(Settings::rendererNames, il.val, Settings::defaultRenderer);
			else if (strciequal(il.prp, iniKeywordDevice))
				sets->device = toVec<u32vec2>(il.val, 0, 0x10);
			else if (strciequal(il.prp, iniKeywordCompression))
				sets->compression = strToEnum(Settings::compressionNames, il.val, Settings::defaultCompression);
			else if (strciequal(il.prp, iniKeywordVSync))
				sets->vsync = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordGpuSelecting))
				sets->gpuSelecting = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordDirection))
				sets->direction = strToEnum(Direction::names, il.val, Settings::defaultDirection);
			else if (strciequal(il.prp, iniKeywordZoom))
				sets->zoom = toNum<float>(il.val);
			else if (strciequal(il.prp, iniKeywordSpacing))
				sets->spacing = toNum<ushort>(il.val);
			else if (strciequal(il.prp, iniKeywordPictureLimit))
				sets->picLim.set(il.val);
			else if (strciequal(il.prp, iniKeywordMaxPictureRes))
				sets->maxPicRes = std::max(toNum<uint>(il.val), Settings::minPicRes);
			else if (strciequal(il.prp, iniKeywordFont))
				sets->font = FileSys::isFont(findFont(toPath(il.val))) ? il.val : Settings::defaultFont;
			else if (strciequal(il.prp, iniKeywordHinting))
				sets->hinting = strToEnum<Settings::Hinting>(Settings::hintingNames, il.val, Settings::defaultHinting);
			else if (strciequal(il.prp, iniKeywordTheme))
				sets->setTheme(il.val, getAvailableThemes());
			else if (strciequal(il.prp, iniKeywordPreview))
				sets->preview = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordShowHidden))
				sets->showHidden = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordTooltips))
				sets->tooltips = toBool(il.val);
			else if (strciequal(il.prp, iniKeywordLibrary))
				sets->setDirLib(il.val, dirSets);
			else if (strciequal(il.prp, iniKeywordScrollSpeed))
				sets->scrollSpeed = toVec<vec2>(il.val);
			else if (strciequal(il.prp, iniKeywordDeadzone))
				sets->setDeadzone(toNum<uint>(il.val));
			break;
		case prpKeyVal:
			if (strciequal(il.prp, iniKeywordDisplay))
				sets->displays[toNum<int>(il.key)] = toVec<ivec4>(il.val);
		}
	}
	sets->unionDisplays();
	return sets;
}

void FileSys::saveSettings(const Settings* sets) const {
	fs::path file = dirSets / fileSettings;
	std::ofstream ofs(file, std::ios::binary);
	if (!ofs.good()) {
		logError("Failed to write settings file: ", file);
		return;
	}

	IniLine::writeVal(ofs, iniKeywordMaximized, toStr(sets->maximized));
	IniLine::writeVal(ofs, iniKeywordScreen, Settings::screenModeNames[eint(sets->screen)]);
	for (const auto& [id, rect] : sets->displays)
		IniLine::writeKeyVal(ofs, iniKeywordDisplay, id, rect.x, ' ', rect.y, ' ', rect.w, ' ', rect.h);
	IniLine::writeVal(ofs, iniKeywordResolution, sets->resolution.x, ' ', sets->resolution.y);
	IniLine::writeVal(ofs, iniKeywordRenderer, Settings::rendererNames[eint(sets->renderer)]);
	IniLine::writeVal(ofs, iniKeywordDevice, toStr<0x10>(sets->device));
	IniLine::writeVal(ofs, iniKeywordCompression, Settings::compressionNames[eint(sets->compression)]);
	IniLine::writeVal(ofs, iniKeywordVSync, toStr(sets->vsync));
	IniLine::writeVal(ofs, iniKeywordGpuSelecting, toStr(sets->gpuSelecting));
	IniLine::writeVal(ofs, iniKeywordZoom, sets->zoom);
	IniLine::writeVal(ofs, iniKeywordPictureLimit, PicLim::names[eint(sets->picLim.type)], ' ', sets->picLim.getCount(), ' ', PicLim::memoryString(sets->picLim.getSize()));
	IniLine::writeVal(ofs, iniKeywordMaxPictureRes, sets->maxPicRes);
	IniLine::writeVal(ofs, iniKeywordSpacing, sets->spacing);
	IniLine::writeVal(ofs, iniKeywordDirection, Direction::names[uint8(sets->direction)]);
	IniLine::writeVal(ofs, iniKeywordFont, sets->font);
	IniLine::writeVal(ofs, iniKeywordHinting, Settings::hintingNames[eint(sets->hinting)]);
	IniLine::writeVal(ofs, iniKeywordTheme, sets->getTheme());
	IniLine::writeVal(ofs, iniKeywordPreview, toStr(sets->preview));
	IniLine::writeVal(ofs, iniKeywordShowHidden, toStr(sets->showHidden));
	IniLine::writeVal(ofs, iniKeywordTooltips, toStr(sets->tooltips));
	IniLine::writeVal(ofs, iniKeywordLibrary, sets->getDirLib());
	IniLine::writeVal(ofs, iniKeywordScrollSpeed, sets->scrollSpeed.x, ' ', sets->scrollSpeed.y);
	IniLine::writeVal(ofs, iniKeywordDeadzone, sets->getDeadzone());
}

array<Binding, Binding::names.size()> FileSys::loadBindings() const {
	array<Binding, Binding::names.size()> bindings;
	for (size_t i = 0; i < bindings.size(); ++i)
		bindings[i].reset(Binding::Type(i));

	IniLine il;
	string text = readTextFile(dirSets / fileBindings);
	for (string_view tx = text; tx.length();) {
		if (il.setLine(readNextLine(tx)) != IniLine::Type::prpVal || il.val.length() < 3)
			continue;
		size_t bid = strToEnum<size_t>(Binding::names, il.prp);
		if (bid >= bindings.size())
			continue;

		switch (toupper(il.val[0])) {
		case keyKey[0]:			// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(string(il.val).c_str() + 2));
			break;
		case keyButton[0]:		// joystick button
			bindings[bid].setJbutton(toNum<uint8>(il.val.substr(2)));
			break;
		case keyHat[0]:			// joystick hat
			if (size_t id = std::find_if(il.val.begin() + 2, il.val.end(), [](char c) -> bool { return !isdigit(c); }) - il.val.begin(); id < il.val.size())
				bindings[bid].setJhat(toNum<uint8>(il.val.substr(2, id - 2)), strToVal(Binding::hatNames, il.val.substr(id + 1)));
			break;
		case keyAxisPos[0]:		// joystick axis
			bindings[bid].setJaxis(toNum<uint8>(il.val.substr(3)), il.val[2] != keyAxisNeg[2]);
			break;
		case keyGButton[0]:		// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Binding::gbuttonNames, il.val.substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case keyGAxisPos[0]:	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Binding::gaxisNames, il.val.substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (il.val[2] != keyGAxisNeg[2]));
			break;
		default:
			throw std::runtime_error(std::format("Invalid binding identifier: {}", il.val[0]));
		}
	}
	return bindings;
}

void FileSys::saveBindings(const array<Binding, Binding::names.size()>& bindings) const {
	fs::path file = dirSets / fileBindings;
	std::ofstream ofs(file, std::ios::binary);
	if (!ofs.good()) {
		logError("Failed to write bindings file: ", file);
		return;
	}

	for (size_t i = 0; i < bindings.size(); ++i) {
		if (bindings[i].keyAssigned())
			IniLine::writeVal(ofs, Binding::names[i], keyKey, SDL_GetScancodeName(bindings[i].getKey()));

		if (bindings[i].jbuttonAssigned())
			IniLine::writeVal(ofs, Binding::names[i], keyButton, uint(bindings[i].getJctID()));
		else if (bindings[i].jhatAssigned())
			IniLine::writeVal(ofs, Binding::names[i], keyHat, uint(bindings[i].getJctID()), keySep, Binding::hatNames.at(bindings[i].getJhatVal()));
		else if (bindings[i].jaxisAssigned())
			IniLine::writeVal(ofs, Binding::names[i], (bindings[i].jposAxisAssigned() ? keyAxisPos : keyAxisNeg), uint(bindings[i].getJctID()));

		if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofs, Binding::names[i], keyGButton, Binding::gbuttonNames[eint(bindings[i].getGbutton())]);
		else if (bindings[i].gbuttonAssigned())
			IniLine::writeVal(ofs, Binding::names[i], (bindings[i].gposAxisAssigned() ? keyGAxisPos : keyGAxisNeg), Binding::gaxisNames[eint(bindings[i].getGaxis())]);
	}
}

string_view FileSys::readNextLine(string_view& text) {
	string_view::iterator pos = rng::find_if(text, [](char ch) -> bool { return ch != '\n' && ch != '\r'; });
	string_view::iterator end = std::find_if(pos, text.end(), [](char ch) -> bool { return ch == '\n' || ch == '\r'; });
	text = string_view(end, text.end());
	return string_view(pos, end);
}

string FileSys::readTextFile(const fs::path& file) {
	std::ifstream ifs(file, std::ios::binary);
	if (!ifs.good())
		return string();
	char bom[4];
	std::streampos len = ifs.read(bom, std::extent_v<decltype(bom)>).gcount();
	if (len <= 0)
		return string();

	if (len >= 4) {
		if (!memcmp(bom, "\xFF\xFE\x00\x00", 4 * sizeof(char)))
			return processTextFile<char32_t, std::endian::little>(ifs, 4);
		if (!memcmp(bom, "\x00\x00\xFE\xFF", 4 * sizeof(char)))
			return processTextFile<char32_t, std::endian::big>(ifs, 4);
	}
	if (len >= 3 && !memcmp(bom, "\xEF\xBB\xBF", 3 * sizeof(char)))
		return processTextFile<char, std::endian::native>(ifs, 3);
	if (len >= 2) {
		if (!memcmp(bom, "\xFF\xFE", 2 * sizeof(char)))
			return processTextFile<char16_t, std::endian::little>(ifs, 2);
		if (!memcmp(bom, "\xFE\xFF", 2 * sizeof(char)))
			return processTextFile<char16_t, std::endian::big>(ifs, 2);
	}
	return processTextFile<char, std::endian::native>(ifs, 0);
}

template <Integer C, std::endian bo>
string FileSys::processTextFile(std::ifstream& ifs, std::streampos offs) {
	string text;
	if (std::streampos len = ifs.seekg(0, std::ios::end).tellg(); len > offs) {
		len -= offs;
		len -= len % sizeof(C);
		ifs.seekg(offs);

		if constexpr (sizeof(C) == sizeof(char))
			readFileContent(ifs, text, len);
		else if constexpr (sizeof(C) == sizeof(char16_t)) {
			while (len) {
				if (C ch = readChar<C, bo>(ifs, len); ch < 0xD800)
					writeChar8(text, ch);
				else if (ch < 0xDC00 && len)
					if (C xt = readChar<C, bo>(ifs, len); xt >= 0xDC00)
						writeChar8(text, (char32_t(ch & 0x03FF) << 10) | (xt & 0x03FF));
			}
		} else if constexpr (sizeof(C) == sizeof(char32_t))
			while (len)
				writeChar8(text, readChar<C, bo>(ifs, len));
	}
	return text;
}

template <Integer C, std::endian bo>
C FileSys::readChar(std::ifstream& ifs, std::streampos& len) {
	C ch;
	ifs.read(reinterpret_cast<char*>(&ch), sizeof(ch));
	len -= sizeof(ch);
	if constexpr (bo == std::endian::native)
		return ch;
	else if constexpr (sizeof(ch) == sizeof(char16_t))
		return SDL_Swap16(ch);
	else
		return SDL_Swap32(ch);
}

void FileSys::writeChar8(string& str, char32_t ch) {
	if (ch < 0x80)
		str += ch;
	else if (ch < 0x800)
		str += { char(0xC0 | (ch >> 6)), char(0x80 | (ch & 0x3F)) };
	else if (ch < 0x10000)
		str += { char(0xE0 | (ch >> 12)), char(0x80 | ((ch >> 6) & 0x3F)), char(0x80 | (ch & 0x3F)) };
	else
		str += { char(0xF0 | (ch >> 18)), char(0x80 | ((ch >> 12) & 0x3F)), char(0x80 | ((ch >> 6) & 0x3F)), char(0x80 | (ch & 0x3F)) };
}

vector<cbyte> FileSys::readBinFile(const fs::path& file) {
	vector<cbyte> data;
	if (std::ifstream ifs(file, std::ios::binary | std::ios::ate); ifs.good())
		if (std::streampos len = ifs.tellg(); len > 0) {
			ifs.seekg(0);
			readFileContent(ifs, data, len);
		}
	return data;
}

template <Class T>
void FileSys::readFileContent(std::ifstream& ifs, T& data, std::streampos len) {
	data.resize(len / sizeof(*data.data()));
	if (ifs.read(reinterpret_cast<char*>(data.data()), data.size() * sizeof(*data.data())).gcount() < len)
		data.resize(ifs.gcount() / sizeof(*data.data()));
}

vector<string> FileSys::listDir(const char* drc, bool files, bool dirs, bool showHidden) {
#ifdef _WIN32
	if (strempty(drc))	// if in "root" directory, get drive letters and present them as directories
		return dirs ? listDrives() : vector<string>();
#endif
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(drc) / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dirs : files))
				entries.push_back(swtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc)) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && (showHidden || entry->d_name[0] != '.') && ((entry->d_type != DT_LNK ? entry->d_type == DT_DIR : fs::is_directory(toPath(drc) / entry->d_name)) ? dirs : files))
				entries.emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	rng::sort(entries, StrNatCmp());
	return entries;
}

pair<vector<string>, vector<string>> FileSys::listDirSep(const char* drc, bool showHidden) {
#ifdef _WIN32
	if (strempty(drc))	// if in "root" directory, get drive letters and present them as directories
		return pair(vector<string>(), listDrives());
#endif
	vector<string> files, dirs;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(drc) / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L"..") && (showHidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)))
				(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dirs : files).push_back(swtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc)) {
		while (dirent* entry = readdir(directory))
			if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..") && (showHidden || entry->d_name[0] != '.'))
				((entry->d_type != DT_LNK ? entry->d_type == DT_DIR : fs::is_directory(toPath(drc) / entry->d_name)) ? dirs : files).emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	rng::sort(files, StrNatCmp());
	rng::sort(dirs, StrNatCmp());
	return pair(std::move(files), std::move(dirs));
}

bool FileSys::isPicture(SDL_RWops* ifh, string_view ext) {
	static constexpr int (SDLCALL* const magics[])(SDL_RWops*) = {
		IMG_isJPG,
		IMG_isPNG,
		IMG_isBMP,
		IMG_isGIF,
		IMG_isTIF,
		IMG_isWEBP,
		IMG_isCUR,
		IMG_isICO,
		IMG_isLBM,
		IMG_isPCX,
		IMG_isPNM,
		IMG_isSVG,
		IMG_isXCF,
		IMG_isXPM,
		IMG_isXV,
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		IMG_isAVIF,
		IMG_isJXL,
		IMG_isQOI
#endif
	};
	if (ifh) {
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
			if (test(ifh)) {
				SDL_RWclose(ifh);
				return true;
			}

		if (strciequal(ext, "TGA"))
			if (SDL_Surface* img = IMG_LoadTGA_RW(ifh)) {
				SDL_FreeSurface(img);
				SDL_RWclose(ifh);
				return true;
			}
		SDL_RWclose(ifh);
	}
	return false;
}

bool FileSys::isFont(const fs::path& file) {
	if (std::ifstream ifs(file, std::ios::binary); ifs.good()) {
		char sig[5] = { -1, -1, -1, -1, -1 };
		ifs.read(sig, sizeof(sig));
		return !memcmp(sig, "\0\1\0\0\0", 5) || !memcmp(sig, "OTTO", 4) || !memcmp(sig, "\1fcp", 4);
	}
	return false;
}

bool FileSys::isArchive(const char* file) {
	if (archive* arch = openArchive(file)) {
		archive_read_free(arch);
		return true;
	}
	return false;
}

bool FileSys::isPictureArchive(const char* file) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (isPicture(arch, entry)) {
				archive_read_free(arch);
				return true;
			}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isArchivePicture(const char* file, string_view pname) {
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (isPicture(arch, entry)) {
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

bool FileSys::isPicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 && isPicture(SDL_RWFromConstMem(buffer.get(), bsiz), fileExtension(archive_entry_pathname_utf8(entry)));
}

archive* FileSys::openArchive(const char* file) {
	archive* arch = archive_read_new();
	if (arch) {
		archive_read_support_filter_all(arch);
		archive_read_support_format_all(arch);
#ifdef _WIN32
		if (archive_read_open_filename_w(arch, sstow(file).c_str(), archiveReadBlockSize)) {
#else
		if (archive_read_open_filename(arch, file, archiveReadBlockSize)) {
#endif
			archive_read_free(arch);
			return nullptr;
		}
	}
	return arch;
}

pair<uptr<cbyte[]>, int64> FileSys::readArchiveEntry(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0 || archive_entry_filetype(entry) != AE_IFREG)
		return pair(nullptr, 0);

	uptr<cbyte[]> buffer = std::make_unique_for_overwrite<cbyte[]>(bsiz);
	bsiz = archive_read_data(arch, buffer.get(), bsiz);
	return pair(std::move(buffer), bsiz);
}

vector<string> FileSys::listArchiveFiles(const char* file) {
	vector<string> entries;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_filetype(entry) == AE_IFREG)
				entries.emplace_back(archive_entry_pathname_utf8(entry));

		archive_read_free(arch);
		rng::sort(entries, StrNatCmp());
	}
	return entries;
}

void FileSys::makeArchiveTreeThread(std::atomic<ThreadType>& mode, BrowserResultAsync ra, uintptr_t maxRes) {
	archive* arch = openArchive(ra.curDir.c_str());
	if (!arch) {
		pushEvent(SDL_USEREVENT_ARCHIVE_FINISHED);
		return;
	}

	for (archive_entry* entry; !archive_read_next_header(arch, &entry);) {
		if (mode != ThreadType::archive) {
			archive_read_free(arch);
			return;
		}
		pushEvent(SDL_USEREVENT_ARCHIVE_PROGRESS, std::bit_cast<void*>(archive_entry_ino(entry)));

		ArchiveDir* node = &ra.arch;
		for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
			if (const char* next = strchr(path, '/')) {
				vector<ArchiveDir>::iterator dit = rng::find_if(node->dirs, [path, next](const ArchiveDir& it) -> bool { return std::equal(it.name.begin(), it.name.end(), path, next); });
				node = dit != node->dirs.end() ? std::to_address(dit) : &node->dirs.emplace_back(node, string(path, next));
				path = next + 1;
			} else {
				SDL_Surface* img = loadArchivePicture(arch, entry);
				node->files.emplace_back(path, img ? std::min(uintptr_t(img->w), maxRes) * std::min(uintptr_t(img->h), maxRes) * 4 : 0);
				SDL_FreeSurface(img);
				break;
			}
		}
	}
	archive_read_free(arch);
	ra.arch.sort();
	pushEvent(SDL_USEREVENT_ARCHIVE_FINISHED, new BrowserResultAsync(std::move(ra)));
}

SDL_Surface* FileSys::loadArchivePicture(const char* file, string_view pname) {
	SDL_Surface* img = nullptr;
	if (archive* arch = openArchive(file)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (img = loadArchivePicture(arch, entry); img)
					break;
		archive_read_free(arch);
	}
	return img;
}

SDL_Surface* FileSys::loadArchivePicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 ? IMG_Load_RW(SDL_RWFromMem(buffer.get(), bsiz), SDL_TRUE) : nullptr;
}

void FileSys::moveContentThread(std::atomic<ThreadType>& mode, fs::path src, fs::path dst) {
	uptr<string> errors = std::make_unique<string>();
	std::error_code ec;
	if (fs::create_directories(dst, ec); !ec) {
		vector<string> files = listDir(fromPath(src).c_str());
		for (uintptr_t i = 0, lim = files.size(); i < lim; ++i) {
			if (mode != ThreadType::move)
				break;

			pushEvent(SDL_USEREVENT_MOVE_PROGRESS, std::bit_cast<void*>(i), std::bit_cast<void*>(lim));
			if (fs::rename(src / toPath(files[i]), dst / toPath(files[i]), ec); ec)
				*errors += ec.message() + '\n';
		}
	} else
		*errors += ec.message() + '\n';
	if (!errors->empty())
		errors->pop_back();
	pushEvent(SDL_USEREVENT_MOVE_FINISHED, errors.release());
}

fs::path FileSys::findFont(const fs::path& font) const {
	if (isFont(font))
		return font;
#ifdef _WIN32
	return searchFontDirs(font, { dirConfs, fs::path(_wgetenv(L"LocalAppdata")) / L"Microsoft\\Windows\\Fonts", fs::path(_wgetenv(L"SystemDrive")) / L"Windows\\Fonts"});
#else
	if (string path = searchFontDirs(font, { dirConfs }); !path.empty())
		return path;

	if (fontconfig)
		if (string found = static_cast<FontconfigLocal*>(fontconfig)->search(font.c_str()); !found.empty())
			return found;
	return searchFontDirs(font, { fs::path(getenv("HOME")) / ".fonts", fs::path("/usr/share/fonts") });
#endif
}

string FileSys::searchFontDirs(const fs::path& font, std::initializer_list<fs::path> dirs) {
	for (const fs::path& drc : dirs) {
		try {
			for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied))
				if (strciequal(it.path().stem().native(), font.native()) && isFont(it.path()))
					return fromPath(it.path());
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
	}
	return string();
}

vector<string> FileSys::listFonts() const {
	vector<string> files;
#ifndef _WIN32
	if (fontconfig)
		files = static_cast<FontconfigLocal*>(fontconfig)->list();
#endif
#ifdef _WIN32
	for (const fs::path& drc : { dirConfs, fs::path(_wgetenv(L"LocalAppdata")) / L"Microsoft\\Windows\\Fonts", fs::path(_wgetenv(L"SystemDrive")) / L"Windows\\Fonts" }) {
#else
	for (const fs::path& drc : { dirConfs }) {
#endif
		try {
			for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied))
				if (isFont(it.path()))
					files.push_back(fromPath(it.path().stem()));
		} catch (const std::runtime_error& err) {
			logError(err.what());
		}
	}
	rng::sort(files, StrNatCmp());
	files.erase(rng::unique(files).begin(), files.end());
	return files;
}

#ifdef _WIN32
vector<string> FileSys::listDrives() {
	vector<string> letters;
	DWORD drives = GetLogicalDrives();
	for (char i = 0; i < drivesMax; ++i)
		if (drives & (1 << i))
			letters.push_back(string{ char('A' + i), ':', '\\' });
	return letters;
}
#endif

void SDLCALL FileSys::logWrite(void* userdata, int, SDL_LogPriority priority, const char* message) {
	std::ofstream& ofs = *static_cast<std::ofstream*>(userdata);
	ofs << tmToTimeStr(currentDateTime());
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
		ofs << " VERBOSE: ";
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		ofs << " DEBUG: ";
		break;
	case SDL_LOG_PRIORITY_INFO:
		ofs << " INFO: ";
		break;
	case SDL_LOG_PRIORITY_WARN:
		ofs << " WARN: ";
		break;
	case SDL_LOG_PRIORITY_ERROR:
		ofs << " ERROR: ";
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		ofs << " CRITICAL: ";
	}
	ofs << message << LINEND;
}
