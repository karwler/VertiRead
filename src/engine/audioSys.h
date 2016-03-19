#pragma once

#include "utils/types.h"

class AudioSys {
public:
	AudioSys();

	bool Initialize(const AudioSettings& sets);
	void Cleanup();
	void FreeMusic();
	void FreeSound();

	void Tick(float dSec);
	static void MusicFinishCallback();
	static void ChannelFinishCallback(int channel);

	void PlayPauseMusic();
	void SwitchSong(int step=1);
	void PlaySound(string path);

	AudioSettings Settings() const;
	void setPlaylist(const Playlist& newList);
	int MusicVolume() const;
	void setMusicVolume(int vol);
	int SoundVolume() const;
	void setSoundVolume(int vol);

private:
	AudioSettings sets;
	Playlist playlist;
	uint curSong;
	Mix_Music* curMusic;
	Mix_Chunk* curSound;
	float deltaDelay;
};
