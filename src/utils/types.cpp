#include "types.h"
#include "prog/program.h"
#include "engine/filer.h"

Image::Image(vec2i POS, vec2i SIZ, string TEXN) :
	pos(POS), size(SIZ),
	texname(TEXN)
{}

Text::Text(string TXT, vec2i POS, int SIZE, EColor CLR) :
	size(SIZE),
	pos(POS),
	color(CLR),
	text(TXT)
{}

Shortcut::Shortcut(string NAME, bool setDefaultKey, const vector<SDL_Keysym>& KEYS) :
	keys(KEYS)
{
	SetName(NAME, setDefaultKey);
}

string Shortcut::Name() const {
	return name;
}

bool Shortcut::SetName(string sname, bool setDefaultKey) {
	SDL_Keysym defaultKey;
	if (sname == "back") {
		call = &Program::Event_Back;
		defaultKey.scancode = SDL_SCANCODE_ESCAPE;
	}
	else if (sname == "up") {
		call = &Program::Event_Up;
		defaultKey.scancode = SDL_SCANCODE_UP;
	}
	else if (sname == "down") {
		call = &Program::Event_Down;
		defaultKey.scancode = SDL_SCANCODE_DOWN;
	}
	else if (sname == "left") {
		call = &Program::Event_Left;
		defaultKey.scancode = SDL_SCANCODE_LEFT;
	}
	else if (sname == "right") {
		call = &Program::Event_Right;
		defaultKey.scancode = SDL_SCANCODE_RIGHT;
	}
	else if (sname == "zoom_in") {
		call = &Program::Event_ZoomIn;
		defaultKey.scancode = SDL_SCANCODE_E;
	}
	else if (sname == "zoom_out") {
		call = &Program::Event_ZoomOut;
		defaultKey.scancode = SDL_SCANCODE_Q;
	}
	else if (sname == "play_pause") {
		call = &Program::Event_PlayPauseSong;
		defaultKey.scancode = SDL_SCANCODE_F;
	}
	else if (sname == "next_song") {
		call = &Program::Event_NextSong;
		defaultKey.scancode = SDL_SCANCODE_D;
	}
	else if (sname == "prev_song") {
		call = &Program::Event_PrevSong;
		defaultKey.scancode = SDL_SCANCODE_A;
	}
	else if (sname == "volume_up") {
		call = &Program::Event_VolumeUp;
		defaultKey.scancode = SDL_SCANCODE_W;
	}
	else if (sname == "volume_down") {
		call = &Program::Event_VolumeDown;
		defaultKey.scancode = SDL_SCANCODE_S;
	}
	else if (sname == "fullscreen") {
		call = &Program::Event_ScreenMode;
		defaultKey.scancode = SDL_SCANCODE_L;
	}
	else
		return false;
	name = sname;
	if (setDefaultKey)
		keys.push_back(defaultKey);
	return true;
}

progEFunc Shortcut::Call() const {
	return call;
}

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

GeneralSettings::GeneralSettings()
{}

VideoSettings::VideoSettings(bool VS, bool MAX, bool FSC, vec2i RES, string FONT, string RNDR) :
	vsync(VS),
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	font(FONT),
	renderer(RNDR)
{
	if (font.empty() || !fs::exists(font)) {
#ifdef _WIN32
		font = string(getenv("SystemDrive")) + "\\Windows\\Fonts\\Arial.ttf";
#else
		font = "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf";
#endif
	}
	SetDefaultColors();
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

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

ControlsSettings::ControlsSettings(bool fillMissingBindings, const vector<Shortcut>& SRTCS) :
	shortcuts(SRTCS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
	if (!shortcut("back"))
		shortcuts.push_back(Shortcut("back"));
	if (!shortcut("up"))
		shortcuts.push_back(Shortcut("up"));
	if (!shortcut("down"))
		shortcuts.push_back(Shortcut("down"));
	if (!shortcut("left"))
		shortcuts.push_back(Shortcut("left"));
	if (!shortcut("right"))
		shortcuts.push_back(Shortcut("right"));
	if (!shortcut("zoom_in"))
		shortcuts.push_back(Shortcut("zoom_in"));
	if (!shortcut("zoom_out"))
		shortcuts.push_back(Shortcut("zoom_out"));
	if (!shortcut("play_pause"))
		shortcuts.push_back(Shortcut("play_pause"));
	if (!shortcut("next_song"))
		shortcuts.push_back(Shortcut("next_song"));
	if (!shortcut("prev_song"))
		shortcuts.push_back(Shortcut("prev_song"));
	if (!shortcut("volume_up"))
		shortcuts.push_back(Shortcut("volume_up"));
	if (!shortcut("volume_down"))
		shortcuts.push_back(Shortcut("volume_down"));
	if (!shortcut("fullscreen"))
		shortcuts.push_back(Shortcut("fullscreen"));
}

Shortcut* ControlsSettings::shortcut(string name) {
	for (Shortcut& it : shortcuts)
		if (it.Name() == name)
			return &it;
	return nullptr;
}
