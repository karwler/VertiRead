#pragma once

#include "utils/utils.h"
#include <mutex>
#ifdef _WIN32
#include <windows.h>
#endif

// archive file entry with size for pictures
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
	fs::path rootDir;
	fs::path curDir;
	fs::path file;
	ArchiveDir arch;

	BrowserResultAsync(fs::path&& root, fs::path&& directory, fs::path&& pname = fs::path(), ArchiveDir&& aroot = ArchiveDir());
};

// picture load info
struct BrowserResultPicture : public BrowserResultAsync {
	vector<pair<string, Texture*>> pics;
	std::mutex mpic;
	const bool archive;

	BrowserResultPicture(bool inArch, fs::path&& root, fs::path&& directory, fs::path&& pname = fs::path(), ArchiveDir&& aroot = ArchiveDir());
};

// intermediate picture load buffer
struct BrowserPictureProgress {
	BrowserResultPicture* pnt;
	SDL_Surface* img;
	size_t id;

	BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index);
};

// checks for filename changes
class FileWatch {
private:
	static constexpr size_t esiz = 2048;

#ifdef _WIN32
	HANDLE dirc = INVALID_HANDLE_VALUE;
	OVERLAPPED overlapped{};
	fs::path filter;
	DWORD flags;
#else
	int ino = -1, watch = -1;
#endif
	void* ebuf = nullptr;

public:
	FileWatch(const fs::path& path);
	~FileWatch();

	void set(const fs::path& path);
	void unset();
	pair<vector<pair<bool, string>>, bool> changed();
};
