#include "fileOps.h"
#include "fileOpsFtp.h"
#include "fileOpsSftp.h"
#include "fileOpsSmb.h"
#include "engine/fileSys.h"
#include "engine/optional/glib.h"
#include "engine/optional/secret.h"
#include "utils/compare.h"
#ifdef WITH_ARCHIVE
#include <archive_entry.h>
#endif
#ifndef _WIN32
#include <dirent.h>
#include <netinet/in.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#endif
#include <queue>
#include <semaphore>
#include <stack>

#ifdef _WIN32
#ifdef __MINGW32__
namespace {

struct WinCloseFind {
	void operator()(HANDLE ptr) const noexcept {
		if (ptr && ptr != INVALID_HANDLE_VALUE)
			FindClose(ptr);
	}
};

}
using WinFindPtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, WinCloseFind>;
#else
template <>
struct DefaultHandleClose<HANDLE> {
	void operator()(HANDLE hnd) const noexcept { FindClose(hnd); }
};
using WinFindPtr = sthandle<HANDLE, DefaultHandleClose<HANDLE>, INVALID_HANDLE_VALUE>;
#endif
#else
template <>
struct DefaultHandleClose<DIR*> {
	void operator()(DIR* hnd) const noexcept { closedir(hnd); }
};
#endif

// CREDENTIAL MANAGER

#ifdef CAN_SECRET
static constexpr SecretSchema schema = {
	.name = "org.karwler.vertiread",
	.flags = SECRET_SCHEMA_NONE,
	.attributes = {
		{ "protocol", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "server", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "path", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "user", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "workgroup", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "port", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ "family", SECRET_SCHEMA_ATTRIBUTE_STRING },
		{ nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
	}
};

CredentialManager::CredentialManager() {
	if (!symLibsecret())
		throw std::runtime_error("Unable to instantiate credential manager");

	GError* error = nullptr;
	if (service = secretServiceGetSync(SECRET_SERVICE_OPEN_SESSION | SECRET_SERVICE_LOAD_COLLECTIONS, nullptr, &error); error) {
		string err = error->message;
		gErrorFree(error);
		closeLibsecret();
		throw std::runtime_error(err);
	}
	if (attributes = gHashTableNew(gStrHash, [](gconstpointer a, gconstpointer b) -> gboolean { return !strcmp(static_cast<const char*>(a), static_cast<const char*>(b)); }); !attributes) {
		gObjectUnref(service);
		closeLibsecret();
		throw std::runtime_error("Failed to allocate attribute table");
	}
}

CredentialManager::~CredentialManager() {
	gHashTableUnref(attributes);
	gObjectUnref(service);
	closeLibsecret();
}

vector<string> CredentialManager::loadPasswords(const RemoteLocation& rl) {
	setAttributes(rl);
	vector<string> pwds;
	GError* error = nullptr;
	if (GList* list = secretServiceSearchSync(service, &schema, attributes, SECRET_SEARCH_ALL | SECRET_SEARCH_UNLOCK, nullptr, &error); error)
		gErrorFree(error);
	else if (list) {
		for (GList* it = list; it; it = it->next) {
			auto sitem = static_cast<SecretItem*>(it->data);
			SecretValue* svalue = secretItemGetSecret(sitem);
			if (!svalue) {
				if (error = nullptr; !secretItemLoadSecretSync(sitem, nullptr, &error) || error) {
					if (error)
						gErrorFree(error);
					continue;
				}
				if (svalue = secretItemGetSecret(sitem); !svalue)
					continue;
			}
			if (const gchar* text = secretValueGetText(svalue))
				pwds.emplace_back(text);
		}
		gListFree(list);
	}
	return pwds;
}

void CredentialManager::saveCredentials(const RemoteLocation& rl) {
	setAttributes(rl);
	SecretValue* svalue = secretValueNew(rl.password.data(), (rl.password.length() + 1) * sizeof(char), "string");	// TODO: what should this last argument be?
	GError* error = nullptr;
	secretServiceStoreSync(service, &schema, attributes, SECRET_COLLECTION_DEFAULT, fmt::format("VertiRead credentials for {}://{}@{}", protocolNames[eint(rl.protocol)], rl.user, rl.server).data(), svalue, nullptr, &error);
	if (error) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", error->message);
		gErrorFree(error);
	}
}

void CredentialManager::setAttributes(const RemoteLocation& rl) {
	gHashTableInsert(attributes, keyProtocol, (valProtocol = protocolNames[eint(rl.protocol)]).data());
	gHashTableInsert(attributes, keyServer, (valServer = rl.server).data());
	gHashTableInsert(attributes, keyUser, (valUser = rl.user).data());
	gHashTableInsert(attributes, keyPort, (valPort = toStr(rl.port)).data());
	if (rl.protocol == Protocol::smb) {
		gHashTableInsert(attributes, keyPath, (valPath = rl.path).data());
		gHashTableInsert(attributes, keyWorkgroup, (valWorkgroup = rl.workgroup).data());
		gHashTableRemove(attributes, keyFamily);
	} else {
		gHashTableRemove(attributes, keyPath);
		gHashTableRemove(attributes, keyWorkgroup);
		gHashTableInsert(attributes, keyFamily, (valFamily = RemoteLocation::familyNames[eint(rl.family)]).data());
	}
}
#endif

// FILE OPS

#ifdef WITH_ARCHIVE
FileOps::MakeArchiveTreeData::MakeArchiveTreeData(uptr<BrowserResultArchive>&& res, uint mres) noexcept :
	ra(std::move(res)),
	maxRes(mres)
{}
#endif

FileOps* FileOps::instantiate(const RemoteLocation& rl, vector<string>&& passwords) {
#if defined(WITH_FTP) || defined(CAN_SFTP) || defined(CAN_SMB)
	switch (rl.protocol) {
	using enum Protocol;
#ifdef WITH_FTP
	case ftp:
		return new FileOpsFtp(rl, passwords);
#endif
#ifdef CAN_SFTP
	case sftp:
		return new FileOpsSftp(rl, passwords);
#endif
#ifdef CAN_SMB
	case smb:
		return new FileOpsSmb(rl, std::move(passwords));
#endif
	}
#endif
	return nullptr;	// this line should never be hit, because Browser handles local and remote instantiation separately
}

#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
FileOps::FileType FileOps::modeToType(mode_t mode) noexcept {
	switch (mode & S_IFMT) {
	case S_IFDIR:
		return FileType::directory;
	case S_IFREG:
		return FileType::regular;
	}
	return FileType::none;
}
#endif

bool FileOps::isPicture(const string& path) noexcept {
#if SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
	static constexpr bool (SDLCALL* const magics[])(SDL_RWops*) = {
#else
	static constexpr int (SDLCALL* const magics[])(SDL_RWops*) = {
#endif
		IMG_isJPG,
		IMG_isPNG,
		IMG_isBMP,
		IMG_isWEBP,
		IMG_isGIF,
		IMG_isTIF,
		IMG_isCUR,
		IMG_isICO,
		IMG_isLBM,
		IMG_isPCX,
		IMG_isPNM,
		IMG_isSVG,
		IMG_isXCF,
		IMG_isXPM,
		IMG_isXV,
#if SDL_IMAGE_VERSION_ATLEAST(2, 6, 0)
		IMG_isAVIF,
		IMG_isJXL,
		IMG_isQOI
#endif
	};
#ifndef NDEBUG
	if (path.ends_with(".dat"))
		return true;
#endif
	if (uptr<SDL_RWops> ifh(makeRWops(path)); ifh) {
#if SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
		for (bool (SDLCALL* const test)(SDL_RWops*) : magics)
#else
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
#endif
			if (test(ifh.get()))
				return true;
		if (strciequal(fileExtension(path), "TGA"))
			if (uptr<SDL_Surface> img(IMG_LoadTGA_RW(ifh.get())); img)
				return true;
	}
	return false;
}

bool FileOps::isArchive(ArchiveData& ad) noexcept {
#ifdef WITH_ARCHIVE
	if (uptr<archive> arch(openArchive(ad, false)); arch)
		return true;
#endif
	return false;
}

#ifdef WITH_ARCHIVE
void FileOps::makeArchiveTreeThread(std::stop_token stoken, uptr<MakeArchiveTreeData> md) noexcept {
	try {
		uptr<archive> arch(openArchive(md->ra->arch, true));
		int rc;
		for (archive_entry* entry; (rc = archive_read_next_header(arch.get(), &entry)) == ARCHIVE_OK;) {
			if (stoken.stop_requested()) {
				md->ra->rc = ResultCode::stop;
				pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, md->ra.release());
				return;
			}

			ArchiveDir* node = &md->ra->arch;
			for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
				if (const char* next = strchr(path, '/')) {
					string_view sname(path, next);
					auto dit = rng::find_if(node->dirs, [sname](const ArchiveDir& it) -> bool { return it.name == sname; });
					node = dit != node->dirs.end() ? std::to_address(dit) : &node->dirs.emplace_front(sname);
					path = next + 1;
				} else {
					Data data = readArchiveEntry(arch.get(), entry);
					if (uptr<SDL_Surface> img(IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE)); img)
						node->files.emplace_front(path, true, false);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
					else if (data.size() >= PdfFile::signatureLen && !memcmp(PdfFile::signature, data.data(), PdfFile::signatureLen))
						node->files.emplace_front(path, false, true);
#endif
					else
						node->files.emplace_front(path, false, false);
					break;
				}
			}
		}

		if (rc == ARCHIVE_EOF) {
			std::queue<ArchiveDir*> dirs;
			dirs.push(&md->ra->arch);
			do {
				ArchiveDir* dit = dirs.front();
				dit->finalize();
				for (ArchiveDir& it : dit->dirs)
					dirs.push(&it);
				dirs.pop();
			} while (!dirs.empty());
		} else if (md->ra->arch.pc == ArchiveData::PassCode::ignore)
			md->ra->rc = ResultCode::stop;
		else
			throw std::runtime_error(archive_error_string(arch.get()));
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		md->ra->rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, md->ra.release());
}

SDL_Surface* FileOps::loadArchivePicture(archive* arch, archive_entry* entry) noexcept {
	Data data = readArchiveEntry(arch, entry);
	return IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE);
}

Data FileOps::readArchiveEntry(archive* arch, archive_entry* entry) noexcept {
	if (int64 bsiz = archive_entry_size(entry); bsiz > 0) {
		try {
			Data data(bsiz);
			if (la_ssize_t len = archive_read_data(arch, data.data(), bsiz); len < bsiz) {
				if (len <= 0)
					return Data();
				data.resize(len);
			}
			return data;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	return Data();
}

template <InvocableNothrowR<int, archive*, ArchiveData&> F>
archive* FileOps::initArchive(ArchiveData& ad, bool force, F openArch) {
	uptr<archive> arch(archive_read_new());
	if (arch) {
		archive_read_support_filter_all(arch.get());
		archive_read_support_format_all(arch.get());
		if (ad.pc == ArchiveData::PassCode::set || ad.pc == ArchiveData::PassCode::attempt)
			archive_read_add_passphrase(arch.get(), ad.passphrase.data());
		if (ad.pc <= ArchiveData::PassCode::set)
			archive_read_set_passphrase_callback(arch.get(), &ad, requestArchivePassphrase);
		if (openArch(arch.get(), ad) != ARCHIVE_OK) {
			if (force)
				throw std::runtime_error(archive_error_string(arch.get()));
			return nullptr;
		}
	} else if (force)
		throw std::runtime_error("Failed to allocate archive object");
	return arch.release();
}

const char* FileOps::requestArchivePassphrase(archive* arch, void* data) noexcept {
	auto ad = static_cast<ArchiveData*>(data);
	std::binary_semaphore sem(0);
	pushEvent(SDL_USEREVENT_GENERAL, GeneralEvent::startRequestPassphrase, ad, &sem);
	sem.acquire();	// wait for user input
	if (ad->pc == ArchiveData::PassCode::set)
		return ad->passphrase.data();
	archive_read_set_passphrase_callback(arch, nullptr, nullptr);	// just returning nullptr causes this function to be called again
	return nullptr;
}

SDL_RWops* FileOps::makeArchiveEntryRWops(archive* arch, archive_entry* entry) noexcept {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_IOStreamInterface iface = {
		.version = sizeof(iface),
		.size = sdlArchiveEntrySize,
		.seek = sdlArchiveEntrySeek,
		.read = sdlArchiveEntryRead,
		.write = sdlArchiveEntryWrite,
		.flush = sdlFlush,
		.close = sdlArchiveEntryClose
	};
	auto data = new pair(arch, entry);
	if (SDL_IOStream* ops = SDL_OpenIO(&iface, data))
		return ops;
	delete data;
#else
	SDL_RWops* ops = SDL_AllocRW();
	if (ops) {
		ops->size = sdlArchiveEntrySize;
		ops->seek = sdlArchiveEntrySeek;
		ops->read = sdlArchiveEntryRead;
		ops->write = sdlArchiveEntryWrite;
		ops->close = sdlArchiveEntryClose;
		ops->hidden.unknown.data1 = arch;
		ops->hidden.unknown.data2 = entry;
		return ops;
	}
#endif
	return nullptr;
}

#if SDL_VERSION_ATLEAST(3, 2, 0)
Sint64 SDLCALL FileOps::sdlArchiveEntrySize(void* userdata) noexcept {
	return archive_entry_size(static_cast<pair<archive*, archive_entry*>*>(userdata)->second);
}

Sint64 SDLCALL FileOps::sdlArchiveEntrySeek(void*, Sint64, SDL_IOWhence) noexcept {
	return -1;
}

size_t SDLCALL FileOps::sdlArchiveEntryRead(void* userdata, void* ptr, size_t size, SDL_IOStatus* status) noexcept {
	la_ssize_t len = archive_read_data(static_cast<pair<archive*, archive_entry*>*>(userdata)->first, ptr, size);
	if (len <= 0) {
		*status = !len ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_ERROR;
		return 0;
	}
	return len;
}

size_t SDLCALL FileOps::sdlArchiveEntryWrite(void*, const void*, size_t, SDL_IOStatus* status) noexcept {
	*status = SDL_IO_STATUS_READONLY;
	return 0;
}

bool SDLCALL FileOps::sdlArchiveEntryClose(void* userdata) noexcept {
	delete static_cast<pair<archive*, archive_entry*>*>(userdata);
	return true;
}
#else
Sint64 SDLCALL FileOps::sdlArchiveEntrySize(SDL_RWops* context) noexcept {
	return archive_entry_size(static_cast<archive_entry*>(context->hidden.unknown.data2));
}

Sint64 SDLCALL FileOps::sdlArchiveEntrySeek(SDL_RWops*, Sint64, int) noexcept {
	return -1;
}

size_t SDLCALL FileOps::sdlArchiveEntryRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) noexcept {
	return std::max(archive_read_data(static_cast<archive*>(context->hidden.unknown.data1), ptr, size * maxnum), la_ssize_t(0));
}

size_t SDLCALL FileOps::sdlArchiveEntryWrite(SDL_RWops*, const void*, size_t, size_t) noexcept {
	return 0;
}

int SDLCALL FileOps::sdlArchiveEntryClose(SDL_RWops* context) noexcept {
	SDL_FreeRW(context);
	return 0;
}
#endif
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
bool SDLCALL FileOps::sdlFlush(void*, SDL_IOStatus*) noexcept {
	return true;
}
#endif

#ifndef NDEBUG
SDL_Surface* FileOps::loadPictureSurface(uptr<SDL_RWops>&& ops) {
	if (ops) {
		uint16 w = 0, h = 0;
		SDL_PixelFormatEnum fmt = SDL_PIXELFORMAT_UNKNOWN;
		SDL_RWread(ops.get(), &w, sizeof(uint16), 1);
		SDL_RWread(ops.get(), &h, sizeof(uint16), 1);
		SDL_RWread(ops.get(), &fmt, sizeof(SDL_PixelFormatEnum), 1);
		if (w && h && fmt != SDL_PIXELFORMAT_UNKNOWN)
			if (uptr<SDL_Surface> img(SDL_CreateSurface(w, h, fmt)); img) {
				if (SDL_ISPIXELFORMAT_INDEXED(fmt)) {
					uint16_t cnt;
					SDL_RWread(ops.get(), &cnt, sizeof(uint16), 1);
					SDL_Palette* plt = SDL_AllocPalette(cnt);
					if (!plt)
						return nullptr;
					SDL_RWread(ops.get(), plt->colors, sizeof(SDL_Color), cnt);
					SDL_SetSurfacePalette(img.get(), plt);
					SDL_FreePalette(plt);
				}
				if (int width = img->w * SDL_BYTESPERPIXEL(fmt); img->pitch == width)
					SDL_RWread(ops.get(), img->pixels, 1, size_t(img->pitch) * size_t(img->h));
				else {
					auto px = static_cast<uint8*>(img->pixels);
					for (int r = 0; r < img->h; ++r, px += img->pitch)
						SDL_RWread(ops.get(), px, 1, width);
				}
				return img.release();
			}
	}
	return nullptr;
}
#endif

// FILE OPS LOCAL

FileOpsLocal::FileOpsLocal() noexcept :
#ifdef _WIN32
	overlapped{ .hEvent = CreateEventW(nullptr, false, 0, nullptr) }
#else
	ino(inotify_init1(IN_NONBLOCK))
#endif
{}

FileOpsLocal::~FileOpsLocal() {
#ifdef _WIN32
	CloseHandle(dirc);
	CloseHandle(overlapped.hEvent);
#else
	close(ino);
#endif
}

BrowserResultList FileOpsLocal::listDirectory(const std::stop_token& stoken, const string& path, BrowserListOption opts) {
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
#ifdef _WIN32
	if (!path.empty()) {
		WIN32_FIND_DATAW data;
		WinFindPtr hFind(FindFirstFileW((sstow(path) / L"*").data(), &data));
		if (hFind.get() == INVALID_HANDLE_VALUE)
			throw std::runtime_error(winErrorMessage(GetLastError()));
		do {
			if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (dirs && notDotName(data.cFileName))
						rl.dirs.emplace_back(data.cFileName);
				} else if (files)
					rl.files.emplace_back(data.cFileName);
			}
		} while (!csr.stopReq(stoken) && FindNextFileW(hFind.get(), &data));
		rng::sort(rl.files, Strcomp());
		rng::sort(rl.dirs, Strcomp());
	} else if (dirs) {
		DWORD drives = GetLogicalDrives();	// if in "root" directory, get drive letters and present them as directories
		if (!drives)
			throw std::runtime_error(winErrorMessage(GetLastError()));
		for (char i = 0; i < drivesMax; ++i)
			if (drives & (1 << i))
				rl.dirs.push_back(Cstring{ char('A' + i), ':', '\\' });
	}
#else
	sthandle<DIR*> directory = opendir(path.data());
	if (!directory)
		throw std::runtime_error(strerror(errno));
	struct stat ps;
	for (dirent* entry; !csr.stopReq(stoken) && (entry = readdir(directory));)
		if (hidden || entry->d_name[0] != '.')
			switch (entry->d_type) {
			case DT_DIR:
				if (dirs && notDotName(entry->d_name))
					rl.dirs.emplace_back(entry->d_name);
				break;
			case DT_REG:
				if (files)
					rl.files.emplace_back(entry->d_name);
				break;
			case DT_LNK:
				if (!stat((path / entry->d_name).data(), &ps))
					switch (ps.st_mode & S_IFMT) {
					case S_IFDIR:
						if (dirs)
							rl.dirs.emplace_back(entry->d_name);
						break;
					case S_IFREG:
						if (files)
							rl.files.emplace_back(entry->d_name);
					}
			}
	rng::sort(rl.files, Strcomp());
	rng::sort(rl.dirs, Strcomp());
#endif
	return rl;
}

void FileOpsLocal::deleteEntryThread(std::stop_token stoken, uptr<string> path) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
#ifdef _WIN32
		WIN32_FIND_DATAW data;
		wstring cpd = sstow(*path);
		if (DWORD attr = GetFileAttributesW(cpd.data()); attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
			if (!DeleteFileW(cpd.data()))
				rc = ResultCode::error;
		} else if (HANDLE hnd = FindFirstFileW((cpd / L"*").data(), &data); hnd != INVALID_HANDLE_VALUE) {
			CountedStopReq csr(dirStopCheckInterval);
			std::stack<WinFindPtr> dirs;
			dirs.push(WinFindPtr(hnd));
			do {
				do {
					if (csr.stopReq(stoken)) {
						pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::stop)));
						return;
					}

					if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (notDotName(data.cFileName)) {
							cpd = cpd / data.cFileName;
							if (hnd = FindFirstFileW((cpd / L"*").data(), &data); hnd != INVALID_HANDLE_VALUE)
								dirs.push(WinFindPtr(hnd));
							else
								cpd = parentPath(cpd);
						}
					} else
						DeleteFileW((cpd / data.cFileName).data());
				} while (FindNextFileW(dirs.top().get(), &data));
				do {
					dirs.pop();
					if (!RemoveDirectoryW(cpd.data()))
						rc = ResultCode::error;
					cpd = parentPath(cpd);
				} while (!dirs.empty() && !FindNextFileW(dirs.top().get(), &data));
			} while (!dirs.empty());
		} else
			rc = ResultCode::error;
#else
		if (struct stat ps; lstat(path->data(), &ps) || !S_ISDIR(ps.st_mode)) {
			if (unlink(path->data()))
				rc = ResultCode::error;
		} else if (DIR* dir = opendir(path->data())) {
			CountedStopReq csr(dirStopCheckInterval);
			std::stack<sthandle<DIR*>> dirs;
			dirs.push(dir);
			do {
				while (dirent* entry = readdir(dirs.top())) {
					if (csr.stopReq(stoken)) {
						pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::stop)));
						return;
					}

					if (entry->d_type == DT_DIR) {
						if (notDotName(entry->d_name)) {
							*path = *path / entry->d_name;
							if (dir = opendir(path->data()); dir)
								dirs.push(dir);
							else
								*path = parentPath(*path);
						}
					} else
						unlink((*path / entry->d_name).data());
				}
				dirs.pop();
				if (rmdir(path->data()))
					rc = ResultCode::error;
				*path = parentPath(*path);
			} while (!dirs.empty());
		} else
			rc = ResultCode::error;
#endif
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)));
}

bool FileOpsLocal::renameEntry(const string& oldPath, const string& newPath) noexcept {
#ifdef _WIN32
	return MoveFileW(sstow(oldPath).data(), sstow(newPath).data());
#else
	return !rename(oldPath.data(), newPath.data());
#endif
}

Data FileOpsLocal::readFile(const string& path) noexcept {
	return FileSys::readBinaryFile(path);
}

FileOps::FileType FileOpsLocal::fileType(const string& path) noexcept {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(sstow(path).data());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return FileType::none;
	return attr & FILE_ATTRIBUTE_DIRECTORY ? FileType::directory : FileType::regular;
#else
	struct stat ps;
	return !stat(path.data(), &ps) ? modeToType(ps.st_mode) : FileType::none;
#endif
}

bool FileOpsLocal::isRegular(const string& path) noexcept {
	return FileSys::isRegular(path);
}

bool FileOpsLocal::isDirectory(const string& path) noexcept {
	return FileSys::isDirectory(path);
}

#ifdef WITH_ARCHIVE
archive* FileOpsLocal::openArchive(ArchiveData& ad, bool force) {
	return initArchive(ad, force, [](archive* a, ArchiveData& d) noexcept -> int {
#ifdef _WIN32
		return archive_read_open_filename_w(a, sstow(d.name.data()).data(), archiveReadBlockSize);
#else
		return archive_read_open_filename(a, d.name.data(), archiveReadBlockSize);
#endif
	});
}
#endif

SDL_RWops* FileOpsLocal::makeRWops(const string& path) noexcept {
	return SDL_RWFromFile(path.data(), "rb");
}

void FileOpsLocal::setWatch(const string& path) noexcept {
#ifdef _WIN32
	if (overlapped.hEvent) {
		if (dirc != INVALID_HANDLE_VALUE)
			unsetWatch();

		if (wstring wp = sstow(path); FileSys::isDirectory(wp.data())) {
			dirc = CreateFileW(wp.data(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
			filter.clear();
			wpdir = std::move(wp);
		} else if (wp = sstow(parentPath(path)); FileSys::isDirectory(wp.data())) {
			dirc = CreateFileW(wp.data(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE;
			filter = filename(wp);
			wpdir.clear();
		}
		if (dirc != INVALID_HANDLE_VALUE && !ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr))
			unsetWatch();
	}
#else
	if (ino != -1) {
		if (watch != -1)
			unsetWatch();

		if (struct stat ps; !stat(path.data(), &ps))
			switch (ps.st_mode & S_IFMT) {
			case S_IFDIR:
				watch = inotify_add_watch(ino, path.data(), IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
				wpdir = path;
				break;
			case S_IFREG:
				watch = inotify_add_watch(ino, path.data(), IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF);
				wpdir.clear();
			}
	}
#endif
}

void FileOpsLocal::unsetWatch() noexcept {
#ifdef _WIN32
	CloseHandle(dirc);
	dirc = INVALID_HANDLE_VALUE;
#else
	inotify_rm_watch(ino, watch);
	watch = -1;
#endif
}

bool FileOpsLocal::pollWatch(vector<FileChange>& files) noexcept {
	try {
#ifdef _WIN32
		if (dirc != INVALID_HANDLE_VALUE)
			while (WaitForSingleObject(overlapped.hEvent, 0) == WAIT_OBJECT_0) {
				if (DWORD bytes; !GetOverlappedResult(dirc, &overlapped, &bytes, false)) {
					unsetWatch();
					return true;
				}

				for (auto event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<void*>(ebuf));; event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<void*>(reinterpret_cast<uint8*>(event) + event->NextEntryOffset))) {
					if (wstring_view name(event->FileName, event->FileNameLength / sizeof(wchar_t)); rng::none_of(name, [](wchar_t ch) -> bool { return isDsep(ch); })) {
						switch (event->Action) {
						case FILE_ACTION_ADDED: case FILE_ACTION_RENAMED_NEW_NAME:
							if (filter.empty())
								if (DWORD attr = GetFileAttributesW((wpdir / name).data()); attr != INVALID_FILE_ATTRIBUTES)
									files.emplace_back(name, attr & FILE_ATTRIBUTE_DIRECTORY ? FileChange::addDirectory : FileChange::addFile);
							break;
						case FILE_ACTION_REMOVED: case FILE_ACTION_RENAMED_OLD_NAME:
							if (filter.empty())
								files.emplace_back(name, FileChange::deleteEntry);
							else if (name == filter) {
								unsetWatch();
								return true;
							}
							break;
						case FILE_ACTION_MODIFIED:
							if (name == filter) {
								unsetWatch();
								return true;
							}
						}
					}
					if (!event->NextEntryOffset)
						break;
				}
				if (!ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr)) {
					unsetWatch();
					return true;
				}
			}
#else
		if (watch != -1) {
			struct stat ps;
			ssize_t len;
			for (inotify_event* event; (len = read(ino, ebuf, esiz) > 0);)
				for (ssize_t i = 0; i < len; i += sizeof(inotify_event) + event->len) {
					event = static_cast<inotify_event*>(static_cast<void*>(ebuf + i));
					if (event->mask & (IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF)) {
						unsetWatch();
						return true;
					}
					if (int bias = bool(event->mask & IN_CREATE) + bool(event->mask & IN_MOVED_TO) - bool(event->mask & IN_DELETE) - bool(event->mask & IN_MOVED_FROM)) {
						if (string_view name(event->name, event->len); bias < 0)
							files.emplace_back(name, FileChange::deleteEntry);
						else if (!stat((wpdir / name).data(), &ps))
							files.emplace_back(name, S_ISDIR(ps.st_mode) ? FileChange::addDirectory : FileChange::addFile);
					}
				}
			if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
				unsetWatch();
				return true;
			}
		}
#endif
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return false;
}

bool FileOpsLocal::canWatch() const noexcept {
	return true;
}

string FileOpsLocal::prefix() const {
#ifdef _WIN32
	return string();
#else
	return "/";
#endif
}

bool FileOpsLocal::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::none;
}

// FILE OPS REMOTE

#if defined(WITH_FTP) || defined(CAN_SFTP) || defined(CAN_SMB)
#ifdef WITH_ARCHIVE
archive* FileOpsRemote::openArchive(ArchiveData& ad, bool force) {
	return initArchive(ad, force, [this](archive* a, ArchiveData& d) noexcept -> int {
		if (d.dref)
			return archive_read_open_memory(a, d.dref->data(), d.dref->size());
		d.data = readFile(d.name.data());
		return archive_read_open_memory(a, d.data.data(), d.data.size());
	});
}
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0) && (defined(CAN_SMB) || defined(CAN_SFTP))
size_t FileOpsRemote::sdlReadFinish(ssize_t len, SDL_IOStatus* status) noexcept {
	if (len <= 0) {
		*status = !len ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_ERROR;
		return 0;
	}
	return len;
}

size_t FileOpsRemote::sdlWriteFinish(ssize_t len, SDL_IOStatus* status) noexcept {
	if (len < 0) {
		*status = SDL_IO_STATUS_ERROR;
		return 0;
	}
	return len;
}
#endif

int FileOpsRemote::translateFamily(RemoteLocation::Family family) noexcept {
	static constexpr int families[RemoteLocation::familyNames.size()] = { AF_UNSPEC, AF_INET, AF_INET6 };
	return families[eint(family)];
}
#endif
