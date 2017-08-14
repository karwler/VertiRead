#pragma once

#include "utils/settings.h"

// handles audio playback and contains audio settings
class AudioSys {
public:
	AudioSys(const AudioSettings& SETS=AudioSettings());
	~AudioSys();

	void freeMusic();

	void tick(float dSec);
	static void musicFinishCallback();

	void playPauseMusic();
	void songMuteSwitch();
	void nextSong();
	void prevSong();
	void switchSong(bool play);
	void playSound(const string& name);

	string curSongName() const;
	bool playlistLoaded() const;
	void setPlaylist(const vector<string>& songs);
	void unloadPlaylist();

	const AudioSettings& getSettings() const;
	void setMusicVolume(int vol);
	void setSoundVolume(int vol);
	void setSongDelay(float delay);

private:
	AudioSettings sets;
	vector<string> playlist;	// song paths
	size_t curSong;				// index of the song currently being played
	Mix_Music* curMusic;		// data of the song currently beting played
	bool muted, played;			// whether song has been muted, wheter songs has been played yet since it's been loaded
	float deltaDelay;			// timer between switching songs during playback

	void playMusic();
	int checkVolume(int value);	// adjust volume level if out of range
};
