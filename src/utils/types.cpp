#include "engine/world.h"

// TEXTURE

Texture::Texture(string FILE) :
	tex(nullptr)
{
	LoadTex(FILE);
}

vec2i Texture::Res() const {
	return res;
}

string Texture::File() const {
	return file;
}

void Texture::LoadTex(string path) {
	file = path;
	if (tex)
		SDL_DestroyTexture(tex);

	tex = IMG_LoadTexture(World::winSys()->Renderer(), file.c_str());
	if (tex)
		SDL_QueryTexture(tex, NULL, NULL, &res.x, &res.y);
	else {
		res = 0;
		cerr << "couldn't load texture " << file << endl;
	}
}

// IMAGE

Image::Image(vec2i POS, Texture* TEX, vec2i SIZ) :
	pos(POS),
	texture(TEX)
{
	size = SIZ.hasNull() ? size = texture->Res() : SIZ;
}

Image::Image(vec2i POS, string TEX, vec2i SIZ) :
	pos(POS)
{
	texture = World::library()->getTex(TEX);
	size = SIZ.hasNull() ? size = texture->Res() : SIZ;
}

SDL_Rect Image::getRect() const {
	return {pos.x, pos.y, size.x, size.y};
}

// FONT

FontSet::FontSet(string FILE) :
	file(FILE)
{}

FontSet::~FontSet() {
	for (const pair<int, TTF_Font*>& font : fonts)
		if (font.second)
			TTF_CloseFont(font.second);
}

bool FontSet::CanRun() const {
	TTF_Font* tmp = TTF_OpenFont(file.c_str(), 72);
	if (!tmp)
		return false;
	TTF_CloseFont(tmp);
	return true;
}

TTF_Font* FontSet::Get(int size) {
	if (fonts.count(size) == 0)
		AddSize(size);
	return fonts.at(size);
}

vec2i FontSet::TextSize(string text, int size) {
	vec2i siz;
	if (Get(size))
		TTF_SizeText(fonts.at(size), text.c_str(), &siz.x, &siz.y);
	return siz;
}

void FontSet::AddSize(int size) {
	if (fonts.count(size) == 0) {
		TTF_Font* font = TTF_OpenFont(file.c_str(), size);
		if (font)
			fonts.insert(make_pair(size, font));
		else
			cerr << "couldn't load font " << file << endl;
	}
}

// TEXT

Text::Text(string TXT, vec2i POS, int H, int HSCAL, EColor CLR) :
	pos(POS),
	color(CLR),
	text(TXT)
{
	height = HSCAL == 0 ? H : H - H/HSCAL;
}

vec2i Text::size() const {
	return World::library()->Fonts()->TextSize(text, height);
}

// TEXT EDIT

TextEdit::TextEdit(string TXT, int CPOS) :
	cpos(CPOS),
	text(TXT)
{}

int TextEdit::CursorPos() const {
	return cpos;
}

void TextEdit::SetCursor(int pos) {
	cpos = pos < 0 ? 0 : pos > text.length() ? text.length() : pos;
}

void TextEdit::MoveCursor(int mov, bool loop) {
	cpos += mov;
	if (loop) {
		if (cpos < 0)
			cpos = text.length() + cpos;
		else if (cpos > text.length())
			cpos = cpos - text.length();
	}
	else {
		if (cpos < 0)
			cpos = 0;
		else if (cpos > text.length())
			cpos = text.length();
	}
	World::engine->SetRedrawNeeded();
}

string TextEdit::getText() const {
	return text;
}

void TextEdit::Add(string str) {
	text.insert(cpos, str);
	cpos += str.length();
	World::engine->SetRedrawNeeded();
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
	World::engine->SetRedrawNeeded();
}

// SHORTCUT

Shortcut::Shortcut(SDL_Scancode KEY, void (Program::*CALL)()) :
	key(KEY),
	call(CALL)
{}

// PLAYLIST

Playlist::Playlist(string NAME, const vector<fs::path>& SGS, const vector<string>& BKS) :
	name(NAME),
	songs(SGS),
	books(BKS)
{}

Directory::Directory(string NAME, const vector<string>& DIRS, vector<string> FILS) :
	name(NAME),
	dirs(DIRS),
	files(FILS)
{}

// GENEREAL SETTINGS

GeneralSettings::GeneralSettings(string LANG, string LIB, string PST) :
	language(LANG)
{
	dirLib = LIB.empty() ? Filer::dirSets() + "library"+dsep : LIB;
	if (dirLib[dirLib.length()-1] != dsep[0])
		dirLib += dsep;

	dirPlist = PST.empty() ? Filer::dirSets() + "playlists"+dsep : PST;
	if (dirPlist[dirPlist.length()-1] != dsep[0])
		dirPlist += dsep;
}

string GeneralSettings::libraryParh() const {
	return fs::path(dirLib).is_absolute() ? dirLib : Filer::execDir() + dirLib;
}

string GeneralSettings::playlistParh() const {
	return fs::path(dirPlist).is_absolute() ? dirPlist : Filer::execDir() + dirPlist;
}

// VIDEO SETTINGS

VideoSettings::VideoSettings(bool VS, bool MAX, bool FSC, vec2i RES, string FNT, string RNDR) :
	vsync(VS),
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	renderer(RNDR)
{
	font = Filer::findFont(FNT) ? FNT : "Arial.ttf";

	SetDefaultColors();
}

string VideoSettings::FontPath() const {
	string dir;
	Filer::findFont(font, &dir);
	return dir+font;
}

void VideoSettings::SetDefaultColors() {
	if (colors.count(EColor::background) == 0)
		colors.insert(make_pair(EColor::background, vec4b(10, 10, 10, 255)));
	else
		colors[EColor::background] = vec4b(10, 10, 10, 255);

	if (colors.count(EColor::rectangle) == 0)
		colors.insert(make_pair(EColor::rectangle, vec4b(90, 90, 90, 255)));
	else
		colors[EColor::rectangle] = vec4b(90, 90, 90, 255);

	if (colors.count(EColor::highlighted) == 0)
		colors.insert(make_pair(EColor::highlighted, vec4b(120, 120, 120, 255)));
	else
		colors[EColor::highlighted] = vec4b(120, 120, 120, 255);

	if (colors.count(EColor::darkened) == 0)
		colors.insert(make_pair(EColor::darkened, vec4b(60, 60, 60, 255)));
	else
		colors[EColor::darkened] = vec4b(60, 60, 60, 255);

	if (colors.count(EColor::text) == 0)
		colors.insert(make_pair(EColor::text, vec4b(210, 210, 210, 255)));
	else
		colors[EColor::text] = vec4b(210, 210, 210, 255);
}

// AUDIO SETTINGS

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

// CONTROLS SETTINGS

ControlsSettings::ControlsSettings(vec2f SSP, bool fillMissingBindings, const map<string, Shortcut>& SRTCS, const map<string, SDL_Scancode>& HLDS) :
	scrollSpeed(SSP),
	shortcuts(SRTCS),
	holders(HLDS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
	vector<string> names = {"back", "ok", "page_up", "page_down", "zoom_in", "zoom_out", "zoom_reset", "center_view", "play_pause", "next_song", "prev_song", "volume_up", "volume_down", "next_dir", "prev_dir", "fullscreen"};
	for (string& it : names)
		if (shortcuts.count(it) == 0)
			shortcuts.insert(make_pair(it, GetDefaultShortcut(it)));

	names = {"up", "down", "left", "right", "fast", "slow"};
	for (string& it : names)
		if (holders.count(it) == 0)
			holders.insert(make_pair(it, GetDefaultHolder(it)));
}

Shortcut ControlsSettings::GetDefaultShortcut(string name) {
	if (name == "back")
		return Shortcut(SDL_SCANCODE_ESCAPE, &Program::Event_Back);
	else if (name == "ok")
		return Shortcut(SDL_SCANCODE_RETURN, &Program::Event_Ok);
	else if (name == "page_up")
		return Shortcut(SDL_SCANCODE_PAGEUP, &Program::Event_PageUp);
	else if (name == "page_down")
		return Shortcut(SDL_SCANCODE_PAGEDOWN, &Program::Event_PageDown);
	else if (name == "zoom_in")
		return Shortcut(SDL_SCANCODE_E, &Program::Event_ZoomIn);
	else if (name == "zoom_out")
		return Shortcut(SDL_SCANCODE_Q, &Program::Event_ZoomOut);
	else if (name == "zoom_reset")
		return Shortcut(SDL_SCANCODE_R, &Program::Event_ZoomReset);
	else if (name == "center_view")
		return Shortcut(SDL_SCANCODE_C, &Program::Event_CenterView);
	else if (name == "play_pause")
		return Shortcut(SDL_SCANCODE_F, &Program::Event_PlayPause);
	else if (name == "next_song")
		return Shortcut(SDL_SCANCODE_D, &Program::Event_NextSong);
	else if (name == "prev_song")
		return Shortcut(SDL_SCANCODE_A, &Program::Event_PrevSong);
	else if (name == "volume_up")
		return Shortcut(SDL_SCANCODE_W, &Program::Event_VolumeUp);
	else if (name == "volume_down")
		return Shortcut(SDL_SCANCODE_S, &Program::Event_VolumeDown);
	else if (name == "next_dir")
		return Shortcut(SDL_SCANCODE_P, &Program::Event_NextDir);
	else if (name == "prev_dir")
		return Shortcut(SDL_SCANCODE_O, &Program::Event_PrevDir);
	else if (name == "fullscreen")
		return Shortcut(SDL_SCANCODE_L, &Program::Event_ScreenMode);
	return Shortcut(SDL_SCANCODE_RCTRL, nullptr);
}

SDL_Scancode ControlsSettings::GetDefaultHolder(string name) {
	if (name == "up")
		return SDL_SCANCODE_UP;
	else if (name == "down")
		return SDL_SCANCODE_DOWN;
	else if (name == "left")
		return SDL_SCANCODE_LEFT;
	else if (name == "right")
		return SDL_SCANCODE_RIGHT;
	else if (name == "fast")
		return SDL_SCANCODE_LSHIFT;
	else if (name == "slow")
		return SDL_SCANCODE_LALT;
	return SDL_SCANCODE_RCTRL;
}

// EXCEPTION

Exception::Exception(string MSG, int RV) :
	message(MSG),
	retval(RV)
{}

void Exception::Display() {
	cerr << "ERROR: " << message << " (code " << retval << ")" << endl;
}
