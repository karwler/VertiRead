#include "types.h"
#include "utils/compare.h"
#ifndef _WIN32
#include <sys/inotify.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

// ARCHIVE NODES

ArchiveFile::ArchiveFile(string&& filename, uintptr_t mem) :
	name(std::move(filename)),
	size(mem)
{}

ArchiveDir::ArchiveDir(ArchiveDir* daddy, string&& dirname) :
	parent(daddy),
	name(std::move(dirname))
{}

void ArchiveDir::sort() {
	std::sort(dirs.begin(), dirs.end(), [](const ArchiveDir& a, const ArchiveDir& b) -> bool { return StrNatCmp::less(a.name, b.name); });
	std::sort(files.begin(), files.end(), [](const ArchiveFile& a, const ArchiveFile& b) -> bool { return StrNatCmp::less(a.name, b.name); });
	for (ArchiveDir& it : dirs)
		it.sort();
}

void ArchiveDir::clear() {
	name.clear();
	dirs.clear();
	files.clear();
}

string ArchiveDir::path() const {
	size_t len = 0;
	for (const ArchiveDir* it = this; it->parent; it = it->parent)
		len += it->name.length() + 1;
	string str;
	str.resize(len);

	for (const ArchiveDir* it = this; it->parent; it = it->parent) {
		str[--len] = '/';
		len -= it->name.length();
		std::copy(it->name.begin(), it->name.end(), str.begin() + len);
	}
	return str;
}

pair<ArchiveDir*, ArchiveFile*> ArchiveDir::find(string_view path) {
	if (path.empty())
		return pair(nullptr, nullptr);

	ArchiveDir* node = this;
	size_t p = path.find_first_not_of('/');
	for (size_t e; (e = path.find('/', p)) != string::npos; p = path.find_first_not_of('/', p)) {
		vector<ArchiveDir>::iterator dit = node->findDir(path.substr(p, e - p));
		if (dit == dirs.end())
			return pair(nullptr, nullptr);
		node = &*dit;
	}
	if (p < path.length()) {
		string_view sname = path.substr(p);
		if (vector<ArchiveDir>::iterator dit = node->findDir(sname); dit != dirs.end())
			node = &*dit;
		else if (vector<ArchiveFile>::iterator fit = node->findFile(sname); fit != files.end())
			return pair(node, &*fit);
		else
			return pair(nullptr, nullptr);
	}
	return pair(node, nullptr);
}

vector<ArchiveDir>::iterator ArchiveDir::findDir(string_view dname) {
	vector<ArchiveDir>::iterator dit = std::lower_bound(dirs.begin(), dirs.end(), dname, [](const ArchiveDir& a, string_view b) -> bool { return StrNatCmp::less(a.name, b); });
	return dit != dirs.end() && dit->name == dname ? dit : dirs.end();
}

vector<ArchiveFile>::iterator ArchiveDir::findFile(string_view fname) {
	vector<ArchiveFile>::iterator fit = std::lower_bound(files.begin(), files.end(), fname, [](const ArchiveFile& a, string_view b) -> bool { return StrNatCmp::less(a.name, b); });
	return fit != files.end() && fit->name == fname ? fit : files.end();
}

// RESULT ASYNC

BrowserResultAsync::BrowserResultAsync(fs::path&& root, fs::path&& directory, fs::path&& pname, ArchiveDir&& aroot) :
	rootDir(std::move(root)),
	curDir(std::move(directory)),
	file(std::move(pname)),
	arch(std::move(aroot))
{}

BrowserResultPicture::BrowserResultPicture(bool inArch, fs::path&& root, fs::path&& directory, fs::path&& pname, ArchiveDir&& aroot) :
	BrowserResultAsync(std::move(root), std::move(directory), std::move(pname), std::move(aroot)),
	archive(inArch)
{}

BrowserPictureProgress::BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index) :
	pnt(rp),
	img(pic),
	id(index)
{}

// FILE WATCH

FileWatch::FileWatch(const fs::path& path) :
#ifdef _WIN32
	ebuf(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, esiz))
#else
	ino(inotify_init1(IN_NONBLOCK)),
	ebuf(static_cast<inotify_event*>(mmap(nullptr, esiz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)))
#endif
{
#ifdef _WIN32
	overlapped.hEvent = CreateEventW(nullptr, false, 0, nullptr);
#else
	if (ebuf == MAP_FAILED)
		throw std::runtime_error(strerror(errno));
#endif
	set(path);
}

FileWatch::~FileWatch() {
#ifdef _WIN32
	CloseHandle(dirc);
	CloseHandle(overlapped.hEvent);
	HeapFree(GetProcessHeap(), 0, ebuf);
#else
	close(ino);
	munmap(ebuf, esiz);
#endif
}

void FileWatch::set(const fs::path& path) {
	unset();
	try {
#ifdef _WIN32
		if (overlapped.hEvent) {
			if (fs::is_directory(path)) {
				dirc = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
				flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
				filter.clear();
			} else if (fs::path parent = parentPath(path); fs::is_directory(parent)) {
				dirc = CreateFileW(parent.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
				flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE;
				filter = path.filename();
			}
			if (dirc != INVALID_HANDLE_VALUE && !ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr))
				unset();
		}
#else
		if (ino != -1)
			watch = inotify_add_watch(ino, path.c_str(), fs::is_directory(path) ? IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO : IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF);
#endif
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
	}
}

void FileWatch::unset() {
#ifdef _WIN32
	if (dirc != INVALID_HANDLE_VALUE) {
		CloseHandle(dirc);
		dirc = INVALID_HANDLE_VALUE;
	}
#else
	if (watch != -1) {
		inotify_rm_watch(ino, watch);
		watch = -1;
	}
#endif
}

pair<vector<pair<bool, string>>, bool> FileWatch::changed() {
	vector<pair<bool, string>> files;
#ifdef _WIN32
	if (dirc == INVALID_HANDLE_VALUE)
		return pair(std::move(files), false);
	while (dirc != INVALID_HANDLE_VALUE && WaitForSingleObject(overlapped.hEvent, 0) == WAIT_OBJECT_0) {
		if (DWORD bytes; !GetOverlappedResult(dirc, &overlapped, &bytes, false)) {
			files.clear();
			unset();
			break;
		}

		for (FILE_NOTIFY_INFORMATION* event = static_cast<FILE_NOTIFY_INFORMATION*>(ebuf);; event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<uint8*>(event) + event->NextEntryOffset)) {
			switch (event->Action) {
			case FILE_ACTION_ADDED: case FILE_ACTION_RENAMED_NEW_NAME:
				if (filter.empty())
					files.emplace_back(true, swtos(wstring_view(event->FileName, event->FileNameLength / sizeof(wchar))));
				break;
			case FILE_ACTION_REMOVED: case FILE_ACTION_RENAMED_OLD_NAME:
				if (wstring_view file(event->FileName, event->FileNameLength / sizeof(wchar)); filter.empty())
					files.emplace_back(false, swtos(file));
				else if (file == filter.c_str())
					unset();
				break;
			case FILE_ACTION_MODIFIED:
				if (!filter.empty())
					if (wstring_view file(event->FileName, event->FileNameLength / sizeof(wchar)); file == filter.c_str())
						unset();
			}
			if (dirc == INVALID_HANDLE_VALUE || !event->NextEntryOffset)
				break;
		}
		if (dirc != INVALID_HANDLE_VALUE && !ReadDirectoryChangesW(dirc, ebuf, esiz, true, flags, nullptr, &overlapped, nullptr))
			unset();
	}
	return pair(std::move(files), dirc == INVALID_HANDLE_VALUE);
#else
	if (watch == -1)
		return pair(std::move(files), false);
	do {
		ssize_t len = read(ino, ebuf, esiz);
		if (len < 0)
			break;

		inotify_event* event;
		for (ssize_t i = 0; i < len; i += sizeof(inotify_event) + event->len) {
			event = reinterpret_cast<inotify_event*>(static_cast<void*>(reinterpret_cast<uint8*>(ebuf) + i));
			if (int bias = bool(event->mask & IN_CREATE) + bool(event->mask & IN_MOVED_TO) - bool(event->mask & IN_DELETE) - bool(event->mask & IN_MOVED_FROM))
				files.emplace_back(bias > 0, string(event->name, event->len));
			if (event->mask & (IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF)) {
				files.clear();
				unset();
				break;
			}
		}
	} while (watch != -1);
	return pair(std::move(files), watch == -1);
#endif
}
