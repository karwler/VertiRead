#pragma once

#include "types.h"
#include <mutex>
#include <thread>
#ifdef CAN_SMB
#include <libsmbclient.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

struct archive;
struct archive_entry;
struct _SecretService;
struct _GHashTable;
struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;

#ifdef CAN_SECRET
// loads and stores logins via libsecret
class CredentialManager {
private:
	static constexpr char keyProtocol[] = "protocol";
	static constexpr char keyServer[] = "server";
	static constexpr char keyPath[] = "path";
	static constexpr char keyUser[] = "user";
	static constexpr char keyWorkgroup[] = "workgroup";
	static constexpr char keyPort[] = "port";
	static constexpr char keyFamily[] = "family";

	_SecretService* service = nullptr;
	_GHashTable* attributes = nullptr;

public:
	CredentialManager();
	~CredentialManager();

	vector<string> loadPasswords(const RemoteLocation& rl);
	void saveCredentials(const RemoteLocation& rl);

private:
	void setAttributes(const RemoteLocation& rl, string& portTmp);
};
#endif

// file operations interface
class FileOps {
public:
	virtual ~FileOps() = default;

	static FileOps* instantiate(const RemoteLocation& rl, vector<string>&& passwords);

	virtual vector<string> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) = 0;
	virtual pair<vector<string>, vector<string>> listDirectorySep(string_view path, bool hidden) = 0;	// first is files, second is directories
	virtual bool deleteEntry(string_view base) = 0;	// TODO: test!
	virtual bool renameEntry(string_view oldPath, string_view newPath) = 0;
	virtual vector<byte_t> readFile(string_view path) = 0;
	virtual fs::file_type fileType(string_view path) = 0;
	virtual bool isDirectory(string_view path) = 0;
	virtual bool isPicture(string_view path) = 0;
	virtual SDL_Surface* loadPicture(string_view path) = 0;
	virtual archive* openArchive(string_view path) = 0;
	virtual void setWatch(string_view path) = 0;
	virtual optional<vector<FileChange>> pollWatch() = 0;	// nullopt if the watched file/directory has been renamed or deleted
	virtual FileOpCapabilities capabilities() const = 0;
	virtual string prefix() const = 0;
	virtual bool equals(const RemoteLocation& rl) const = 0;

	bool isArchive(string_view path);
	bool isPictureArchive(string_view path);
	bool isArchivePicture(string_view path, string_view pname);
	vector<string> listArchiveFiles(string_view path);
	void makeArchiveTreeThread(std::stop_token stoken, BrowserResultAsync&& ra, uintptr_t maxRes);
	SDL_Surface* loadArchivePicture(string_view path, string_view pname);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);

protected:
#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
	static fs::file_type modeToType(mode_t mode);
#endif
	template <Integer C> static bool notDotName(const C* name);
	static bool isPicture(SDL_RWops* ifh, string_view ext);
private:
	static bool isPicture(archive* arch, archive_entry* entry);
	static pair<uptr<byte_t[]>, int64> readArchiveEntry(archive* arch, archive_entry* entry);
};

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

#ifdef _WIN32
	HANDLE dirc = INVALID_HANDLE_VALUE;
	OVERLAPPED overlapped;
	fs::path filter;
	DWORD flags;
#else
	int ino, watch = -1;
#endif
	byte_t* ebuf;
	fs::path wpdir;

public:
	FileOpsLocal();
	~FileOpsLocal() final;

	vector<string> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) final;
	pair<vector<string>, vector<string>> listDirectorySep(string_view path, bool hidden) final;
	bool deleteEntry(string_view base) final;
	bool renameEntry(string_view oldPath, string_view newPath) final;
	vector<byte_t> readFile(string_view path) final;
	fs::file_type fileType(string_view path) final;
	bool isDirectory(string_view path) final;
	bool isPicture(string_view path) final;
	SDL_Surface* loadPicture(string_view path) final;
	archive* openArchive(string_view path) final;
	void setWatch(string_view path) final;
	optional<vector<FileChange>> pollWatch() final;
	FileOpCapabilities capabilities() const final;
	string prefix() const final;
	bool equals(const RemoteLocation& rl) const final;

	static vector<byte_t> readFile(const fs::path& path);
	static bool isDirectory(const fs::path& path);
private:
#ifdef _WIN32
	static vector<string> listDrives();
#endif
	std::nullopt_t unsetWatch();
};

#if defined(CAN_SMB) || defined(CAN_SFTP)
// convenience layer for handling files from remote locations
class FileOpsRemote : public FileOps {
protected:
	std::mutex mlock;
	const string server;

public:
	FileOpsRemote(string&& srv);
	~FileOpsRemote() override = default;

	string prefix() const final;

	bool isPicture(string_view path) final;
	SDL_Surface* loadPicture(string_view path) final;
	archive* openArchive(string_view path) final;
};

inline FileOpsRemote::FileOpsRemote(string&& srv) :
	server(std::move(srv))
{}
#endif

#ifdef CAN_SMB
// samba file operations
class FileOpsSmb final : public FileOpsRemote {
private:
	static constexpr uint notifyTimeout = 200;

	SMBCCTX* ctx = nullptr;
	smbc_open_fn sopen;
	smbc_read_fn sread;
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
	string pwd;
	SMBCFILE* wndir = nullptr;
	string filter;
	string wpdir;
	uint32 flags;

public:
	FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords);
	~FileOpsSmb() final;

	vector<string> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) final;
	pair<vector<string>, vector<string>> listDirectorySep(string_view path, bool hidden) final;
	bool deleteEntry(string_view base) final;
	bool renameEntry(string_view oldPath, string_view newPath) final;
	vector<byte_t> readFile(string_view path) final;
	fs::file_type fileType(string_view path) final;
	bool isDirectory(string_view path) final;
	void setWatch(string_view path) final;
	optional<vector<FileChange>> pollWatch() final;
	FileOpCapabilities capabilities() const final;
	bool equals(const RemoteLocation& rl) const final;

private:
	std::nullopt_t unsetWatch();
	static void logMsg(void* data, int level, const char* msg);
};
#endif

#ifdef CAN_SFTP
// SFTP file operations
class FileOpsSftp final : public FileOpsRemote {
private:
	_LIBSSH2_SESSION* session = nullptr;
	_LIBSSH2_SFTP* sftp;
	const string user;
	int sock = -1;
	const uint16 port;

public:
	FileOpsSftp(const RemoteLocation& rl, const vector<string>& passwords);
	~FileOpsSftp() final;

	vector<string> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) final;
	pair<vector<string>, vector<string>> listDirectorySep(string_view path, bool hidden) final;
	bool deleteEntry(string_view base) final;
	bool renameEntry(string_view oldPath, string_view newPath) final;
	vector<byte_t> readFile(string_view path) final;
	fs::file_type fileType(string_view path) final;
	bool isDirectory(string_view path) final;
	void setWatch(string_view) final {}
	optional<vector<FileChange>> pollWatch() final;
	FileOpCapabilities capabilities() const final;
	bool equals(const RemoteLocation& rl) const final;

private:
	void authenticate(const vector<string>& passwords);
};
#endif
