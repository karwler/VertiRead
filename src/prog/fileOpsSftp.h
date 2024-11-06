#pragma once

#ifdef CAN_SFTP
#include "fileOps.h"
#include "engine/network.h"

struct _LIBSSH2_SESSION;
struct _LIBSSH2_SFTP;
struct _LIBSSH2_SFTP_HANDLE;

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

	BrowserResultList listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept override;
	bool renameEntry(const string& oldPath, const string& newPath) noexcept override;
	Data readFile(const string& path) noexcept override;
	FileType fileType(const string& path) noexcept override;
	bool isRegular(const string& path) noexcept override;
	bool isDirectory(const string& path) noexcept override;
	void setWatch(const string&) noexcept override {}
	void unsetWatch() noexcept override {}
	bool pollWatch(vector<FileChange>& files) noexcept override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) noexcept override;

private:
	void cleanup() noexcept;
	bool hasAttributeFlags(string_view path, ulong flags) noexcept;
	Cstring lastError() const;

#if SDL_VERSION_ATLEAST(3, 2, 0)
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
