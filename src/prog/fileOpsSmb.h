#pragma once

#ifdef CAN_SMB
#include "fileOps.h"
#include <libsmbclient.h>

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

	struct SmbCloseDir {
		FileOpsSmb* fops;

		SmbCloseDir(FileOpsSmb* f) noexcept : fops(f) {}

		void operator()(SMBCFILE* hnd) const noexcept { fops->sclose(fops->ctx, hnd); }
	};

public:
	FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords);
	~FileOpsSmb() override;

	BrowserResultList listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) override;
	void deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept override;
	bool renameEntry(const string& oldPath, const string& newPath) noexcept override;
	Data readFile(const string& path) noexcept override;
	FileType fileType(const string& path) noexcept override;
	bool isRegular(const string& path) noexcept override;
	bool isDirectory(const string& path) noexcept override;
	void setWatch(const string& path) noexcept override;
	void unsetWatch() noexcept override;
	bool pollWatch(vector<FileChange>& files) noexcept override;
	bool canWatch() const noexcept override;
	string prefix() const override;
	bool equals(const RemoteLocation& rl) const noexcept override;

protected:
	SDL_RWops* makeRWops(const string& path) noexcept override;

private:
	void cleanup() noexcept;
	bool hasModeFlags(const char* path, mode_t mdes) noexcept;
	static void logMsg(void* data, int level, const char* msg) noexcept;
#ifdef WITH_SDL3
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
