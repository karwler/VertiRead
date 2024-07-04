#include "fileOps.h"
#include "utils/compare.h"
#ifdef CAN_SECRET
#include "engine/optional/glib.h"
#include "engine/optional/secret.h"
#endif
#ifdef CAN_SMB
#include "engine/optional/smbclient.h"
#endif
#ifdef CAN_SFTP
#include <netdb.h>
#include <netinet/tcp.h>
#include "engine/optional/ssh2.h"
#endif
#include <format>
#include <queue>
#include <semaphore>
#ifdef WITH_ARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif
#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
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
		throw std::runtime_error(err);
	}
	if (attributes = gHashTableNew(gStrHash, [](gconstpointer a, gconstpointer b) -> gboolean { return !strcmp(static_cast<const char*>(a), static_cast<const char*>(b)); }); !attributes) {
		gObjectUnref(service);
		throw std::runtime_error("Failed to allocate attribute table");
	}
}

CredentialManager::~CredentialManager() {
	gHashTableUnref(attributes);
	gObjectUnref(service);
}

vector<string> CredentialManager::loadPasswords(const RemoteLocation& rl) {
	string portTmp;
	setAttributes(rl, portTmp);
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
	string portTmp;
	setAttributes(rl, portTmp);
	SecretValue* svalue = secretValueNew(rl.password.data(), (rl.password.length() + 1) * sizeof(char), "string");	// TODO: what should this last argument be?
	GError* error = nullptr;
	secretServiceStoreSync(service, &schema, attributes, SECRET_COLLECTION_DEFAULT, std::format("VertiRead credentials for {}://{}{}", protocolNames[eint(rl.protocol)], rl.server, rl.path).data(), svalue, nullptr, &error);
	if (error) {
		logError(error);
		gErrorFree(error);
	}
}

void CredentialManager::setAttributes(const RemoteLocation& rl, string& portTmp) {
	portTmp = toStr(rl.port);
	gHashTableInsert(attributes, const_cast<char*>(keyProtocol), const_cast<char*>(protocolNames[eint(rl.protocol)]));
	gHashTableInsert(attributes, const_cast<char*>(keyServer), const_cast<char*>(rl.server.data()));
	gHashTableInsert(attributes, const_cast<char*>(keyPath), const_cast<char*>(rl.path.data()));
	gHashTableInsert(attributes, const_cast<char*>(keyUser), const_cast<char*>(rl.user.data()));
	gHashTableInsert(attributes, const_cast<char*>(keyPort), portTmp.data());
	if (rl.protocol == Protocol::smb) {
		gHashTableInsert(attributes, const_cast<char*>(keyWorkgroup), const_cast<char*>(rl.workgroup.data()));
		gHashTableRemove(attributes, keyFamily);
	} else {
		gHashTableRemove(attributes, keyWorkgroup);
		gHashTableInsert(attributes, const_cast<char*>(keyFamily), const_cast<char*>(familyNames[eint(rl.family)]));
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
#if defined(CAN_SMB) || defined(CAN_SFTP)
	switch (rl.protocol) {
	using enum Protocol;
#ifdef CAN_SMB
	case smb:
		if (symSmbclient())
			return new FileOpsSmb(rl, std::move(passwords));
#endif
#ifdef CAN_SFTP
	case sftp:
		if (symLibssh2())
			return new FileOpsSftp(rl, passwords);
#endif
	}
#endif
	return new FileOpsLocal;	// this line should never be hit, because Browser handles local and remote instantiation separately
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
	static constexpr int (SDLCALL* const magics[])(SDL_RWops*) = {
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
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
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
	return ops;
}

Sint64 SDLCALL FileOps::sdlArchiveEntrySize(SDL_RWops* context) {
	return archive_entry_size(static_cast<archive_entry*>(context->hidden.unknown.data2));
}

Sint64 SDLCALL FileOps::sdlArchiveEntrySeek(SDL_RWops*, Sint64, int) {
	return -1;
}

size_t SDLCALL FileOps::sdlArchiveEntryRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) {
	return std::max(archive_read_data(static_cast<archive*>(context->hidden.unknown.data1), ptr, size * maxnum), la_ssize_t(0));
}

size_t SDLCALL FileOps::sdlArchiveEntryWrite(SDL_RWops*, const void*, size_t, size_t) {
	return 0;
}

int SDLCALL FileOps::sdlArchiveEntryClose(SDL_RWops* context) {
	SDL_FreeRW(context);
	return 0;
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

FileOpsLocal::FileOpsLocal() :
#ifdef _WIN32
	ebuf(static_cast<byte_t*>(HeapAlloc(GetProcessHeap(), 0, esiz)))
#else
	ebuf(static_cast<byte_t*>(mmap(nullptr, esiz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)))
#endif
{
#ifdef _WIN32
	overlapped.hEvent = ebuf ? CreateEventW(nullptr, false, 0, nullptr) : nullptr;
#else
	ino = ebuf != MAP_FAILED ? inotify_init1(IN_NONBLOCK) : -1;
#endif
}

FileOpsLocal::~FileOpsLocal() {
#ifdef _WIN32
	CloseHandle(dirc);
	CloseHandle(overlapped.hEvent);
	HeapFree(GetProcessHeap(), 0, ebuf);
#else
	close(ino);
	munmap(ebuf, esiz);
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

bool FileOpsLocal::equals(const RemoteLocation& rl) const {
	return rl.protocol == Protocol::none;
}

// FILE OPS REMOTE

#if defined(CAN_SMB) || defined(CAN_SFTP)
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

string FileOpsRemote::prefix() const {
	return std::format("{}://{}", protocolNames[eint(Protocol::smb)], server);
}
#endif

// FILE OPS SMB

#ifdef CAN_SMB
FileOpsSmb::FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords) :
	FileOpsRemote(rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep)))	// server contains share
{
	if (ctx = smbcNewContext(); !ctx)
		throw std::runtime_error("Failed to create SMB context");
	try {
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
	smbcFreeContext(ctx, 0);
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
		sclose(ctx, fh);
	}
	return nullptr;
}

Sint64 SDLCALL FileOpsSmb::sdlSize(SDL_RWops* context) {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	struct stat ps;
	return !self->sfstat(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), &ps) ? ps.st_size : -1;
}

Sint64 SDLCALL FileOpsSmb::sdlSeek(SDL_RWops* context, Sint64 offset, int whence) {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return self->slseek(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), offset, whence);
}

size_t SDLCALL FileOpsSmb::sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return std::max(self->sread(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), ptr, size * maxnum), ssize_t(0));
}

size_t SDLCALL FileOpsSmb::sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	return std::max(self->swrite(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2), ptr, size * num), ssize_t(0));
}

int SDLCALL FileOpsSmb::sdlClose(SDL_RWops* context) {
	auto self = static_cast<FileOpsSmb*>(context->hidden.unknown.data1);
	std::lock_guard lockg(self->mlock);
	int rc = self->sclose(self->ctx, static_cast<SMBCFILE*>(context->hidden.unknown.data2));
	SDL_FreeRW(context);
	return rc;
}

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

bool FileOpsSmb::equals(const RemoteLocation& rl) const {
	return rl.protocol == Protocol::smb && (rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep))) == server && rl.port == smbcGetPort(ctx) && rl.user == smbcGetUser(ctx) && rl.workgroup == smbcGetWorkgroup(ctx);
}

void FileOpsSmb::logMsg(void*, int level, const char* msg) {
	logError("SMB level ", level, ": ", msg);
}
#endif

// FILE OPS SFTP

#ifdef CAN_SFTP
FileOpsSftp::FileOpsSftp(const RemoteLocation& rl, const vector<string>& passwords) :
	FileOpsRemote(valcp(rl.server)),
	user(rl.user),
	port(rl.port)
{
	if (sshInit(0))
		throw std::runtime_error("Failed to initialize SSH");
	try {
		static constexpr int families[familyNames.size()] = { AF_UNSPEC, AF_INET, AF_INET6 };
		addrinfo hints = {
			.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV,
			.ai_family = families[eint(rl.family)],
			.ai_socktype = SOCK_STREAM,
			.ai_protocol = IPPROTO_TCP
		};
		addrinfo* addr;
		if (int rc = getaddrinfo(rl.server.data(), toStr(port).data(), &hints, &addr))
			throw std::runtime_error(std::format("Failed to resolve address: {}", gai_strerror(rc)));

		for (addrinfo* it = addr; it; it = it->ai_next) {
			if (sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol); sock == -1)
				continue;
			if (!connect(sock, it->ai_addr, it->ai_addrlen))
				break;
			close(sock);
			sock = -1;
		}
		freeaddrinfo(addr);
		if (sock == -1)
			throw std::runtime_error("Failed to connect");

		if (session = sshSessionInitEx(nullptr, nullptr, nullptr, nullptr); !session)
			throw std::runtime_error("Failed to initialize SSH session");
		sshSessionSetBlocking(session, 1);
		if (sshSessionHandshake(session, sock))
			throw std::runtime_error("Failed to establish an SSH connection");

		authenticate(passwords);
		if (sftp = sftpInit(session); !sftp)
			throw std::runtime_error("failed to initialize SFTP session");
	} catch (...) {
		cleanup();
		throw;
	}
}

FileOpsSftp::~FileOpsSftp() {
	cleanup();
}

void FileOpsSftp::authenticate(const vector<string>& passwords) {
	if (char* userauthlist = sshUserauthList(session, user.data(), user.length()))
		for (char* next, *pos = userauthlist; *pos; pos = next) {
			for (; *pos == ','; ++pos);
			for (next = pos; *next && *next != ','; ++next);
			if (string_view method(pos, next); method == "password") {
				for (const string& pwd : passwords)
					if (!sshUserauthPasswordEx(session, user.data(), user.length(), pwd.data(), pwd.length(), nullptr))
						return;
				throw std::runtime_error("Authentication failed");
			}
		}
	throw std::runtime_error("No supported authentication methods found");
}

void FileOpsSftp::cleanup() noexcept {
	if (sftp)
		sftpShutdown(sftp);
	if (session) {
		sshSessionDisconnectEx(session, SSH_DISCONNECT_BY_APPLICATION, "shutdown", "");
		sshSessionFree(session);
	}
	close(sock);
	sshExit();
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
		sftpClose(fh);
	}
	return nullptr;
}

Sint64 SDLCALL FileOpsSftp::sdlSize(SDL_RWops* context) {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpFstatEx(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), &attr, 0) && (attr.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attr.filesize : -1;
}

Sint64 SDLCALL FileOpsSftp::sdlSeek(SDL_RWops* context, Sint64 offset, int whence) {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	auto fh = static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2);
	uint64_t pos;
	switch (whence) {
	case RW_SEEK_SET:
		pos = offset;
		break;
	case RW_SEEK_CUR:
		pos = sftpTell64(fh) + whence;
		break;
	case RW_SEEK_END: {
		LIBSSH2_SFTP_ATTRIBUTES attr;
		if (sftpFstatEx(fh, &attr, 0) || !(attr.flags & LIBSSH2_SFTP_ATTR_SIZE))
			return -1;
		pos = attr.filesize + whence;
		break; }
	default:
		return -1;
	}
	sftpSeek64(fh, pos);
	return pos;
}

size_t SDLCALL FileOpsSftp::sdlRead(SDL_RWops* context, void* ptr, size_t size, size_t maxnum) {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	return std::max(sftpRead(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), static_cast<char*>(ptr), size * maxnum), ssize_t(0));
}

size_t SDLCALL FileOpsSftp::sdlWrite(SDL_RWops* context, const void* ptr, size_t size, size_t num) {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	return std::max(sftpWrite(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), static_cast<const char*>(ptr), size * num), ssize_t(0));
}

int SDLCALL FileOpsSftp::sdlClose(SDL_RWops* context) {
	std::lock_guard lockg(static_cast<FileOpsSftp*>(context->hidden.unknown.data1)->mlock);
	int rc = sftpClose(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2));
	SDL_FreeRW(context);
	return rc;
}

bool FileOpsSftp::pollWatch(vector<FileChange>&) {
	return false;
}

bool FileOpsSftp::canWatch() const noexcept {
	return false;
}

bool FileOpsSftp::equals(const RemoteLocation& rl) const {
	return rl.protocol == Protocol::sftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
