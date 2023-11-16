#include "fileSys.h"
#include "prog/fileOps.h"
#include <regex>
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef CAN_FONTCFG
#include "optional/fontconfig.h"
using namespace LibFontconfig;
#endif

namespace {

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
	RegistryIterator(HKEY root, const wchar_t* path) { RegOpenKeyExW(root, path, 0, KEY_READ, &key); }
	~RegistryIterator() { RegCloseKey(key); }

	bool next();
	operator bool() const { return key; }
	DWORD getType() const { return type; }
	wstring_view getString() const { return wstring_view(data, dlen / sizeof(wchar_t) - 1); }
};

bool RegistryIterator::next() {
	nlen = std::extent_v<decltype(name)>;
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

	string search(const char* font);
	void list(char32_t first, char32_t last, vector<fs::path>& fonts);
	void list(char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
};

Fontconfig::Fontconfig() :
	config(fcInitLoadConfigAndFonts())
{
	if (!config)
		throw std::runtime_error("Failed to init fontconfig");
}

string Fontconfig::search(const char* font) {
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

void Fontconfig::list(char32_t first, char32_t last, vector<fs::path>& fonts) {
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(std::format(":charset={:X} {:X}", uint32(first), uint32(last)).c_str()))) {
		if (FcObjectSet* objects = fcObjectSetBuild(FC_FILE, FC_CHARSET, nullptr)) {
			if (FcFontSet* flist = fcFontList(config, pattern, objects)) {
				for (int i = 0; i < flist->nfont; ++i)
					if (FcChar8* file; fcPatternGetString(flist->fonts[i], FC_FILE, 0, &file) == FcResultMatch)
						fonts.emplace_back(reinterpret_cast<const char*>(file));
				fcFontSetDestroy(flist);
			}
			fcObjectSetDestroy(objects);
		}
		fcPatternDestroy(pattern);
	}
}

void Fontconfig::list(char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(std::format(":charset={:X} {:X}", uint32(first), uint32(last)).c_str()))) {
		if (FcObjectSet* objects = fcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_CHARSET, nullptr)) {
			if (FcFontSet* flist = fcFontList(config, pattern, objects)) {
				FcChar8* family;
				FcChar8* style;
				FcChar8* file;
				for (int i = 0; i < flist->nfont; ++i)
					if (fcPatternGetString(flist->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch && fcPatternGetString(flist->fonts[i], FC_FILE, 0, &file) == FcResultMatch) {
						if (fcPatternGetString(flist->fonts[i], FC_STYLE, 0, &style) == FcResultMatch)
							fonts.emplace_back(std::format("{} {}", reinterpret_cast<char*>(family), reinterpret_cast<char*>(style)), reinterpret_cast<char*>(file));
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

}

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

#ifdef CAN_FONTCFG
	try {
		if (symFontconfig())
			fontconfig = new Fontconfig;
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
#endif
}

FileSys::~FileSys() {
#ifdef CAN_FONTCFG
	delete static_cast<Fontconfig*>(fontconfig);
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
	auto sets = new Settings(dirSets, getAvailableThemes());
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
				sets->zoom = std::clamp(toNum<int8>(il.val), int8(-Settings::zoomLimit), Settings::zoomLimit);
			else if (strciequal(il.prp, iniKeywordSpacing))
				sets->spacing = toNum<ushort>(il.val);
			else if (strciequal(il.prp, iniKeywordPictureLimit))
				sets->picLim.set(il.val);
			else if (strciequal(il.prp, iniKeywordMaxPictureRes))
				sets->maxPicRes = std::max(toNum<uint>(il.val), Settings::minPicRes);
			else if (strciequal(il.prp, iniKeywordFont))
				sets->font = FileSys::isFont(findFont(toPath(il.val))) ? il.val : Settings::defaultFont;	// will get sanitized in DrawSys if necessary
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
				sets->dirLib = il.val;
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
	IniLine::writeVal(ofs, iniKeywordZoom, int(sets->zoom));
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
	IniLine::writeVal(ofs, iniKeywordLibrary, sets->dirLib);
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

		if constexpr (sizeof(C) == sizeof(char)) {
			text.resize(len);
			if (ifs.read(text.data(), text.length()).gcount() < len)
				text.resize(ifs.gcount());
		} else if constexpr (sizeof(C) == sizeof(char16_t)) {
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

bool FileSys::isFont(const fs::path& file) {
	if (std::ifstream ifs(file, std::ios::binary); ifs.good()) {
		char sig[5] = { -1, -1, -1, -1, -1 };
		ifs.read(sig, sizeof(sig));
		return !memcmp(sig, "\0\1\0\0\0", 5) || !memcmp(sig, "OTTO", 4) || !memcmp(sig, "\1fcp", 4);
	}
	return false;
}

void FileSys::moveContentThread(std::stop_token stoken, fs::path src, fs::path dst) {
	uptr<string> errors = std::make_unique<string>();
	std::error_code ec;
	if (fs::create_directories(dst, ec); !ec) {
		vector<fs::path> entries;
		for (const fs::directory_entry& it : fs::directory_iterator(src, fs::directory_options::skip_permission_denied, ec))
			entries.push_back(it.path().filename());
		for (uintptr_t i = 0, lim = entries.size(); i < lim; ++i) {
			if (stoken.stop_requested())
				break;

			pushEvent(SDL_USEREVENT_THREAD_MOVE, ThreadEvent::progress, std::bit_cast<void*>(i), std::bit_cast<void*>(lim));
			if (fs::rename(src / entries[i], dst / entries[i], ec); ec)
				*errors += ec.message() + '\n';
		}
	} else
		*errors += ec.message() + '\n';
	if (!errors->empty())
		errors->pop_back();
	pushEvent(SDL_USEREVENT_THREAD_MOVE, ThreadEvent::finished, errors.release());
}

fs::path FileSys::findFont(const fs::path& font) const {
	if (isFont(font))
		return font;
	if (fs::path path = searchFontDirectory(font, dirConfs); !path.empty())
		return path;
#ifdef CAN_FONTCFG
	if (fontconfig)
		if (string path = static_cast<Fontconfig*>(fontconfig)->search(font.c_str()); !path.empty())
			return path;
#endif
#ifdef _WIN32
	if (fs::path path = searchFontRegistry(font); !path.empty())
		return path;
#endif
	if (fs::path path = searchFontDirectory(font, localFontDir()); !path.empty())
		return path;
	return searchFontDirectory(font, systemFontDir());
}

fs::path FileSys::searchFontDirectory(const fs::path& font, const fs::path& drc) {
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec))
		if (it.is_regular_file(ec))
			if (fs::path fname = it.path().filename(); (strciequal(fname.native(), font.native()) || strciequal(fname.stem().native(), font.native())) && isFont(it.path()))
				return it.path();
	return fs::path();
}

#ifdef _WIN32
fs::path FileSys::searchFontRegistry(const fs::path& font) {
	for (HKEY root : { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE })
		for (RegistryIterator rit(root, fontsKey); rit.next();)
			if (rit.getType() == REG_SZ) {
				string_view fname = rit.getString();
				if (wstring_view freduce = filename(fname); strciequal(freduce, font.native()) || strciequal(delExtension(freduce), font.native()))
					return root == HKEY_CURRENT_USER ? fname : systemFontDir() / fname;
			}
	return fs::path();
}
#endif

vector<fs::path> FileSys::listFontFiles(FT_Library lib, char32_t first, char32_t last) const {
	vector<fs::path> fonts;
	listFontFilesInDirectory(lib, dirConfs, first, last, fonts);
#ifdef CAN_FONTCFG
	if (fontconfig)
		static_cast<Fontconfig*>(fontconfig)->list(first, last, fonts);
	else {
		listFontFilesInDirectory(lib, localFontDir(), first, last, fonts);
		listFontFilesInDirectory(lib, systemFontDir(), first, last, fonts);
	}
#elif defined(_WIN32) && !defined(__MINGW32__)
	listFontFilesInRegistry<HKEY_CURRENT_USER>(lib, first, last, fonts);
	listFontFilesInRegistry<HKEY_LOCAL_MACHINE>(lib, first, last, fonts);
#else
	listFontFilesInDirectory(lib, localFontDir(), first, last, fonts);
	listFontFilesInDirectory(lib, systemFontDir(), first, last, fonts);
#endif
	return fonts;
}

void FileSys::listFontFilesInDirectory(FT_Library lib, const fs::path& drc, char32_t first, char32_t last, vector<fs::path>& fonts) {
	vector<byte_t> fdata;
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec))
		if (it.is_regular_file(ec))
			if (FT_Face face = openFace(lib, it.path(), first, last, fdata); face) {
				FT_Done_Face(face);
				fonts.push_back(it.path());
			}
}

#if defined(_WIN32) && !defined(__MINGW32__)
template <HKEY root>
void FileSys::listFontFilesInRegistry(FT_Library lib, char32_t first, char32_t last, vector<fs::path>& fonts) {
	vector<byte_t> fdata;
	fs::path gfpath;
	if constexpr (root == HKEY_LOCAL_MACHINE)
		gfpath = systemFontDir();
	if (RegistryIterator rit(root, fontsKey); rit) {
		while (rit.next())
			if (rit.getType() == REG_SZ) {
				fs::path fpath;
				if constexpr (root == HKEY_CURRENT_USER)
					fpath = rit.getString();
				else
					fpath = gfpath / rit.getString();
				if (FT_Face face = openFace(lib, fpath, first, last, fdata); face) {
					FT_Done_Face(face);
					fonts.push_back(std::move(fpath));
				}
			}
	} else
		listFontFilesInDirectory(lib, root == HKEY_CURRENT_USER ? localFontDir() : gfpath, first, last, fonts);
}
#endif

void FileSys::listFontFamiliesThread(std::stop_token stoken, fs::path cdir, string desired, char32_t first, char32_t last) {
	vector<pair<Cstring, Cstring>> fonts;
	FT_Library lib;
	if (FT_Error err = FT_Init_FreeType(&lib)) {
		pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0, new FontListResult(vector<Cstring>(), nullptr, 0, FT_Error_String(err)));
		return;
	}
	listFontFamiliesInDirectoryThread(stoken, lib, cdir, first, last, fonts);

	string desiredPath;
	if (!isFont(toPath(desired))) {
		vector<pair<Cstring, Cstring>>::iterator it = rng::find_if(fonts, [&desired](const pair<Cstring, Cstring>& fp) -> bool {
			string_view fname = filename(fp.second.data());
			return strciequal(fname, desired) || strciequal(delExtension(fname), desired);
		});
		if (it != fonts.end())
			desiredPath = it->second;
	}

#ifdef CAN_FONTCFG
	try {
		if (!stoken.stop_requested() && symFontconfig())
			Fontconfig().list(first, last, fonts);
	} catch (const std::runtime_error&) {
		listFontFamiliesInDirectoryThread(stoken, lib, localFontDir(), first, last, fonts);
		listFontFamiliesInDirectoryThread(stoken, lib, systemFontDir(), first, last, fonts);
	}
#elif defined(_WIN32) && !defined(__MINGW32__)
	listFontFamiliesInRegistryThread<HKEY_CURRENT_USER>(stoken, lib, first, last, fonts);
	listFontFamiliesInRegistryThread<HKEY_LOCAL_MACHINE>(stoken, lib, first, last, fonts);
#else
	listFontFamiliesInDirectoryThread(stoken, lib, localFontDir(), first, last, fonts);
	listFontFamiliesInDirectoryThread(stoken, lib, systemFontDir(), first, last, fonts);
#endif
	FT_Done_FreeType(lib);
	rng::sort(fonts, [](const pair<Cstring, Cstring>& a, const pair<Cstring, Cstring>& b) -> bool {
		int cmp = strcmp(a.first.data(), b.first.data());
		return cmp < 0 || (!cmp && strcmp(a.second.data(), b.second.data()) < 0);
	});

	size_t sel;
	if (!desiredPath.empty())
		sel = rng::find_if(fonts, [&desiredPath](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), desiredPath.c_str()); }) - fonts.begin();
	else if (vector<pair<Cstring, Cstring>>::iterator it = rng::find_if(fonts, [&desired](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), desired.c_str()); }); it != fonts.end())
		sel = it - fonts.begin();
	else {
		fonts.emplace(fonts.begin(), desired, Cstring());
		sel = 0;
	}

	vector<Cstring> families(fonts.size());
	uptr<Cstring[]> files = std::make_unique_for_overwrite<Cstring[]>(fonts.size());
	for (size_t i = 0; i < fonts.size(); ++i) {
		families[i] = std::move(fonts[i].first);
		files[i] = std::move(fonts[i].second);
	}
	pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0, new FontListResult(std::move(families), std::move(files), sel, string()));
}

void FileSys::listFontFamiliesInDirectoryThread(std::stop_token stoken, FT_Library lib, const fs::path& drc, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	vector<byte_t> fdata;
	std::error_code ec;
	for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec)) {
		if (stoken.stop_requested())
			break;
		if (it.is_regular_file(ec))
			if (FT_Face face = openFace(lib, it.path(), first, last, fdata)) {
				fonts.emplace_back(std::format("{} {}", face->family_name, face->style_name), it.path());
				FT_Done_Face(face);
			}
	}
}

#if defined(_WIN32) && !defined(__MINGW32__)
template <HKEY root>
void FileSys::listFontFamiliesInRegistryThread(std::stop_token stoken, FT_Library lib, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	vector<byte_t> fdata;
	fs::path gfpath;
	if constexpr (root == HKEY_LOCAL_MACHINE)
		gfpath = systemFontDir();
	if (RegistryIterator rit(root, fontsKey); rit) {
		while (!stoken.stop_requested() && rit.next())
			if (rit.getType() == REG_SZ) {
				fs::path fpath;
				if constexpr (root == HKEY_CURRENT_USER)
					fpath = rit.getString();
				else
					fpath = gfpath / rit.getString();
				if (FT_Face face = openFace(lib, fpath, first, last, fdata)) {
					fonts.emplace_back(std::format("{} {}", face->family_name, face->style_name), fpath);
					FT_Done_Face(face);
				}
			}
	} else
		listFontFamiliesInDirectoryThread(stoken, lib, root == HKEY_CURRENT_USER ? localFontDir() : gfpath, first, last, fonts);
}
#endif

FT_Face FileSys::openFace(FT_Library lib, const fs::path& file, char32_t first, char32_t last, vector<byte_t>& fdata) {
	fdata = FileOpsLocal::readFile(file);
	if (FT_Face face; !FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(fdata.data()), fdata.size(), 0, &face)) {
		char32_t ch;
		for (ch = first; FT_Get_Char_Index(face, ch) && ch <= last; ++ch);
		if (ch > last)
			return face;
	}
	return nullptr;
}

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
