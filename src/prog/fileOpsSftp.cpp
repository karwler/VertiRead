#ifdef CAN_SFTP
#include "fileOpsSftp.h"
#include "engine/optional/ssh2.h"
#include "utils/compare.h"
#ifndef _WIN32
#include <netdb.h>
#endif
#include <stack>

template <>
struct DefaultHandleClose<LIBSSH2_SFTP_HANDLE*> {
	void operator()(LIBSSH2_SFTP_HANDLE* hnd) const noexcept { sftpClose(hnd); }
};

FileOpsSftp::FileOpsSftp(const RemoteLocation& rl, const vector<string>& passwords) :
	server(valcp(rl.server)),
	user(rl.user),
	port(rl.port)
{
	if (!symLibssh2())
		throw std::runtime_error("Failed to load libssh2");
	try {
		if (sshInit(0))
			throw std::runtime_error("Failed to initialize SSH");

		addrinfo* addrv = resolveAddress(rl.server.data(), port, translateFamily(rl.family));
		for (addrinfo* it = addrv; it; it = it->ai_next) {
			if (sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol); sock == INVALID_SOCKET)
				continue;
			if (!connect(sock, it->ai_addr, it->ai_addrlen))
				break;
			close(sock);
			sock = INVALID_SOCKET;
		}
		freeaddrinfo(addrv);
		if (sock == INVALID_SOCKET)
			throw std::runtime_error("Failed to connect");

		if (session = sshSessionInitEx(nullptr, nullptr, nullptr, nullptr); !session)
			throw std::runtime_error("Failed to initialize SSH session");
		sshSessionSetBlocking(session, 1);
		if (sshSessionHandshake(session, sock))
			throw std::runtime_error("Failed to establish an SSH connection");

		if (char* userauthlist = sshUserauthList(session, user.data(), user.length()))
			for (char* next, *pos = userauthlist; *pos; pos = next) {
				for (; *pos == ','; ++pos);
				for (next = pos; *next && *next != ','; ++next);
				if (string_view method(pos, next); strciequal(method, "password")) {
					for (const string& pwd : passwords)
						if (!sshUserauthPasswordEx(session, user.data(), user.length(), pwd.data(), pwd.length(), nullptr))
							break;
					break;
				}
			}
		if (!sshUserauthAuthenticated(session))
			throw std::runtime_error("Authentication failed");
		if (sftp = sftpInit(session); !sftp)
			throw std::runtime_error("Failed to initialize SFTP session");
	} catch (const std::exception&) {
		cleanup();
		throw;
	}
}

FileOpsSftp::~FileOpsSftp() {
	cleanup();
}

void FileOpsSftp::cleanup() noexcept {
	if (sftp)
		sftpShutdown(sftp);
	if (session) {
		sshSessionDisconnectEx(session, SSH_DISCONNECT_BY_APPLICATION, "shutdown", "");
		sshSessionFree(session);
	}
	if (sock != INVALID_SOCKET)
		close(sock);
	sshExit();
	closeLibssh2();
}

BrowserResultList FileOpsSftp::listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	sthandle<LIBSSH2_SFTP_HANDLE*> dir = sftpOpenEx(sftp, path.data(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR);
	if (!dir)
		throw std::runtime_error(lastError().data());
	char nbuf[2048];
	LIBSSH2_SFTP_ATTRIBUTES attr;
	for (int nlen; !csr.stopReq(stoken) && (nlen = sftpReaddirEx(dir, nbuf, sizeof(nbuf), nullptr, 0, &attr)) > 0;)
		if ((hidden || nbuf[0] != '.') && (attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS))
			switch (attr.permissions & LIBSSH2_SFTP_S_IFMT) {
			case LIBSSH2_SFTP_S_IFDIR:
				if (dirs && notDotName(nbuf))
					rl.dirs.emplace_back(nbuf, nlen);
				break;
			case LIBSSH2_SFTP_S_IFREG:
				if (files)
					rl.files.emplace_back(nbuf, nlen);
				break;
			case LIBSSH2_SFTP_S_IFLNK:
				if (string fpath = path / string_view(nbuf, nlen); !sftpStatEx(sftp, fpath.data(), fpath.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS))
					switch (attr.permissions & LIBSSH2_SFTP_S_IFMT) {
					case LIBSSH2_SFTP_S_IFDIR:
						if (dirs)
							rl.dirs.emplace_back(nbuf, nlen);
						break;
					case LIBSSH2_SFTP_S_IFREG:
						if (files)
							rl.files.emplace_back(nbuf, nlen);
					}
			}
	rng::sort(rl.files, Strcomp());
	rng::sort(rl.dirs, Strcomp());
	return rl;
}

void FileOpsSftp::deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		std::lock_guard lockg(mlock);
		LIBSSH2_SFTP_ATTRIBUTES attr;
		if (sftpStatEx(sftp, path->data(), path->length(), LIBSSH2_SFTP_LSTAT, &attr) || !(attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) || !LIBSSH2_SFTP_S_ISDIR(attr.permissions)) {
			if (sftpUnlinkEx(sftp, path->data(), path->length()))
				rc = ResultCode::error;
		} else if (LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, path->data(), path->length(), 0, 0, LIBSSH2_SFTP_OPENDIR)) {
			CountedStopReq csr(dirStopCheckInterval);
			std::stack<sthandle<LIBSSH2_SFTP_HANDLE*>> dirs;
			dirs.push(dir);
			char nbuf[2048];
			do {
				for (int nlen; (nlen = sftpReaddirEx(dirs.top(), nbuf, sizeof(nbuf), nullptr, 0, &attr)) > 0;) {
					if (csr.stopReq(stoken)) {
						pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::stop)));
						return;
					}

					if ((attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && LIBSSH2_SFTP_S_ISDIR(attr.permissions)) {
						if (notDotName(nbuf)) {
							*path = *path / string_view(nbuf, nlen);
							if (dir = sftpOpenEx(sftp, path->data(), path->length(), 0, 0, LIBSSH2_SFTP_OPENDIR); dir)
								dirs.push(dir);
							else
								*path = parentPath(*path);
						}
					} else {
						string file = *path / string_view(nbuf, nlen);
						sftpUnlinkEx(sftp, file.data(), file.length());
					}
				}
				dirs.pop();
				if (sftpRmdirEx(sftp, path->data(), path->length()))
					rc = ResultCode::error;
				*path = parentPath(*path);
			} while (!dirs.empty());
		} else
			rc = ResultCode::error;
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)));
}

bool FileOpsSftp::renameEntry(const string& oldPath, const string& newPath) noexcept {
	std::lock_guard lockg(mlock);
	return !sftpRenameEx(sftp, oldPath.data(), oldPath.length(), newPath.data(), newPath.length(), LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);
}

Data FileOpsSftp::readFile(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	Data data;
	if (LIBSSH2_SFTP_HANDLE* fh = sftpOpenEx(sftp, path.data(), path.length(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE)) {
		if (LIBSSH2_SFTP_ATTRIBUTES attr; !sftpFstatEx(fh, &attr, 0) && (attr.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
			try {
				data.resize(attr.filesize);
				if (ssize_t len = sftpRead(fh, reinterpret_cast<char*>(data.data()), data.size()); len < ssize_t(data.size()))
					data.resize(std::max(len, ssize_t(0)));
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				data.clear();
			}
		}
		sftpClose(fh);
	}
	return data;
}

FileOps::FileType FileOpsSftp::fileType(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ? modeToType(attr.permissions) : FileType::none;
}

bool FileOpsSftp::isRegular(const string& path) noexcept {
	return hasAttributeFlags(path, LIBSSH2_SFTP_S_IFREG);
}

bool FileOpsSftp::isDirectory(const string& path) noexcept {
	return hasAttributeFlags(path, LIBSSH2_SFTP_S_IFDIR);
}

bool FileOpsSftp::hasAttributeFlags(string_view path, ulong flags) noexcept {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && (attr.permissions & flags);
}

Cstring FileOpsSftp::lastError() const {
	char* str;
	int len;
	sshSessionLastError(session, &str, &len, 0);
	return Cstring(str, len);
}

SDL_RWops* FileOpsSftp::makeRWops(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	if (LIBSSH2_SFTP_HANDLE* fh = sftpOpenEx(sftp, path.data(), path.length(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE)) {
#ifdef WITH_SDL3
		SDL_IOStreamInterface iface = {
			.version = sizeof(iface),
			.size = sdlSize,
			.seek = sdlSeek,
			.read = sdlRead,
			.write = sdlWrite,
			.flush = sdlFlush,
			.close = sdlClose
		};
		auto data = new pair(this, fh);
		if (SDL_IOStream* ops = SDL_OpenIO(&iface, data))
			return ops;
		delete data;
#else
		if (SDL_RWops* ops = SDL_AllocRW()) {
			ops->size = sdlSize;
			ops->seek = sdlSeek;
			ops->read = sdlRead;
			ops->write = sdlWrite;
			ops->close = sdlClose;
			ops->hidden.unknown.data1 = this;
			ops->hidden.unknown.data2 = fh;
			return ops;
		}
#endif
		sftpClose(fh);
	}
	return nullptr;
}

#ifdef WITH_SDL3
Sint64 SDLCALL FileOpsSftp::sdlSize(void* userdata) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSftp*, LIBSSH2_SFTP_HANDLE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpFstatEx(fh, &attr, 0) && (attr.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attr.filesize : -1;
}

Sint64 SDLCALL FileOpsSftp::sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSftp*, LIBSSH2_SFTP_HANDLE*>*>(userdata);
	return self->sdlSeek(fh, offset, whence);
}

size_t SDLCALL FileOpsSftp::sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSftp*, LIBSSH2_SFTP_HANDLE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	return sdlReadFinish(sftpRead(fh, static_cast<char*>(ptr), size), status);
}

size_t SDLCALL FileOpsSftp::sdlWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSftp*, LIBSSH2_SFTP_HANDLE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	return sdlWriteFinish(sftpWrite(fh, static_cast<const char*>(ptr), size), status);
}

bool SDLCALL FileOpsSftp::sdlClose(void* userdata) noexcept {
	auto sf = static_cast<pair<FileOpsSftp*, LIBSSH2_SFTP_HANDLE*>*>(userdata);
	std::lock_guard lockg(sf->first->mlock);
	int rc = sftpClose(sf->second);
	delete sf;
	return !rc;
}
#else
Sint64 SDLCALL FileOpsSftp::sdlSize(SDL_RWops* context) noexcept {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpFstatEx(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), &attr, 0) && (attr.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attr.filesize : -1;
}

Sint64 SDLCALL FileOpsSftp::sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept {
	return static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->sdlSeek(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), offset, whence);
}

size_t SDLCALL FileOpsSftp::sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	return std::max(sftpRead(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), static_cast<char*>(ptr), size * maxnum), ssize_t(0));
}

size_t SDLCALL FileOpsSftp::sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	return std::max(sftpWrite(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), static_cast<const char*>(ptr), size * num), ssize_t(0));
}

int SDLCALL FileOpsSftp::sdlClose(SDL_RWops* context) noexcept {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	int rc = sftpClose(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2));
	SDL_FreeRW(context);
	return rc;
}
#endif

Sint64 FileOpsSftp::sdlSeek(LIBSSH2_SFTP_HANDLE* fh, Sint64 offset, SDL_IOWhence whence) noexcept {
	std::lock_guard lockg(mlock);
	uint64 pos;
	switch (whence) {
	case RW_SEEK_SET:
		pos = offset;
		break;
	case RW_SEEK_CUR:
		pos = sftpTell64(fh) + offset;
		break;
	case RW_SEEK_END: {
		LIBSSH2_SFTP_ATTRIBUTES attr;
		if (sftpFstatEx(fh, &attr, 0) || !(attr.flags & LIBSSH2_SFTP_ATTR_SIZE))
			return -1;
		pos = attr.filesize + offset;
		break; }
	default:
		return -1;
	}
	sftpSeek64(fh, pos);
	return pos;
}

bool FileOpsSftp::pollWatch(vector<FileChange>&) noexcept {
	return false;
}

bool FileOpsSftp::canWatch() const noexcept {
	return false;
}

string FileOpsSftp::prefix() const {
	return fmt::format("{}://{}", protocolNames[eint(Protocol::sftp)], server);
}

bool FileOpsSftp::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::sftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
