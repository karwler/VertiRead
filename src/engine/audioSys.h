#pragma once

#include "utils/types.h"

class AudioSys {
public:
	AudioSys(const AudioSettings& SETS=AudioSettings());
	~AudioSys();

	void Initialize();
	void Cleanup();
	void FreeMusic();
	void FreeSound();

	void Tick(float dSec);
	static void MusicFinishCallback();
	static void ChannelFinishCallback(int channel);

	void PlayPauseMusic();
	void SwitchSong(int step=1);
	void PlaySound(const string& file);

	AudioSettings Settings() const;
	void LoadPlaylist(const vector<string>& newList);
	int MusicVolume() const;
	void MusicVolume(int vol);
	int SoundVolume() const;
	void SoundVolume(int vol);

private:
	AudioSettings sets;
	vector<string> playlist;
	uint curSong;
	Mix_Music* curMusic;
	Mix_Chunk* curSound;
	float deltaDelay;
};
