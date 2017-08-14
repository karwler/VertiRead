#include "world.h"

AudioSys::AudioSys(const AudioSettings& SETS) :
	sets(SETS),
	curMusic(nullptr),
	muted(false),
	played(false),
	deltaDelay(0.f)
{
	// initialize and set up Mix
	int flags = Mix_Init(MIX_INIT_FLAC | MIX_INIT_FLUIDSYNTH | MIX_INIT_MOD | MIX_INIT_MODPLUG | MIX_INIT_MP3 | MIX_INIT_OGG);
	if (!(flags & MIX_INIT_FLAC))
		cerr << "couldn't initialize flac" << endl << Mix_GetError() << endl;
	if (!(flags & MIX_INIT_FLUIDSYNTH))
		cerr << "couldn't initialize fluidsynth" << endl << Mix_GetError() << endl;
	if (!(flags & MIX_INIT_MOD))
		cerr << "couldn't initialize mod" << endl << Mix_GetError() << endl;
	if (!(flags & MIX_INIT_MODPLUG))
		cerr << "couldn't initialize modplug" << endl << Mix_GetError() << endl;
	if (!(flags & MIX_INIT_MP3))
		cerr << "couldn't initialize mp3" << endl << Mix_GetError() << endl;
	if (!(flags & MIX_INIT_OGG))
		cerr << "couldn't initialize ogg" << endl << Mix_GetError() << endl;

	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096))
		cerr << "couldn't open audio\n" << Mix_GetError() << endl;
	Mix_AllocateChannels(1);
	Mix_HookMusicFinished(musicFinishCallback);

	// set volume levels
	setMusicVolume(sets.musicVolume);
	setSoundVolume(sets.soundVolume);
}

AudioSys::~AudioSys() {
	freeMusic();
	Mix_CloseAudio();
	Mix_Quit();
}

void AudioSys::freeMusic() {
	if (curMusic) {
		Mix_FreeMusic(curMusic);
		curMusic = nullptr;
	}
}

void AudioSys::tick(float dSec) {
	// check if it's time yet to switch between songs. if so, do so
	if (deltaDelay != 0.f) {
		deltaDelay -= dSec;
		if (deltaDelay <= 0.f) {
			deltaDelay = 0.f;
			switchSong(1);
		}
	}
}

void AudioSys::musicFinishCallback() {
	World::audioSys()->deltaDelay = World::audioSys()->sets.songDelay;	// song has finished so reset timer for next song
}

void AudioSys::playPauseMusic() {
	if (!curMusic)
		switchSong(true);	// load music if there's none
	else if (!played)
		playMusic();		// call play if song hasn't been played since load
	else if (Mix_PausedMusic())
		Mix_ResumeMusic();	// if it's paused, resume playback
	else
		Mix_PauseMusic();	// if it's playing, pause playback
}

void AudioSys::playMusic() {
	played = true;
	Mix_PlayMusic(curMusic, 0);
}

void AudioSys::songMuteSwitch() {
	muted = !muted;
	Mix_VolumeMusic(muted ? 0 : sets.musicVolume);
}

void AudioSys::nextSong() {
	curSong = (curSong == playlist.size()-1) ? 0 : curSong + 1;
	switchSong(played && !Mix_PausedMusic());
}

void AudioSys::prevSong() {
	curSong = (curSong == 0) ? playlist.size()-1 : curSong - 1;
	switchSong(played && !Mix_PausedMusic());
}

void AudioSys::switchSong(bool play) {
	// reset values
	freeMusic();
	played = false;
	deltaDelay = 0.f;
	if (playlist.empty())
		return;

	// load song data (aka music)
	curMusic = Mix_LoadMUS(playlist[curSong].c_str());
	if (curMusic && play)
		playMusic();	// play if not paused
}

void AudioSys::playSound(const string& name) {
	Mix_Chunk* sound = World::library()->sound(name);
	if (sound)
		Mix_PlayChannel(0, sound, 0);
}

string AudioSys::curSongName() const {
	return filename(playlist[curSong]);
}

bool AudioSys::playlistLoaded() const {
	return playlist.size() != 0;
}

void AudioSys::setPlaylist(const vector<string>& songs) {
	unloadPlaylist();

	for (const string& it : songs)
		if (Mix_Music* snd = Mix_LoadMUS(it.c_str())) {	// add only valid sound files
			Mix_FreeMusic(snd);
			playlist.push_back(it);
		}
}

void AudioSys::unloadPlaylist() {
	freeMusic();
	playlist.clear();
	curSong = 0;
}

const AudioSettings& AudioSys::getSettings() const {
	return sets;
}

void AudioSys::setMusicVolume(int vol) {
	sets.musicVolume = checkVolume(vol);
	Mix_VolumeMusic(sets.musicVolume);
}

void AudioSys::setSoundVolume(int vol) {
	sets.soundVolume = checkVolume(vol);
	Mix_Volume(0, sets.soundVolume);
}

void AudioSys::setSongDelay(float delay) {
	sets.songDelay = delay;
}

int AudioSys::checkVolume(int value) {
	if (value >= MIX_MAX_VOLUME)
		return MIX_MAX_VOLUME;
	else if (value <= 0)
		return 0;
	return value;
}
