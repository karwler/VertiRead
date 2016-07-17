#pragma once

#include "utils/types.h"

class AudioSys {
public:
	AudioSys(const AudioSettings& SETS=AudioSettings());
	~AudioSys();

	uint8 Initialize();
	void FreeMusic();

	void Tick(float dSec);
	static void MusicFinishCallback();

	void PlayPauseMusic();
	void SwitchSong(int step=1);
	void PlaySound(const string& name);

	void LoadPlaylist(const Playlist& newList);
	void UnloadPlaylist();

	AudioSettings Settings() const;
	void MusicVolume(int vol);
	void SoundVolume(int vol);
	void SongDelay(float delay);

private:
	AudioSettings sets;
	vector<string> playlist;
	uint curSong;
	Mix_Music* curMusic;
	float deltaDelay;

	int CheckVolume(int value);
};
