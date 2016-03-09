#include "types.h"
#include "prog/program.h"
#include "engine/filer.h"

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

bool Shortcut::SetName(string NAME, bool setDefaultKey) {
	SDL_Keysym defaultKey;
	if (NAME == "up") {
		call = &Program::Event_Up;
		defaultKey.scancode = SDL_SCANCODE_UP;
	}
	else if (NAME == "down") {
		call = &Program::Event_Down;
		defaultKey.scancode = SDL_SCANCODE_DOWN;
	}
	else if (NAME == "left") {
		call = &Program::Event_Left;
		defaultKey.scancode = SDL_SCANCODE_LEFT;
	}
	else if (NAME == "right") {
		call = &Program::Event_Right;
		defaultKey.scancode = SDL_SCANCODE_RIGHT;
	}
	else if (NAME == "zoom_in") {
		call = &Program::Event_ZoomIn;
		defaultKey.scancode = SDL_SCANCODE_E;
	}
	else if (NAME == "zoom_out") {
		call = &Program::Event_ZoomOut;
		defaultKey.scancode = SDL_SCANCODE_Q;
	}
	else if (NAME == "play_pause_song") {
		call = &Program::Event_PlayPauseSong;
		defaultKey.scancode = SDL_SCANCODE_F;
	}
	else if (NAME == "next_song") {
		call = &Program::Event_NextSong;
		defaultKey.scancode = SDL_SCANCODE_D;
	}
	else if (NAME == "prev_song") {
		call = &Program::Event_PrevSong;
		defaultKey.scancode = SDL_SCANCODE_A;
	}
	else if (NAME == "volume_up") {
		call = &Program::Event_VolumeUp;
		defaultKey.scancode = SDL_SCANCODE_W;
	}
	else if (NAME == "volume_down") {
		call = &Program::Event_VolumeDown;
		defaultKey.scancode = SDL_SCANCODE_S;
	}
	else if (NAME == "fullscreen") {
		call = &Program::Event_ScreenMode;
		defaultKey.scancode = SDL_SCANCODE_L;
	}
	else
		return false;
	name = NAME;
	if (setDefaultKey)
		keys.push_back(defaultKey);
	return true;
}

progEFunc Shortcut::Call() const {
	return call;
}

Playlist::Playlist(string NAME, vector<string> SGS, vector<string> BKS) :
	name(NAME),
	songs(SGS),
	books(BKS)
{}

Directory::Directory(string NAME, vector<string> DIRS, vector<string> FILS) :
	name(NAME),
	dirs(DIRS),
	files(FILS)
{}

GeneralSettings::GeneralSettings()
{}

VideoSettings::VideoSettings(bool VS, bool MAX, bool FSC, vec2i RES, string FONT, string RNDR, map<EColor, vec4b> CLRS) :
	vsync(VS),
	maximized(MAX), fullscreen(FSC),
	resolution(RES),
	font(FONT),
	renderer(RNDR),
	colors(CLRS)
{
	if (font.empty() || !fs::exists(font)) {
#ifdef _WIN32
		font = string(getenv("SystemDrive")) + "\\Windows\\Fonts\\Arial.ttf";
#else
		font = "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf";
#endif
	}
	FillMissingColors();
}

void VideoSettings::FillMissingColors() {
	if (colors.count(EColor::background) == 0)
		colors.insert(make_pair(EColor::background, vec4b(10, 10, 10, 255)));
	else {
		if (colors[EColor::background].x == -1)
			colors[EColor::background].x = 10;
		if (colors[EColor::background].y == -1)
			colors[EColor::background].y = 10;
		if (colors[EColor::background].z == -1)
			colors[EColor::background].z = 10;
		if (colors[EColor::background].a == -1)
			colors[EColor::background].a = 255;
	}

	if (colors.count(EColor::rectangle) == 0)
		colors.insert(make_pair(EColor::rectangle, vec4b(90, 90, 90, 255)));
	else {
		if (colors[EColor::rectangle].x == -1)
			colors[EColor::rectangle].x = 90;
		if (colors[EColor::rectangle].y == -1)
			colors[EColor::rectangle].y = 90;
		if (colors[EColor::rectangle].z == -1)
			colors[EColor::rectangle].z = 90;
		if (colors[EColor::rectangle].a == -1)
			colors[EColor::rectangle].a = 255;
	}

	if (colors.count(EColor::highlighted) == 0)
		colors.insert(make_pair(EColor::highlighted, vec4b(120, 120, 120, 255)));
	else {
		if (colors[EColor::highlighted].x == -1)
			colors[EColor::highlighted].x = 120;
		if (colors[EColor::highlighted].y == -1)
			colors[EColor::highlighted].y = 120;
		if (colors[EColor::highlighted].z == -1)
			colors[EColor::highlighted].z = 120;
		if (colors[EColor::highlighted].a == -1)
			colors[EColor::highlighted].a = 255;
	}

	if (colors.count(EColor::darkened) == 0)
		colors.insert(make_pair(EColor::darkened, vec4b(60, 60, 60, 255)));
	else {
		if (colors[EColor::darkened].x == -1)
			colors[EColor::darkened].x = 60;
		if (colors[EColor::darkened].y == -1)
			colors[EColor::darkened].y = 60;
		if (colors[EColor::darkened].z == -1)
			colors[EColor::darkened].z = 60;
		if (colors[EColor::darkened].a == -1)
			colors[EColor::darkened].a = 255;
	}

	if (colors.count(EColor::text) == 0)
		colors.insert(make_pair(EColor::text, vec4b(210, 210, 210, 255)));
	else {
		if (colors[EColor::text].x == -1)
			colors[EColor::text].x = 210;
		if (colors[EColor::text].y == -1)
			colors[EColor::text].y = 210;
		if (colors[EColor::text].z == -1)
			colors[EColor::text].z = 210;
		if (colors[EColor::text].a == -1)
			colors[EColor::text].a = 255;
	}
}

AudioSettings::AudioSettings(int MV, int SV, float SD) :
	musicVolume(MV),
	soundVolume(SV),
	songDelay(SD)
{}

ControlsSettings::ControlsSettings(bool fillMissingBindings, vector<Shortcut> SRTCS) :
	shortcuts(SRTCS)
{
	if (fillMissingBindings)
		FillMissingBindings();
}

void ControlsSettings::FillMissingBindings() {
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
	if (!shortcut("play_pause_song"))
		shortcuts.push_back(Shortcut("play_pause_song"));
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
