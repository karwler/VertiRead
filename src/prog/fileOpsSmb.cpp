#ifdef CAN_SMB
#include "fileOpsSmb.h"
#include "engine/optional/smbclient.h"
#include "utils/compare.h"
#include <stack>

FileOpsSmb::FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords) :
	serverShare(rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep)))
{
	if (!symSmbclient())
		throw std::runtime_error("Failed to load libsmbclient");
	try {
		if (ctx = smbcNewContext(); !ctx)
			throw std::runtime_error("Failed to create SMB context");
		if (!smbcInitContext(ctx))
			throw std::runtime_error("Failed to init SMB context");
		smbcSetContext(ctx);
#ifndef NDEBUG
		smbcSetDebug(ctx, 1);
#endif
		smbcSetLogCallback(ctx, this, logMsg);

		if (!((sopen = smbcGetFunctionOpen(ctx))
			&& (sread = smbcGetFunctionRead(ctx))
			&& (swrite = smbcGetFunctionWrite(ctx))
			&& (slseek = smbcGetFunctionLseek(ctx))
			&& (sclose = smbcGetFunctionClose(ctx))
			&& (sstat = smbcGetFunctionStat(ctx))
			&& (sfstat = smbcGetFunctionFstat(ctx))
			&& (sopendir = smbcGetFunctionOpendir(ctx))
			&& (sreaddir = smbcGetFunctionReaddir(ctx))
			&& (sclosedir = smbcGetFunctionClosedir(ctx))
			&& (sunlink = smbcGetFunctionUnlink(ctx))
			&& (srmdir = smbcGetFunctionRmdir(ctx))
			&& (srename = smbcGetFunctionRename(ctx))
			&& (snotify = smbcGetFunctionNotify(ctx))
		))
			throw std::runtime_error("Failed to get SMB context functions");

		smbcSetFunctionAuthDataWithContext(ctx, [](SMBCCTX* c, const char*, const char*, char*, int, char*, int, char* pw, int pwlen) {
			auto self = static_cast<FileOpsSmb*>(smbcGetOptionUserData(c));
			std::copy_n(self->pwd.data(), std::min(int(self->pwd.length()) + 1, pwlen), pw);
		});
		smbcSetUser(ctx, rl.user.data());
		smbcSetWorkgroup(ctx, rl.workgroup.data());
		smbcSetPort(ctx, rl.port);
		smbcSetOptionUserData(ctx, this);

		string spath = prefix();
		for (string& it : passwords)
			if (pwd = std::move(it); SMBCFILE* dir = sopendir(ctx, spath.data())) {
				sclosedir(ctx, dir);
				return;
			}
		throw std::runtime_error(fmt::format("Failed to open share '{}'", spath));
	} catch (const std::exception&) {
		cleanup();
		throw;
	}
}

FileOpsSmb::~FileOpsSmb() {
	cleanup();
}

void FileOpsSmb::cleanup() noexcept {
	if (wndir)
		sclosedir(ctx, wndir);
	if (ctx)
		smbcFreeContext(ctx, 1);
	closeSmbclient();
}

BrowserResultList FileOpsSmb::listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	sthandle<SMBCFILE*, SmbCloseDir> dir(sopendir(ctx, path.data()), this);
	if (!dir)
		throw std::runtime_error(strerror(errno));
	struct stat ps;
	for (smbc_dirent* it; !csr.stopReq(stoken) && (it = sreaddir(ctx, dir));)
		if (hidden || it->name[0] != '.')
			switch (it->smbc_type) {
			case SMBC_DIR:
				if (dirs && notDotName(it->name))
					rl.dirs.emplace_back(it->name, it->namelen);
				break;
			case SMBC_FILE:
				if (files)
					rl.files.emplace_back(it->name, it->namelen);
				break;
			case SMBC_LINK:
				if (!sstat(ctx, (path / string_view(it->name, it->namelen)).data(), &ps))
					switch (ps.st_mode & S_IFMT) {
					case S_IFDIR:
						if (dirs)
							rl.dirs.emplace_back(it->name, it->namelen);
						break;
					case S_IFREG:
						if (files)
							rl.files.emplace_back(it->name, it->namelen);
					}
			}
	rng::sort(rl.files, Strcomp());
	rng::sort(rl.dirs, Strcomp());
	return rl;
}

void FileOpsSmb::deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		std::lock_guard lockg(mlock);
		if (struct stat ps; sstat(ctx, path->data(), &ps) || !S_ISDIR(ps.st_mode)) {
			if (sunlink(ctx, path->data()))
				rc = ResultCode::error;
		} else if (SMBCFILE* dir = sopendir(ctx, path->data())) {
			CountedStopReq csr(dirStopCheckInterval);
			std::stack<sthandle<SMBCFILE*, SmbCloseDir>> dirs;
			dirs.emplace(dir, this);
			do {
				while (smbc_dirent* it = sreaddir(ctx, dirs.top())) {
					if (csr.stopReq(stoken)) {
						pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::stop)));
						return;
					}

					if (it->smbc_type == SMBC_DIR) {
						if (notDotName(it->name)) {
							*path = *path / string_view(it->name, it->namelen);
							if (dir = sopendir(ctx, path->data()); dir)
								dirs.emplace(dir, this);
							else
								*path = parentPath(*path);
						}
					} else
						sunlink(ctx, (*path / string_view(it->name, it->namelen)).data());
				}
				dirs.pop();
				if (srmdir(ctx, path->data()))
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

bool FileOpsSmb::renameEntry(const string& oldPath, const string& newPath) noexcept {
	std::lock_guard lockg(mlock);
	return !srename(ctx, oldPath.data(), ctx, newPath.data());
}

Data FileOpsSmb::readFile(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	Data data;
	if (SMBCFILE* fh = sopen(ctx, path.data(), O_RDONLY, 0)) {
		if (struct stat ps; !sfstat(ctx, fh, &ps)) {
			try {
				data.resize(ps.st_size);
				if (ssize_t len = sread(ctx, fh, data.data(), data.size()); len < ssize_t(data.size()))
					data.resize(std::max(len, ssize_t(0)));
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				data.clear();
			}
		}
		sclose(ctx, fh);
	}
	return data;
}

FileOps::FileType FileOpsSmb::fileType(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, path.data(), &ps) ? modeToType(ps.st_mode) : FileType::none;
}

bool FileOpsSmb::isRegular(const string& path) noexcept {
	return hasModeFlags(path.data(), S_IFREG);
}

bool FileOpsSmb::isDirectory(const string& path) noexcept {
	return hasModeFlags(path.data(), S_IFDIR);
}

bool FileOpsSmb::hasModeFlags(const char* path, mode_t mdes) noexcept {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, path, &ps) && (ps.st_mode & mdes);
}

SDL_RWops* FileOpsSmb::makeRWops(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	if (SMBCFILE* fh = sopen(ctx, path.data(), O_RDONLY, 0)) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
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
		sclose(ctx, fh);
	}
	return nullptr;
}

#if SDL_VERSION_ATLEAST(3, 2, 0)
Sint64 SDLCALL FileOpsSmb::sdlSize(void* userdata) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSmb*, SMBCFILE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	struct stat ps;
	return !self->sfstat(self->ctx, fh, &ps) ? ps.st_size : -1;
}

Sint64 SDLCALL FileOpsSmb::sdlSeek(void* userdata, Sint64 offset, SDL_IOWhence whence) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSmb*, SMBCFILE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	return self->slseek(self->ctx, fh, offset, whence);
}

size_t SDLCALL FileOpsSmb::sdlRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSmb*, SMBCFILE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	return sdlReadFinish(self->sread(self->ctx, fh, ptr, size), status);
}

size_t SDLCALL FileOpsSmb::sdlWrite(void* userdata, const void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	auto [self, fh] = *static_cast<pair<FileOpsSmb*, SMBCFILE*>*>(userdata);
	std::lock_guard lockg(self->mlock);
	return sdlWriteFinish(self->swrite(self->ctx, fh, ptr, size), status);
}

bool SDLCALL FileOpsSmb::sdlClose(void* userdata) noexcept {
	auto sf = static_cast<pair<FileOpsSmb*, SMBCFILE*>*>(userdata);
	std::lock_guard lockg(sf->first->mlock);
	int rc = sf->first->sclose(sf->first->ctx, sf->second);
	delete sf;
	return !rc;
}
#else
Sint64 SDLCALL FileOpsSmb::sdlSize(SDL_RWops* context) noexcept {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	struct stat ps;
	return !self->sfstat(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), &ps) ? ps.st_size : -1;
}

Sint64 SDLCALL FileOpsSmb::sdlSeek(SDL_RWops* context, Sint64 offset, int whence) noexcept {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return self->slseek(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), offset, whence);
}

size_t SDLCALL FileOpsSmb::sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return std::max(self->sread(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), ptr, size * maxnum), ssize_t(0));
}

size_t SDLCALL FileOpsSmb::sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) noexcept {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return std::max(self->swrite(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), ptr, size * num), ssize_t(0));
}

int SDLCALL FileOpsSmb::sdlClose(SDL_RWops* context) noexcept {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	int rc = self->sclose(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2));
	SDL_FreeRW(context);
	return rc;
}
#endif

void FileOpsSmb::setWatch(const string& path) noexcept {
	std::lock_guard lockg(mlock);
	if (wndir)
		unsetWatch();

	struct stat ps;
	if (!sstat(ctx, path.data(), &ps) && S_ISDIR(ps.st_mode)) {
		wndir = sopendir(ctx, path.data());
		flags = SMBC_NOTIFY_CHANGE_FILE_NAME | SMBC_NOTIFY_CHANGE_DIR_NAME;
		filter.clear();
		wpdir = path;
	} else if (string wp(parentPath(path)); !sstat(ctx, wp.data(), &ps) && S_ISDIR(ps.st_mode)) {
		wndir = sopendir(ctx, wp.data());
		flags = SMBC_NOTIFY_CHANGE_FILE_NAME | SMBC_NOTIFY_CHANGE_SIZE;
		filter = filename(wp);
		wpdir.clear();
	}
}

void FileOpsSmb::unsetWatch() noexcept {
	sclosedir(ctx, wndir);
	wndir = nullptr;
}

bool FileOpsSmb::pollWatch(vector<FileChange>& files) noexcept {
	if (wndir) {
		bool closeWatch = false;
		tuple<FileOpsSmb*, vector<FileChange>*, bool*> tfc(this, &files, &closeWatch);
		std::lock_guard lockg(mlock);
		int rc = snotify(ctx, wndir, 0, flags, notifyTimeout, [](const smbc_notify_callback_action* actions, size_t numActions, void* data) -> int {
			try {
				auto [self, fp, cw] = *static_cast<tuple<FileOpsSmb*, vector<FileChange>*, bool*>*>(data);
				struct stat ps;
				for (size_t i = 0; i < numActions; ++i) {
					switch (actions[i].action) {
					case SMBC_NOTIFY_ACTION_ADDED: case SMBC_NOTIFY_ACTION_NEW_NAME:
						if (self->filter.empty() && !self->sstat(self->ctx, (self->wpdir / actions[i].filename).data(), &ps))
							fp->emplace_back(actions[i].filename, S_ISDIR(ps.st_mode) ? FileChange::addDirectory : FileChange::addFile);
						break;
					case SMBC_NOTIFY_ACTION_REMOVED: case SMBC_NOTIFY_ACTION_OLD_NAME:
						if (self->filter.empty())
							fp->emplace_back(actions[i].filename, FileChange::deleteEntry);
						else if (actions[i].filename == self->filter) {
							*cw = true;
							return 1;
						}
						break;
					case SMBC_NOTIFY_ACTION_MODIFIED:
						if (!self->filter.empty() && actions[i].filename == self->filter) {
							*cw = true;
							return 1;
						}
					}
				}
			} catch (const std::exception& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			}
			return 1;
		}, &tfc);
		if (rc || closeWatch) {
			unsetWatch();
			return true;
		}
	}
	return false;
}

bool FileOpsSmb::canWatch() const noexcept {
	return true;
}

string FileOpsSmb::prefix() const {
	return fmt::format("{}://{}", protocolNames[eint(Protocol::smb)], serverShare);
}

bool FileOpsSmb::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::smb && (rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep))) == serverShare && rl.port == smbcGetPort(ctx) && rl.user == smbcGetUser(ctx) && rl.workgroup == smbcGetWorkgroup(ctx);
}

void FileOpsSmb::logMsg(void*, int level, const char* msg) noexcept {
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SMB level %d: %s", level, msg);
}
#endif
