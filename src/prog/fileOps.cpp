#include "fileOps.h"
#include "engine/optional/glib.h"
#include "engine/optional/secret.h"
#include "engine/optional/smbclient.h"
#include "engine/optional/ssh2.h"
#include "utils/compare.h"
#ifdef WITH_ARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <format>
#include <queue>
#include <semaphore>

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
	secretServiceStoreSync(service, &schema, attributes, SECRET_COLLECTION_DEFAULT, std::format("VertiRead credentials for {}://{}@{}", protocolNames[eint(rl.protocol)], rl.user, rl.server).data(), svalue, nullptr, &error);
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
fs::file_type FileOps:: modeToType(mode_t mode) noexcept {
	switch (mode & S_IFMT) {
	case S_IFIFO:
		return fs::file_type::fifo;
	case S_IFCHR:
		return fs::file_type::character;
	case S_IFDIR:
		return fs::file_type::directory;
	case S_IFBLK:
		return fs::file_type::block;
	case S_IFREG:
		return fs::file_type::regular;
	case S_IFLNK:
		return fs::file_type::symlink;
	case S_IFSOCK:
		return fs::file_type::socket;
	}
	return fs::file_type::unknown;
}
#endif

bool FileOps::isPicture(const string& path) {
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
	if (SDL_RWops* ifh = makeRWops(path)) {
#if SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
		for (bool (SDLCALL* const test)(SDL_RWops*) : magics)
#else
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
#endif
			if (test(ifh)) {
				SDL_RWclose(ifh);
				return true;
			}

		if (strciequal(fileExtension(path), "TGA"))
			if (SDL_Surface* img = IMG_LoadTGA_RW(ifh)) {
				SDL_FreeSurface(img);
				SDL_RWclose(ifh);
				return true;
			}
		SDL_RWclose(ifh);
	}
	return false;
}

bool FileOps::isArchive(ArchiveData& ad) {
#ifdef WITH_ARCHIVE
	if (archive* arch = openArchive(ad)) {
		archive_read_free(arch);
		return true;
	}
#endif
	return false;
}

#ifdef WITH_ARCHIVE
void FileOps::makeArchiveTreeThread(std::stop_token stoken, uptr<MakeArchiveTreeData> md) {
	archive* arch = openArchive(md->ra->arch, &md->ra->error);
	if (!arch) {
		md->ra->rc = ResultCode::error;
		pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, md->ra.release());
		return;
	}

	int rc;
	for (archive_entry* entry; (rc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK;) {
		if (stoken.stop_requested()) {
			archive_read_free(arch);
			md->ra->rc = ResultCode::stop;
			pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, md->ra.release());
			return;
		}

		ArchiveDir* node = &md->ra->arch;
		for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
			if (const char* next = strchr(path, '/')) {
				string_view sname(path, next);
				std::forward_list<ArchiveDir>::iterator dit = rng::find_if(node->dirs, [sname](const ArchiveDir& it) -> bool { return it.name == sname; });
				node = dit != node->dirs.end() ? std::to_address(dit) : &node->dirs.emplace_front(sname);
				path = next + 1;
			} else {
				if (Data data = readArchiveEntry(arch, entry); SDL_Surface* img = IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE)) {
					node->files.emplace_front(path, std::min(uint(img->w), md->maxRes) * std::min(uint(img->h), md->maxRes) * 4);
					SDL_FreeSurface(img);
				}
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
				else if (data.size() >= PdfFile::signatureLen && !memcmp(PdfFile::signature, data.data(), PdfFile::signatureLen))
					node->files.emplace_front(path);
#endif
				else
					node->files.emplace_front(path, 0);
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
	else {
		md->ra->error = archive_error_string(arch);
		md->ra->rc = ResultCode::error;
	}
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, md->ra.release());
}

SDL_Surface* FileOps::loadArchivePicture(archive* arch, archive_entry* entry) {
	Data data = readArchiveEntry(arch, entry);
	return IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE);
}

Data FileOps::readArchiveEntry(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0)
		return Data();

	Data data(bsiz);
	if (la_ssize_t len = archive_read_data(arch, data.data(), bsiz); len < bsiz) {
		if (len <= 0)
			return Data();
		data.resize(len);
	}
	return data;
}

template <InvocableR<int, archive*, ArchiveData&> F>
archive* FileOps::initArchive(ArchiveData& ad, Cstring* error, F openArch) {
	archive* arch = archive_read_new();
	if (arch) {
		archive_read_support_filter_all(arch);
		archive_read_support_format_all(arch);
		if (ad.pc == ArchiveData::PassCode::set || ad.pc == ArchiveData::PassCode::attempt)
			archive_read_add_passphrase(arch, ad.passphrase.data());
		if (ad.pc <= ArchiveData::PassCode::set)
			archive_read_set_passphrase_callback(arch, &ad, requestArchivePassphrase);
		if (openArch(arch, ad) != ARCHIVE_OK) {
			if (error)
				*error = archive_error_string(arch);
			archive_read_free(arch);
			return nullptr;
		}
	} else if (error)
		*error = "Failed to allocate archive object";
	return arch;
}

const char* FileOps::requestArchivePassphrase(archive* arch, void* data) {
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
#if SDL_VERSION_ATLEAST(3, 0, 0)
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
	SDL_IOStream* ops = SDL_OpenIO(&iface, data);
	if (!ops)
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
	}
#endif
	return ops;
}

#if SDL_VERSION_ATLEAST(3, 0, 0)
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

#if SDL_VERSION_ATLEAST(3, 0, 0)
bool SDLCALL FileOps::sdlFlush(void*, SDL_IOStatus*) noexcept {
	return true;
}
#endif

template <Pointer T, class F>
bool FileOps::checkStopDeleteProcess(CountedStopReq& csr, std::stop_token stoken, std::stack<T>& dirs, F dclose) {
	bool done = csr.stopReq(stoken);
	if (done) {
		while (!dirs.empty()) {
			dclose(dirs.top());
			dirs.pop();
		}
		pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::stop)));
	}
	return done;
}

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

BrowserResultList FileOpsLocal::listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) {
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
#ifdef _WIN32
	if (!path.empty()) {
		WIN32_FIND_DATAW data;
		if (HANDLE hFind = FindFirstFileW((sstow(path) / L"*").data(), &data); hFind != INVALID_HANDLE_VALUE) {
			do {
				if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
					if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (dirs && notDotName(data.cFileName))
							rl.dirs.emplace_back(data.cFileName);
					} else if (files)
						rl.files.emplace_back(data.cFileName);
				}
			} while (!csr.stopReq(stoken) && FindNextFileW(hFind, &data));
			FindClose(hFind);
			rng::sort(rl.files, Strcomp());
			rng::sort(rl.dirs, Strcomp());
		} else
			rl.error = winErrorMessage(GetLastError());
	} else if (dirs) {
		if (DWORD drives = GetLogicalDrives()) {	// if in "root" directory, get drive letters and present them as directories
			for (char i = 0; i < drivesMax; ++i)
				if (drives & (1 << i))
					rl.dirs.push_back(Cstring{ char('A' + i), ':', '\\' });
		} else
			rl.error = winErrorMessage(GetLastError());
	}
#else
	if (DIR* directory = opendir(path.data())) {
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
		closedir(directory);
		rng::sort(rl.files, Strcomp());
		rng::sort(rl.dirs, Strcomp());
	} else
		rl.error = strerror(errno);
#endif
	return rl;
}

void FileOpsLocal::deleteEntryThread(std::stop_token stoken, uptr<string> path) {
	ResultCode rc = ResultCode::ok;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	fs::path cpd = toPath(*path);
	if (DWORD attr = GetFileAttributesW(cpd.c_str()); attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (!DeleteFileW(cpd.c_str()))
			rc = ResultCode::error;
	} else if (HANDLE hnd = FindFirstFileW((cpd / L"*").c_str(), &data); hnd != INVALID_HANDLE_VALUE) {
		CountedStopReq csr(dirStopCheckInterval);
		std::stack<HANDLE> dirs;
		dirs.push(hnd);
		do {
			do {
				if (checkStopDeleteProcess(csr, stoken, dirs, FindClose))
					return;

				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (notDotName(data.cFileName)) {
						cpd /= data.cFileName;
						if (hnd = FindFirstFileW((cpd / L"*").c_str(), &data); hnd != INVALID_HANDLE_VALUE)
							dirs.push(hnd);
						else
							cpd = cpd.parent_path();
					}
				} else
					DeleteFileW((cpd / data.cFileName).c_str());
			} while (FindNextFileW(dirs.top(), &data));
			do {
				FindClose(dirs.top());
				if (!RemoveDirectoryW(cpd.c_str()))
					rc = ResultCode::error;
				cpd = cpd.parent_path();
				dirs.pop();
			} while (!dirs.empty() && !FindNextFileW(dirs.top(), &data));
		} while (!dirs.empty());
	} else
		rc = ResultCode::error;
#else
	if (struct stat ps; lstat(path->data(), &ps) || !S_ISDIR(ps.st_mode)) {
		if (unlink(path->data()))
			rc = ResultCode::error;
	} else if (DIR* dir = opendir(path->data())) {
		CountedStopReq csr(dirStopCheckInterval);
		std::stack<DIR*> dirs;
		dirs.push(dir);
		do {
			while (dirent* entry = readdir(dirs.top())) {
				if (checkStopDeleteProcess(csr, stoken, dirs, closedir))
					return;

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
			closedir(dirs.top());
			if (rmdir(path->data()))
				rc = ResultCode::error;
			*path = parentPath(*path);
			dirs.pop();
		} while (!dirs.empty());
	} else
		rc = ResultCode::error;
#endif
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)));
}

bool FileOpsLocal::renameEntry(const string& oldPath, const string& newPath) {
#ifdef _WIN32
	return MoveFileW(sstow(oldPath).data(), sstow(newPath).data());
#else
	return !rename(oldPath.data(), newPath.data());
#endif
}

Data FileOpsLocal::readFile(const string& path) {
#ifdef _WIN32
	return readFile(sstow(path).data());
#else
	return readFile(path.data());
#endif
}

Data FileOpsLocal::readFile(const fs::path::value_type* path) {
	Data data;
#ifdef _WIN32
	if (HANDLE fh = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr); fh != INVALID_HANDLE_VALUE) {
		if (LARGE_INTEGER siz; GetFileSizeEx(fh, &siz)) {
			data.resize(siz.QuadPart);
			if (DWORD len; ReadFile(fh, data.data(), data.size(), &len, nullptr)) {
				if (len < data.size())
					data.resize(len);
			} else
				data.clear();
		}
		CloseHandle(fh);
	}
#else
	if (int fd = open(path, O_RDONLY); fd != -1) {
		if (struct stat ps; !fstat(fd, &ps)) {
			data.resize(ps.st_size);
			if (ssize_t len = read(fd, data.data(), data.size()); len < ps.st_size)
				data.resize(std::max(len, ssize_t(0)));
		}
		close(fd);
	}
#endif
	return data;
}

fs::file_type FileOpsLocal::fileType(const string& path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(sstow(path).data());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return fs::file_type::not_found;
	return attr & FILE_ATTRIBUTE_DIRECTORY ? fs::file_type::directory : fs::file_type::regular;
#else
	struct stat ps;
	return !stat(path.data(), &ps) ? modeToType(ps.st_mode) : fs::file_type::not_found;
#endif
}

bool FileOpsLocal::isRegular(const string& path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(sstow(path).data());
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
	return hasModeFlags(path.data(), S_IFREG);
#endif
}

bool FileOpsLocal::isDirectory(const string& path) {
#ifdef _WIN32
	return isDirectory(sstow(path).data());
#else
	return hasModeFlags(path.data(), S_IFDIR);
#endif
}

#ifdef _WIN32
bool FileOpsLocal::isDirectory(const wchar_t* path) noexcept {
	DWORD attr = GetFileAttributesW(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

#else

bool FileOpsLocal::hasModeFlags(const char* path, mode_t flags) noexcept {
	struct stat ps;
	return !stat(path, &ps) && (ps.st_mode & flags);
}
#endif

#ifdef WITH_ARCHIVE
archive* FileOpsLocal::openArchive(ArchiveData& ad, Cstring* error) {
	return initArchive(ad, error, [](archive* a, ArchiveData& d) -> int {
#ifdef _WIN32
		return archive_read_open_filename_w(a, sstow(d.name.data()).data(), archiveReadBlockSize);
#else
		return archive_read_open_filename(a, d.name.data(), archiveReadBlockSize);
#endif
	});
}
#endif

SDL_RWops* FileOpsLocal::makeRWops(const string& path) {
	return SDL_RWFromFile(path.data(), "rb");
}

void FileOpsLocal::setWatch(const string& path) {
#ifdef _WIN32
	if (overlapped.hEvent) {
		if (dirc != INVALID_HANDLE_VALUE)
			unsetWatch();

		if (wstring wp = sstow(path); isDirectory(wp.data())) {
			dirc = CreateFileW(wp.data(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
			filter.clear();
			wpdir = std::move(wp);
		} else if (wp = sstow(parentPath(path)); isDirectory(wp.data())) {
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

void FileOpsLocal::unsetWatch() {
#ifdef _WIN32
	CloseHandle(dirc);
	dirc = INVALID_HANDLE_VALUE;
#else
	inotify_rm_watch(ino, watch);
	watch = -1;
#endif
}

bool FileOpsLocal::pollWatch(vector<FileChange>& files) {
#ifdef _WIN32
	if (dirc != INVALID_HANDLE_VALUE)
		while (WaitForSingleObject(overlapped.hEvent, 0) == WAIT_OBJECT_0) {
			if (DWORD bytes; !GetOverlappedResult(dirc, &overlapped, &bytes, false)) {
				unsetWatch();
				return true;
			}

			for (auto event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<void*>(ebuf));; event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<void*>(reinterpret_cast<byte_t*>(event) + event->NextEntryOffset))) {
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
		for (inotify_event* event; (len = read(ino, ebuf, esiz) > 0);)	// TODO: should I use poll first?
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
archive* FileOpsRemote::openArchive(ArchiveData& ad, Cstring* error) {
	return initArchive(ad, error, [this](archive* a, ArchiveData& d) -> int {
		if (d.dref)
			return archive_read_open_memory(a, d.dref->data(), d.dref->size());
		d.data = readFile(d.name.data());
		return archive_read_open_memory(a, d.data.data(), d.data.size());
	});
}
#endif

#if SDL_VERSION_ATLEAST(3, 0, 0) && (defined(CAN_SMB) || defined(CAN_SFTP))
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

int FileOpsRemote::translateFamily(RemoteLocation::Family family) {
	static constexpr int families[RemoteLocation::familyNames.size()] = { AF_UNSPEC, AF_INET, AF_INET6 };
	return families[eint(family)];
}
#endif

// FILE OPS SMB

#ifdef CAN_SMB
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
		throw std::runtime_error(std::format("Failed to open share '{}'", spath));
	} catch (...) {
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

BrowserResultList FileOpsSmb::listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	if (SMBCFILE* dir = sopendir(ctx, path.data())) {
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
		sclosedir(ctx, dir);
		rng::sort(rl.files, Strcomp());
		rng::sort(rl.dirs, Strcomp());
	} else
		rl.error = strerror(errno);
	return rl;
}

void FileOpsSmb::deleteEntryThread(std::stop_token stoken, uptr<string> path) {
	std::lock_guard lockg(mlock);
	ResultCode rc = ResultCode::ok;
	if (struct stat ps; sstat(ctx, path->data(), &ps) || !S_ISDIR(ps.st_mode)) {
		if (sunlink(ctx, path->data()))
			rc = ResultCode::error;
	} else if (SMBCFILE* dir = sopendir(ctx, path->data())) {
		CountedStopReq csr(dirStopCheckInterval);
		std::stack<SMBCFILE*> dirs;
		dirs.push(dir);
		do {
			while (smbc_dirent* it = sreaddir(ctx, dirs.top())) {
				if (checkStopDeleteProcess(csr, stoken, dirs, [this](SMBCFILE* dh) { sclosedir(ctx, dh); }))
					return;

				if (it->smbc_type == SMBC_DIR) {
					if (notDotName(it->name)) {
						*path = *path / string_view(it->name, it->namelen);
						if (dir = sopendir(ctx, path->data()); dir)
							dirs.push(dir);
						else
							*path = parentPath(*path);
					}
				} else
					sunlink(ctx, (*path / string_view(it->name, it->namelen)).data());
			}
			sclosedir(ctx, dirs.top());
			if (srmdir(ctx, path->data()))
				rc = ResultCode::error;
			*path = parentPath(*path);
			dirs.pop();
		} while (!dirs.empty());
	} else
		rc = ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)));
}

bool FileOpsSmb::renameEntry(const string& oldPath, const string& newPath) {
	std::lock_guard lockg(mlock);
	return !srename(ctx, oldPath.data(), ctx, newPath.data());
}

Data FileOpsSmb::readFile(const string& path) {
	std::lock_guard lockg(mlock);
	Data data;
	if (SMBCFILE* fh = sopen(ctx, path.data(), O_RDONLY, 0)) {
		if (struct stat ps; !sfstat(ctx, fh, &ps)) {
			data.resize(ps.st_size);
			if (ssize_t len = sread(ctx, fh, data.data(), data.size()); len < ssize_t(data.size()))
				data.resize(std::max(len, ssize_t(0)));
		}
		sclose(ctx, fh);
	}
	return data;
}

fs::file_type FileOpsSmb::fileType(const string& path) {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, path.data(), &ps) ? modeToType(ps.st_mode) : fs::file_type::not_found;
}

bool FileOpsSmb::isRegular(const string& path) {
	return hasModeFlags(path.data(), S_IFREG);
}

bool FileOpsSmb::isDirectory(const string& path) {
	return hasModeFlags(path.data(), S_IFDIR);
}

bool FileOpsSmb::hasModeFlags(const char* path, mode_t mdes) {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, path, &ps) && (ps.st_mode & mdes);
}

SDL_RWops* FileOpsSmb::makeRWops(const string& path) {
	std::lock_guard lockg(mlock);
	if (SMBCFILE* fh = sopen(ctx, path.data(), O_RDONLY, 0)) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
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

#if SDL_VERSION_ATLEAST(3, 0, 0)
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

void FileOpsSmb::setWatch(const string& path) {
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

void FileOpsSmb::unsetWatch() {
	sclosedir(ctx, wndir);
	wndir = nullptr;
}

bool FileOpsSmb::pollWatch(vector<FileChange>& files) {
	if (wndir) {
		bool closeWatch = false;
		tuple<FileOpsSmb*, vector<FileChange>*, bool*> tfc(this, &files, &closeWatch);
		std::lock_guard lockg(mlock);
		int rc = snotify(ctx, wndir, 0, flags, notifyTimeout, [](const smbc_notify_callback_action* actions, size_t numActions, void* data) -> int {
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
	return std::format("{}://{}", protocolNames[eint(Protocol::smb)], serverShare);
}

bool FileOpsSmb::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::smb && (rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep))) == serverShare && rl.port == smbcGetPort(ctx) && rl.user == smbcGetUser(ctx) && rl.workgroup == smbcGetWorkgroup(ctx);
}

void FileOpsSmb::logMsg(void*, int level, const char* msg) noexcept {
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SMB level %d: %s", level, msg);
}
#endif

// FILE OPS SFTP

#ifdef CAN_SFTP
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
	} catch (...) {
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

BrowserResultList FileOpsSftp::listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	if (LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, path.data(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR)) {
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
		sftpClose(dir);
		rng::sort(rl.files, Strcomp());
		rng::sort(rl.dirs, Strcomp());
	} else
		rl.error = lastError();
	return rl;
}

void FileOpsSftp::deleteEntryThread(std::stop_token stoken, uptr<string> path) {
	std::lock_guard lockg(mlock);
	ResultCode rc = ResultCode::ok;
	LIBSSH2_SFTP_ATTRIBUTES attr;
	if (sftpStatEx(sftp, path->data(), path->length(), LIBSSH2_SFTP_LSTAT, &attr) || !(attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) || !LIBSSH2_SFTP_S_ISDIR(attr.permissions)) {
		if (sftpUnlinkEx(sftp, path->data(), path->length()))
			rc = ResultCode::error;
	} else if (LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, path->data(), path->length(), 0, 0, LIBSSH2_SFTP_OPENDIR)) {
		CountedStopReq csr(dirStopCheckInterval);
		std::stack<LIBSSH2_SFTP_HANDLE*> dirs;
		dirs.push(dir);
		char nbuf[2048];
		do {
			for (int nlen; (nlen = sftpReaddirEx(dirs.top(), nbuf, sizeof(nbuf), nullptr, 0, &attr)) > 0;) {
				if (checkStopDeleteProcess(csr, stoken, dirs, sftpClose))
					return;

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
			sftpClose(dirs.top());
			if (sftpRmdirEx(sftp, path->data(), path->length()))
				rc = ResultCode::error;
			*path = parentPath(*path);
			dirs.pop();
		} while (!dirs.empty());
	} else
		rc = ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)));
}

bool FileOpsSftp::renameEntry(const string& oldPath, const string& newPath) {
	std::lock_guard lockg(mlock);
	return !sftpRenameEx(sftp, oldPath.data(), oldPath.length(), newPath.data(), newPath.length(), LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);
}

Data FileOpsSftp::readFile(const string& path) {
	std::lock_guard lockg(mlock);
	Data data;
	if (LIBSSH2_SFTP_HANDLE* fh = sftpOpenEx(sftp, path.data(), path.length(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE)) {
		if (LIBSSH2_SFTP_ATTRIBUTES attr; !sftpFstatEx(fh, &attr, 0) && (attr.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
			data.resize(attr.filesize);
			if (ssize_t len = sftpRead(fh, reinterpret_cast<char*>(data.data()), data.size()); len < ssize_t(data.size()))
				data.resize(std::max(len, ssize_t(0)));
		}
		sftpClose(fh);
	}
	return data;
}

fs::file_type FileOpsSftp::fileType(const string& path) {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ? modeToType(attr.permissions) : fs::file_type::not_found;
}

bool FileOpsSftp::isRegular(const string& path) {
	return hasAttributeFlags(path, LIBSSH2_SFTP_S_IFREG);
}

bool FileOpsSftp::isDirectory(const string& path) {
	return hasAttributeFlags(path, LIBSSH2_SFTP_S_IFDIR);
}

bool FileOpsSftp::hasAttributeFlags(string_view path, ulong flags) {
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

SDL_RWops* FileOpsSftp::makeRWops(const string& path) {
	std::lock_guard lockg(mlock);
	if (LIBSSH2_SFTP_HANDLE* fh = sftpOpenEx(sftp, path.data(), path.length(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE)) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
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
			ops->close = sdlClose;;
			ops->hidden.unknown.data1 = this;
			ops->hidden.unknown.data2 = fh;
			return ops;
		}
#endif
		sftpClose(fh);
	}
	return nullptr;
}

#if SDL_VERSION_ATLEAST(3, 0, 0)
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

bool FileOpsSftp::pollWatch(vector<FileChange>&) {
	return false;
}

bool FileOpsSftp::canWatch() const noexcept {
	return false;
}

string FileOpsSftp::prefix() const {
	return std::format("{}://{}", protocolNames[eint(Protocol::sftp)], server);
}

bool FileOpsSftp::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::sftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif

// FILE OPS FTP

#ifdef WITH_FTP
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
		for (vector<string>::const_iterator it = passwords.begin(); !done && it != passwords.end(); ++it) {
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
	} catch (...) {
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

BrowserResultList FileOpsFtp::listDirectory(std::stop_token stoken, const string& path, BrowserListOption opts) {
	std::lock_guard lockg(mlock);
	auto [files, dirs, hidden] = unpackListOptions(opts);
	CountedStopReq csr(dirStopCheckInterval);
	BrowserResultList rl;
	try {
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

				string_view::iterator pos = line.begin() + 1;
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

		for (vector<string>::iterator it = links.begin(); !csr.stopReq(stoken) && it != links.end(); ++it)
			switch (statFile(recvPi, (*it)[0] == '/' ? *it : joinPaths(string_view(path), string_view(*it)))) {
			case fs::file_type::regular:
				if (files)
					rl.files.emplace_back(*it);
				break;
			case fs::file_type::directory:
				if (dirs)
					rl.dirs.emplace_back(*it);
			}

		rng::sort(rl.files, Strcomp());
		rng::sort(rl.dirs, Strcomp());
	} catch (const std::runtime_error& err) {
		rl.error = err.what();
	}
	return rl;
}

void FileOpsFtp::deleteEntryThread(std::stop_token, uptr<string>) {
	pushEvent(SDL_USEREVENT_THREAD_DELETE_FINISHED, 0, std::bit_cast<void*>(uintptr_t(ResultCode::error)));
}

bool FileOpsFtp::renameEntry(const string& oldPath, const string& newPath) {
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

Data FileOpsFtp::readFile(const string& path) {
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
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return Data();
}

fs::file_type FileOpsFtp::fileType(const string& path) {
	std::lock_guard lockg(mlock);
	FtpReceiver recvPi;
	return statFile(recvPi, path);
}

bool FileOpsFtp::isRegular(const string& path) {
	return fileType(path) == fs::file_type::regular;
}

bool FileOpsFtp::isDirectory(const string& path) {
	return fileType(path) == fs::file_type::directory;
}

fs::file_type FileOpsFtp::statFile(FtpReceiver& recvPi, string_view path) {
	fs::file_type ret = fs::file_type::unknown;
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
						ret = fs::file_type::regular;
					else if (strciequal(tn, "dir"))
						ret = fs::file_type::directory;
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
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return fs::file_type::not_found;
	}
	return ret;
}

string_view FileOpsFtp::getMlstType(string_view line) {
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

	string_view::reverse_iterator fend = std::find_if(path.rbegin(), path.rend(), notDsep);
	string_view::reverse_iterator fpos = std::find_if(fend, path.rend(), isDsep);
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
		string_view::iterator ci = cwd.begin(), di = dst.begin();
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
	return std::format("{}: {}{} {}", msg, reply.cmd, !reply.cont ? "" : "-", reply.args);
}

SDL_RWops* FileOpsFtp::makeRWops(const string& path) {
#if SDL_VERSION_ATLEAST(3, 0, 0)
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

#if SDL_VERSION_ATLEAST(3, 0, 0)
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

bool FileOpsFtp::pollWatch(vector<FileChange>&) {
	return false;
}

bool FileOpsFtp::canWatch() const noexcept {
	return false;
}

string FileOpsFtp::prefix() const {
	return std::format("{}://{}", protocolNames[eint(Protocol::ftp)], server);
}

bool FileOpsFtp::equals(const RemoteLocation& rl) const noexcept {
	return rl.protocol == Protocol::ftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
