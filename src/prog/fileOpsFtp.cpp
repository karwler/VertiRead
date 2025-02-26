#ifdef WITH_FTP
#include "fileOpsFtp.h"
#include "utils/compare.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

FileOpsFtp::FileOpsFtp(const RemoteLocation& rl, const vector<string>& passwords) :
	addrPi(pi.startConnection(rl.server.data(), rl.port, translateFamily(rl.family), timeoutPi)),
	server(valcp(rl.server)),
	user(rl.user),
	port(rl.port)
{
	FtpReceiver recvPi;
	FtpReply reply;
	while ((reply = recvPi.getReply(pi)).isCont(220));
	if (reply != 220)
		throw std::runtime_error(replyError("Failed to connect", reply));

	try {
		bool auth = false, epsv = false, utf8 = false;
		if (reply = recvPi.sendCmd(pi, "FEAT"); !reply.isCont(211))
			throw std::runtime_error(replyError("Failed to get features", reply));
		while ((reply = recvPi.getReply(pi)).entry) {
			if (strciequal(reply.cmd, "AUTH"))
				auth = true;
			else if (strciequal(reply.cmd, "EPSV"))
				epsv = true;
			else if (strciequal(reply.cmd, "MLST") && std::regex_search(reply.args, std::regex(R"r((^|;)type\*?;?(\w|$))r", std::regex::icase)))
				featMlst = true;
			else if (strciequal(reply.cmd, "TVFS"))
				featTvfs = true;
			else if (strciequal(reply.cmd, "UTF8"))
				utf8 = true;
		}
		if (reply != 211)
			throw std::runtime_error(replyError("Failed to get features", reply));
		if (addrPi.g.sa_family == AF_INET6 && !epsv)
			throw std::runtime_error("No IPv6 support");
		if (!featTvfs)
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No TVFS support");
		if (utf8) {
			if (reply = recvPi.sendCmd(pi, "OPTS UTF8 ON"); reply != 200 && reply != 202)
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", replyError("Failed to enable UTF-8", reply).data());
		} else
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No UTF-8 support");

#if defined(CAN_GNUTLS) || defined(CAN_OPENSSL)
		if (auth) {
			if (rl.encrypt != RemoteLocation::Encrypt::off) {
				if (NetConnection::initTlsLib()) {
					if (reply = recvPi.sendCmd(pi, "AUTH TLS"); reply == 234 || reply == 334) {
						pi.setTimeout(0);
						pi.startTls(tlsData);
						pi.setTimeout(timeoutPi);
						if (reply = recvPi.sendCmd(pi, "PBSZ 0"); reply != 200)
							throw std::runtime_error(replyError("Failed to set protection buffer size", reply));
						if (reply = recvPi.sendCmd(pi, "PROT P"); reply != 200)
							throw std::runtime_error(replyError("Failed to set data protection", reply));
					} else if (reply == 500 || reply == 502) {
						handleAuthWarning(rl, replyError("Failed to establish encrypted connection", reply).data());
						NetConnection::closeTlsLib(tlsData);
					} else
						throw std::runtime_error(replyError("Failed to authenticate", reply));
				} else
					handleAuthWarning(rl, "Failed to initialize TLS");
			}
		} else
			handleAuthWarning(rl, "No AUTH support");
#endif

		bool done = false;
		for (auto it = passwords.begin(); !done && it != passwords.end(); ++it) {
			if (reply = recvPi.sendCmd(pi, "USER", rl.user); reply == 331) {
				if (reply = recvPi.sendCmd(pi, "PASS", *it); reply == 230 || reply == 202)
					done = true;
				else if (reply != 530)
					break;
			} else if (reply == 230)
				done = true;
			else
				break;
		}
		if (!done)
			throw std::runtime_error(replyError("Failed to log in", reply));

		if (featMlst) {
			if (reply = recvPi.sendCmd(pi, "OPTS MLST type;"); reply != 200 || !std::regex_match(reply.args, std::regex(R"r(MLST\s+OPTS\s+type;?)r", std::regex::icase))) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", replyError("Failed to set MLST", reply).data());
				featMlst = false;
			}
		} else
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No MLST support");
	} catch (const std::exception&) {
		cleanup();
		throw;
	}
}

FileOpsFtp::~FileOpsFtp() {
	cleanup();
}

void FileOpsFtp::cleanup() noexcept {
	if (pi) {
		try {
			FtpReceiver().sendCmd(pi, "QUIT");
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
		pi.disconnect();
	}
	NetConnection::closeTlsLib(tlsData);
}

void FileOpsFtp::handleAuthWarning(const RemoteLocation& rl, const char* msg) {
	if (rl.encrypt == RemoteLocation::Encrypt::force)
		throw std::runtime_error(msg);
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", msg);
}

BrowserResultList FileOpsFtp::listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	FtpReceiver recvPi, recvDtp;
	string_view farg = prepareFileOp(recvPi, path);
	NetConnection dtp = initPassive(recvPi);
	FtpReply reply = recvPi.sendCmd(pi, featMlst ? "MLSD" : "LIST", sanitizePath(farg));
	if (reply != 150)
		throw std::runtime_error(replyError("Failed to list directory", reply));
	if (pi.tls())
		dtp.startTls(tlsData);
	dtp.setTimeout(timeoutDtp);

	vector<string> links;
	if (featMlst) {
		for (string_view line; !csr.stopReq(stoken) && recvDtp.getLine(dtp, line); recvDtp.advanceLine(line.length())) {
			if (line.empty())
				continue;

		if (string_view type = getMlstType(line); !type.empty())
			if (size_t pos = type.end() - line.begin(); pos + 1 < line.length())
#ifdef _WIN32
				if (string_view name(line.begin() + pos + 1 + isSpace(line[pos + 1]), line.end()); !name.empty()) {
#else
				if (string_view name(line.begin() + pos + 1 + isSpace(line[pos + 1]), line.end()); !name.empty() && (hidden || name[0] != '.')) {
#endif
					if (strciequal(type, "file")) {
						if (files)
							rl.files.emplace_back(name);
					} else if (strciequal(type, "dir")) {
						if (dirs)
							rl.dirs.emplace_back(name);
					} else if (strciequal(type, "OS.UNIX=symlink"))
						links.emplace_back(name);
				}
		}
	} else {
		constexpr string_view arrow = " -> ";
		for (string_view line; !csr.stopReq(stoken) && recvDtp.getLine(dtp, line); recvDtp.advanceLine(line.length())) {
			if (line.empty())
				continue;

			auto pos = line.begin() + 1;
			for (uint i = 0; i < 8; ++i) {	// skip permissions, count, user, group, size, month, day, time
				for (; pos != line.end() && notSpace(*pos); ++pos);
				for (; pos != line.end() && isSpace(*pos); ++pos);
			}
#ifdef _WIN32
			if (pos != line.end())
#else
			if (pos != line.end() && (hidden || pos[0] != '.'))
#endif
				switch (tolower(line[0])) {
				case '-':
					if (files)
						rl.files.emplace_back(std::to_address(pos), line.end() - pos);
					break;
				case 'd':
					if (dirs && (pos[0] != '.' || (pos + 1 != line.end() && (pos[1] != '.' || pos + 2 != line.end()))))
						rl.dirs.emplace_back(std::to_address(pos), line.end() - pos);
					break;
				case 'l':
					if (size_t loc = line.find(arrow, pos - line.begin()); loc != string::npos)
						links.emplace_back(pos, line.begin() + loc);
				}
		}
	}
	dtp.disconnect();
	if (reply = recvPi.getReply(pi); reply != 226)
		throw std::runtime_error(replyError("Failed to list directory", reply));

	for (auto it = links.begin(); !csr.stopReq(stoken) && it != links.end(); ++it)
		switch (statFile(recvPi, (*it)[0] == '/' ? *it : joinPaths(string_view(path), string_view(*it)))) {
		using enum FileType;
		case regular:
			if (files)
				rl.files.emplace_back(*it);
			break;
		case directory:
			if (dirs)
				rl.dirs.emplace_back(*it);
		}
	rng::sort(rl.files, Strcomp());
	rng::sort(rl.dirs, Strcomp());
	return rl;
}

void FileOpsFtp::deleteEntryThread(std::stop_token, uptr<string>) noexcept {
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::error)));
}

bool FileOpsFtp::renameEntry(const string& oldPath, const string& newPath) noexcept {
	std::lock_guard lockg(mlock);
	try {
		FtpReceiver recvPi;
		FtpReply reply = recvPi.sendCmd(pi, "RNFR", sanitizePath(oldPath));
		if (reply != 350)
			throw std::runtime_error(replyError("Failed to rename file", reply));
		if (reply = recvPi.sendCmd(pi, "RNTO", sanitizePath(newPath)); reply != 250)
			throw std::runtime_error(replyError("Failed to rename file", reply));
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return false;
	}
	return true;
}

Data FileOpsFtp::readFile(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	try {
		FtpReceiver recvPi;
		string_view farg = prepareFileOp(recvPi, path);
		NetConnection dtp = initPassive(recvPi);
		FtpReply reply = recvPi.sendCmd(pi, "RETR", sanitizePath(farg));
		if (reply != 150)
			throw std::runtime_error(replyError("Failed to retrieve file", reply));
		if (pi.tls())
			dtp.startTls(tlsData);
		dtp.setTimeout(timeoutDtp);

		size_t reserve = 0;
		std::smatch sm;
		Data data = FtpReceiver::getData(dtp, std::regex_search(reply.args, sm, rgxSize) && std::from_chars(std::to_address(sm[1].first), std::to_address(sm[1].second), reserve, 10).ec == std::error_code() ? reserve : 0);
		dtp.disconnect();
		if (recvPi.getReply(pi) != 226)
			throw std::runtime_error(replyError("Failed to retrieve file", reply));
		return data;
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return Data();
}

FileOps::FileType FileOpsFtp::fileType(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	FtpReceiver recvPi;
	return statFile(recvPi, path);
}

bool FileOpsFtp::isRegular(const string& path) noexcept {
	return fileType(path) == FileType::regular;
}

bool FileOpsFtp::isDirectory(const string& path) noexcept {
	return fileType(path) == FileType::directory;
}

FileOps::FileType FileOpsFtp::statFile(FtpReceiver& recvPi, string_view path) noexcept {
	FileType ret = FileType::none;
	try {
		FtpReply reply;
		ushort type;
		string_view farg = prepareFileOp(recvPi, path);
		if (featMlst) {
			if (reply = recvPi.sendCmd(pi, "MLST", sanitizePath(farg)); !reply.isCont(250))
				throw std::runtime_error(replyError("Failed to stat file", reply));
			type = reply.code;
			if (reply = recvPi.getReply(pi); !reply.code) {
				if (string_view tn = getMlstType(reply.cmd); !tn.empty()) {
					if (strciequal(tn, "file"))
						ret = FileType::regular;
					else if (strciequal(tn, "dir"))
						ret = FileType::directory;
				}
				while (!(reply = recvPi.getReply(pi)).code);
			}
		} else {
			if (reply = recvPi.sendCmd(pi, "STAT", sanitizePath(farg)); !reply.isCont(212) && !reply.isCont(213))
				throw std::runtime_error(replyError("Failed to stat file", reply));
			type = reply.code;
			while (!(reply = recvPi.getReply(pi)).code);
		}
		if (reply != type)
			throw std::runtime_error(replyError("Failed to stat file", reply));
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return FileType::none;
	}
	return ret;
}

string_view FileOpsFtp::getMlstType(string_view line) noexcept {
	if (size_t pos = line.find('='); pos != string::npos)
		if (size_t end = line.find(';', ++pos); end != string::npos)
			return line.substr(pos, end - pos);
	return string_view();
}

NetConnection FileOpsFtp::initPassive(FtpReceiver& recvPi) {
	FtpReply reply = recvPi.sendCmd(pi, addrPi.g.sa_family == AF_INET ? "PASV" : "EPSV");
	IpAddress addrDtp{};
	std::smatch sm;
	if (reply == 227) {
		uint8 h0, h1, h2, h3, p0, p1;
		if (!std::regex_search(reply.args, sm, rgxPasv)
			|| std::from_chars(std::to_address(sm[1].first), std::to_address(sm[1].second), h0, 10).ec != std::error_code()
			|| std::from_chars(std::to_address(sm[2].first), std::to_address(sm[2].second), h1, 10).ec != std::error_code()
			|| std::from_chars(std::to_address(sm[3].first), std::to_address(sm[3].second), h2, 10).ec != std::error_code()
			|| std::from_chars(std::to_address(sm[4].first), std::to_address(sm[4].second), h3, 10).ec != std::error_code()
			|| std::from_chars(std::to_address(sm[5].first), std::to_address(sm[5].second), p0, 10).ec != std::error_code()
			|| std::from_chars(std::to_address(sm[6].first), std::to_address(sm[6].second), p1, 10).ec != std::error_code()
			|| !(p0 || p1)
		)
			throw std::runtime_error(replyError("Invalid passive reply", reply));

		addrDtp.v4.sin_family = AF_INET;
		addrDtp.v4.sin_port = p0 | (p1 << 8);
		addrDtp.v4.sin_addr.s_addr = h0 | (h1 << 8) | (h2 << 16) | (h3 << 24);
	} else if (reply == 229) {
		uint16 nprt;
		if (!std::regex_search(reply.args, sm, rgxEpsv)
			|| std::from_chars(std::to_address(sm[3].first), std::to_address(sm[3].second), nprt, 10).ec != std::error_code()
			|| !nprt
		)
			throw std::runtime_error(replyError("Invalid passive reply", reply));

		if (!sm[1].length() || !sm[2].length())
			addrDtp = addrPi;
		else {
			bool inet = *sm[1].first == '1';
			addrDtp.g.sa_family = inet ? AF_INET : AF_INET6;
			if (inet_pton(addrDtp.g.sa_family, sm[2].str().data(), inet ? &addrDtp.v4.sin_addr : static_cast<void*>(&addrDtp.v6.sin6_addr)) <= 0)
				throw std::runtime_error(replyError("Invalid passive address", reply));
		}
		(addrDtp.g.sa_family == AF_INET ? addrDtp.v4.sin_port : addrDtp.v6.sin6_port) = SDL_SwapBE16(nprt);
	} else
		throw std::runtime_error(replyError("Failed to set passive", reply));
	return addrDtp;
}

string_view FileOpsFtp::prepareFileOp(FtpReceiver& recvPi, string_view path) {
	if (featTvfs)
		return path;

	auto fend = std::find_if(path.rbegin(), path.rend(), notDsep);
	auto fpos = std::find_if(fend, path.rend(), isDsep);
	try {
		FtpReply reply = recvPi.sendCmd(pi, "PWD");
		if (reply != 257)
			throw std::runtime_error(replyError("Failed to get working directory", reply));

		string currentDir;
		if (reply.args[0] == '"')
			readQuoteString(reply.args.data() + 1, currentDir);
		else
			currentDir = std::move(reply.args);
		string_view cwd = currentDir;
		string_view dst(path.begin(), std::find_if(fpos, path.rend(), notDsep).base());
		auto ci = cwd.begin(), di = dst.begin();
		pathCompare(ci, cwd.end(), di, dst.end());
		for (; ci != cwd.end(); ci = std::find_if(std::find_if(ci, cwd.end(), isDsep), cwd.end(), notDsep))
			if (reply = recvPi.sendCmd(pi, "CDUP"); reply != 200 && reply != 250)
				throw std::runtime_error(replyError("Failed to change directory", reply));
		for (string_view::iterator next; di != dst.end(); di = std::find_if(next, dst.end(), notDsep)) {
			next = std::find_if(di, dst.end(), isDsep);
			if (reply = recvPi.sendCmd(pi, "CWD", string_view(di, next)); reply != 200 && reply != 250)
				throw std::runtime_error(replyError("Failed to change directory", reply));
		}
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return path;
	}
	return string_view(fpos.base(), fend.base());
}

string FileOpsFtp::replyError(string_view msg, const FtpReply& reply) {
	return fmt::format("{}: {}{} {}", msg, reply.cmd, !reply.cont ? "" : "-", reply.args);
}

SDL_RWops* FileOpsFtp::makeRWops(const string& path) noexcept {
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
	auto data = new pair(this, FileCache(string(sanitizePath(path))));
	if (SDL_IOStream* ops = SDL_OpenIO(&iface, data))
		return ops;
	delete data;
#else
	if (SDL_RWops* ops = SDL_AllocRW()) {
		ops->size = sdlSize;
		ops->seek = sdlSeek;
		ops->read = sdlRead;
		ops->write = sdlWrite;
		ops->close = sdlClose;;
		ops->hidden.unknown.data1 = this;
		ops->hidden.unknown.data2 = new FileCache(string(sanitizePath(path)));
		return ops;
	}
#endif
	return nullptr;
}

#ifdef WITH_SDL3
Sint64 SDLCALL FileOpsFtp::sdlSize(void* userdata) noexcept {
	FileCache& fc = static_cast<pair<FileOpsFtp*, FileCache>*>(userdata)->second;
	return fc.done ? fc.data.size() : -1;
}

Sint64 SDLCALL FileOpsFtp::sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept {
	return sdlSeek(prepareFileCache(userdata), offset, whence);
}

size_t SDLCALL FileOpsFtp::sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	FileCache& fc = prepareFileCache(userdata);
	size_t len = std::min(size, fc.data.size() - fc.pos);
	if (len) {
		memcpy(ptr, fc.data.data() + fc.pos, len);
		fc.pos += len;
	} else
		*status = SDL_IO_STATUS_EOF;
	return len;
}

size_t SDLCALL FileOpsFtp::sdlWrite(void*, const void*, size_t, SDL_IOStatus* status) noexcept {
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

bool SDLCALL FileOpsFtp::sdlClose(void* userdata) noexcept {
	delete static_cast<pair<FileOpsFtp*, FileCache>*>(userdata);
	return true;
}

FileOpsFtp::FileCache& FileOpsFtp::prepareFileCache(void* userdata) noexcept {
	auto& [self, fc] = *static_cast<pair<FileOpsFtp*, FileCache>*>(userdata);
	return self->prepareFileCache(fc);
}
#else
Sint64 SDLCALL FileOpsFtp::sdlSize(SDL_RWops* context) noexcept {
	auto fc = static_cast<FileCache*>(context->hidden.unknown.data2);
	return fc->done ? fc->data.size() : -1;
}

Sint64 SDLCALL FileOpsFtp::sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept {
	return sdlSeek(prepareFileCache(context), offset, whence);
}

size_t SDLCALL FileOpsFtp::sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept {
	FileCache& fc = prepareFileCache(context);
	size_t len = std::min(size * maxnum, fc.data.size() - fc.pos);
	memcpy(ptr, fc.data.data() + fc.pos, len);
	fc.pos += len;
	return len;
}

size_t SDLCALL FileOpsFtp::sdlWrite(SDL_RWops*, const void*, size_t, size_t) noexcept {
	return 0;
}

int SDLCALL FileOpsFtp::sdlClose(SDL_RWops* context) noexcept {
	delete static_cast<FileCache*>(context->hidden.unknown.data2);
	SDL_FreeRW(context);
	return 0;
}

FileOpsFtp::FileCache& FileOpsFtp::prepareFileCache(SDL_RWops* context) noexcept {
	return static_cast<FileOpsFtp*>(context->hidden.unknown.data1)->prepareFileCache(*static_cast<FileCache*>(context->hidden.unknown.data2));
}
#endif

Sint64 FileOpsFtp::sdlSeek(FileCache& fc, Sint64 offset, SDL_IOWhence whence) noexcept {
	switch (whence) {
	case RW_SEEK_SET:
		fc.pos = offset;
		break;
	case RW_SEEK_CUR:
		fc.pos += offset;
		break;
	case RW_SEEK_END:
		fc.pos = fc.data.size() + offset;
		break;
	default:
		return -1;
	}
	return fc.pos <= fc.data.size() ? fc.pos : fc.pos = fc.data.size();
}

FileOpsFtp::FileCache& FileOpsFtp::prepareFileCache(FileCache& fc) noexcept {
	if (!fc.done) {
		fc.data = readFile(fc.path);
		fc.done = true;
	}
	return fc;
}

#ifdef _WIN32
string FileOpsFtp::sanitizePath(string_view path) {
	string ret(path);
	rng::replace(ret, '\\', '/');
	return ret;
}
#endif

bool FileOpsFtp::pollWatch(vector<FileChange>&) noexcept {
	return false;
}

bool FileOpsFtp::canWatch() const noexcept {
	return false;
}

string FileOpsFtp::prefix() const {
	return fmt::format("{}://{}", protocolNames[eint(Protocol::ftp)], server);
}

bool FileOpsFtp::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::ftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
