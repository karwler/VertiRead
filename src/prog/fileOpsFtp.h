#pragma once

#ifdef WITH_FTP
#include "fileOps.h"
#include "engine/network.h"
#include <regex>

class FileOpsFtp final : public FileOpsRemote {
private:
	struct FileCache {
		Data data;
		string path;
		size_t pos = 0;
		bool done = false;

		FileCache(string&& fpath) noexcept : path(std::move(fpath)) {}
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
	FileType statFile(FtpReceiver& recvPi, string_view path) noexcept;
	NetConnection initPassive(FtpReceiver& recvPi);
	string_view prepareFileOp(FtpReceiver& recvPi, string_view path);
	static string_view getMlstType(string_view line) noexcept;
	static string replyError(string_view msg, const FtpReply& reply);
	void handleAuthWarning(const RemoteLocation& rl, const char* msg);

#if SDL_VERSION_ATLEAST(3, 2, 0)
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
