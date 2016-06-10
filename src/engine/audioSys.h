#pragma once

#include "utils/types.h"

class AudioSys {
public:
	AudioSys(const AudioSettings& SETS=AudioSettings());
	~AudioSys();

	void Initialize();
	void Cleanup();
	void FreeMusic();

	void Tick(float dSec);
	static void MusicFinishCallback();

	void PlayPauseMusic();
	void SwitchSong(int step=1);
	void PlaySound(const string& name);

	AudioSettings Settings() const;
	void LoadPlaylist(const Playlist& newList);
	void UnloadPlaylist();

	int MusicVolume() const;
	void MusicVolume(int vol);
	int SoundVolume() const;
	void SoundVolume(int vol);

private:
	AudioSettings sets;
	vector<string> playlist;
	uint curSong;
	Mix_Music* curMusic;
	float deltaDelay;

	int CheckVolume(int value);
};
