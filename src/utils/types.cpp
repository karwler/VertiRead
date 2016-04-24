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
	text(TXT),
	cpos(CPOS)
{}

int TextEdit::CursorPos() const {
	return cpos;
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

void TextEdit::Add(cstr c) {
	if (cpos == text.length())
		text.append(c);
	else
		text.insert(cpos, c);
	cpos++;
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

Shortcut::Shortcut(string NAME, bool setDefaultKey, bool setDefaultCall) :
	name(""),
	key(SDL_SCANCODE_ESCAPE),
	call(nullptr)
{
	SetDefaultByName(NAME, setDefaultKey, setDefaultCall);
}

Shortcut::Shortcut(string NAME, SDL_Scancode KEY, void (Program::*CALL)()) :
	key(KEY),
	name(NAME),
	call(CALL)
{}

bool Shortcut::SetDefaultByName(string sname, bool setDefaultKey, bool setDefaultCall) {
	name = sname;
	SDL_Scancode defaultKey;
	void (Program::*defaultCall)();

	if (name == "back") {
		defaultCall = &Program::Event_Back;
		defaultKey = SDL_SCANCODE_ESCAPE;
	}
	else if (name == "page_up") {
		defaultCall = &Program::Event_PageUp;
		defaultKey = SDL_SCANCODE_PAGEUP;
	}
	else if (name == "page_down") {
		defaultCall = &Program::Event_PageDown;
		defaultKey = SDL_SCANCODE_PAGEDOWN;
	}
	else if (name == "zoom_in") {
		defaultCall = &Program::Event_ZoomIn;
		defaultKey = SDL_SCANCODE_E;
	}
	else if (name == "zoom_out") {
		defaultCall = &Program::Event_ZoomOut;
		defaultKey = SDL_SCANCODE_Q;
	}
	else if (name == "zoom_reset") {
		defaultCall = &Program::Event_ZoomReset;
		defaultKey = SDL_SCANCODE_R;
	}
	else if (name == "center_view") {
		defaultCall = &Program::Event_CenterView;
		defaultKey = SDL_SCANCODE_C;
	}
	else if (name == "play_pause") {
		defaultCall = &Program::Event_PlayPause;
		defaultKey = SDL_SCANCODE_F;
	}
	else if (name == "next_song") {
		defaultCall = &Program::Event_NextSong;
		defaultKey = SDL_SCANCODE_D;
	}
	else if (name == "prev_song") {
		defaultCall = &Program::Event_PrevSong;
		defaultKey = SDL_SCANCODE_A;
	}
	else if (name == "volume_up") {
		defaultCall = &Program::Event_VolumeUp;
		defaultKey = SDL_SCANCODE_W;
	}
	else if (name == "volume_down") {
		defaultCall = &Program::Event_VolumeDown;
		defaultKey = SDL_SCANCODE_S;
	}
	else if (name == "next_dir") {
		defaultCall = &Program::Event_NextDir;
		defaultKey = SDL_SCANCODE_P;
	}
	else if (name == "prev_dir") {
		defaultCall = &Program::Event_PrevDir;
		defaultKey = SDL_SCANCODE_O;
	}
	else if (name == "fullscreen") {
		defaultCall = &Program::Event_ScreenMode;
		defaultKey = SDL_SCANCODE_L;
	}
	else
		return false;

	if (setDefaultKey)
		key = defaultKey;
	if (setDefaultCall)
		call = defaultCall;
	return true;
}

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

GeneralSettings::GeneralSettings()
{}

// VIDEO SETTINGS

VideoSettings::VideoSettings(bool VS, bool MAX, bool FSC, vec2i RES, string FNT, string RNDR) :
	vsync(VS),
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	renderer(RNDR)
{
	string tempFont = fs::path(FNT).is_absolute() ? FNT : Filer::dirFonts() + FNT;
	font = fs::is_regular_file(tempFont) ? FNT : "Arial.ttf";

	SetDefaultColors();
}

string VideoSettings::FontPath() const {
	return fs::path(font).is_absolute() ? font : Filer::dirFonts() + font;
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

ControlsSettings::ControlsSettings(vec2f SSP, bool fillMissingBindings, const vector<Shortcut>& SRTCS) :
	scrollSpeed(SSP),
	shortcuts(SRTCS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
	vector<string> names = {"back", "page_up", "page_down", "zoom_in", "zoom_out", "zoom_reset", "center_view", "play_pause", "next_song", "prev_song", "volume_up", "volume_down", "next_dir", "prev_dir", "fullscreen"};
	for (string& it : names)
		if (!shortcut(it))
			shortcuts.push_back(Shortcut(it, true, true));
}

Shortcut* ControlsSettings::shortcut(string name) {
	for (Shortcut& it : shortcuts)
		if (it.name == name)
			return &it;
	return nullptr;
}

// EXCEPTION

Exception::Exception(string MSG, int RV) :
	message(MSG),
	retval(RV)
{}

void Exception::Display() {
	cerr << "ERROR: " << message << " (code " << retval << ")" << endl;
}
