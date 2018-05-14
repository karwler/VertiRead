#pragma once

#include "utils/utils.h"

// handles audio playback
class Player {
public:
	Player();
	~Player();

	bool init(const string& book);
	static void musicFinishCallback();

	void playPauseMusic();
	void songMuteSwitch();
	void nextSong();
	void prevSong();

	string curSongName() const { return filename(*curSong); }
	void setVolume(int vol);

private:
	uset<string> playlist;			// song paths
	uset<string>::iterator curSong;	// pointer to the song currently being played
	Mix_Music* curMusic;			// data of the song currently beting played
	bool muted, played;				// whether song has been muted, wheter songs has been played yet since it's been loaded

	void loadSong(bool play);
};
