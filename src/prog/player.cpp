#include "engine/world.h"

// AUDIO SYS

Player::Player() :
	curMusic(nullptr),
	muted(false),
	played(false)
{}

Player::~Player() {
	if (curMusic)
		Mix_FreeMusic(curMusic);
	Mix_CloseAudio();
}

bool Player::init(const string& book) {
	// set up mix
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)) {
		cerr << "Couldn't open audio:" << endl << Mix_GetError() << endl;
		return false;
	}
	Mix_HookMusicFinished(musicFinishCallback);
	Mix_VolumeMusic(World::winSys()->sets.getVolume());

	// merge songs from all playlists with this book name (playlist should be already unloaded at this point)
	for (string& file : Filer::listDir(World::winSys()->sets.getDirPlist(), FTYPE_FILE)) {
		Playlist plist = Filer::getPlaylist(delExt(file));
		if (plist.books.find(book) != plist.books.end())
			playlist.insert(plist.songs.begin(), plist.songs.end());
	}
	curSong = playlist.begin();
	return !playlist.empty();
}

void Player::musicFinishCallback() {
	World::program()->getPlayer()->nextSong();
}

void Player::playPauseMusic() {
	if (!curMusic)			// load music if there's none
		loadSong(true);
	else if (!played) {		// call play if song hasn't been played since load
		played = true;
		Mix_PlayMusic(curMusic, 0);
	} else if (Mix_PausedMusic())	// if it's paused, resume playback
		Mix_ResumeMusic();
	else					// if it's playing, pause playback
		Mix_PauseMusic();
}

void Player::songMuteSwitch() {
	muted = !muted;
	Mix_VolumeMusic(muted ? 0 : World::winSys()->sets.getVolume());
}

void Player::nextSong() {
	curSong = (curSong == std::prev(playlist.end())) ? playlist.begin() : std::next(curSong);
	loadSong(played && !Mix_PausedMusic());
}

void Player::prevSong() {
	curSong = (curSong == playlist.begin()) ? std::prev(playlist.end()) : std::prev(curSong);
	loadSong(played && !Mix_PausedMusic());
}

void Player::loadSong(bool play) {
	if (curMusic)
		Mix_FreeMusic(curMusic);

	curMusic = Mix_LoadMUS(curSong->c_str());
	played = curMusic && play;
	if (played)
		Mix_PlayMusic(curMusic, 0);
}

void Player::setVolume(int vol) {
	Mix_VolumeMusic(World::winSys()->sets.setVolume(vol));
}
