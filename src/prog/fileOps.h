#pragma once

#include "types.h"
#ifdef WITH_ARCHIVE
#include <archive.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <SDL_image.h>
#include <mutex>

struct _SecretService;
struct _GHashTable;

#ifdef WITH_ARCHIVE
namespace std {

template <>
struct default_delete<archive> {
	void operator()(archive* ptr) const noexcept { archive_read_free(ptr); }
};

}
#endif

#ifdef CAN_SECRET
// loads and stores logins via libsecret
class CredentialManager {
private:
	static inline char keyProtocol[] = "protocol";
	static inline char keyServer[] = "server";
	static inline char keyPath[] = "path";
	static inline char keyUser[] = "user";
	static inline char keyWorkgroup[] = "workgroup";
	static inline char keyPort[] = "port";
	static inline char keyFamily[] = "family";

	_SecretService* service;
	_GHashTable* attributes;
	string valProtocol, valServer, valPath, valUser, valWorkgroup, valPort, valFamily;

public:
	CredentialManager();	// throws if libsecrent couldn't be loaded
	~CredentialManager();

	vector<string> loadPasswords(const RemoteLocation& rl);
	void saveCredentials(const RemoteLocation& rl);

private:
	void setAttributes(const RemoteLocation& rl);
};
#endif

// file operations interface
class FileOps {
public:
	enum class FileType : uint8 {
		none,
		regular,
		directory
	};

#ifdef WITH_ARCHIVE
	struct MakeArchiveTreeData {
		uptr<BrowserResultArchive> ra;
		uint maxRes;

		MakeArchiveTreeData(uptr<BrowserResultArchive>&& res, uint mres) noexcept;
	};
#endif

	virtual ~FileOps() = default;

	static FileOps* instantiate(const RemoteLocation& rl, vector<string>&& passwords);	// loads smbclient or libssh2 if the protocol matches SMB or SFTP

	virtual BrowserResultList listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) = 0;
	virtual void deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept = 0;	// TODO: test!
	virtual bool renameEntry(const string& oldPath, const string& newPath) noexcept = 0;
	virtual Data readFile(const string& path) noexcept = 0;
	virtual FileType fileType(const string& path) noexcept = 0;
	virtual bool isRegular(const string& path) noexcept = 0;
	virtual bool isDirectory(const string& path) noexcept = 0;
#ifdef WITH_ARCHIVE
	virtual archive* openArchive(ArchiveData& ad, bool force) = 0;
#endif
	virtual void setWatch(const string& path) noexcept = 0;
	virtual void unsetWatch() noexcept = 0;
	virtual bool pollWatch(vector<FileChange>& files) noexcept = 0;	// returns true if the watched file/directory has been renamed or deleted
	virtual bool canWatch() const noexcept = 0;
	virtual string prefix() const = 0;
	virtual bool equals(const RemoteLocation& rl) const noexcept = 0;

	bool isPicture(const string& path) noexcept;
	bool isPdf(const string& path) noexcept;	// loads Poppler if the file is has a PDF signature
	bool isArchive(ArchiveData& ad) noexcept;
	SDL_Surface* loadPicture(const string& path) noexcept;
#ifdef WITH_ARCHIVE
	void makeArchiveTreeThread(std::stop_token stoken, uptr<MakeArchiveTreeData> md) noexcept;
	static Data readArchiveEntry(archive* arch, archive_entry* entry) noexcept;
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry) noexcept;
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	PdfFile loadPdf(const string& path, bool force);	// loads mupdf/Poppler if necessary
#ifdef WITH_ARCHIVE
	static PdfFile loadArchivePdf(archive* arch, archive_entry* entry, bool force);
#endif
#endif

protected:
	virtual SDL_RWops* makeRWops(const string& path) noexcept = 0;

#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
	static FileType modeToType(mode_t mode) noexcept;
#endif
	static tuple<bool, bool, bool> unpackListOptions(BrowserListOption opts) noexcept;
	template <Integer C> static bool notDotName(const C* name) noexcept;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	static bool SDLCALL sdlFlush(void* userdata, SDL_IOStatus* status) noexcept;
#endif
#ifdef WITH_ARCHIVE
	template <InvocableNothrowR<int, archive*, ArchiveData&> F> static archive* initArchive(ArchiveData& ad, bool force, F openArch);
private:
	static const char* requestArchivePassphrase(archive* arch, void* data) noexcept;
	static SDL_RWops* makeArchiveEntryRWops(archive* arch, archive_entry* entry) noexcept;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	static Sint64 SDLCALL sdlArchiveEntrySize(void* userdata) noexcept;
	static Sint64 SDLCALL sdlArchiveEntrySeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept;
	static size_t SDLCALL sdlArchiveEntryRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static size_t SDLCALL sdlArchiveEntryWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static bool SDLCALL sdlArchiveEntryClose(void* userdata) noexcept;
#else
	static Sint64 SDLCALL sdlArchiveEntrySize(SDL_RWops* context) noexcept;
	static Sint64 SDLCALL sdlArchiveEntrySeek(SDL_RWops* context, Sint64 offset, int whence) noexcept;
	static size_t SDLCALL sdlArchiveEntryRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept;
	static size_t SDLCALL sdlArchiveEntryWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept;
	static int SDLCALL sdlArchiveEntryClose(SDL_RWops* context) noexcept;
#endif
#endif
#ifndef NDEBUG
	static SDL_Surface* loadPictureSurface(uptr<SDL_RWops>&& ops);
#endif
};

inline bool FileOps::isPdf(const string& path) noexcept {
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	return PdfFile::canOpen(uptr<SDL_RWops>(makeRWops(path)));
#else
	return false;
#endif
}

inline SDL_Surface* FileOps::loadPicture(const string& path) noexcept {
#ifndef NDEBUG
	if (path.ends_with(".dat"))
		return loadPictureSurface(uptr<SDL_RWops>(makeRWops(path)));
#endif
	return IMG_Load_RW(makeRWops(path), SDL_TRUE);
}

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
inline PdfFile FileOps::loadPdf(const string& path, bool force) {
	return PdfFile(uptr<SDL_RWops>(makeRWops(path)), force);
}

#ifdef WITH_ARCHIVE
inline PdfFile FileOps::loadArchivePdf(archive* arch, archive_entry* entry, bool force) {
	return PdfFile(uptr<SDL_RWops>(makeArchiveEntryRWops(arch, entry)), force);
}
#endif
#endif

inline tuple<bool, bool, bool> FileOps::unpackListOptions(BrowserListOption opts) noexcept {
	return tuple(opts & BLO_FILES, opts & BLO_DIRS, opts & BLO_HIDDEN);
}

template <Integer C>
bool FileOps::notDotName(const C* name) noexcept {
	return name[0] != '.' || (name[1] != '\0' && (name[1] != '.' || name[2] != '\0'));
}

// local file operations
class FileOpsLocal final : public FileOps {
private:
#ifdef _WIN32
	static constexpr char drivesMax = 26;
#endif
	static constexpr size_t archiveReadBlockSize = 10240;
	static constexpr size_t esiz = 2048;
	static constexpr uint dirStopCheckInterval = 64;

	alignas(void*) uint8 ebuf[esiz];
#ifdef _WIN32
	wstring wpdir;
	wstring filter;
	HANDLE dirc = INVALID_HANDLE_VALUE;
	OVERLAPPED overlapped;
	DWORD flags;
#else
	string wpdir;
	int ino, watch = -1;
#endif

public:
	FileOpsLocal() noexcept;
	~FileOpsLocal() override;

	BrowserResultList listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept override;
	bool renameEntry(const string& oldPath, const string& newPath) noexcept override;
	Data readFile(const string& path) noexcept override;
	FileType fileType(const string& path) noexcept override;
	bool isRegular(const string& path) noexcept override;
	bool isDirectory(const string& path) noexcept override;
#ifdef WITH_ARCHIVE
	archive* openArchive(ArchiveData& ad, bool force) override;
#endif
	void setWatch(const string& path) noexcept override;
	void unsetWatch() noexcept override;
	bool pollWatch(vector<FileChange>& files) noexcept override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) noexcept override;
};

// convenience layer for handling files from remote locations
#if defined(WITH_FTP) || defined(CAN_SFTP) || defined(CAN_SMB)
class FileOpsRemote : public FileOps {
protected:
	static constexpr uint dirStopCheckInterval = 16;

	std::mutex mlock;

#ifdef WITH_ARCHIVE
public:
	archive* openArchive(ArchiveData& ad, bool force) override;
#endif

protected:
#if SDL_VERSION_ATLEAST(3, 2, 0) && (defined(CAN_SMB) || defined(CAN_SFTP))
	static size_t sdlReadFinish(ssize_t len, SDL_IOStatus* status) noexcept;
	static size_t sdlWriteFinish(ssize_t len, SDL_IOStatus* status) noexcept;
#endif
	static int translateFamily(RemoteLocation::Family family) noexcept;
};
#endif
