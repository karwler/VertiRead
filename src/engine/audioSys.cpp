#include "world.h"

AudioSys::AudioSys(const AudioSettings& SETS) :
	sets(SETS),
	curMusic(nullptr),
	deltaDelay(0.f)
{
	Initialize();
}

AudioSys::~AudioSys() {
	Cleanup();
}

void AudioSys::Initialize() {
	Mix_Init(MIX_INIT_FLAC | MIX_INIT_FLUIDSYNTH | MIX_INIT_MOD | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG);
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096);
	Mix_AllocateChannels(1);
	
	Mix_HookMusicFinished(MusicFinishCallback);

	MusicVolume(sets.musicVolume);
	SoundVolume(sets.soundVolume);
}

void AudioSys::Cleanup() {
	FreeMusic();
	Mix_CloseAudio();
	Mix_Quit();
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
	if (!curMusic) {
		curSong = 0;
		SwitchSong(0);
	}
	else if (Mix_PausedMusic())
		Mix_ResumeMusic();
	else
		Mix_PauseMusic();
}

void AudioSys::SwitchSong(int step) {
	FreeMusic();
	deltaDelay = 0.f;
	if (playlist.empty())
		return;

	curSong += step;
	if (curSong >= playlist.size()) {
		if (step < 0)
			curSong = playlist.size() - 1;
		else
			curSong = 0;
	}
	if (curMusic = Mix_LoadMUS(playlist[curSong].c_str()))
		Mix_PlayMusic(curMusic, 0);
}

void AudioSys::PlaySound(const string& name) {
	Mix_PlayChannel(0, World::library()->getSound(name), 0);
}

void AudioSys::LoadPlaylist(const Playlist& newList) {
	FreeMusic();

	playlist.resize(newList.songs.size());
	for (uint i=0; i!=newList.songs.size(); i++)
		playlist[i] = newList.songPath(i);
}

void AudioSys::UnloadPlaylist() {
	FreeMusic();
	playlist.clear();
}

AudioSettings AudioSys::Settings() const {
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
