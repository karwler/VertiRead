#include "audioSys.h"
#include "world.h"

AudioSys::AudioSys() :
	curMusic(nullptr),
	curSound(nullptr)
{}

bool AudioSys::Initialize(const AudioSettings& sets) {
	Mix_Init(MIX_INIT_FLAC | MIX_INIT_FLUIDSYNTH | MIX_INIT_MOD | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG);
	Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096);
	Mix_AllocateChannels(1);
	
	Mix_HookMusicFinished(MusicFinishCallback);
	Mix_ChannelFinished(ChannelFinishCallback);

	setMusicVolume(sets.musicVolume);
	setSoundVolume(sets.soundVolume);
	return true;
}

void AudioSys::Cleanup() {
	FreeMusic();
	FreeSound();
	Mix_CloseAudio();
	Mix_Quit();
}

void AudioSys::FreeMusic() {
	if (curMusic)
		Mix_FreeMusic(curMusic);
}

void AudioSys::FreeSound() {
	if (curSound)
		Mix_FreeChunk(curSound);
}

void AudioSys::Tick(float dSec) {
	if (deltaDelay != 0) {
		deltaDelay -= dSec;
		if (deltaDelay <= 0) {
			deltaDelay = 0;
			SwitchSong(1);
		}
	}
}

void AudioSys::MusicFinishCallback() {
	World::audioSys()->deltaDelay = World::audioSys()->sets.songDelay;
}

void AudioSys::ChannelFinishCallback(int channel) {
	World::audioSys()->FreeSound();
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
	curSong += step;
	if (curSong >= playlist.songs.size()) {
		if (step < 0)
			curSong = playlist.songs.size() - 1;
		else
			curSong = 0;
	}
	curMusic = Mix_LoadMUS(playlist.songs[curSong].c_str());
	Mix_PlayMusic(curMusic, 0);
}

void AudioSys::PlaySound(string name) {
	FreeSound();
	curSound = Mix_LoadWAV(name.c_str());
	Mix_PlayChannel(0, curSound, 0);
}

AudioSettings AudioSys::Settings() const {
	return sets;
}

void AudioSys::setPlaylist(const Playlist& newList) {
	FreeMusic();
	playlist = newList;
}

int AudioSys::MusicVolume() const {
	return sets.musicVolume;
}

void AudioSys::setMusicVolume(int vol) {
	if (vol > 128)
		sets.musicVolume = 128;
	else if (vol < 0)
		sets.musicVolume = 0;
	else
		sets.musicVolume = vol;
	Mix_VolumeMusic(sets.musicVolume);
}

int AudioSys::SoundVolume() const {
	return sets.soundVolume;
}

void AudioSys::setSoundVolume(int vol) {
	if (vol > 128)
		sets.soundVolume = 128;
	else if (vol < 0)
		sets.soundVolume = 0;
	else
		sets.soundVolume = vol;
	Mix_Volume(0, sets.soundVolume);
}
