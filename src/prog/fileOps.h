#pragma once

#include "types.h"
#include "engine/network.h"
#ifdef CAN_SMB
#include <libsmbclient.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <SDL_image.h>
#include <mutex>
#include <regex>
#include <stack>

struct archive;
struct archive_entry;
struct _SecretService;
struct _GHashTable;
struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;
struct _LIBSSH2_SFTP_HANDLE;

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
#ifdef WITH_ARCHIVE
	struct MakeArchiveTreeData {
		uptr<BrowserResultArchive> ra;
		uint maxRes;

		MakeArchiveTreeData(uptr<BrowserResultArchive>&& res, uint mres) noexcept;
	};
#endif

	virtual ~FileOps() = default;

	static FileOps* instantiate(const RemoteLocation& rl, vector<string>&& passwords);	// loads smbclient or libssh2 if the protocol matches SMB or SFTP

	virtual BrowserResultList listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) = 0;
	virtual void deleteEntryThread(std::stop_token stoken, uptr<string> path) = 0;	// TODO: test!
	virtual bool renameEntry(const string& oldPath, const string& newPath) = 0;
	virtual Data readFile(const string& path) = 0;
	virtual fs::file_type fileType(const string& path) = 0;
	virtual bool isRegular(const string& path) = 0;
	virtual bool isDirectory(const string& path) = 0;
#ifdef WITH_ARCHIVE
	virtual archive* openArchive(ArchiveData& ad, Cstring* error = nullptr) = 0;
#endif
	virtual void setWatch(const string& path) = 0;
	virtual void unsetWatch() = 0;
	virtual bool pollWatch(vector<FileChange>& files) = 0;	// returns true if the watched file/directory has been renamed or deleted
	virtual bool canWatch() const noexcept = 0;
	virtual string prefix() const = 0;
	virtual bool equals(const RemoteLocation& rl) const noexcept = 0;

	bool isPicture(const string& path);
	bool isPdf(const string& path);	// loads Poppler if the file is has a PDF signature
	bool isArchive(ArchiveData& ad);
	SDL_Surface* loadPicture(const string& path);
#ifdef WITH_ARCHIVE
	void makeArchiveTreeThread(std::stop_token stoken, uptr<MakeArchiveTreeData> md);
	static Data readArchiveEntry(archive* arch, archive_entry* entry);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	PdfFile loadPdf(const string& path, Cstring* error = nullptr);	// loads Poppler if necessary
#ifdef WITH_ARCHIVE
	static PdfFile loadArchivePdf(archive* arch, archive_entry* entry, Cstring* error = nullptr);
#endif
#endif

protected:
	virtual SDL_RWops* makeRWops(const string& path) = 0;

#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
	static fs::file_type modeToType(mode_t mode) noexcept;
#endif
	static tuple<bool, bool, bool> unpackListOptions(BrowserListOption opts);
	template <Integer C> static bool notDotName(const C* name);
	template <Pointer T, class F> static bool checkStopDeleteProcess(CountedStopReq& csr, std::stop_token stoken, std::stack<T>& dirs, F dclose);
#if SDL_VERSION_ATLEAST(3, 0, 0)
	static bool SDLCALL sdlFlush(void* userdata, SDL_IOStatus* status) noexcept;
#endif
#ifdef WITH_ARCHIVE
	template <InvocableR<int, archive*, ArchiveData&> F> static archive* initArchive(ArchiveData& ad, Cstring* error, F openArch);
private:
	static const char* requestArchivePassphrase(archive* arch, void* data);
	static SDL_RWops* makeArchiveEntryRWops(archive* arch, archive_entry* entry) noexcept;
#if SDL_VERSION_ATLEAST(3, 0, 0)
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
};

inline bool FileOps::isPdf(const string& path) {
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	return PdfFile::canOpen(makeRWops(path));
#else
	return false;
#endif
}

inline SDL_Surface* FileOps::loadPicture(const string& path) {
	return IMG_Load_RW(makeRWops(path), SDL_TRUE);
}

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
inline PdfFile FileOps::loadPdf(const string& path, Cstring* error) {
	return PdfFile(makeRWops(path), error);
}

#ifdef WITH_ARCHIVE
inline PdfFile FileOps::loadArchivePdf(archive* arch, archive_entry* entry, Cstring* error) {
	return PdfFile(makeArchiveEntryRWops(arch, entry), error);
}
#endif
#endif

inline tuple<bool, bool, bool> FileOps::unpackListOptions(BrowserListOption opts) {
	return tuple(opts & BLO_FILES, opts & BLO_DIRS, opts & BLO_HIDDEN);
}

template <Integer C>
bool FileOps::notDotName(const C* name) {
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

	alignas(void*) byte_t ebuf[esiz];
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

	BrowserResultList listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
#ifdef WITH_ARCHIVE
	archive* openArchive(ArchiveData& ad, Cstring* error = nullptr) override;
#endif
	void setWatch(const string& path) override;
	void unsetWatch() override;
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

	static Data readFile(const fs::path::value_type* path);

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
#ifdef _WIN32
	static bool isDirectory(const wchar_t* path) noexcept;
#else
	static bool hasModeFlags(const char* path, mode_t flags) noexcept;
#endif
};

// convenience layer for handling files from remote locations
#if defined(WITH_FTP) || defined(CAN_SFTP) || defined(CAN_SMB)
class FileOpsRemote : public FileOps {
protected:
	static constexpr uint dirStopCheckInterval = 16;

	std::mutex mlock;

#ifdef WITH_ARCHIVE
public:
	archive* openArchive(ArchiveData& ad, Cstring* error = nullptr) override;
#endif

protected:
#if SDL_VERSION_ATLEAST(3, 0, 0) && (defined(CAN_SMB) || defined(CAN_SFTP))
	static size_t sdlReadFinish(ssize_t len, SDL_IOStatus* status) noexcept;
	static size_t sdlWriteFinish(ssize_t len, SDL_IOStatus* status) noexcept;
#endif
	static int translateFamily(RemoteLocation::Family family);
};
#endif

// SMB file operations
#ifdef CAN_SMB
class FileOpsSmb final : public FileOpsRemote {
private:
	static constexpr uint notifyTimeout = 200;

	SMBCCTX* ctx;
	smbc_open_fn sopen;
	smbc_read_fn sread;
	smbc_write_fn swrite;
	smbc_lseek_fn slseek;
	smbc_close_fn sclose;
	smbc_stat_fn sstat;
	smbc_fstat_fn sfstat;
	smbc_opendir_fn sopendir;
	smbc_readdir_fn sreaddir;
	smbc_closedir_fn sclosedir;
	smbc_unlink_fn sunlink;
	smbc_rmdir_fn srmdir;
	smbc_rename_fn srename;
	smbc_notify_fn snotify;
	const string serverShare;
	string pwd;
	SMBCFILE* wndir = nullptr;
	string filter;
	string wpdir;
	uint32 flags;

public:
	FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords);
	~FileOpsSmb() override;

	BrowserResultList listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	void setWatch(const string& path) override;
	void unsetWatch() override;
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
	void cleanup() noexcept;
	bool hasModeFlags(const char* path, mode_t mdes);
	static void logMsg(void* data, int level, const char* msg) noexcept;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	static Sint64 SDLCALL sdlSize(void* userdata) noexcept;
	static Sint64 SDLCALL sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept;
	static size_t SDLCALL sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static size_t SDLCALL sdlWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static bool SDLCALL sdlClose(void* userdata) noexcept;
#else
	static Sint64 SDLCALL sdlSize(SDL_RWops* context) noexcept;
	static Sint64 SDLCALL sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept;
	static size_t SDLCALL sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept;
	static size_t SDLCALL sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept;
	static int SDLCALL sdlClose(SDL_RWops* context) noexcept;
#endif
};
#endif

// SFTP file operations
#ifdef CAN_SFTP
class FileOpsSftp final : public FileOpsRemote {
private:
	_LIBSSH2_SESSION* session = nullptr;
	_LIBSSH2_SFTP* sftp = nullptr;
	const string server, user;
	SOCKET sock = INVALID_SOCKET;
	const uint16 port;

public:
	FileOpsSftp(const RemoteLocation& rl, const vector<string>& passwords);
	~FileOpsSftp() override;

	BrowserResultList listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	void setWatch(const string&) override {}
	void unsetWatch() override {}
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
	void cleanup() noexcept;
	bool hasAttributeFlags(string_view path, ulong flags);
	Cstring lastError() const;

#if SDL_VERSION_ATLEAST(3, 0, 0)
	static Sint64 SDLCALL sdlSize(void* userdata) noexcept;
	static Sint64 SDLCALL sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept;
	static size_t SDLCALL sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static size_t SDLCALL sdlWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static bool SDLCALL sdlClose(void* userdata) noexcept;
#else
	static Sint64 SDLCALL sdlSize(SDL_RWops* context) noexcept;
	static Sint64 SDLCALL sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept;
	static size_t SDLCALL sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept;
	static size_t SDLCALL sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept;
	static int SDLCALL sdlClose(SDL_RWops* context) noexcept;
#endif
	Sint64 sdlSeek(_LIBSSH2_SFTP_HANDLE* fh, Sint64 offset, SDL_IOWhence whence) noexcept;
};
#endif

// FTP/FTPS file operations
#ifdef WITH_FTP
class FileOpsFtp final : public FileOpsRemote {
private:
	struct FileCache {
		Data data;
		string path;
		size_t pos = 0;
		bool done = false;

		FileCache(string&& fpath) : path(std::move(fpath)) {}
	};

	static constexpr uint timeoutPi = 10;
	static constexpr uint timeoutDtp = 20;

	NetConnection pi;
	IpAddress addrPi{};
	TlsData tlsData{};
	const std::regex rgxPasv = std::regex(R"r((\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+))r", std::regex::optimize);
	const std::regex rgxEpsv = std::regex(R"r(\|\s*([12]?)\s*\|\s*(.*?)\s*\|\s*(\d+)\s*\|)r", std::regex::optimize);
	const std::regex rgxSize = std::regex(R"r(\((\d+)\s*bytes\)\s*$)r", std::regex::icase | std::regex::optimize);
	const string server, user;
	const uint16 port;
	bool featMlst = false;
	bool featTvfs = false;

public:
	FileOpsFtp(const RemoteLocation& rl, const vector<string>& passwords);
	~FileOpsFtp() override;

	BrowserResultList listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	void setWatch(const string&) override {}
	void unsetWatch() override {}
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
	void cleanup() noexcept;
	fs::file_type statFile(FtpReceiver& recvPi, string_view path);
	NetConnection initPassive(FtpReceiver& recvPi);
	string_view prepareFileOp(FtpReceiver& recvPi, string_view path);
	static string_view getMlstType(string_view line);
	static string replyError(string_view msg, const FtpReply& reply);
	void handleAuthWarning(const RemoteLocation& rl, const char* msg);

#if SDL_VERSION_ATLEAST(3, 0, 0)
	static Sint64 SDLCALL sdlSize(void* userdata) noexcept;
	static Sint64 SDLCALL sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept;
	static size_t SDLCALL sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static size_t SDLCALL sdlWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept;
	static bool SDLCALL sdlClose(void* userdata) noexcept;
	static FileCache& prepareFileCache(void* userdata) noexcept;
#else
	static Sint64 SDLCALL sdlSize(SDL_RWops* context) noexcept;
	static Sint64 SDLCALL sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept;
	static size_t SDLCALL sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept;
	static size_t SDLCALL sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept;
	static int SDLCALL sdlClose(SDL_RWops* context) noexcept;
	static FileCache& prepareFileCache(SDL_RWops* context) noexcept;
#endif
	static Sint64 sdlSeek(FileCache& fc, Sint64 offset, SDL_IOWhence whence) noexcept;
	FileCache& prepareFileCache(FileCache& fc) noexcept;
#ifdef _WIN32
	static string sanitizePath(string_view path);
#else
	static string_view sanitizePath(string_view path) { return path; }
#endif
};
#endif
