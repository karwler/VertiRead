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
	else if (sname == "zoom_in") {
		call = &Program::Event_ZoomIn;
		defaultKey.scancode = SDL_SCANCODE_E;
	}
	else if (sname == "zoom_out") {
		call = &Program::Event_ZoomOut;
		defaultKey.scancode = SDL_SCANCODE_Q;
	}
	else if (sname == "zoom_reset") {
		call = &Program::Event_ZoomReset;
		defaultKey.scancode = SDL_SCANCODE_R;
	}
	else if (sname == "center_view") {
		call = &Program::Event_CenterView;
		defaultKey.scancode = SDL_SCANCODE_C;
	}
	else if (sname == "play_pause") {
		call = &Program::Event_PlayPause;
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
	vector<string> names;
	names.push_back("back");
	names.push_back("zoom_in");
	names.push_back("zoom_out");
	names.push_back("zoom_reset");
	names.push_back("center_view");
	names.push_back("play_pause");
	names.push_back("next_song");
	names.push_back("prev_song");
	names.push_back("volume_up");
	names.push_back("volume_down");
	names.push_back("fullscreen");

	for (string& it : names)
		if (!shortcut(it))
			shortcuts.push_back(Shortcut(it));
}

Shortcut* ControlsSettings::shortcut(string name) {
	for (Shortcut& it : shortcuts)
		if (it.Name() == name)
			return &it;
	return nullptr;
}
