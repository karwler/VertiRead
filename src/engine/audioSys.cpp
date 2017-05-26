#include "world.h"

AudioSys::AudioSys(const AudioSettings& SETS) :
	sets(SETS),
	curSong(0),
	curMusic(nullptr),
	muted(false),
	played(false),
	deltaDelay(0.f)
{}

AudioSys::~AudioSys() {
	FreeMusic();
	Mix_CloseAudio();
	Mix_Quit();
}

uint8 AudioSys::Initialize() {
	uint8 retval = 0;

	int flags = Mix_Init(MIX_INIT_FLAC | MIX_INIT_FLUIDSYNTH | MIX_INIT_MOD | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG);
	if (flags == 0) {
		cerr << "couldn't initialize audio system\n" << Mix_GetError() << endl;
		return 2;
	}
	if (!(flags & MIX_INIT_FLAC)) {
		cerr << "couldn't initialize flac" << endl << Mix_GetError() << endl;
		retval = 1;
	}
	if (!(flags & MIX_INIT_FLUIDSYNTH)) {
		cerr << "couldn't initialize fluidsynth" << endl << Mix_GetError() << endl;
		retval = 1;
	}
	if (!(flags & MIX_INIT_MOD)) {
		cerr << "couldn't initialize mod" << endl << Mix_GetError() << endl;
		retval = 1;
	}
	if (!(flags & MIX_INIT_MODPLUG)) {
		cerr << "couldn't initialize modplug" << endl << Mix_GetError() << endl;
		retval = 1;
	}
	if (!(flags & MIX_INIT_MP3)) {
		cerr << "couldn't initialize mp3" << endl << Mix_GetError() << endl;
		retval = 1;
	}
	if (!(flags & MIX_INIT_OGG)) {
		cerr << "couldn't initialize ogg" << endl << Mix_GetError() << endl;
		retval = 1;
	}

	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096)) {
		cerr << "couldn't open audio\n" << Mix_GetError() << endl;
		return 3;
	}
	Mix_AllocateChannels(1);

	Mix_HookMusicFinished(MusicFinishCallback);

	MusicVolume(sets.musicVolume);
	SoundVolume(sets.soundVolume);

	return retval;
}

void AudioSys::FreeMusic() {
	if (curMusic) {
		Mix_FreeMusic(curMusic);
		curMusic = nullptr;
	}
}

void AudioSys::Tick(float dSec) {
	if (deltaDelay != 0.f) {
		deltaDelay -= dSec;
		if (deltaDelay <= 0.f) {
			deltaDelay = 0.f;
			SwitchSong(1);
		}
	}
}

void AudioSys::MusicFinishCallback() {
	World::audioSys()->deltaDelay = World::audioSys()->sets.songDelay;
}

void AudioSys::PlayPauseMusic() {
	if (!curMusic)
		SwitchSong(true);
	else if (!played)
		PlayMusic();
	else if (Mix_PausedMusic())
		Mix_ResumeMusic();
	else
		Mix_PauseMusic();
}

void AudioSys::PlayMusic() {
	played = true;
	Mix_PlayMusic(curMusic, 0);
}

void AudioSys::SongMuteSwitch() {
	muted = !muted;
	Mix_VolumeMusic(muted ? 0 : sets.musicVolume);
}

void AudioSys::NextSong() {
	if (curSong == playlist.size()-1)
		curSong = 0;
	else
		curSong++;
	SwitchSong(played && !Mix_PausedMusic());
}

void AudioSys::PrevSong() {
	if (curSong == 0)
		curSong = playlist.size()-1;
	else
		curSong--;
	SwitchSong(played && !Mix_PausedMusic());
}

void AudioSys::SwitchSong(bool play) {
	FreeMusic();
	played = false;
	deltaDelay = 0.f;
	if (playlist.empty())
		return;

	curMusic = Mix_LoadMUS(playlist[curSong].c_str());
	if (curMusic && play)
		PlayMusic();
}

void AudioSys::PlaySound(const string& name) {
	Mix_Chunk* sound = World::library()->getSound(name);
	if (sound)
		Mix_PlayChannel(0, sound, 0);
}

string AudioSys::curSongName() const {
	return filename(playlist[curSong]);
}

bool AudioSys::PlaylistLoaded() const {
	return playlist.size() != 0;
}

void AudioSys::LoadPlaylist(const Playlist& newList) {
	FreeMusic();

	for (size_t i=0; i!=newList.songs.size(); i++) {
		string path = newList.songPath(i);
		Mix_Music* tmp = Mix_LoadMUS(path.c_str());
		if (tmp) {
			Mix_FreeMusic(tmp);
			playlist.push_back(path);
		}
	}
}

void AudioSys::UnloadPlaylist() {
	FreeMusic();
	playlist.clear();
	curSong = 0;
}

const AudioSettings& AudioSys::Settings() const {
	return sets;
}

void AudioSys::MusicVolume(int vol) {
	sets.musicVolume = CheckVolume(vol);
	Mix_VolumeMusic(sets.musicVolume);
}

void AudioSys::SoundVolume(int vol) {
	sets.soundVolume = CheckVolume(vol);
	Mix_Volume(0, sets.soundVolume);
}

void AudioSys::SongDelay(float delay) {
	sets.songDelay = delay;
}

int AudioSys::CheckVolume(int value) {
	if (value >= MIX_MAX_VOLUME)
		return MIX_MAX_VOLUME;
	else if (value <= 0)
		return 0;
	return value;
}
