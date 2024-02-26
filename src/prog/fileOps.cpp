#include "fileOps.h"
#include "utils/compare.h"
#if defined(CAN_SECRET) || defined(CAN_PDF)
#include "engine/optional/glib.h"
#endif
#ifdef CAN_PDF
#include "engine/optional/poppler.h"
#endif
#ifdef CAN_SECRET
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
#include <stack>
#include <archive.h>
#include <archive_entry.h>
#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/mman.h>
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
	SecretValue* svalue = secretValueNew(rl.password.data(), (rl.password.length() + 1) * sizeof(char), "string");	// TODO: what should this last argument be?
	GError* error = nullptr;
	secretServiceStoreSync(service, &schema, attributes, SECRET_COLLECTION_DEFAULT, std::format("VertiRead credentials for {}://{}{}", protocolNames[eint(rl.protocol)], rl.server, rl.path).data(), svalue, nullptr, &error);
	if (error) {
		logError(error);
		gErrorFree(error);
	}
}

void CredentialManager::setAttributes(const RemoteLocation& rl, string& portTmp) {
	gHashTableInsert(attributes, const_cast<char*>(keyProtocol), const_cast<char*>(protocolNames[eint(rl.protocol)]));
	gHashTableInsert(attributes, const_cast<char*>(keyServer), const_cast<char*>(rl.server.data()));
	gHashTableInsert(attributes, const_cast<char*>(keyPath), const_cast<char*>(rl.path.data()));
	gHashTableInsert(attributes, const_cast<char*>(keyUser), const_cast<char*>(rl.user.data()));
	if (rl.protocol == Protocol::smb)
		gHashTableInsert(attributes, const_cast<char*>(keyWorkgroup), const_cast<char*>(rl.workgroup.data()));
	else
		gHashTableRemove(attributes, keyWorkgroup);
	portTmp = toStr(rl.port);
	gHashTableInsert(attributes, const_cast<char*>(keyPort), portTmp.data());
	if (rl.protocol == Protocol::sftp)
		gHashTableInsert(attributes, const_cast<char*>(keyFamily), const_cast<char*>(familyNames[eint(rl.family)]));
	else
		gHashTableRemove(attributes, keyFamily);
}
#endif

// FILE OPS

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

bool FileOps::isPdf(const string& path) {
	bool ok = false;
#ifdef CAN_PDF
	if (SDL_RWops* ifh = makeRWops(path)) {
		byte_t sig[signaturePdf.size()];
		ok = SDL_RWread(ifh, sig, 1, sizeof(sig)) == sizeof(sig) && rng::equal(sig, signaturePdf) && symPoppler();
		SDL_RWclose(ifh);
	}
#endif
	return ok;
}

bool FileOps::isArchive(ArchiveData& ad) {
	if (archive* arch = openArchive(ad)) {
		archive_read_free(arch);
		return true;
	}
	return false;
}

void FileOps::makeArchiveTreeThread(std::stop_token stoken, BrowserResultArchive* ra, uint maxRes) {
	archive* arch = openArchive(ra->arch, &ra->error);
	if (!arch) {
		ra->rc = ResultCode::error;
		pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, ra);
		return;
	}

	int rc;
	for (archive_entry* entry; (rc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK;) {
		if (stoken.stop_requested()) {
			archive_read_free(arch);
			ra->rc = ResultCode::stop;
			pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, ra);
			return;
		}

		ArchiveDir* node = &ra->arch;
		for (const char* path = archive_entry_pathname_utf8(entry); *path;) {
			if (const char* next = strchr(path, '/')) {
				string_view sname(path, next);
				std::forward_list<ArchiveDir>::iterator dit = rng::find_if(node->dirs, [sname](const ArchiveDir& it) -> bool { return it.name == sname; });
				node = dit != node->dirs.end() ? std::to_address(dit) : &node->dirs.emplace_front(sname);
				path = next + 1;
			} else {
				if (Data data = readArchiveEntry(arch, entry); SDL_Surface* img = IMG_Load_RW(SDL_RWFromConstMem(data.data(), data.size()), SDL_TRUE)) {
					node->files.emplace_front(path, std::min(uint(img->w), maxRes) * std::min(uint(img->h), maxRes) * 4);
					SDL_FreeSurface(img);
				} else if (data.size() >= signaturePdf.size() && std::equal(signaturePdf.begin(), signaturePdf.end(), data.data()))
					node->files.emplace_front(path);
				else
					node->files.emplace_front(path, 0);
				break;
			}
		}
	}

	if (rc == ARCHIVE_EOF) {
		std::queue<ArchiveDir*> dirs;
		dirs.push(&ra->arch);
		do {
			ArchiveDir* dit = dirs.front();
			dit->finalize();
			for (ArchiveDir& it : dit->dirs)
				dirs.push(&it);
			dirs.pop();
		} while (!dirs.empty());
	} else if (ra->arch.pc == ArchiveData::PassCode::ignore)
		ra->rc = ResultCode::stop;
	else {
		ra->error = archive_error_string(arch);
		ra->rc = ResultCode::error;
	}
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, 0, ra);
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

#ifdef CAN_PDF
PdfFile FileOps::loadPdfChecked(SDL_RWops* ops, string* error) {
	PdfFile pdf;
	if (ops) {
		if (int64 esiz = SDL_RWsize(ops); esiz > int64(signaturePdf.size())) {
			byte_t sig[signaturePdf.size()];
			if (size_t len = SDL_RWread(ops, sig, 1, sizeof(sig)); len == sizeof(sig) && rng::equal(signaturePdf, sig) && symPoppler()) {
				pdf.resize(esiz);
				rng::copy(sig, pdf.data());
				size_t toRead = esiz - signaturePdf.size();
				if (len = SDL_RWread(ops, pdf.data() + signaturePdf.size(), sizeof(byte_t), toRead); len) {
					if (len < toRead)
						pdf.resize(signaturePdf.size() + len);

					GError* gerr = nullptr;
					GBytes* bytes = gBytesNewStatic(pdf.data(), pdf.size());
					if (pdf.pdoc = popplerDocumentNewFromBytes(bytes, nullptr, &gerr); !pdf.pdoc) {
						if (error)
							*error = gerr->message;
						gErrorFree(gerr);
					}
					gBytesUnref(bytes);
				}
			} else if (error)
				*error = "Missing PDF signature";
		}
		SDL_RWclose(ops);
	}
	if (!pdf.pdoc) {
		pdf.clear();
		if (error && error->empty())
			*error = "Failed to read PDF data";
	}
	return pdf;
}
#endif

template <InvocableR<int, archive*, ArchiveData&> F>
archive* FileOps::initArchive(ArchiveData& ad, string* error, F openArch) {
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

SDL_RWops* FileOps::makeArchiveEntryRWops(archive* arch, archive_entry* entry) {
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

vector<Cstring> FileOpsLocal::listDirectory(const string& path, bool files, bool dirs, bool hidden) {
#ifdef _WIN32
	if (path.empty())	// if in "root" directory, get drive letters and present them as directories
		return dirs ? FileOpsLocal::listDrives() : vector<Cstring>();
#endif
	vector<Cstring> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(path) / L"*").data(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (dirs && wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
						entries.emplace_back(data.cFileName);
				} else if (files)
					entries.emplace_back(data.cFileName);
			}
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
#else
	if (DIR* directory = opendir(path.data())) {
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
					if (!stat((path / entry->d_name).data(), &ps))
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
		rng::sort(entries, Strcomp());
	}
	return entries;
}

pair<vector<Cstring>, vector<Cstring>> FileOpsLocal::listDirectorySep(const string& path, bool hidden) {
#ifdef _WIN32
	if (path.empty())	// if in "root" directory, get drive letters and present them as directories
		return pair(vector<Cstring>(), listDrives());
#endif
	vector<Cstring> files, dirs;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW((sstow(path) / L"*").data(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (hidden || !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (notDotName(data.cFileName))
						dirs.emplace_back(data.cFileName);
				} else
					files.emplace_back(data.cFileName);
			}
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
#else
	if (DIR* directory = opendir(path.data())) {
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
					if (!stat((path / entry->d_name).data(), &ps))
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
		rng::sort(files, Strcomp());
		rng::sort(dirs, Strcomp());
	}
	return pair(std::move(files), std::move(dirs));
}

#ifdef _WIN32
vector<Cstring> FileOpsLocal::listDrives() {
	vector<Cstring> letters;
	DWORD drives = GetLogicalDrives();
	for (char i = 0; i < drivesMax; ++i)
		if (drives & (1 << i))
			letters.push_back(Cstring{ char('A' + i), ':', '\\' });
	return letters;
}
#endif

bool FileOpsLocal::deleteEntry(const string& base) {
	bool ok = true;
#ifdef _WIN32
	fs::path path = toPath(base);
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
	if (struct stat ps; lstat(base.data(), &ps))
		return false;
	else if (!S_ISDIR(ps.st_mode))
		return !unlink(base.data());
	DIR* dir = opendir(base.data());
	if (!dir)
		return false;

	fs::path path = toPath(base);
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
bool FileOpsLocal::isDirectory(const wchar_t* path) {
	DWORD attr = GetFileAttributesW(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

#else

bool FileOpsLocal::hasModeFlags(const char* path, mode_t flags) {
	struct stat ps;
	return !stat(path, &ps) && (ps.st_mode & flags);
}
#endif

archive* FileOpsLocal::openArchive(ArchiveData& ad, string* error) {
	return initArchive(ad, error, [](archive* a, ArchiveData& d) -> int {
#ifdef _WIN32
		return archive_read_open_filename_w(a, sstow(d.name.data()).data(), archiveReadBlockSize);
#else
		return archive_read_open_filename(a, d.name.data(), archiveReadBlockSize);
#endif
	});
}

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

bool FileOpsLocal::pollWatch(vector<FileChange>& files) {
#ifdef _WIN32
	if (dirc == INVALID_HANDLE_VALUE)
		return false;

	while (WaitForSingleObject(overlapped.hEvent, 0) == WAIT_OBJECT_0) {
		if (DWORD bytes; !GetOverlappedResult(dirc, &overlapped, &bytes, false))
			return unsetWatch();

		for (auto event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ebuf);; event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<byte_t*>(event) + event->NextEntryOffset)) {
			if (wstring_view name(event->FileName, event->FileNameLength / sizeof(wchar_t)); rng::none_of(name, [](wchar_t ch) -> bool { return isDsep(ch); })) {
				switch (event->Action) {
				case FILE_ACTION_ADDED: case FILE_ACTION_RENAMED_NEW_NAME:
					if (filter.empty())
						if (DWORD attr = GetFileAttributesW((wpdir / name).data()); attr != INVALID_FILE_ATTRIBUTES)
							files.emplace_back(swtos(name), attr & FILE_ATTRIBUTE_DIRECTORY ? FileChange::addDirectory : FileChange::addFile);
					break;
				case FILE_ACTION_REMOVED: case FILE_ACTION_RENAMED_OLD_NAME:
					if (filter.empty())
						files.emplace_back(swtos(name), FileChange::deleteEntry);
					else if (name == filter)
						return unsetWatch();
					break;
				case FILE_ACTION_MODIFIED:
					if (name == filter)
						return unsetWatch();
				}
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
				else if (!stat((wpdir / name).data(), &ps))
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

bool FileOpsLocal::canWatch() const {
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
archive* FileOpsRemote::openArchive(ArchiveData& ad, string* error) {
	return initArchive(ad, error, [this](archive* a, ArchiveData& d) -> int {
		if (d.dref)
			return archive_read_open_memory(a, d.dref->data(), d.dref->size());
		d.data = readFile(d.name.data());
		return archive_read_open_memory(a, d.data.data(), d.data.size());
	});
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
	smbcFreeContext(ctx, 0);
	throw std::runtime_error(std::format("Failed to open share '{}'", spath));
}

FileOpsSmb::~FileOpsSmb() {
	if (ctx) {
		if (wndir)
			sclosedir(ctx, wndir);
		smbcFreeContext(ctx, 0);
	}
}

vector<Cstring> FileOpsSmb::listDirectory(const string& path, bool files, bool dirs, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<Cstring> entries;
	if (SMBCFILE* dir = sopendir(ctx, path.data())) {
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
					if (!sstat(ctx, (path / name).data(), &ps))
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
		rng::sort(entries, Strcomp());
	}
	return entries;
}

pair<vector<Cstring>, vector<Cstring>> FileOpsSmb::listDirectorySep(const string& path, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<Cstring> files, dirs;
	if (SMBCFILE* dir = sopendir(ctx, path.data())) {
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
					if (!sstat(ctx, (path / name).data(), &ps))
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
		rng::sort(files, Strcomp());
		rng::sort(dirs, Strcomp());
	}
	return pair(std::move(files), std::move(dirs));
}

bool FileOpsSmb::deleteEntry(const string& base) {
	std::lock_guard lockg(mlock);
	if (struct stat ps; sstat(ctx, base.data(), &ps))
		return false;
	else if (!S_ISDIR(ps.st_mode))
		return !sunlink(ctx, base.data());
	SMBCFILE* dir = sopendir(ctx, base.data());
	if (!dir)
		return false;

	string path = base;
	std::stack<SMBCFILE*> dirs;
	dirs.push(dir);
	bool ok = true;
	do {
		while (smbc_dirent* it = sreaddir(ctx, dirs.top())) {
			string_view name(it->name, it->namelen);
			if (it->smbc_type == SMBC_DIR) {
				if (notDotName(it->name)) {
					path = std::move(path) / name;
					if (dir = sopendir(ctx, path.data()); dir)
						dirs.push(dir);
					else
						path = parentPath(path);
				}
			} else
				sunlink(ctx, (path / name).data());
		}
		sclosedir(ctx, dirs.top());
		if (srmdir(ctx, path.data()))
			ok = false;
		path = parentPath(path);
		dirs.pop();
	} while (!dirs.empty());
	return ok;
}

bool FileOpsSmb::renameEntry(const string& oldPath, const string& newPath) {
	return !srename(ctx, oldPath.data(), ctx, newPath.data());
}

Data FileOpsSmb::readFile(const string& path) {
	std::lock_guard lockg(mlock);
	Data data;
	if (SMBCFILE* fh = sopen(ctx, path.data(), O_RDONLY, 0)) {	// TODO: check if this works if there's no open handle to the directory/share
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
	return !sstat(ctx, path, &ps) && (ps.st_mode & mdes);	// TODO: is this right with S_IFDIR and S_IFREG?
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
		wndir = sopendir(ctx, string(path).data());
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
	return (rc || closeWatch) && unsetWatch();
}

bool FileOpsSmb::unsetWatch() {
	sclosedir(ctx, wndir);
	wndir = nullptr;
	return true;
}

bool FileOpsSmb::canWatch() const {
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
		if (getaddrinfo(rl.server.data(), toStr(port).data(), &hints, &addr) || !addr)
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
				if (!sshUserauthPasswordEx(session, user.data(), user.length(), pwd.data(), pwd.length(), nullptr))
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
						if (!sshUserauthPublickeyFromfileEx(session, user.data(), user.length(), pub.c_str(), prv.c_str(), pwd.data()))
							return;
		}
		throw std::runtime_error("Authentication failed");
	}
}

vector<Cstring> FileOpsSftp::listDirectory(const string& path, bool files, bool dirs, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<Cstring> entries;
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
					if (string fpath = string(path) / name; !sftpStatEx(sftp, fpath.data(), fpath.length(), LIBSSH2_SFTP_STAT, &attr))
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
		rng::sort(entries, Strcomp());
	}
	return entries;
}

pair<vector<Cstring>, vector<Cstring>> FileOpsSftp::listDirectorySep(const string& path, bool hidden) {
	std::lock_guard lockg(mlock);
	vector<Cstring> files, dirs;
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
					if (string fpath = string(path) / name; !sftpStatEx(sftp, fpath.data(), fpath.length(), LIBSSH2_SFTP_STAT, &attr))
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
		rng::sort(files, Strcomp());
		rng::sort(dirs, Strcomp());
	}
	return pair(std::move(files), std::move(dirs));
}

bool FileOpsSftp::deleteEntry(const string& base) {
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
	string path = base;
	char name[2048], longentry[2048];	// TODO: no need for longentry
	bool ok = true;
	do {
		while (sftpReaddirEx(dirs.top(), name, sizeof(name), longentry, sizeof(longentry), &attr) > 0) {
			if (S_ISDIR(attr.flags)) {
				if (notDotName(name)) {
					path = std::move(path) / name;
					if (dir = sftpOpenEx(sftp, path.data(), path.length(), 0, 0, LIBSSH2_SFTP_OPENDIR); dir)
						dirs.push(dir);
					else
						path = parentPath(path);
				}
			} else {
				string file = path / name;
				sftpUnlinkEx(sftp, file.data(), file.length());
			}
		}
		sftpClose(dirs.top());
		if (sftpRmdirEx(sftp, path.data(), path.length()))
			ok = false;
		path = parentPath(path);
		dirs.pop();
	} while (!dirs.empty());
	return ok;
}

bool FileOpsSftp::renameEntry(const string& oldPath, const string& newPath) {
	return !sftpRenameEx(sftp, oldPath.data(), oldPath.length(), newPath.data(), newPath.length(), LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE);
}

Data FileOpsSftp::readFile(const string& path) {
	std::lock_guard lockg(mlock);
	Data data;
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

fs::file_type FileOpsSftp::fileType(const string& path) {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) ? modeToType(attr.flags) : fs::file_type::not_found;
}

bool FileOpsSftp::isRegular(const string& path) {
	return hasAttributeFlags(path, S_IFREG);
}

bool FileOpsSftp::isDirectory(const string& path) {
	return hasAttributeFlags(path, S_IFDIR);
}

bool FileOpsSftp::hasAttributeFlags(string_view path, ulong flags) {
	std::lock_guard lockg(mlock);
	LIBSSH2_SFTP_ATTRIBUTES attr;
	return !sftpStatEx(sftp, path.data(), path.length(), LIBSSH2_SFTP_STAT, &attr) && (attr.flags & flags);
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
	return !sftpFstatEx(static_cast<LIBSSH2_SFTP_HANDLE*>(context->hidden.unknown.data2), &attr, 0) ? attr.filesize : -1;
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
		if (sftpFstatEx(fh, &attr, 0))
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

bool FileOpsSftp::canWatch() const {
	return false;
}

bool FileOpsSftp::equals(const RemoteLocation& rl) const {
	return rl.protocol == Protocol::sftp && rl.server == server && rl.port == port && rl.user == user;
}
#endif
