#pragma once

#include "utils/settings.h"

class AudioSys {
public:
	AudioSys(const AudioSettings& SETS=AudioSettings());
	~AudioSys();

	void FreeMusic();

	void Tick(float dSec);
	static void MusicFinishCallback();

	void PlayPauseMusic();
	void SongMuteSwitch();
	void NextSong();
	void PrevSong();
	void SwitchSong(bool play);
	void PlaySound(const string& name);

	string curSongName() const;
	bool PlaylistLoaded() const;
	void LoadPlaylist(const Playlist& newList);
	void UnloadPlaylist();

	const AudioSettings& Settings() const;
	void MusicVolume(int vol);
	void SoundVolume(int vol);
	void SongDelay(float delay);

private:
	AudioSettings sets;
	vector<string> playlist;
	size_t curSong;
	Mix_Music* curMusic;
	bool muted, played;
	float deltaDelay;

	void PlayMusic();
	int CheckVolume(int value);
};
