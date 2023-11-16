#include "fileOps.h"
#include "utils/compare.h"
#ifdef CAN_SECRET
#include "engine/optional/secret.h"
using namespace LibSecret;
#endif
#ifdef CAN_SMB
#include "engine/optional/smbclient.h"
using namespace LibSmbclient;
#endif
#ifdef CAN_SFTP
#include <netdb.h>
#include <netinet/tcp.h>
#include "engine/optional/ssh2.h"
using namespace LibSsh2;
#endif
#include <stack>
#include <archive.h>
#include <archive_entry.h>
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <sys/inotify.h>
#include <sys/mman.h>
#include <unistd.h>
#include <SDL2/SDL_image.h>
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
		std::runtime_error err(error->message);
		gErrorFree(error);
		throw err;
	}
	attributes = gHashTableNew(gStrHash, [](gconstpointer a, gconstpointer b) -> gboolean { return !strcmp(static_cast<const char*>(a), static_cast<const char*>(b)); });
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
	SecretValue* svalue = secretValueNew(rl.password.c_str(), (rl.password.length() + 1) * sizeof(char), "string");	// TODO: what should this last argument be?
	GError* error = nullptr;
	secretServiceStoreSync(service, &schema, attributes, SECRET_COLLECTION_DEFAULT, std::format("VertiRead credentials for {}://{}{}", protocolNames[eint(rl.protocol)], rl.server, rl.path).c_str(), svalue, nullptr, &error);
	if (error) {
		logError(error);
		gErrorFree(error);
	}
}

void CredentialManager::setAttributes(const RemoteLocation& rl, string& portTmp) {
	gHashTableInsert(attributes, const_cast<char*>(keyProtocol), const_cast<char*>(protocolNames[eint(rl.protocol)]));
	gHashTableInsert(attributes, const_cast<char*>(keyServer), const_cast<char*>(rl.server.c_str()));
	gHashTableInsert(attributes, const_cast<char*>(keyPath), const_cast<char*>(rl.path.c_str()));
	gHashTableInsert(attributes, const_cast<char*>(keyUser), const_cast<char*>(rl.user.c_str()));
	if (rl.protocol == Protocol::smb)
		gHashTableInsert(attributes, const_cast<char*>(keyWorkgroup), const_cast<char*>(rl.workgroup.c_str()));
	else
		gHashTableRemove(attributes, keyWorkgroup);
	portTmp = toStr(rl.port);
	gHashTableInsert(attributes, const_cast<char*>(keyPort), const_cast<char*>(portTmp.c_str()));
	if (rl.protocol == Protocol::sftp)
		gHashTableInsert(attributes, const_cast<char*>(keyFamily), const_cast<char*>(familyNames[eint(rl.family)]));
	else
		gHashTableRemove(attributes, keyFamily);
}
#endif

// FILE OPS

FileOps::~FileOps() {}

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
			return new FileOpsSftp(rl, std::move(passwords));
#endif
	}
#endif
	return new FileOpsLocal;	// this line should never be hit, because Browser handles local and remote instantiation separately
}

#if !defined(_WIN32) || defined(CAN_SMB) || defined(CAN_SFTP)
fs::file_type FileOps:: modeToType(mode_t mode) {
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

bool FileOps::isPicture(SDL_RWops* ifh, string_view ext) {
	static constexpr int (SDLCALL* const magics[])(SDL_RWops*) = {
		IMG_isJPG,
		IMG_isPNG,
		IMG_isBMP,
		IMG_isGIF,
		IMG_isTIF,
		IMG_isWEBP,
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
	if (ifh) {
		for (int (SDLCALL* const test)(SDL_RWops*) : magics)
			if (test(ifh)) {
				SDL_RWclose(ifh);
				return true;
			}

		if (strciequal(ext, "TGA"))
			if (SDL_Surface* img = IMG_LoadTGA_RW(ifh)) {
				SDL_FreeSurface(img);
				SDL_RWclose(ifh);
				return true;
			}
		SDL_RWclose(ifh);
	}
	return false;
}

bool FileOps::isPicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 && isPicture(SDL_RWFromConstMem(buffer.get(), bsiz), fileExtension(archive_entry_pathname_utf8(entry)));
}

bool FileOps::isArchive(string_view path) {
	if (archive* arch = openArchive(path)) {
		archive_read_free(arch);
		return true;
	}
	return false;
}

bool FileOps::isPictureArchive(string_view path) {
	if (archive* arch = openArchive(path)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (isPicture(arch, entry)) {
				archive_read_free(arch);
				return true;
			}
		archive_read_free(arch);
	}
	return false;
}

bool FileOps::isArchivePicture(string_view path, string_view pname) {
	if (archive* arch = openArchive(path)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (isPicture(arch, entry)) {
					archive_read_free(arch);
					return true;
				}
		archive_read_free(arch);
	}
	return false;
}

pair<uptr<byte_t[]>, int64> FileOps::readArchiveEntry(archive* arch, archive_entry* entry) {
	int64 bsiz = archive_entry_size(entry);
	if (bsiz <= 0 || archive_entry_filetype(entry) != AE_IFREG)
		return pair(nullptr, 0);

	uptr<byte_t[]> buffer = std::make_unique_for_overwrite<byte_t[]>(bsiz);
	bsiz = archive_read_data(arch, buffer.get(), bsiz);
	return pair(std::move(buffer), bsiz);
}

vector<string> FileOps::listArchiveFiles(string_view path) {
	vector<string> entries;
	if (archive* arch = openArchive(path)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_filetype(entry) == AE_IFREG)
				entries.emplace_back(archive_entry_pathname_utf8(entry));

		archive_read_free(arch);
		rng::sort(entries, StrNatCmp());
	}
	return entries;
}

void FileOps::makeArchiveTreeThread(std::stop_token stoken, BrowserResultAsync&& ra, uintptr_t maxRes) {
	archive* arch = openArchive(ra.curDir);
	if (!arch) {
		pushEvent(SDL_USEREVENT_THREAD_ARCHIVE, ThreadEvent::finished);
		return;
	}

	for (archive_entry* entry; !archive_read_next_header(arch, &entry);) {
		if (stoken.stop_requested()) {
			archive_read_free(arch);
			return;
		}
		pushEvent(SDL_USEREVENT_THREAD_ARCHIVE, ThreadEvent::progress, std::bit_cast<void*>(archive_entry_ino(entry)));

		ArchiveDir* node = &ra.arch;
		for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
			if (const char* next = strchr(path, '/')) {
				vector<ArchiveDir>::iterator dit = rng::find_if(node->dirs, [path, next](const ArchiveDir& it) -> bool { return std::equal(it.name.begin(), it.name.end(), path, next); });
				node = dit != node->dirs.end() ? std::to_address(dit) : &node->dirs.emplace_back(node, string(path, next));
				path = next + 1;
			} else {
				SDL_Surface* img = loadArchivePicture(arch, entry);
				node->files.emplace_back(path, img ? std::min(uintptr_t(img->w), maxRes) * std::min(uintptr_t(img->h), maxRes) * 4 : 0);
				SDL_FreeSurface(img);
				break;
			}
		}
	}
	archive_read_free(arch);
	ra.arch.sort();
	pushEvent(SDL_USEREVENT_THREAD_ARCHIVE, ThreadEvent::finished, new BrowserResultAsync(std::move(ra)));
}

SDL_Surface* FileOps::loadArchivePicture(string_view path, string_view pname) {
	SDL_Surface* img = nullptr;
	if (archive* arch = openArchive(path)) {
		for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
			if (archive_entry_pathname_utf8(entry) == pname)
				if (img = loadArchivePicture(arch, entry); img)
					break;
		archive_read_free(arch);
	}
	return img;
}

SDL_Surface* FileOps::loadArchivePicture(archive* arch, archive_entry* entry) {
	auto [buffer, bsiz] = readArchiveEntry(arch, entry);
	return bsiz > 0 ? IMG_Load_RW(SDL_RWFromMem(buffer.get(), bsiz), SDL_TRUE) : nullptr;
}

// FILE OPS LOCAL

FileOpsLocal::FileOpsLocal() :
#ifdef _WIN32
	ebuf(static_cast<byte_t*>(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, esiz)))
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

vector<string> FileOpsLocal::listDirectory(string_view path, bool files, bool dirs, bool hidden) {
#ifdef _WIN32
	if (path.empty())	// if in "root" directory, get drive letters and present them as directories
		return dirs ? FileOpsLocal::listDrives() : vector<string>();
#endif
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(path) / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (dirs && wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
						entries.push_back(swtos(data.cFileName));
				} else if (files)
					entries.push_back(swtos(data.cFileName));
			}
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
#else
	if (string drc(path); DIR* directory = opendir(drc.c_str())) {
		struct stat ps;
		while (dirent* entry = readdir(directory))
			if (hidden || entry->d_name[0] != '.')
				switch (entry->d_type) {
				case DT_DIR:
					if (dirs && strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
						entries.emplace_back(entry->d_name);
					break;
				case DT_REG:
					if (files)
						entries.emplace_back(entry->d_name);
					break;
				case DT_LNK:
					if (!stat((drc / entry->d_name).c_str(), &ps))
						switch (ps.st_mode & S_IFMT) {
						case S_IFDIR:
							if (dirs)
								entries.emplace_back(entry->d_name);
							break;
						case S_IFREG:
							if (files)
								entries.emplace_back(entry->d_name);
						}
				}
		closedir(directory);
#endif
		rng::sort(entries, StrNatCmp());
	}
	return entries;
}

pair<vector<string>, vector<string>> FileOpsLocal::listDirectorySep(string_view path, bool hidden) {
#ifdef _WIN32
	if (path.empty())	// if in "root" directory, get drive letters and present them as directories
		return pair(vector<string>(), listDrives());
#endif
	vector<string> files, dirs;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(path) / L"*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (notDotName(data.cFileName))
						dirs.push_back(swtos(data.cFileName));
				} else
					files.push_back(swtos(data.cFileName));
			}
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
#else
	if (string drc(path); DIR* directory = opendir(drc.c_str())) {
		struct stat ps;
		while (dirent* entry = readdir(directory))
			if (hidden || entry->d_name[0] != '.')
				switch (entry->d_type) {
				case DT_DIR:
					if (notDotName(entry->d_name))
						dirs.emplace_back(entry->d_name);
					break;
				case DT_REG:
					files.emplace_back(entry->d_name);
					break;
				case DT_LNK:
					if (!stat((drc / entry->d_name).c_str(), &ps))
						switch (ps.st_mode & S_IFMT) {
						case S_IFDIR:
							dirs.emplace_back(entry->d_name);
							break;
						case S_IFREG:
							files.emplace_back(entry->d_name);
						}
				}
		closedir(directory);
#endif
		rng::sort(files, StrNatCmp());
		rng::sort(dirs, StrNatCmp());
	}
	return pair(std::move(files), std::move(dirs));
}

#ifdef _WIN32
vector<string> FileOpsLocal::listDrives() {
	vector<string> letters;
	DWORD drives = GetLogicalDrives();
	for (char i = 0; i < drivesMax; ++i)
		if (drives & (1 << i))
			letters.push_back(string{ char('A' + i), ':', '\\' });
	return letters;
}
#endif

bool FileOpsLocal::deleteEntry(string_view base) {
	bool ok = true;
	fs::path path = toPath(base);
#ifdef _WIN32
	if (DWORD attr = GetFileAttributesW(path.c_str()); attr == INVALID_FILE_ATTRIBUTES)
		return false;
	else if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
		return DeleteFileW(path.c_str());
	WIN32_FIND_DATAW data;
	HANDLE hnd = FindFirstFileW((path / L"*").c_str(), &data);
	if (hnd == INVALID_HANDLE_VALUE)
		return false;

	std::stack<HANDLE> handles;
	handles.push(hnd);
	do {
		do {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (notDotName(data.cFileName)) {
					path /= data.cFileName;
					if (hnd = FindFirstFileW((path / L"*").c_str(), &data); hnd != INVALID_HANDLE_VALUE)
						handles.push(hnd);
					else
						path = path.parent_path();
				}
			} else
				DeleteFileW((path / data.cFileName).c_str());
		} while (FindNextFileW(handles.top(), &data));
		do {
			FindClose(handles.top());
			if (!RemoveDirectoryW(path.c_str()))
				ok = false;
			path = path.parent_path();
			handles.pop();
		} while (!handles.empty() && !FindNextFileW(handles.top(), &data));
	} while (!handles.empty());
#else
	if (struct stat ps; lstat(path.c_str(), &ps))
		return false;
	else if (!S_ISDIR(ps.st_mode))
		return !unlink(path.c_str());
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return false;

	std::stack<DIR*> dirs;
	dirs.push(dir);
	do {
		while (dirent* entry = readdir(dirs.top())) {
			if (entry->d_type == DT_DIR) {
				if (notDotName(entry->d_name)) {
					path /= entry->d_name;
					if (dir = opendir(path.c_str()); dir)
						dirs.push(dir);
					else
						path = path.parent_path();
				}
			} else
				unlink((path / entry->d_name).c_str());
		}
		closedir(dirs.top());
		if (rmdir(path.c_str()))
			ok = false;
		path = path.parent_path();
		dirs.pop();
	} while (!dirs.empty());
#endif
	return ok;
}

bool FileOpsLocal::renameEntry(string_view oldPath, string_view newPath) {
#ifdef _WIN32
	return MoveFileW(sstow(oldPath).c_str(), sstow(newPath).c_str());
#else
	return !rename(string(oldPath).c_str(), string(newPath).c_str());
#endif
}

vector<byte_t> FileOpsLocal::readFile(string_view path) {
	return readFile(toPath(path));
}

vector<byte_t> FileOpsLocal::readFile(const fs::path& path) {
	vector<byte_t> data;
#ifdef _WIN32
	if (HANDLE fh = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr); fh != INVALID_HANDLE_VALUE) {
		if (LARGE_INTEGER siz; GetFileSizeEx(fh, &siz)) {
			data.resize(siz.QuadPart);
			if (DWORD len; ReadFile(fh, data.data(), data.size(), &len, nullptr)) {
				if (len < data.size())
					data.resize(len);
			} else
				data.clear();
		}
		CloseHandle(fh);
	};
#else
	if (int fd = open(path.c_str(), O_RDONLY); fd != -1) {
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

fs::file_type FileOpsLocal::fileType(string_view path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(sstow(path).c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return fs::file_type::not_found;
	return attr & FILE_ATTRIBUTE_DIRECTORY ? fs::file_type::directory : fs::file_type::regular;
#else
	struct stat ps;
	return !stat(string(path).c_str(), &ps) ? modeToType(ps.st_mode) : fs::file_type::not_found;
#endif
}

bool FileOpsLocal::isDirectory(string_view path) {
	return isDirectory(toPath(path));
}

bool FileOpsLocal::isDirectory(const fs::path& path) {
#ifdef _WIN32
	DWORD attr = GetFileAttributesW(path.c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat ps;
	return !stat(path.c_str(), &ps) && S_ISDIR(ps.st_mode);
#endif
}

bool FileOpsLocal::isPicture(string_view path) {
	return FileOps::isPicture(SDL_RWFromFile(string(path).c_str(), "rb"), fileExtension(path));
}

SDL_Surface* FileOpsLocal::loadPicture(string_view path) {
	return IMG_Load(string(path).c_str());
}

archive* FileOpsLocal::openArchive(string_view path) {
	archive* arch = archive_read_new();
	if (arch) {
		archive_read_support_filter_all(arch);
		archive_read_support_format_all(arch);
#ifdef _WIN32
		if (archive_read_open_filename_w(arch, sstow(path).c_str(), archiveReadBlockSize)) {
#else
		if (archive_read_open_filename(arch, string(path).c_str(), archiveReadBlockSize)) {
#endif
			archive_read_free(arch);
			return nullptr;
		}
	}
	return arch;
}

void FileOpsLocal::setWatch(string_view path) {
#ifdef _WIN32
	if (overlapped.hEvent) {
		if (dirc != INVALID_HANDLE_VALUE)
			unsetWatch();

		if (fs::path wp = toPath(path); isDirectory(wp)) {
			dirc = CreateFileW(wp.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
			filter.clear();
			wpdir = std::move(wp);
		} else if (wp = toPath(parentPath(path)); isDirectory(wp)) {
			dirc = CreateFileW(wp.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE;
			filter = wp.filename();
			wpdir.clear();
		}
		if (dirc != INVALID_HANDLE_VALUE && !ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr))
			unsetWatch();
	}
#else
	if (ino != -1) {
		if (watch != -1)
			unsetWatch();

		fs::path wp = toPath(path);
		if (struct stat ps; !stat(wp.c_str(), &ps))
			switch (ps.st_mode & S_IFMT) {
			case S_IFDIR:
				watch = inotify_add_watch(ino, wp.c_str(), IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
				wpdir = std::move(wp);
				break;
			case S_IFREG:
				watch = inotify_add_watch(ino, wp.c_str(), IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF);
				wpdir.clear();
			}
	}
#endif
}

bool FileOpsLocal::pollWatch(vector<FileChange>& files) {
#ifdef _WIN32
	if (dirc == INVALID_HANDLE_VALUE)
		return false;

	while (WaitForSingleObject(overlapped.hEvent, 0) == WAIT_OBJECT_0) {
		if (DWORD bytes; !GetOverlappedResult(dirc, &overlapped, &bytes, false))
			return unsetWatch();

		for (auto event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ebuf);; event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<byte_t*>(event) + event->NextEntryOffset)) {
			switch (event->Action) {
			case FILE_ACTION_ADDED: case FILE_ACTION_RENAMED_NEW_NAME:
				if (filter.empty()) {
					wstring_view name(event->FileName, event->FileNameLength / sizeof(wchar_t));
					if (DWORD attr = GetFileAttributesW((wpdir / name).c_str()); attr != INVALID_FILE_ATTRIBUTES)
						files.emplace_back(swtos(name), attr & FILE_ATTRIBUTE_DIRECTORY ? FileChange::addDirectory : FileChange::addFile);
				}
				break;
			case FILE_ACTION_REMOVED: case FILE_ACTION_RENAMED_OLD_NAME:
				if (wstring_view file(event->FileName, event->FileNameLength / sizeof(wchar_t)); filter.empty())
					files.emplace_back(swtos(file), FileChange::deleteEntry);
				else if (file == filter.c_str())
					return unsetWatch();
				break;
			case FILE_ACTION_MODIFIED:
				if (!filter.empty())
					if (wstring_view file(event->FileName, event->FileNameLength / sizeof(wchar_t)); file == filter.c_str())
						return unsetWatch();
			}
			if (!event->NextEntryOffset)
				break;
		}
		if (!ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr))
			return unsetWatch();
	}
	return false;
#else
	if (watch == -1)
		return false;

	struct stat ps;
	ssize_t len;
	for (inotify_event* event; (len = read(ino, ebuf, esiz) > 0);)	// TODO: should I use poll first?
		for (ssize_t i = 0; i < len; i += sizeof(inotify_event) + event->len) {
			event = static_cast<inotify_event*>(static_cast<void*>(ebuf + i));
			if (event->mask & (IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF))
				return unsetWatch();
			if (int bias = bool(event->mask & IN_CREATE) + bool(event->mask & IN_MOVED_TO) - bool(event->mask & IN_DELETE) - bool(event->mask & IN_MOVED_FROM)) {
				if (string name(event->name, event->len); bias < 0)
					files.emplace_back(std::move(name), FileChange::deleteEntry);
				else if (!stat((wpdir / name).c_str(), &ps))
					files.emplace_back(std::move(name), S_ISDIR(ps.st_mode) ? FileChange::addDirectory : FileChange::addFile);
			}
		}
	return len < 0 && errno != EAGAIN && errno != EWOULDBLOCK && unsetWatch();
#endif
}

bool FileOpsLocal::unsetWatch() {
#ifdef _WIN32
	CloseHandle(dirc);
	dirc = INVALID_HANDLE_VALUE;
#else
	inotify_rm_watch(ino, watch);
	watch = -1;
#endif
	return true;
}

FileOpCapabilities FileOpsLocal::capabilities() const {
	return FileOpCapabilities::remove | FileOpCapabilities::rename | FileOpCapabilities::watch;
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
bool FileOpsRemote::isPicture(string_view path) {
	vector<byte_t> data = readFile(path);
	return !data.empty() && FileOps::isPicture(SDL_RWFromConstMem(data.data(), data.size()), fileExtension(path));
}

SDL_Surface* FileOpsRemote::loadPicture(string_view path) {
	vector<byte_t> data = readFile(path);
	return !data.empty() ? IMG_Load_RW(SDL_RWFromMem(data.data(), data.size()), SDL_TRUE) : nullptr;
}

archive* FileOpsRemote::openArchive(string_view path) {
	archive* arch = archive_read_new();
	if (arch) {
		archive_read_support_filter_all(arch);
		archive_read_support_format_all(arch);
		if (vector<byte_t> data = readFile(path); archive_read_open_memory(arch, data.data(), data.size())) {
			archive_read_free(arch);
			return nullptr;
		}
	}
	return arch;
}

string FileOpsRemote::prefix() const {
	return std::format("{}://{}", protocolNames[eint(Protocol::smb)], server);
}
#endif

// FILE OPS SMB

#ifdef CAN_SMB
FileOpsSmb::FileOpsSmb(const RemoteLocation& rl, vector<string>&& passwords) :
	FileOpsRemote(rl.server / string_view(rl.path.begin(), rng::find_if(rl.path, isDsep))),	// server contains share
	ctx(smbcNewContext())
{
	if (!ctx)
		throw std::runtime_error("Failed to create smb context");
	if (!smbcInitContext(ctx)) {
		smbcFreeContext(ctx, 0);
		throw std::runtime_error("Failed to init smb context");
	}
	smbcSetContext(ctx);
#ifndef NDEBUG
	if (smbcSetDebug)
		smbcSetDebug(ctx, 1);
#endif
	if (smbcSetLogCallback)
		smbcSetLogCallback(ctx, this, logMsg);

	sopen = smbcGetFunctionOpen(ctx);
	sread = smbcGetFunctionRead(ctx);
	sclose = smbcGetFunctionClose(ctx);
	sstat = smbcGetFunctionStat(ctx);
	sfstat = smbcGetFunctionFstat(ctx);
	sopendir = smbcGetFunctionOpendir(ctx);
	sreaddir = smbcGetFunctionReaddir(ctx);
	sclosedir = smbcGetFunctionClosedir(ctx);
	sunlink = smbcGetFunctionUnlink ? smbcGetFunctionUnlink(ctx) : nullptr;
	srmdir = smbcGetFunctionRmdir ? smbcGetFunctionRmdir(ctx) : nullptr;
	srename = smbcGetFunctionRename ? smbcGetFunctionRename(ctx) : nullptr;
	snotify = smbcGetFunctionNotify ? smbcGetFunctionNotify(ctx) : nullptr;

	smbcSetFunctionAuthDataWithContext(ctx, [](SMBCCTX* c, const char*, const char*, char*, int, char*, int, char* pw, int pwlen) {
		auto self = static_cast<FileOpsSmb*>(smbcGetOptionUserData(c));
		std::copy_n(self->pwd.c_str(), std::min(int(self->pwd.length()) + 1, pwlen), pw);
	});
	smbcSetUser(ctx, rl.user.c_str());
	smbcSetWorkgroup(ctx, rl.workgroup.c_str());
	smbcSetPort(ctx, rl.port);
	smbcSetOptionUserData(ctx, this);

	string spath = prefix();
	for (string& it : passwords)
		if (pwd = std::move(it); SMBCFILE* dir = sopendir(ctx, spath.c_str())) {
			sclosedir(ctx, dir);
			return;
		}
	smbcFreeContext(ctx, 0);
	throw std::runtime_error(std::format("Failed to open share '{}'", spath));
}

FileOpsSmb::~FileOpsSmb() {
	sclosedir(ctx, wndir);
	smbcFreeContext(ctx, 0);
}

vector<string> FileOpsSmb::listDirectory(string_view path, bool files, bool dirs, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<string> entries;
	if (string drc(path); SMBCFILE* dir = sopendir(ctx, drc.c_str())) {
		struct stat ps;
		while (smbc_dirent* it = sreaddir(ctx, dir)) {
			if (hidden || it->name[0] != '.') {
				string_view name(it->name, it->namelen);
				switch (it->smbc_type) {
				case SMBC_DIR:
					if (dirs && notDotName(it->name))	// TODO: can it even be a dot name and is it null terminated?
						entries.emplace_back(name);
					break;
				case SMBC_FILE:
					if (files)
						entries.emplace_back(name);
					break;
				case SMBC_LINK:
					if (!sstat(ctx, (drc / name).c_str(), &ps))
						switch (ps.st_mode & S_IFMT) {	// TODO: is this right?
						case S_IFDIR:
							if (dirs)
								entries.emplace_back(name);
							break;
						case S_IFREG:
							if (files)
								entries.emplace_back(name);
						}
				}
			}
		}
		sclosedir(ctx, dir);
		rng::sort(entries, StrNatCmp());
	}
	return entries;
}

pair<vector<string>, vector<string>> FileOpsSmb::listDirectorySep(string_view path, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<string> files, dirs;
	if (string drc(path); SMBCFILE* dir = sopendir(ctx, drc.c_str())) {
		struct stat ps;
		while (smbc_dirent* it = sreaddir(ctx, dir)) {
			if (hidden || it->name[0] != '.') {
				string_view name(it->name, it->namelen);
				switch (it->smbc_type) {
				case SMBC_DIR:
					if (notDotName(it->name))	// TODO: can name be dots and is it null terminated?
						dirs.emplace_back(name);
					break;
				case SMBC_FILE:
					files.emplace_back(name);
					break;
				case SMBC_LINK:
					if (!sstat(ctx, (drc / name).c_str(), &ps))
						switch (ps.st_mode & S_IFMT) {	// TODO: is this right?
						case S_IFDIR:
							dirs.emplace_back(name);
							break;
						case S_IFREG:
							files.emplace_back(name);
						}
				}
			}
		}
		sclosedir(ctx, dir);
		rng::sort(files, StrNatCmp());
		rng::sort(dirs, StrNatCmp());
	}
	return pair(std::move(files), std::move(dirs));
}

bool FileOpsSmb::deleteEntry(string_view base) {
	if (!(sunlink && srmdir))
		return false;
	std::lock_guard lockg(mlock);
	string path(base);
	if (struct stat ps; sstat(ctx, path.c_str(), &ps))
		return false;
	else if (!S_ISDIR(ps.st_mode))
		return !sunlink(ctx, path.c_str());
	SMBCFILE* dir = sopendir(ctx, path.c_str());
	if (!dir)
		return false;

	std::stack<SMBCFILE*> dirs;
	dirs.push(dir);
	bool ok = true;
	do {
		while (smbc_dirent* it = sreaddir(ctx, dirs.top())) {
			string_view name(it->name, it->namelen);
			if (it->smbc_type == SMBC_DIR) {
				if (notDotName(it->name)) {
					path = std::move(path) / name;
					if (dir = sopendir(ctx, path.c_str()); dir)
						dirs.push(dir);
					else
						path = parentPath(path);
				}
			} else
				sunlink(ctx, (path / name).c_str());
		}
		sclosedir(ctx, dirs.top());
		if (srmdir(ctx, path.c_str()))
			ok = false;
		path = parentPath(path);
		dirs.pop();
	} while (!dirs.empty());
	return ok;
}

bool FileOpsSmb::renameEntry(string_view oldPath, string_view newPath) {
	return srename && !srename(ctx, string(oldPath).c_str(), ctx, string(newPath).c_str());
}

vector<byte_t> FileOpsSmb::readFile(string_view path) {
	std::lock_guard lockg(mlock);
	vector<byte_t> data;
	if (SMBCFILE* fh = sopen(ctx, string(path).c_str(), O_RDONLY, 0)) {	// TODO: check if this works if there's no open handle to the directory/share
		if (struct stat ps; !sfstat(ctx, fh, &ps)) {
			data.resize(ps.st_size);
			if (ssize_t len = sread(ctx, fh, data.data(), data.size()); len < ssize_t(data.size()))
				data.resize(std::max(len, ssize_t(0)));
		}
		sclose(ctx, fh);
	}
	return data;
}

fs::file_type FileOpsSmb::fileType(string_view path) {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, string(path).c_str(), &ps) ? modeToType(ps.st_mode) : fs::file_type::not_found;
};

bool FileOpsSmb::isDirectory(string_view path) {
	std::lock_guard lockg(mlock);
	struct stat ps;
	return !sstat(ctx, string(path).c_str(), &ps) && S_ISDIR(ps.st_mode);	// TODO: is this right?
}

void FileOpsSmb::setWatch(string_view path) {
	if (snotify) {
		std::lock_guard lockg(mlock);
		if (wndir)
			unsetWatch();

		struct stat ps;
		if (string wp(path); !sstat(ctx, wp.c_str(), &ps) && S_ISDIR(ps.st_mode)) {
			wndir = sopendir(ctx, string(path).c_str());
			flags = SMBC_NOTIFY_CHANGE_FILE_NAME | SMBC_NOTIFY_CHANGE_DIR_NAME;
			filter.clear();
			wpdir = path;
		} else if (wp = parentPath(path); !sstat(ctx, wp.c_str(), &ps) && S_ISDIR(ps.st_mode)) {
			wndir = sopendir(ctx, string(wp).c_str());
			flags = SMBC_NOTIFY_CHANGE_FILE_NAME | SMBC_NOTIFY_CHANGE_SIZE;
			filter = filename(wp);
			wpdir.clear();
		}
	}
}

bool FileOpsSmb::pollWatch(vector<FileChange>& files) {
	if (!wndir)
		return false;

	bool closeWatch = false;
	tuple<FileOpsSmb*, vector<FileChange>*, bool*> tfc(this, &files, &closeWatch);
	std::lock_guard lockg(mlock);
	int rc = snotify(ctx, wndir, 0, flags, notifyTimeout, [](const smbc_notify_callback_action* actions, size_t numActions, void* data) -> int {
		auto [self, fp, cw] = *static_cast<tuple<FileOpsSmb*, vector<FileChange>*, bool*>*>(data);
		struct stat ps;
		for (size_t i = 0; i < numActions; ++i) {
			switch (actions[i].action) {
			case SMBC_NOTIFY_ACTION_ADDED: case SMBC_NOTIFY_ACTION_NEW_NAME:
				if (self->filter.empty() && !self->sstat(self->ctx, (self->wpdir / actions[i].filename).c_str(), &ps))
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
	return (rc || closeWatch) && unsetWatch();
}

bool FileOpsSmb::unsetWatch() {
	sclosedir(ctx, wndir);
	wndir = nullptr;
	return true;
}

FileOpCapabilities FileOpsSmb::capabilities() const {
	FileOpCapabilities caps = FileOpCapabilities::none;
	if (sunlink && srmdir)
		caps |= FileOpCapabilities::remove;
	if (snotify)
		caps |= FileOpCapabilities::watch;
	return caps;
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
		addrinfo hints = {
			.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV,
			.ai_family = array<int, familyNames.size()>{ AF_UNSPEC, AF_INET, AF_INET6 }[eint(rl.family)],
			.ai_socktype = SOCK_STREAM,
			.ai_protocol = IPPROTO_TCP
		};
		addrinfo* addr;
		if (getaddrinfo(rl.server.c_str(), toStr(port).c_str(), &hints, &addr) || !addr)
			throw std::runtime_error("Failed to resolve address");

		for (addrinfo* it = addr; it; it = it->ai_next) {
			if (sock = socket(hints.ai_family, SOCK_STREAM, IPPROTO_TCP); sock == -1)
				continue;
			if (connect(sock, addr->ai_addr, addr->ai_addrlen)) {
				close(sock);
				sock = -1;
			}
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
	} catch (const std::runtime_error&) {
		sshSessionDisconnectEx(session, SSH_DISCONNECT_BY_APPLICATION, "shutdown", "");
		sshSessionFree(session);
		close(sock);
		sshExit();
		throw;
	}
}

FileOpsSftp::~FileOpsSftp() {
	sftpShutdown(sftp);
	sshSessionDisconnectEx(session, SSH_DISCONNECT_BY_APPLICATION, "shutdown", "");
	sshSessionFree(session);
	close(sock);
	sshExit();
}

void FileOpsSftp::authenticate(const vector<string>& passwords) {
	if (!sshUserauthList)
		return;
	if (char* userauthlist = sshUserauthList(session, user.data(), user.length())) {
		bool canPassword = false, canKey = false;
		for (char* next, *pos = userauthlist; *pos; pos = next) {
			for (; *pos == ','; ++pos);
			for (next = pos; *next && *next != ','; ++next);
			if (string_view method(pos, next); method == "password")
				canPassword = sshUserauthPasswordEx;
			else if (method == "publickey")
				canKey = sshUserauthPublickeyFromfileEx;
		}
		if (!(canPassword || canKey))
			throw std::runtime_error("No supported authentication methods found");

		if (canPassword)
			for (const string& pwd : passwords)
				if (!sshUserauthPasswordEx(session, user.data(), user.length(), pwd.c_str(), pwd.length(), nullptr))
					return;
		if (canKey) {
			constexpr string_view hostsFile = "known_hosts";
			vector<fs::path> publicKeys, privateKeys;
			std::error_code ec;
			for (const fs::path& drc : { fs::path(getenv("HOME")) / ".ssh", fs::path("/etc/ssh") })
				for (const fs::directory_entry& it : fs::recursive_directory_iterator(drc, fs::directory_options::follow_directory_symlink | fs::directory_options::skip_permission_denied, ec))
					if (it.is_regular_file() && strncmp(it.path().filename().c_str(), hostsFile.data(), hostsFile.length()))
						(it.path().extension() != ".pub" ? privateKeys : publicKeys).push_back(it.path());

			for (const fs::path& pub : publicKeys)
				for (const fs::path& prv : privateKeys)
					for (const string& pwd : passwords)
						if (!sshUserauthPublickeyFromfileEx(session, user.data(), user.length(), pub.c_str(), prv.c_str(), pwd.c_str()))
							return;
		}
		throw std::runtime_error("Authentication failed");
	}
}

vector<string> FileOpsSftp::listDirectory(string_view path, bool files, bool dirs, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<string> entries;
	if (LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, path.data(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR)) {
		char name[2048], longentry[2048];	// TODO: no need for longentry
		LIBSSH2_SFTP_ATTRIBUTES attr;
		while (sftpReaddirEx(dir, name, sizeof(name), longentry, sizeof(longentry), &attr) > 0) {
			if (hidden || name[0] != '.')
				switch (attr.flags & S_IFMT) {
				case S_IFDIR:
					if (dirs && notDotName(name))	// TODO: can this happen?
						entries.emplace_back(name);
					break;
				case S_IFREG:
					if (files)
						entries.emplace_back(name);
					break;
				case S_IFLNK:
					if (string fpath = string(path) / name; !sftpStatEx(sftp, fpath.c_str(), fpath.length(), LIBSSH2_SFTP_STAT, &attr))
						switch (attr.flags & S_IFMT) {
						case S_IFDIR:
							if (dirs)
								entries.emplace_back(name);
							break;
						case S_IFREG:
							if (files)
								entries.emplace_back(name);
						}
				}
		}
		sftpClose(dir);
		rng::sort(entries, StrNatCmp());
	}
	return entries;
}

pair<vector<string>, vector<string>> FileOpsSftp::listDirectorySep(string_view path, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<string> files, dirs;
	if (LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, path.data(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR)) {
		char name[2048], longentry[2048];	// TODO: no need for longentry
		LIBSSH2_SFTP_ATTRIBUTES attr;
		while (sftpReaddirEx(dir, name, sizeof(name), longentry, sizeof(longentry), &attr) > 0) {
			if (hidden || name[0] != '.')
				switch (attr.flags & S_IFMT) {
				case S_IFDIR:
					if (notDotName(name))	// TODO: can this happen?
						dirs.emplace_back(name);
					break;
				case S_IFREG:
					files.emplace_back(name);
					break;
				case S_IFLNK:
					if (string fpath = string(path) / name; !sftpStatEx(sftp, fpath.c_str(), fpath.length(), LIBSSH2_SFTP_STAT, &attr))
						switch (attr.flags & S_IFMT) {
						case S_IFDIR:
							dirs.emplace_back(name);
							break;
						case S_IFREG:
							files.emplace_back(name);
						}
				}
		}
		sftpClose(dir);
		rng::sort(files, StrNatCmp());
		rng::sort(dirs, StrNatCmp());
	}
	return pair(std::move(files), std::move(dirs));
}

bool FileOpsSftp::deleteEntry(string_view base) {
	if (!(sftpUnlinkEx && sftpRmdirEx))
		return false;
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	if (sftpStatEx(sftp, base.data(), base.length(), LIBSSH2_SFTP_LSTAT, &attr))
		return false;
	else if (!S_ISDIR(attr.flags))
		return !sftpUnlinkEx(sftp, base.data(), base.length());
	LIBSSH2_SFTP_HANDLE* dir = sftpOpenEx(sftp, base.data(), base.length(), 0, 0, LIBSSH2_SFTP_OPENDIR);
	if (!dir)
		return false;

	std::stack<LIBSSH2_SFTP_HANDLE*> dirs;
	dirs.push(dir);
	string path(base);
	char name[2048], longentry[2048];	// TODO: no need for longentry
	bool ok = true;
	do {
		while (sftpReaddirEx(dirs.top(), name, sizeof(name), longentry, sizeof(longentry), &attr) > 0) {
			if (S_ISDIR(attr.flags)) {
				if (notDotName(name)) {
					path = std::move(path) / name;
					if (dir = sftpOpenEx(sftp, path.c_str(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR); dir)
						dirs.push(dir);
					else
						path = parentPath(path);
				}
			} else {
				string file = path / name;
				sftpUnlinkEx(sftp, file.c_str(), file.length());
			}
		}
		sftpClose(dirs.top());
		if (sftpRmdirEx(sftp, path.c_str(), path.length()))
			ok = false;
		path = parentPath(path);
		dirs.pop();
	} while (!dirs.empty());
	return ok;
}

bool FileOpsSftp::renameEntry(string_view oldPath, string_view newPath) {
	return sftpRenameEx && !sftpRenameEx(sftp, oldPath.data(), oldPath.length(), newPath.data(), newPath.length(), LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);
}

vector<byte_t> FileOpsSftp::readFile(string_view path) {
	std::lock_guard lockg(mlock);
	vector<byte_t> data;
	if (LIBSSH2_SFTP_HANDLE* fh = sftpOpenEx(sftp, path.data(), path.length(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE)) {
		if (LIBSSH2_SFTP_ATTRIBUTES attr; !sftpFstatEx(fh, &attr, 0)) {
			data.resize(attr.filesize);
			if (ssize_t len = sftpRead(fh, reinterpret_cast<char*>(data.data()), data.size()); len < ssize_t(data.size()))
				data.resize(std::max(len, ssize_t(0)));
		}
		sftpClose(fh);
	}
	return data;
}

fs::file_type FileOpsSftp::fileType(string_view path) {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) ? modeToType(attr.flags) : fs::file_type::not_found;
};

bool FileOpsSftp::isDirectory(string_view path) {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & S_IFDIR);
}

bool FileOpsSftp::pollWatch(vector<FileChange>&) {
	return false;
}

FileOpCapabilities FileOpsSftp::capabilities() const {
	FileOpCapabilities caps = FileOpCapabilities::none;
	if (sftpUnlinkEx && sftpRmdirEx)
		caps |= FileOpCapabilities::remove;
	if (sftpRenameEx)
		caps |= FileOpCapabilities::rename;
	return caps;
}

bool FileOpsSftp::equals(const RemoteLocation& rl) const {
	return rl.protocol == Protocol::sftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
