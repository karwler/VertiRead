#include "engine/world.h"
#include <algorithm>

// TEXTURE

Texture::Texture(const string& FILE) :
	surface(nullptr)
{
	LoadSurface(FILE);
}

Texture::Texture(const string& FILE, SDL_Surface* SURF) :
	surface(SURF),
	file(FILE)
{}

string Texture::File() const {
	return file;
}

vec2i Texture::Res() const {
	return surface ? vec2i(surface->w, surface->h) : 0;
}

void Texture::LoadSurface(const string& path) {
	file = path;
	if (surface)
		SDL_FreeSurface(surface);
	
	surface = IMG_Load(file.c_str());
	if (!surface)
		cerr << "couldn't load surface " << file << endl;
}

// IMAGE

Image::Image(const vec2i& POS, Texture* TEX, const vec2i& SIZ) :
	pos(POS),
	texture(TEX)
{
	size = SIZ.hasNull() ? texture->Res() : SIZ;
}

Image::Image(const vec2i& POS, const string& TEX, const vec2i& SIZ) :
	pos(POS)
{
	texture = World::library()->getTex(TEX);
	size = SIZ.hasNull() ? texture->Res() : SIZ;
}

SDL_Rect Image::getRect() const {
	return { pos.x, pos.y, size.x, size.y };
}

// FONT

bool FontSet::Initialize(const string& FILE) {
	TTF_Font* tmp = TTF_OpenFont(FILE.c_str(), 72);
	if (!tmp) {
		cerr << "couldn't open font " << file << endl << TTF_GetError() << endl;
		file.clear();
		return false;
	}
	TTF_CloseFont(tmp);
	file = FILE;
	return true;
}

void FontSet::Clear() {
	for (const pair<int, TTF_Font*>& it : fonts)
		TTF_CloseFont(it.second);
	fonts.clear();
}

bool FontSet::CanRun() const {
	return !file.empty();
}

TTF_Font* FontSet::Get(int size) {
	return (CanRun() && fonts.count(size) == 0) ? AddSize(size) : fonts.at(size);
}

TTF_Font* FontSet::AddSize(int size) {
	TTF_Font* font = TTF_OpenFont(file.c_str(), size);
	if (font)
		fonts.insert(make_pair(size, font));
	else
		cerr << "couldn't load font " << file << endl << TTF_GetError() << endl;
	return font;
}

vec2i FontSet::TextSize(const string& text, int size) {
	vec2i siz;
	TTF_Font* font = Get(size);
	if (font)
		TTF_SizeUTF8(font, text.c_str(), &siz.x, &siz.y);
	return siz;
}

// TEXT

Text::Text(const string& TXT, const vec2i& POS, int H, EColor CLR, int HSCAL) :
	pos(POS),
	color(CLR),
	text(TXT)
{
	height = (HSCAL == 0) ? H : H - H/HSCAL;
}

void Text::SetPosToRect(const SDL_Rect& rect, ETextAlign align, int offset) {
	int len = size().x;

	switch (align) {
	case ETextAlign::left:
		pos.x = rect.x+offset;
		break;
	case ETextAlign::center:
		pos.x = rect.x + (rect.w-len)/2;
		break;
	case ETextAlign::right:
		pos.x = rect.x+rect.w-len-offset;
	}
	pos.y = rect.y;
}

vec2i Text::size() const {
	return World::library()->Fonts()->TextSize(text, height);
}

// TEXT EDIT

TextEdit::TextEdit(const string& TXT, ETextType TYPE, size_t CPOS) :
	type(TYPE),
	text(TXT)
{
	SetCursor(CPOS);
	CheckText();
}

size_t TextEdit::CursorPos() const {
	return cpos;
}

void TextEdit::SetCursor(size_t pos) {
	cpos = (pos > text.length()) ? text.length() : pos;
}

void TextEdit::MoveCursor(bool right, bool loop) {
	if (loop) {
		if (right)
			cpos = (cpos == text.length()) ? 0 : cpos+1;
		else
			cpos = (cpos == 0) ? text.length() : cpos-1;
	}
	else {
		if (right && cpos != text.length())
			cpos++;
		else if (!right && cpos != 0)
			cpos--;
	}
}

string TextEdit::Text() const {
	return text;
}

void TextEdit::Text(const string& str, bool resetCpos) {
	text = str;
	
	if (resetCpos)
		cpos = 0;
	else
		CheckCaret();
}

void TextEdit::Add(const string& str) {
	text.insert(cpos, str);
	cpos += str.length();
	CheckText();
}

void TextEdit::Delete(bool current) {
	if (current) {
		if (cpos != text.length())
			text.erase(cpos, 1);
	}
	else if (cpos != 0) {
		cpos--;
		text.erase(cpos, 1);
	}
}

void TextEdit::CheckCaret() {
	if (cpos > text.length())
		cpos = text.length();
}

void TextEdit::CheckText() {
	if (type == ETextType::text)
		return;

	bool foundDot = false;
	for (size_t i=0; i!=text.length(); i++) {
		if (type == ETextType::integer) {
			if (text[i] < '0' || text[i] > '9')
				text.erase(i, 1);
		}
		else if (type == ETextType::floating) {
			if (text[i] == '.') {
				if (foundDot)
					text.erase(i, 1);
				else
					foundDot = true;
			}
			else if (text[i] < '0' || text[i] > '9')
				text.erase(i, 1);
		}
	}
}

// SHORTCUT

Shortcut::Shortcut()
{
	ClearKey();
	ClearCtr();
}

Shortcut::Shortcut(SDL_Scancode KEY)
{
	Key(KEY);
	ClearCtr();
}

Shortcut::Shortcut(uint8 JCT, EKey jctType)
{
	ClearKey();
	switch (jctType) {
	case K_BUTTON:
		JButton(JCT);
		break;
	case K_HAT:
		JHat(JCT);
		break;
	case K_AXIS_P:
		JAxis(JCT, true);
		break;
	case K_AXIS_N:
		JAxis(JCT, false);
		break;
	default:
		ClearCtr();
	}
}

Shortcut::Shortcut(SDL_Scancode KEY, uint8 JCT, EKey jctType)
{
	Key(KEY);
	switch (jctType) {
	case K_BUTTON:
		JButton(JCT);
		break;
	case K_HAT:
		JHat(JCT);
		break;
	case K_AXIS_P:
		JAxis(JCT, true);
		break;
	case K_AXIS_N:
		JAxis(JCT, false);
		break;
	default:
		ClearCtr();
	}
}

Shortcut::~Shortcut() {}

bool Shortcut::KeyAssigned() const {
	return assigned & K_KEY;
}

SDL_Scancode Shortcut::Key() const {
	return key;
}

void Shortcut::Key(SDL_Scancode KEY) {
	key = KEY;
	assigned |= K_KEY;
}

void Shortcut::ClearKey() {
	assigned ^= assigned & K_KEY;
}

bool Shortcut::JButtonAssigned() const {
	return assigned & K_BUTTON;
}

bool Shortcut::JHatAssigned() const {
	return assigned & K_HAT;
}

bool Shortcut::JAxisAssigned() const {
	return (assigned & K_AXIS_P) || (assigned & K_AXIS_N);
}

bool Shortcut::JPosAxisAssigned() const {
	return assigned & K_AXIS_P;
}

bool Shortcut::JNegAxisAssigned() const {
	return assigned & K_AXIS_N;
}

uint8 Shortcut::JCtr() const {
	return jctr;
}

void Shortcut::JButton(uint8 BUTT) {
	jctr = BUTT;
	assigned ^= assigned & (K_HAT | K_AXIS_P | K_AXIS_N);
	assigned |= K_BUTTON;
}

void Shortcut::JHat(uint8 HAT) {
	jctr = HAT;
	assigned ^= assigned & (K_BUTTON | K_AXIS_P | K_AXIS_N);
	assigned |= K_HAT;
}

void Shortcut::JAxis(uint8 AXIS, bool positive) {
	jctr = AXIS;
	if (positive) {
		assigned ^= assigned & (K_BUTTON | K_HAT | K_AXIS_N);
		assigned |= K_AXIS_P;
	}
	else {
		assigned ^= assigned & (K_BUTTON | K_HAT | K_AXIS_P);
		assigned |= K_AXIS_N;
	}
}

void Shortcut::ClearCtr() {
	assigned &= K_KEY;
}

Shortcut::EKey operator~(Shortcut::EKey a) {
	return static_cast<Shortcut::EKey>(~static_cast<uint8>(a));
}
Shortcut::EKey operator&(Shortcut::EKey a, Shortcut::EKey b) {
	return static_cast<Shortcut::EKey>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
Shortcut::EKey operator&=(Shortcut::EKey& a, Shortcut::EKey b) {
	return a = static_cast<Shortcut::EKey>(static_cast<uint8>(a) & static_cast<uint8>(b));
}
Shortcut::EKey operator^(Shortcut::EKey a, Shortcut::EKey b) {
	return static_cast<Shortcut::EKey>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
Shortcut::EKey operator^=(Shortcut::EKey& a, Shortcut::EKey b) {
	return a = static_cast<Shortcut::EKey>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}
Shortcut::EKey operator|(Shortcut::EKey a, Shortcut::EKey b) {
	return static_cast<Shortcut::EKey>(static_cast<uint8>(a) | static_cast<uint8>(b));
}
Shortcut::EKey operator|=(Shortcut::EKey& a, Shortcut::EKey b) {
	return a = static_cast<Shortcut::EKey>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

// SHORTCUT KEY

ShortcutKey::ShortcutKey(void (Program::*CALL)()) :
	Shortcut(),
	call(CALL)
{}

ShortcutKey::ShortcutKey(SDL_Scancode KEY, void (Program::*CALL)()) :
	Shortcut(KEY),
	call(CALL)
{}

ShortcutKey::ShortcutKey(uint8 JCT, EKey jctType, void (Program::*CALL)()) :
	Shortcut(JCT, jctType),
	call(CALL)
{}

ShortcutKey::ShortcutKey(SDL_Scancode KEY, uint8 JCT, EKey jctType, void (Program::*CALL)()) :
	Shortcut(KEY, JCT, jctType),
	call(CALL)
{}

ShortcutKey::~ShortcutKey() {}

// SHORTCUT AXIS

ShortcutAxis::ShortcutAxis(void (Program::*CALL)(float)) :
	Shortcut(),
	call(CALL)
{}

ShortcutAxis::ShortcutAxis(SDL_Scancode KEY, void (Program::*CALL)(float)) :
	Shortcut(KEY),
	call(CALL)
{}

ShortcutAxis::ShortcutAxis(uint8 JCT, EKey jctType, void (Program::*CALL)(float)) :
	Shortcut(JCT, jctType),
	call(CALL)
{}

ShortcutAxis::ShortcutAxis(SDL_Scancode KEY, uint8 JCT, EKey jctType, void (Program::*CALL)(float)) :
	Shortcut(KEY, JCT, jctType),
	call(CALL)
{}

ShortcutAxis::~ShortcutAxis() {}

// PLAYLIST

Playlist::Playlist(const string& NAME, const vector<string>& SGS, const vector<string>& BKS) :
	name(NAME),
	songs(SGS),
	books(BKS)
{}

string Playlist::songPath(uint id) const {
	return isAbsolute(songs[id]) ? songs[id] : parentPath(World::scene()->Settings().PlaylistPath() + name) + songs[id];
}

Directory::Directory(const string& NAME, const vector<string>& DIRS, const vector<string>& FILS) :
	name(NAME),
	dirs(DIRS),
	files(FILS)
{}

// EXCEPTION

Exception::Exception(const string& MSG, int RV) :
	message(MSG),
	retval(RV)
{}

void Exception::Display() {
	cerr << "ERROR: " << message << " (code " << retval << ")" << endl;
}

// GENEREAL SETTINGS

GeneralSettings::GeneralSettings(const string& LANG, const string& LIB, const string& PST)
{
	Lang(LANG);
	DirLib(LIB);
	DirPlist(PST);
}

string GeneralSettings::Lang() const {
	return lang;
}

void GeneralSettings::Lang(const string& language) {
	lang = language;
	std::transform(lang.begin(), lang.end(), lang.begin(), tolower);

	if (!Filer::Exists(Filer::dirLangs+lang+".ini"))
		lang = "english";
}

string GeneralSettings::DirLib() const {
	return dirLib;
}

string GeneralSettings::LibraryPath() const {
	return isAbsolute(dirLib) ? dirLib : Filer::dirExec + dirLib;
}

void GeneralSettings::DirLib(const string& dir) {
	dirLib = dir.empty() ? Filer::dirSets + "library"+dsep : appendDsep(dir);
}

string GeneralSettings::DirPlist() const {
	return dirPlist;
}

string GeneralSettings::PlaylistPath() const {
	return isAbsolute(dirPlist) ? dirPlist : Filer::dirExec + dirPlist;
}

void GeneralSettings::DirPlist(const string& dir) {
	dirPlist = dir.empty() ? Filer::dirSets + "playlists"+dsep : appendDsep(dir);
}

// VIDEO SETTINGS

VideoSettings::VideoSettings(bool MAX, bool FSC, const vec2i& RES, const string& FNT, const string& RNDR) :
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	renderer(RNDR)
{
	SetFont(FNT);
	SetDefaultTheme();
}

string VideoSettings::Font() const {
	return font;
}

string VideoSettings::Fontpath() const {
	return fontpath;
}

void VideoSettings::SetFont(const string& newFont) {
	fontpath = Filer::FindFont(newFont);
	font = fontpath.empty() ? "" : newFont;
}

void VideoSettings::SetDefaultTheme() {
	theme.clear();
	colors = GetDefaultColors();
}

map<EColor, vec4b> VideoSettings::GetDefaultColors() {
	map<EColor, vec4b> colors = {
		make_pair(EColor::background, GetDefaultColor(EColor::background)),
		make_pair(EColor::rectangle, GetDefaultColor(EColor::rectangle)),
		make_pair(EColor::highlighted, GetDefaultColor(EColor::highlighted)),
		make_pair(EColor::darkened, GetDefaultColor(EColor::darkened)),
		make_pair(EColor::text, GetDefaultColor(EColor::text))
	};
	return colors;
}

vec4b VideoSettings::GetDefaultColor(EColor color) {
	switch (color) {
	case EColor::background:
		return DEFAULT_COLOR_BACKGROUND;
	case EColor::rectangle:
		return DEFAULT_COLOR_RECTANGLE;
	case EColor::highlighted:
		return DEFAULT_COLOR_HIGHLIGHTED;
	case EColor::darkened:
		return DEFAULT_COLOR_DARKENED;
	case EColor::text:
		return DEFAULT_COLOR_TEXT;
	}
	return 0;	// just so msvc doesn't bitch around
}

// AUDIO SETTINGS

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

// CONTROLS SETTINGS

ControlsSettings::ControlsSettings(const vec2f& SSP, bool fillMissingBindings, const map<string, Shortcut*>& CTS) :
	scrollSpeed(SSP),
	shortcuts(CTS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
	vector<string> names = { SHORTCUT_OK, SHORTCUT_BACK, SHORTCUT_ZOOM_IN, SHORTCUT_ZOOM_OUT, SHORTCUT_ZOOM_RESET, SHORTCUT_CENTER_VIEW, SHORTCUT_FAST, SHORTCUT_SLOW, SHORTCUT_PLAY_PAUSE, SHORTCUT_FULLSCREEN, SHORTCUT_NEXT_DIR, SHORTCUT_PREV_DIR, SHORTCUT_NEXT_SONG, SHORTCUT_PREV_SONG, SHORTCUT_VOLUME_UP, SHORTCUT_VOLUME_DOWN, SHORTCUT_PAGE_UP, SHORTCUT_PAGE_DOWN, SHORTCUT_UP, SHORTCUT_DOWN, SHORTCUT_RIGHT, SHORTCUT_LEFT, SHORTCUT_CURSOR_UP, SHORTCUT_CURSOR_DOWN, SHORTCUT_CURSOR_RIGHT, SHORTCUT_CURSOR_LEFT };
	for (string& it : names)
		if (shortcuts.count(it) == 0) {
			Shortcut* sc = GetDefaultShortcut(it);
			if (sc)
				shortcuts.insert(make_pair(it, sc));
		}
}

Shortcut* ControlsSettings::GetDefaultShortcut(const string& name) {
	if (name == SHORTCUT_OK)
		return new ShortcutKey(DEFAULT_KEY_OK, DEFAULT_CBINDING_OK, DEFAULT_CTYPE_0, &Program::Event_Ok);
	else if (name == SHORTCUT_BACK)
		return new ShortcutKey(DEFAULT_KEY_BACK, DEFAULT_CBINDING_BACK, DEFAULT_CTYPE_0, &Program::Event_Back);
	else if (name == SHORTCUT_ZOOM_IN)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_IN, DEFAULT_CBINDING_ZOOM_IN, DEFAULT_CTYPE_0, &Program::Event_ZoomIn);
	else if (name == SHORTCUT_ZOOM_OUT)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_OUT, DEFAULT_CBINDING_ZOOM_OUT, DEFAULT_CTYPE_0, &Program::Event_ZoomOut);
	else if (name == SHORTCUT_ZOOM_RESET)
		return new ShortcutKey(DEFAULT_KEY_ZOOM_RESET, DEFAULT_CBINDING_ZOOM_RESET, DEFAULT_CTYPE_0, &Program::Event_ZoomReset);
	else if (name == SHORTCUT_CENTER_VIEW)
		return new ShortcutKey(DEFAULT_KEY_CENTER_VIEW, DEFAULT_CBINDING_CENTER_VIEW, DEFAULT_CTYPE_0, &Program::Event_CenterView);
	else if (name == SHORTCUT_FAST)
		return new ShortcutAxis(DEFAULT_KEY_FAST, DEFAULT_CBINDING_FAST, DEFAULT_CTYPE_0);
	else if (name == SHORTCUT_SLOW)
		return new ShortcutAxis(DEFAULT_KEY_SLOW, DEFAULT_CBINDING_SLOW, DEFAULT_CTYPE_0);
	else if (name == SHORTCUT_PLAY_PAUSE)
		return new ShortcutKey(DEFAULT_KEY_PLAY_PAUSE, DEFAULT_CBINDING_PLAY_PAUSE, DEFAULT_CTYPE_0, &Program::Event_PlayPause);
	else if (name == SHORTCUT_FULLSCREEN)
		return new ShortcutKey(DEFAULT_KEY_FULLSCREEN, DEFAULT_CBINDING_FULLSCREEN, DEFAULT_CTYPE_0, &Program::Event_ScreenMode);
	else if (name == SHORTCUT_NEXT_DIR)
		return new ShortcutKey(DEFAULT_KEY_NEXT_DIR, DEFAULT_CBINDING_NEXT_DIR, DEFAULT_CTYPE_1, &Program::Event_NextDir);
	else if (name == SHORTCUT_PREV_DIR)
		return new ShortcutKey(DEFAULT_KEY_PREV_DIR, DEFAULT_CBINDING_PREV_DIR, DEFAULT_CTYPE_1, &Program::Event_PrevDir);
	else if (name == SHORTCUT_NEXT_SONG)
		return new ShortcutKey(DEFAULT_KEY_NEXT_SONG, DEFAULT_CBINDING_DPAD_RIGHT, DEFAULT_CTYPE_2, &Program::Event_NextSong);
	else if (name == SHORTCUT_PREV_SONG)
		return new ShortcutKey(DEFAULT_KEY_PREV_SONG, DEFAULT_CBINDING_DPAD_LEFT, DEFAULT_CTYPE_2, &Program::Event_PrevSong);
	else if (name == SHORTCUT_VOLUME_UP)
		return new ShortcutKey(DEFAULT_KEY_VOLUME_UP, DEFAULT_CBINDING_DPAD_UP, DEFAULT_CTYPE_2, &Program::Event_VolumeUp);
	else if (name == SHORTCUT_VOLUME_DOWN)
		return new ShortcutKey(DEFAULT_KEY_VOLUME_DOWN, DEFAULT_CBINDING_DPAD_DOWN, DEFAULT_CTYPE_2, &Program::Event_VolumeDown);
	else if (name == SHORTCUT_PAGE_UP)
		return new ShortcutKey(DEFAULT_KEY_PAGE_UP, &Program::Event_PageUp);
	else if (name == SHORTCUT_PAGE_DOWN)
		return new ShortcutKey(DEFAULT_KEY_PAGE_DOWN, &Program::Event_PageDown);
	else if (name == SHORTCUT_UP)
		return new ShortcutAxis(DEFAULT_KEY_UP, DEFAULT_CBINDING_VERTICAL, DEFAULT_CTYPE_4, &Program::Event_Up);
	else if (name == SHORTCUT_DOWN)
		return new ShortcutAxis(DEFAULT_KEY_DOWN, DEFAULT_CBINDING_VERTICAL, DEFAULT_CTYPE_3, &Program::Event_Down);
	else if (name == SHORTCUT_RIGHT)
		return new ShortcutAxis(DEFAULT_KEY_RIGHT, DEFAULT_CBINDING_HORIZONTAL, DEFAULT_CTYPE_3, &Program::Event_Right);
	else if (name == SHORTCUT_LEFT)
		return new ShortcutAxis(DEFAULT_KEY_LEFT, DEFAULT_CBINDING_HORIZONTAL, DEFAULT_CTYPE_4, &Program::Event_Left);
	else if (name == SHORTCUT_CURSOR_UP)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_VERT, DEFAULT_CTYPE_4, &Program::Event_CursorUp);
	else if (name == SHORTCUT_CURSOR_DOWN)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_VERT, DEFAULT_CTYPE_3, &Program::Event_CursorDown);
	else if (name == SHORTCUT_CURSOR_RIGHT)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_HORI, DEFAULT_CTYPE_3, &Program::Event_CursorRight);
	else if (name == SHORTCUT_CURSOR_LEFT)
		return new ShortcutAxis(DEFAULT_CBINDING_CURSOR_HORI, DEFAULT_CTYPE_4, &Program::Event_CursorLeft);
	return nullptr;
}
