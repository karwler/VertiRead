#include "fileSys.h"
#include "prog/fileOps.h"
#include <fstream>
#include <regex>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <SDL_filesystem.h>
#ifdef CAN_FONTCFG
#include "optional/fontconfig.h"
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

	string search(const char* font);
	void list(char32_t first, char32_t last, vector<fs::path>& fonts);
	void list(char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts);
};

Fontconfig::Fontconfig() {
	if (config = fcInitLoadConfigAndFonts(); !config)
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
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(std::format(":charset={:X} {:X}", uint32(first), uint32(last)).data()))) {
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
	if (FcPattern* pattern = fcNameParse(reinterpret_cast<const FcChar8*>(std::format(":charset={:X} {:X}", uint32(first), uint32(last)).data()))) {
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

	CsvText(const char* str);

	template <bool fill = true> Code readField();

	static string makeLine(const vector<string>& fields);
};

CsvText::CsvText(const char* str) :
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
			for (end = strchr(++text, '"'); end && end[1] == '"'; end = strchr(end + 2, '"')) {
				field.append(text, end);
				text = end + 1;
			}
			if (end) {
				field.append(text, end++);
				text = end + strcspn(end, ",\r\n");
			} else {
				size_t elen = strlen(text);
				field.append(text, elen);
				text += elen;
			}
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

string CsvText::makeLine(const vector<string>& fields) {
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

FileSys::ListFontFamiliesData::ListFontFamiliesData(fs::path&& dir, string&& selected, char32_t from, char32_t to) noexcept :
	cdir(std::move(dir)),
	desired(std::move(selected)),
	first(from),
	last(to)
{}

FileSys::MoveContentData::MoveContentData(fs::path&& sdir, fs::path&& ddir) noexcept :
	src(std::move(sdir)),
	dst(std::move(ddir))
{}

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
		if (logFile = SDL_RWFromFile(fromPath(dirSets / std::format("log_{}.txt", tmToDateStr(currentDateTime()))).data(), "wb"); logFile)
			SDL_LogSetOutputFunction(logWrite, logFile);
		else
			logError("Failed to create log file: ", SDL_GetError());
	}

	// check if all (more or less) necessary files and directories exist
	if (!fs::create_directories(dirSets, ec) && ec)
		logError("Failed to create settings directory: ", ec.message());
	if (!fs::is_directory(dirIcons(), ec))
		logError("Failed to find icons directory: ", ec.message());
	if (!fs::is_regular_file(dirConfs / fileThemes, ec))
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
	if (logFile) {
		SDL_LogSetOutputFunction(nullptr, nullptr);
		SDL_RWclose(logFile);
	}
}

vector<string> FileSys::getAvailableThemes() const {
	vector<string> themes;
	IniLine il;
	const fs::path* locations[2] = { &dirSets, &dirConfs };
	for (size_t i = 0; i < std::size(locations) && themes.empty(); ++i) {
		string text = readTextFile(*locations[i] / fileThemes);
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

vector<string> FileSys::getLastPage(string_view book) const {
	string text = readTextFile(dirSets / fileBooks);
	CsvText csv = text.data();
	for (CsvText::Code cc; (cc = csv.readField()) != CsvText::Code::end;)
		if (cc == CsvText::Code::field) {
			if (csv.field == book) {
				vector<string> paths;
				while (paths.size() < 3 && (cc = csv.readField()) != CsvText::Code::end) {
					paths.push_back(std::move(csv.field));
					if (cc == CsvText::Code::last)
						break;
				}
				if (!paths.empty())
					return paths;
			} else
				while (csv.readField<false>() == CsvText::Code::field);
		}
	return vector<string>();
}

void FileSys::saveLastPage(const vector<string>& paths) const {
	fs::path file = dirSets / fileBooks;
	string text = readTextFile(file);
	CsvText csv = text.data();
	CsvText::Code cc;
	while ((cc = csv.readField()) != CsvText::Code::end) {
		if (csv.field == paths[0]) {
			for (; cc == CsvText::Code::field; cc = csv.readField<false>());
			break;
		}
		for (; cc == CsvText::Code::field; cc = csv.readField<false>());
	}

	if (std::ofstream ofs(file, cc == CsvText::Code::end ? std::ios::binary | std::ios::app : std::ios::binary); ofs.good()) {
		string line = CsvText::makeLine(paths);
		if (cc == CsvText::Code::end) {
			if (!text.empty() && text.back() != '\n' && text.back() != '\r')
				ofs.write(LINEND, strlen(LINEND));
			ofs.write(line.data(), line.length());
			ofs.write(LINEND, strlen(LINEND));
		} else {
			text.replace(csv.lineStart - text.data(), csv.lineEnd - csv.lineStart, line);
			ofs.write(text.data(), text.length());
		}
	} else
		logError("Failed to write books file: ", file);
}

Settings* FileSys::loadSettings(const uset<string>* cmdFlags) const {
	auto sets = new Settings(dirSets, getAvailableThemes());
	IniLine il;
	string text = readTextFile(dirSets / fileSettings);
	for (string_view tx = text; tx.length();) {
		switch (il.setLine(readNextLine(tx))) {
		using enum IniLine::Type;
		case prpVal:
			if (strciequal(il.prp, iniKeywordMaximized))
				sets->maximized = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordScreen))
				sets->screen = strToEnum(Settings::screenModeNames, trim(il.val), Settings::defaultScreenMode);
			else if (strciequal(il.prp, iniKeywordResolution))
				sets->resolution = toVec<ivec2>(il.val);
			else if (strciequal(il.prp, iniKeywordRenderer))
				sets->renderer = Settings::getRenderer(il.val);
			else if (strciequal(il.prp, iniKeywordDevice))
				sets->device = toVec<u32vec2>(il.val, 0, 0x10);
			else if (strciequal(il.prp, iniKeywordCompression))
				sets->compression = strToEnum(Settings::compressionNames, trim(il.val), Settings::defaultCompression);
			else if (strciequal(il.prp, iniKeywordVSync))
				sets->vsync = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordGpuSelecting))
				sets->gpuSelecting = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordDirection))
				sets->direction = strToEnum(Direction::names, trim(il.val), Settings::defaultDirection);
			else if (strciequal(il.prp, iniKeywordZoom))
				sets->setZoom(il.val);
			else if (strciequal(il.prp, iniKeywordSpacing))
				sets->spacing = toNum<ushort>(il.val);
			else if (strciequal(il.prp, iniKeywordPictureLimit))
				sets->picLim.set(il.val);
			else if (strciequal(il.prp, iniKeywordMaxPictureRes))
				sets->maxPicRes = std::max(toNum<uint>(il.val), Settings::minPicRes);
			else if (strciequal(il.prp, iniKeywordFont))
				sets->font = isFont(findFont(toPath(il.val))) ? il.val : Settings::defaultFont;	// will get sanitized in DrawSys if necessary
			else if (strciequal(il.prp, iniKeywordHinting))
				sets->hinting = strToEnum<Settings::Hinting>(Settings::hintingNames, trim(il.val), Settings::defaultHinting);
			else if (strciequal(il.prp, iniKeywordTheme))
				sets->setTheme(il.val, getAvailableThemes());
			else if (strciequal(il.prp, iniKeywordPreview))
				sets->preview = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordShowHidden))
				sets->showHidden = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordTooltips))
				sets->tooltips = toBool(trim(il.val));
			else if (strciequal(il.prp, iniKeywordLibrary))
				sets->dirLib = il.val;
			else if (strciequal(il.prp, iniKeywordScrollSpeed))
				sets->scrollSpeed = toVec<vec2>(il.val);
			else if (strciequal(il.prp, iniKeywordDeadzone))
				sets->setDeadzone(toNum<uint>(il.val));
			break;
		case prpKeyVal:
			if (strciequal(il.prp, iniKeywordDisplay))
				sets->displays.emplace_back(toVec<ivec4>(il.val), toNum<int>(il.key));
		}
	}
	sets->unionDisplays();
	if (cmdFlags)
		sets->setRenderer(*cmdFlags);
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
	for (const Settings::Display& it : sets->displays)
		IniLine::writeKeyVal(ofs, iniKeywordDisplay, it.did, it.rect.x, ' ', it.rect.y, ' ', it.rect.w, ' ', it.rect.h);
	IniLine::writeVal(ofs, iniKeywordResolution, sets->resolution.x, ' ', sets->resolution.y);
	IniLine::writeVal(ofs, iniKeywordRenderer, Settings::rendererNames[eint(sets->renderer)]);
	IniLine::writeVal(ofs, iniKeywordDevice, toStr<0x10>(sets->device));
	IniLine::writeVal(ofs, iniKeywordCompression, Settings::compressionNames[eint(sets->compression)]);
	IniLine::writeVal(ofs, iniKeywordVSync, toStr(sets->vsync));
	IniLine::writeVal(ofs, iniKeywordGpuSelecting, toStr(sets->gpuSelecting));
	IniLine::writeVal(ofs, iniKeywordZoom, Settings::zoomNames[eint(sets->zoomType)], ' ', int(sets->zoom));
	IniLine::writeVal(ofs, iniKeywordPictureLimit, PicLim::names[eint(sets->picLim.type)], ' ', sets->picLim.count, ' ', PicLim::memoryString(sets->picLim.size));
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
		if (il.setLine(readNextLine(tx)) != IniLine::Type::prpVal)
			continue;
		size_t bid = strToEnum<size_t>(Binding::names, il.prp);
		if (bid >= bindings.size())
			continue;
		string_view bdsc = trim(il.val);
		if (bdsc.length() < 3)
			continue;

		switch (toupper(bdsc[0])) {
		case keyKey[0]:			// keyboard key
			bindings[bid].setKey(SDL_GetScancodeFromName(string(bdsc).data() + 2));
			break;
		case keyButton[0]:		// joystick button
			bindings[bid].setJbutton(toNum<uint8>(bdsc.substr(2)));
			break;
		case keyHat[0]:			// joystick hat
			if (size_t id = std::find_if(bdsc.begin() + 2, bdsc.end(), [](char c) -> bool { return !isdigit(c); }) - bdsc.begin(); id < bdsc.length())
				bindings[bid].setJhat(toNum<uint8>(bdsc.substr(2, id - 2)), Binding::hatNameToValue(bdsc.substr(id + 1)));
			break;
		case keyAxisPos[0]:		// joystick axis
			bindings[bid].setJaxis(toNum<uint8>(bdsc.substr(3)), bdsc[2] != keyAxisNeg[2]);
			break;
		case keyGButton[0]:		// gamepad button
			if (SDL_GameControllerButton cid = strToEnum<SDL_GameControllerButton>(Binding::gbuttonNames, bdsc.substr(2)); cid < SDL_CONTROLLER_BUTTON_MAX)
				bindings[bid].setGbutton(cid);
			break;
		case keyGAxisPos[0]:	// gamepad axis
			if (SDL_GameControllerAxis cid = strToEnum<SDL_GameControllerAxis>(Binding::gaxisNames, bdsc.substr(3)); cid < SDL_CONTROLLER_AXIS_MAX)
				bindings[bid].setGaxis(cid, (bdsc[2] != keyGAxisNeg[2]));
			break;
		default:
			throw std::runtime_error(std::format("Invalid binding identifier: {}", bdsc[0]));
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
			IniLine::writeVal(ofs, Binding::names[i], keyHat, uint(bindings[i].getJctID()), keySep, Binding::hatValueToName(bindings[i].getJhatVal()));
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
	std::streampos len = ifs.read(bom, std::size(bom)).gcount();
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
		char sig[5];
		return ifs.read(sig, sizeof(sig)).gcount() == sizeof(sig) && (!memcmp(sig, "\0\1\0\0\0", 5) || !memcmp(sig, "OTTO", 4) || !memcmp(sig, "\1fcp", 4));
	}
	return false;
}

void FileSys::moveContentThread(std::stop_token stoken, uptr<MoveContentData> md) {
	uptr<string> errors = std::make_unique<string>();
	std::error_code ec;
	if (fs::create_directories(md->dst, ec); !ec) {
		vector<fs::path> entries;
		for (const fs::directory_entry& it : fs::directory_iterator(md->src, fs::directory_options::skip_permission_denied, ec))
			entries.push_back(it.path().filename());
		for (uintptr_t i = 0, lim = entries.size(); i < lim; ++i) {
			if (stoken.stop_requested())
				break;

			pushEvent(SDL_USEREVENT_THREAD_MOVE, ThreadEvent::progress, std::bit_cast<void*>(i), std::bit_cast<void*>(lim));
			if (fs::rename(md->src / entries[i], md->dst / entries[i], ec); ec)
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
	if (fontconfig) {
#ifdef _WIN32
		string path = static_cast<Fontconfig*>(fontconfig)->search(swtos(font.native()).data());
#else
		string path = static_cast<Fontconfig*>(fontconfig)->search(font.c_str());
#endif
		if (!path.empty())
			return path;
	}
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
				wstring_view fname = rit.getString();
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

void FileSys::listFontFilesInDirectory(FT_Library lib, const fs::path& drc, char32_t first, char32_t last, vector<fs::path>& fonts) {
	Data fdata;
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
	Data fdata;
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

void FileSys::listFontFamiliesThread(std::stop_token stoken, uptr<ListFontFamiliesData> ld) {
	vector<pair<Cstring, Cstring>> fonts;
	FT_Library lib;
	if (FT_Error err = FT_Init_FreeType(&lib)) {
		pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0, new FontListResult(vector<Cstring>(), nullptr, 0, FT_Error_String(err)));
		return;
	}
	listFontFamiliesInDirectorySubthread(stoken, lib, ld->cdir, ld->first, ld->last, fonts);

	string desiredPath;
	if (!isFont(toPath(ld->desired))) {
		vector<pair<Cstring, Cstring>>::iterator it = rng::find_if(fonts, [&ld](const pair<Cstring, Cstring>& fp) -> bool {
			string_view fname = filename(fp.second.data());
			return strciequal(fname, ld->desired) || strciequal(delExtension(fname), ld->desired);
		});
		if (it != fonts.end())
			desiredPath = it->second.data();
	}

	bool skip = false;
#ifdef CAN_FONTCFG
	try {
		if (!stoken.stop_requested() && symFontconfig()) {
			Fontconfig().list(ld->first, ld->last, fonts);
			skip = true;
		}
	} catch (const std::runtime_error&) {}
#endif
	if (!skip) {
#if defined(_WIN32) && !defined(__MINGW32__)
		listFontFamiliesInRegistrySubthread<HKEY_CURRENT_USER>(stoken, lib, ld->first, ld->last, fonts);
		listFontFamiliesInRegistrySubthread<HKEY_LOCAL_MACHINE>(stoken, lib, ld->first, ld->last, fonts);
#else
		listFontFamiliesInDirectorySubthread(stoken, lib, localFontDir(), ld->first, ld->last, fonts);
		listFontFamiliesInDirectorySubthread(stoken, lib, systemFontDir(), ld->first, ld->last, fonts);
#endif
	}
	FT_Done_FreeType(lib);
	rng::sort(fonts, [](const pair<Cstring, Cstring>& a, const pair<Cstring, Cstring>& b) -> bool {
		int cmp = strcmp(a.first.data(), b.first.data());
		return cmp < 0 || (!cmp && strcmp(a.second.data(), b.second.data()) < 0);
	});

	size_t sel;
	if (!desiredPath.empty())
		sel = rng::find_if(fonts, [&desiredPath](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), desiredPath.data()); }) - fonts.begin();
	else if (vector<pair<Cstring, Cstring>>::iterator it = rng::find_if(fonts, [&ld](const pair<Cstring, Cstring>& fp) -> bool { return !strcmp(fp.second.data(), ld->desired.data()); }); it != fonts.end())
		sel = it - fonts.begin();
	else {
		fonts.emplace(fonts.begin(), ld->desired, Cstring());
		sel = 0;
	}

	vector<Cstring> families(fonts.size());
	uptr<Cstring[]> files = std::make_unique<Cstring[]>(fonts.size());
	for (size_t i = 0; i < fonts.size(); ++i) {
		families[i] = std::move(fonts[i].first);
		files[i] = std::move(fonts[i].second);
	}
	pushEvent(SDL_USEREVENT_THREAD_FONTS_FINISHED, 0, new FontListResult(std::move(families), std::move(files), sel, string()));
}

void FileSys::listFontFamiliesInDirectorySubthread(const std::stop_token& stoken, FT_Library lib, const fs::path& drc, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	Data fdata;
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
void FileSys::listFontFamiliesInRegistrySubthread(const std::stop_token& stoken, FT_Library lib, char32_t first, char32_t last, vector<pair<Cstring, Cstring>>& fonts) {
	Data fdata;
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
		listFontFamiliesInDirectorySubthread(stoken, lib, root == HKEY_CURRENT_USER ? localFontDir() : gfpath, first, last, fonts);
}
#endif

FT_Face FileSys::openFace(FT_Library lib, const fs::path& file, char32_t first, char32_t last, Data& fdata) {
	fdata = FileOpsLocal::readFile(file.c_str());
	if (FT_Face face; !FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte*>(fdata.data()), fdata.size(), 0, &face)) {
		char32_t ch;
		for (ch = first; FT_Get_Char_Index(face, ch) && ch <= last; ++ch);
		if (ch > last)
			return face;
	}
	return nullptr;
}

void SDLCALL FileSys::logWrite(void* userdata, int, SDL_LogPriority priority, const char* message) {
	auto ofs = static_cast<SDL_RWops*>(userdata);
	string dtime = tmToTimeStr(currentDateTime());
	SDL_RWwrite(ofs, dtime.data(), sizeof(*dtime.data()), dtime.length());
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
		SDL_RWwrite(ofs, " VERBOSE", sizeof(char), 8);
		break;
	case SDL_LOG_PRIORITY_DEBUG:
		SDL_RWwrite(ofs, " DEBUG", sizeof(char), 6);
		break;
	case SDL_LOG_PRIORITY_INFO:
		SDL_RWwrite(ofs, " INFO", sizeof(char), 5);
		break;
	case SDL_LOG_PRIORITY_WARN:
		SDL_RWwrite(ofs, " WARN", sizeof(char), 5);
		break;
	case SDL_LOG_PRIORITY_ERROR:
		SDL_RWwrite(ofs, " ERROR", sizeof(char), 6);
		break;
	case SDL_LOG_PRIORITY_CRITICAL:
		SDL_RWwrite(ofs, " CRITICAL", sizeof(char), 9);
	}
	SDL_RWwrite(ofs, ": ", sizeof(char), 2);
	SDL_RWwrite(ofs, message, sizeof(*message), strlen(message));
	SDL_RWwrite(ofs, LINEND, sizeof(char), strlen(LINEND));
}
