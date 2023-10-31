#pragma once

#include "utils/utils.h"

enum UserEvent : uint32 {
	SDL_USEREVENT_READER_PROGRESS = SDL_USEREVENT,
	SDL_USEREVENT_READER_FINISHED,
	SDL_USEREVENT_PREVIEW_PROGRESS,
	SDL_USEREVENT_PREVIEW_FINISHED,
	SDL_USEREVENT_ARCHIVE_PROGRESS,
	SDL_USEREVENT_ARCHIVE_FINISHED,
	SDL_USEREVENT_MOVE_PROGRESS,
	SDL_USEREVENT_MOVE_FINISHED,
	SDL_USEREVENT_FONTS_FINISHED,
	SDL_USEREVENT_MAX
};

template <Invocable<SDL_UserEvent&> F>
void cleanupEvent(UserEvent type, F dealloc) {
	array<SDL_Event, 16> events;
	while (int num = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, type, type)) {
		if (num < 0)
			throw std::runtime_error(SDL_GetError());
		for (int i = 0; i < num; ++i)
			dealloc(events[i].user);
	}
}

void pushEvent(UserEvent code, void* data1 = nullptr, void* data2 = nullptr);

enum class FileOpCapabilities : uint8 {
	none = 0x0,
	remove = 0x1,
	rename = 0x2,
	watch = 0x4
};

enum class Protocol : uint8 {
	none,
	smb,
	sftp
};
constexpr array protocolNames = {
	"",
	"smb",
	"sftp"
};
constexpr array<uint16, protocolNames.size()> protocolPorts = {
	0,
	445,
	22
};

enum class Family : uint8 {
	any,
	v4,
	v6
};
constexpr array familyNames = {
	"any",
	"IPv4",
	"IPv6"
};

// connection information about a network location
struct RemoteLocation {
	string server;
	string path;
	string user;
	string workgroup;
	string password;
	uint16 port;
	Protocol protocol = Protocol::none;
	Family family = Family::any;

	static Protocol getProtocol(string_view str);
	static RemoteLocation fromPath(string_view str, Protocol proto);
private:
	static uint16 sanitizePort(string_view port, Protocol protocol);
};

// a new or deleted directory entry
struct FileChange {
	enum Type {
		deleteEntry,
		addFile,
		addDirectory
	};

	string name;
	Type type;

	FileChange(string&& entry, Type change);
};

// archive file with image size
struct ArchiveFile {
	string name;
	uintptr_t size;

	ArchiveFile() = default;
	ArchiveFile(const ArchiveFile& af) = default;
	ArchiveFile(ArchiveFile&& af) = default;
	ArchiveFile(string&& filename, uintptr_t mem);

	ArchiveFile& operator=(const ArchiveFile& af) = default;
	ArchiveFile& operator=(ArchiveFile&& af) = default;
};

// archive directory node
struct ArchiveDir {
	ArchiveDir* parent = nullptr;
	string name;
	vector<ArchiveDir> dirs;
	vector<ArchiveFile> files;

	ArchiveDir() = default;
	ArchiveDir(const ArchiveDir& ad) = default;
	ArchiveDir(ArchiveDir&& ad) = default;
	ArchiveDir(ArchiveDir* daddy, string&& dirname);

	ArchiveDir& operator=(const ArchiveDir& ad) = default;
	ArchiveDir& operator=(ArchiveDir&& ad) = default;

	void sort();
	void clear();
	string path() const;
	pair<ArchiveDir*, ArchiveFile*> find(string_view path);
	vector<ArchiveDir>::iterator findDir(string_view dname);
	vector<ArchiveFile>::iterator findFile(string_view fname);
};

// archive/picture info
struct BrowserResultAsync {
	string rootDir;
	string curDir;
	string file;
	ArchiveDir arch;

	BrowserResultAsync(string&& root, string&& directory, string&& pname = string(), ArchiveDir&& aroot = ArchiveDir());
};

// picture load info
struct BrowserResultPicture : public BrowserResultAsync {
	vector<pair<string, Texture*>> pics;
	std::mutex mpic;
	const bool archive;

	BrowserResultPicture(bool inArch, string&& root, string&& directory, string&& pname = string(), ArchiveDir&& aroot = ArchiveDir());
};

// intermediate picture load buffer
struct BrowserPictureProgress {
	BrowserResultPicture* pnt;
	SDL_Surface* img;
	size_t id;

	BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index);
};

// list of font families, files and which to select
struct FontListResult {
	vector<string> families;
	uptr<string[]> files;
	size_t select;
	string error;

	FontListResult(vector<string>&& fa, uptr<string[]>&& fl, size_t id, string&& msg);
};

