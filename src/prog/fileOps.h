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
struct _PopplerDocument;
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
protected:	// TODO: maybe change back to private
	static constexpr array<byte_t, 5> signaturePdf = { '%'_b, 'P'_b, 'D'_b, 'F'_b, '-'_b };

public:
	virtual ~FileOps();

	static FileOps* instantiate(const RemoteLocation& rl, vector<string>&& passwords);	// loads smbclient or libssh2 if the protocol matches SMB or SFTP

	virtual vector<Cstring> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) = 0;
	virtual pair<vector<Cstring>, vector<Cstring>> listDirectorySep(string_view path, bool hidden) = 0;	// first is files, second is directories
	virtual bool deleteEntry(string_view base) = 0;	// TODO: test!
	virtual bool renameEntry(string_view oldPath, string_view newPath) = 0;
	virtual Data readFile(string_view path) = 0;
	virtual fs::file_type fileType(string_view path) = 0;
	virtual bool isRegular(string_view path) = 0;
	virtual bool isDirectory(string_view path) = 0;
	virtual bool isPicture(string_view path) = 0;
	virtual SDL_Surface* loadPicture(string_view path) = 0;
#ifdef CAN_PDF
	virtual pair<_PopplerDocument*, Data> loadPdf(string_view path, string* error = nullptr) = 0;	// loads Poppler if necessary
#endif
	virtual archive* openArchive(string_view path, string* error = nullptr) = 0;
	virtual void setWatch(string_view path) = 0;
	virtual bool pollWatch(vector<FileChange>& files) = 0;	// returns true if the watched file/directory has been renamed or deleted
	virtual FileOpCapabilities capabilities() const = 0;
	virtual string prefix() const = 0;
	virtual bool equals(const RemoteLocation& rl) const = 0;

	bool isPdf(string_view path);	// loads Poppler if the file is has a PDF signature
	bool isArchive(string_view path);
	void makeArchiveTreeThread(std::stop_token stoken, BrowserResultArchive* ra, uint maxRes);
	static bool isPicture(SDL_RWops* ifh, string_view ext);
	static SDL_Surface* loadArchivePicture(archive* arch, archive_entry* entry);
#ifdef CAN_PDF
	static pair<_PopplerDocument*, Data> loadArchivePdf(archive* arch, archive_entry* entry, string* error = nullptr);
#endif
	static Data readArchiveEntry(archive* arch, archive_entry* entry);

protected:
	virtual size_t readFileStart(string_view path, byte_t* buf, size_t n) = 0;
#ifdef CAN_PDF
	template <class H, class F> static pair<_PopplerDocument*, Data> loadPdfChecked(H fd, size_t esiz, F eread, string* error);
#endif
#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
	static fs::file_type modeToType(mode_t mode);
#endif
	template <Integer C> static bool notDotName(const C* name);
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

	vector<Cstring> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(string_view path, bool hidden) override;
	bool deleteEntry(string_view base) override;
	bool renameEntry(string_view oldPath, string_view newPath) override;
	Data readFile(string_view path) override;
	fs::file_type fileType(string_view path) override;
	bool isRegular(string_view path) override;
	bool isDirectory(string_view path) override;
	bool isPicture(string_view path) override;
	SDL_Surface* loadPicture(string_view path) override;
#ifdef CAN_PDF
	pair<_PopplerDocument*, Data> loadPdf(string_view path, string* error = nullptr) override;
#endif
	archive* openArchive(string_view path, string* error = nullptr) override;
	void setWatch(string_view path) override;
	bool pollWatch(vector<FileChange>& files) override;
	FileOpCapabilities capabilities() const override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const override;

	static Data readFile(const fs::path& path);

protected:
	size_t readFileStart(string_view path, byte_t* buf, size_t n) override;

private:
#ifdef _WIN32
	static bool isDirectory(const wchar_t* path);
	static vector<Cstring> listDrives();
#else
	static bool hasModeFlags(const char* path, mode_t flags);
#endif
	bool unsetWatch();
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

	string prefix() const override;

	bool isPicture(string_view path) override;
	SDL_Surface* loadPicture(string_view path) override;
	archive* openArchive(string_view path, string* error = nullptr) override;
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
	~FileOpsSmb() override;

	vector<Cstring> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(string_view path, bool hidden) override;
	bool deleteEntry(string_view base) override;
	bool renameEntry(string_view oldPath, string_view newPath) override;
	Data readFile(string_view path) override;
	fs::file_type fileType(string_view path) override;
	bool isRegular(string_view path) override;
	bool isDirectory(string_view path) override;
#ifdef CAN_PDF
	pair<_PopplerDocument*, Data> loadPdf(string_view path, string* error = nullptr) override;
#endif
	void setWatch(string_view path) override;
	bool pollWatch(vector<FileChange>& files) override;
	FileOpCapabilities capabilities() const override;
	bool equals(const RemoteLocation& rl) const override;

protected:
	size_t readFileStart(string_view path, byte_t* buf, size_t n) override;

private:
	bool hasModeFlags(string_view path, mode_t mdes);
	bool unsetWatch();
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
	~FileOpsSftp() override;

	vector<Cstring> listDirectory(string_view path, bool files = true, bool dirs = true, bool hidden = true) override;
	pair<vector<Cstring>, vector<Cstring>> listDirectorySep(string_view path, bool hidden) override;
	bool deleteEntry(string_view base) override;
	bool renameEntry(string_view oldPath, string_view newPath) override;
	Data readFile(string_view path) override;
	fs::file_type fileType(string_view path) override;
	bool isRegular(string_view path) override;
	bool isDirectory(string_view path) override;
#ifdef CAN_PDF
	pair<_PopplerDocument*, Data> loadPdf(string_view path, string* error = nullptr) override;
#endif
	void setWatch(string_view) override {}
	bool pollWatch(vector<FileChange>& files) override;
	FileOpCapabilities capabilities() const override;
	bool equals(const RemoteLocation& rl) const override;

protected:
	size_t readFileStart(string_view path, byte_t* buf, size_t n) override;

private:
	void authenticate(const vector<string>& passwords);
	bool hasAttributeFlags(string_view path, ulong flags);
};
#endif
