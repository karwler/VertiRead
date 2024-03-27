#pragma once

#include "types.h"
#include <stack>
#ifdef CAN_SMB
#include <libsmbclient.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif
#include <SDL_image.h>

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

	_SecretService* service;
	_GHashTable* attributes;

public:
	CredentialManager();	// throws if libsecrent couldn't be loaded
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

	static FileOps* instantiate(const RemoteLocation& rl, vector<string>&& passwords);	// loads smbclient or libssh2 if the protocol matches SMB or SFTP

	virtual vector<Cstring> listDirectory(std::stop_token stoken, const string& path, bool files = true, bool dirs = true, bool hidden = true) = 0;
	virtual pair<vector<Cstring>, vector<Cstring>> listDirectorySep(std::stop_token stoken, const string& path, bool hidden) = 0;	// first is files, second is directories
	virtual void deleteEntryThread(std::stop_token stoken, string path) = 0;	// TODO: test!
	virtual bool renameEntry(const string& oldPath, const string& newPath) = 0;
	virtual Data readFile(const string& path) = 0;
	virtual fs::file_type fileType(const string& path) = 0;
	virtual bool isRegular(const string& path) = 0;
	virtual bool isDirectory(const string& path) = 0;
	virtual archive* openArchive(ArchiveData& ad, string* error = nullptr) = 0;
	virtual void setWatch(const string& path) = 0;
	virtual void unsetWatch() = 0;
	virtual bool pollWatch(vector<FileChange>& files) = 0;	// returns true if the watched file/directory has been renamed or deleted
	virtual bool canWatch() const noexcept = 0;
	virtual string prefix() const = 0;
	virtual bool equals(const RemoteLocation& rl) const = 0;

	bool isPicture(const string& path);
	bool isPdf(const string& path);	// loads Poppler if the file is has a PDF signature
	bool isArchive(ArchiveData& ad);
	SDL_Surface* loadPicture(const string& path);
	void makeArchiveTreeThread(std::stop_token stoken, BrowserResultArchive* ra, uint maxRes);
	static Data readArchiveEntry(archive* arch, archive_entry* entry);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	PdfFile loadPdf(const string& path, string* error = nullptr);	// loads Poppler if necessary
	static PdfFile loadArchivePdf(archive* arch, archive_entry* entry, string* error = nullptr);
#endif

protected:
	virtual SDL_RWops* makeRWops(const string& path) = 0;

#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
	static fs::file_type modeToType(mode_t mode) noexcept;
#endif
	template <Integer C> static bool notDotName(const C* name);
	template <Pointer T, class F> static bool checkStopDeleteProcess(CountedStopReq& csr, std::stop_token stoken, std::stack<T>& dirs, F dclose);
	template <InvocableR<int, archive*, ArchiveData&> F> static archive* initArchive(ArchiveData& ad, string* error, F openArch);
private:
	static const char* requestArchivePassphrase(archive* arch, void* data);
	static SDL_RWops* makeArchiveEntryRWops(archive* arch, archive_entry* entry) noexcept;
	static Sint64 SDLCALL sdlArchiveEntrySize(SDL_RWops* context);
	static Sint64 SDLCALL sdlArchiveEntrySeek(SDL_RWops* context, Sint64 offset, int whence);
	static size_t SDLCALL sdlArchiveEntryRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum);
	static size_t SDLCALL sdlArchiveEntryWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num);
	static int SDLCALL sdlArchiveEntryClose(SDL_RWops* context);
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
inline PdfFile FileOps::loadPdf(const string& path, string* error) {
	return PdfFile(makeRWops(path), error);
}

inline PdfFile FileOps::loadArchivePdf(archive* arch, archive_entry* entry, string* error) {
	return PdfFile(makeArchiveEntryRWops(arch, entry), error);
}
#endif

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

	byte_t* ebuf;
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
	FileOpsLocal();
	~FileOpsLocal() override;

	vector<Cstring> listDirectory(std::stop_token stoken, const string& path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(std::stop_token stoken, const string& path, bool hidden) override;
	void deleteEntryThread(std::stop_token stoken, string path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	archive* openArchive(ArchiveData& ad, string* error = nullptr) override;
	void setWatch(const string& path) override;
	void unsetWatch() override;
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const override;

	static Data readFile(const fs::path::value_type* path);

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
#ifdef _WIN32
	static bool isDirectory(const wchar_t* path) noexcept;
	static vector<Cstring> listDrives();
#else
	static bool hasModeFlags(const char* path, mode_t flags) noexcept;
#endif
};

#if defined(CAN_SMB) || defined(CAN_SFTP)
// convenience layer for handling files from remote locations
class FileOpsRemote : public FileOps {
protected:
	static constexpr uint dirStopCheckInterval = 16;

	std::mutex mlock;
	const string server;

public:
	FileOpsRemote(string&& srv);

	string prefix() const override;
	archive* openArchive(ArchiveData& ad, string* error = nullptr) override;
};

inline FileOpsRemote::FileOpsRemote(string&& srv) :
	server(std::move(srv))
{}
#endif

#ifdef CAN_SMB
// SMB file operations
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
	string pwd;
	SMBCFILE* wndir = nullptr;
	string filter;
	string wpdir;
	uint32 flags;

public:
	FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords);
	~FileOpsSmb() override;

	vector<Cstring> listDirectory(std::stop_token stoken, const string& path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(std::stop_token stoken, const string& path, bool hidden) override;
	void deleteEntryThread(std::stop_token stoken, string path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	void setWatch(const string& path) override;
	void unsetWatch() override;
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	bool equals(const RemoteLocation& rl) const override;

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
	void cleanup() noexcept;
	bool hasModeFlags(const char* path, mode_t mdes);
	static void logMsg(void* data, int level, const char* msg);
	static Sint64 SDLCALL sdlSize(SDL_RWops* context);
	static Sint64 SDLCALL sdlSeek(SDL_RWops* context, Sint64 offset, int whence);
	static size_t SDLCALL sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum);
	static size_t SDLCALL sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num);
	static int SDLCALL sdlClose(SDL_RWops* context);
};
#endif

#ifdef CAN_SFTP
// SFTP file operations
class FileOpsSftp final : public FileOpsRemote {
private:
	_LIBSSH2_SESSION* session = nullptr;
	_LIBSSH2_SFTP* sftp = nullptr;
	const string user;
	int sock = -1;
	const uint16 port;

public:
	FileOpsSftp(const RemoteLocation& rl, const vector<string>& passwords);
	~FileOpsSftp() override;

	vector<Cstring> listDirectory(std::stop_token stoken, const string& path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(std::stop_token stoken, const string& path, bool hidden) override;
	void deleteEntryThread(std::stop_token stoken, string path) override;
	bool renameEntry(const string& oldPath, const string& newPath) override;
	Data readFile(const string& path) override;
	fs::file_type fileType(const string& path) override;
	bool isRegular(const string& path) override;
	bool isDirectory(const string& path) override;
	void setWatch(const string&) override {}
	void unsetWatch() override {}
	bool pollWatch(vector<FileChange>& files) override;
	bool canWatch() const noexcept override;
	bool equals(const RemoteLocation& rl) const override;

protected:
	SDL_RWops* makeRWops(const string& path) override;

private:
	void authenticate(const vector<string>& passwords);
	void cleanup() noexcept;
	bool hasAttributeFlags(string_view path, ulong flags);

	static Sint64 SDLCALL sdlSize(SDL_RWops* context);
	static Sint64 SDLCALL sdlSeek(SDL_RWops* context, Sint64 offset, int whence);
	static size_t SDLCALL sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum);
	static size_t SDLCALL sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num);
	static int SDLCALL sdlClose(SDL_RWops* context);
};
#endif
