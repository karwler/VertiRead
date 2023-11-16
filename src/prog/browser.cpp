#include "browser.h"
#include "fileOps.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/world.h"
#include "utils/compare.h"
#include <functional>
#include <archive.h>
#include <archive_entry.h>

namespace std {

template <>
struct default_delete<SDL_Surface> {
	void operator()(SDL_Surface* ptr) const {
		SDL_FreeSurface(ptr);
	}
};

}

Browser::~Browser() {
	stopThread();
	delete fsop;
}

uptr<RemoteLocation> Browser::prepareFileOps(string_view path) {
	Protocol proto = RemoteLocation::getProtocol(path);
	if (proto == Protocol::none) {
		if (!dynamic_cast<FileOpsLocal*>(fsop)) {	// if remote to local, else local to local
			delete fsop;
			fsop = new FileOpsLocal;
		}
		return nullptr;
	}
	RemoteLocation rl = RemoteLocation::fromPath(path, proto);
	return fsop->equals(rl) ? nullptr : std::make_unique<RemoteLocation>(std::move(rl));	// ? same connection : make new connection
}

template <Invocable<FileOps*, const RemoteLocation&> F>
auto Browser::beginRemoteOps(const RemoteLocation& location, vector<string>&& passwords, F func) {
	FileOps* backup = fsop;
	fsop = FileOps::instantiate(location, passwords.empty() ? vector<string>{ location.password } : std::move(passwords));	// will throw if can't connect
	try {
		return func(backup, location);	// func should try to use the new connection and must delete backup once nothing can throw anymore
	} catch (const std::runtime_error&) {
		delete fsop;
		fsop = backup;	// rollback if it's not a valid location
		throw;
	}
}

void Browser::start(string&& root, const RemoteLocation& location, vector<string>&& passwords) {
	beginRemoteOps(location, std::move(passwords), [this, &root](FileOps* backup, const RemoteLocation& rl) {
		start(std::move(root), valcp(rl.path));
		delete backup;
	});
}

void Browser::start(string&& root, string&& path) {
	if (root.empty())
		root = fsop->prefix();
	else if (!fsop->isDirectory(root))
		throw std::runtime_error(std::format("Browser root '{}' isn't a directory", root));

	if (!path.empty()) {
		if (!fsop->isDirectory(path))
			throw std::runtime_error(std::format("Browser directory '{}' isn't a directory", path));
		if (!isSubpath(path, root))
			throw std::runtime_error(std::format("Browser directory '{}' isn't a subpath of root '{}'", path, root));
		curDir = std::move(path);
	} else
		curDir = root;
	rootDir = std::move(root);
	fsop->setWatch(curDir);
}

bool Browser::goTo(const RemoteLocation& location, vector<string>&& passwords) {
	return beginRemoteOps(location, std::move(passwords), [this](FileOps* backup, const RemoteLocation& rl) -> bool {
		bool wait = goTo(fsop->prefix() / rl.path);
		delete backup;
		return wait;
	});
}

bool Browser::goTo(string_view path) {
	switch (fsop->fileType(path)) {
	using enum fs::file_type;
	case regular:
		if (fsop->isPicture(path)) {
			startLoadPictures(new BrowserResultPicture(false, isSubpath(path, rootDir) ? string() : fsop->prefix(), string(parentPath(path)), string(filename(path))));
			return true;
		}
		if (fsop->isArchive(path)) {
			startArchive(BrowserResultAsync(isSubpath(path, rootDir) ? string() : fsop->prefix(), string(path)));
			return true;
		}
		if (path = parentPath(path); !fsop->isDirectory(path))
			break;
	case directory:
		if (!isSubpath(path, rootDir))
			rootDir = fsop->prefix();
		curDir = path;
		arch.clear();
		curNode = nullptr;
		fsop->setWatch(curDir);
		return false;
	}
	throw std::runtime_error(std::format("Invalid path '{}'", path));
}

bool Browser::openPicture(string&& rdir, string&& dirc, string&& file) {
	if (rdir.empty() || !fsop->isDirectory(rdir) || !isSubpath(file, rdir))
		rdir = fsop->prefix();

	switch (fsop->fileType(dirc)) {
	using enum fs::file_type;
	case regular:
		if (fsop->isArchive(dirc)) {
			startArchive(BrowserResultAsync(std::move(rdir), std::move(dirc), std::move(file)));
			return true;
		}
		break;
	case directory:
		startLoadPictures(new BrowserResultPicture(false, std::move(rdir), std::move(dirc), std::move(file)));
		return true;
	}
	return false;
}

bool Browser::goIn(string_view dname) {
	if (curNode) {
		if (vector<ArchiveDir>::iterator dit = curNode->findDir(dname); dit != curNode->dirs.end()) {
			curNode = std::to_address(dit);
			return true;
		}
	} else if (string newPath = curDir / dname; fsop->isDirectory(newPath)) {
		curDir = std::move(newPath);
		fsop->setWatch(curDir);
		return true;
	}
	return false;
}

bool Browser::goFile(string_view fname) {
	if (curNode) {
		if (vector<ArchiveFile>::iterator fit = curNode->findFile(fname); fit != curNode->files.end() && fit->size) {
			startLoadPictures(curNode->path() + fname);
			return true;
		}
	} else if (string path = curDir / fname; fsop->isPicture(path)) {
		startLoadPictures(string(fname));
		return true;
	} else if (fsop->isArchive(path)) {
		startArchive(BrowserResultAsync(string(), std::move(path)));
		return true;
	}
	return false;
}

bool Browser::goUp() {
	if (curDir == rootDir)
		return false;

	if (curNode) {
		if (curNode != &arch)
			curNode = curNode->parent;
		else {
			curDir = parentPath(curDir);
			arch.clear();
			curNode = nullptr;
			fsop->setWatch(curDir);
		}
	} else {
		curDir = parentPath(curDir);
		fsop->setWatch(curDir);
	}
	return true;
}

bool Browser::goNext(bool fwd, string_view picname) {
	if (!picname.empty())
		if (string file = curNode ? nextArchiveFile(picname, fwd) : nextDirFile(picname, fwd); !file.empty()) {
			startLoadPictures(std::move(file), fwd);
			return true;
		}

	if (curNode)
		shiftArchive(fwd);
	else
		shiftDir(fwd);
	startLoadPictures(string(), fwd);
	return true;
}

void Browser::shiftDir(bool fwd) {
	if (curDir != rootDir) {
		// find current directory and set it to the path of the next valid directory in the parent directory
		string dir(parentPath(curDir));
		string_view dname = filename(curDir);
		vector<string> dirs = fsop->listDirectory(dir, false, true, World::sets()->showHidden);
		if (vector<string>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), dname, StrNatCmp()); di != dirs.end() && *di == dname)
			foreachAround(dirs, di, fwd, &Browser::nextDir, dir);
	}
}

bool Browser::nextDir(string_view dit, const string& pdir) {
	string idir = pdir / dit;
	vector<string> files = fsop->listDirectory(idir, true, false, World::sets()->showHidden);
	if (vector<string>::iterator fi = rng::find_if(files, [this, &idir](const string& it) -> bool { return fsop->isPicture(idir / it); }); fi != files.end()) {
		curDir = std::move(idir);
		fsop->setWatch(curDir);
		return true;
	}
	return false;
}

void Browser::shiftArchive(bool fwd) {
	if (curNode != &arch) {
		// find current directory and set it to the node of the next valid directory in the parent directory
		if (vector<ArchiveDir>::iterator di = curNode->parent->findDir(curNode->name); di != curNode->parent->dirs.end())
			foreachAround(curNode->parent->dirs, di, fwd, &Browser::nextArchiveDir);
	} else {
		// get list of archive files in the same directory, find the current file and select the next one
		string pdir(parentPath(curDir));
		string_view aname = filename(curDir);
		vector<string> files = fsop->listDirectory(pdir, true, false, World::sets()->showHidden);
		if (vector<string>::iterator fi = std::lower_bound(files.begin(), files.end(), aname, StrNatCmp()); fi != files.end() && *fi == aname)
			foreachAround(files, fi, fwd, &Browser::nextArchive, pdir);
	}
}

bool Browser::nextArchiveDir(ArchiveDir& dit) {
	if (vector<ArchiveFile>::iterator fi = rng::find_if(dit.files, [](const ArchiveFile& it) -> bool { return it.size; }); fi != dit.files.end()) {
		curNode = &dit;
		return true;
	}
	return false;
}

bool Browser::nextArchive(string_view ait, const string& pdir) {
	if (string path = pdir / ait; fsop->isPictureArchive(path)) {
		curDir = std::move(path);
		fsop->setWatch(curDir);
		return true;
	}
	return false;
}

template <Class T, MemberFunction F, class... A>
bool Browser::foreachFAround(vector<T>& vec, vector<T>::iterator start, F func, A&&... args) {
	return std::find_if(start + 1, vec.end(), [this, func, &args...](T& it) -> bool { return (this->*func)(it, args...); }) != vec.end()
		|| std::find_if(vec.begin(), start, [this, func, &args...](T& it) -> bool { return (this->*func)(it, args...); }) != start;
}

template <Class T, MemberFunction F, class... A>
bool Browser::foreachRAround(vector<T>& vec, vector<T>::reverse_iterator start, F func, A&&... args) {
	return std::find_if(start + 1, vec.rend(), [this, func, &args...](T& it) -> bool { return (this->*func)(it, args...); }) != vec.rend()
		|| (!vec.empty() && std::find_if(vec.rbegin(), start, [this, func, &args...](T& it) -> bool { return (this->*func)(it, args...); }) != start);
}

template <Class T, MemberFunction F, class... A>
bool Browser::foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F func, A&&... args) {
	return fwd ? foreachFAround(vec, start, func, args...) : foreachRAround(vec, std::make_reverse_iterator(start + 1), func, args...);
}

string Browser::nextDirFile(string_view file, bool fwd) const {
	if (!file.empty()) {
		vector<string> files = fsop->listDirectory(curDir, true, false, World::sets()->showHidden);
		vector<string>::iterator fit = std::lower_bound(files.begin(), files.end(), file, StrNatCmp());
		for (size_t mov = btom<size_t>(fwd), i = fit - files.begin() + (fit != files.end() && (*fit == file || !fwd) ? mov : 0); i < files.size(); i += mov)
			if (fsop->isPicture(curDir / files[i]))
				return files[i];
	}
	return string();
}

string Browser::nextArchiveFile(string_view file, bool fwd) const {
	if (!file.empty())
		for (size_t mov = btom<size_t>(fwd), i = curNode->findFile(file) - curNode->files.begin() + mov; i < curNode->files.size(); i += mov)
			if (curNode->files[i].size)
				return curNode->files[i].name;
	return string();
}

string Browser::currentLocation() const {
	if (curNode) {
		string npath = curNode->path();
		return !npath.empty() ? curDir / string_view(npath.begin(), npath.end() - 1) : curDir;
	}
	return curDir;
}

string Browser::curDirSuffix() const {
	if (curNode) {
		string npath = curNode->path();
		return !npath.empty() ? npath.substr(0, npath.length() - 1) : string();
	}
	return string();
}

pair<vector<string>, vector<string>> Browser::listCurDir() const {
	if (curNode) {
		pair<vector<string>, vector<string>> out(curNode->files.size(), curNode->dirs.size());
		rng::transform(curNode->files, out.first.begin(), [](const ArchiveFile& it) -> string { return it.name; });
		rng::transform(curNode->dirs, out.second.begin(), [](const ArchiveDir& it) -> string { return it.name; });
		return out;
	}
	return fsop->listDirectorySep(curDir, World::sets()->showHidden);
}

vector<string> Browser::listDirDirs(string_view path) const {
	return fsop->listDirectory(path, false, true, World::sets()->showHidden);
}

bool Browser::deleteEntry(string_view ename) {
	return !curNode && fsop->deleteEntry(curDir / ename);
}

bool Browser::renameEntry(string_view oldName, string_view newName) {
	return !curNode && fsop->renameEntry(curDir / oldName, curDir / newName);
}

FileOpCapabilities Browser::fileOpCapabilities() const {
	return !curNode ? fsop->capabilities() : FileOpCapabilities::none;
}

bool Browser::directoryUpdate(vector<FileChange>& files) {
	bool gone = fsop->pollWatch(files);
	if (curNode && gone) {
		arch.clear();
		curNode = &arch;
	}
	return gone;
}

void Browser::stopThread() {
	if (thread.joinable()) {
		thread = std::jthread();
		switch (curThread) {
		using enum ThreadType;
		case preview:
			cleanupPreview();
			break;
		case reader:
			cleanupLoadPictures();
			break;
		case archive:
			cleanupArchive();
		}
		curThread = ThreadType::none;
	}
}

void Browser::startArchive(BrowserResultAsync&& ra) {
	stopThread();
	curThread = ThreadType::archive;
	thread = std::jthread(std::bind_front(&FileOps::makeArchiveTreeThread, fsop), std::move(ra), World::sets()->maxPicRes);
}

bool Browser::finishArchive(BrowserResultAsync&& ra) {
	auto [dir, fil] = ra.arch.find(ra.file);
	if (fil) {
		startLoadPictures(new BrowserResultPicture(true, std::move(ra.rootDir), std::move(ra.curDir), std::move(ra.file), std::move(ra.arch)));
		return false;
	}
	if (!ra.rootDir.empty())
		rootDir = std::move(ra.rootDir);
	curDir = std::move(ra.curDir);
	arch = std::move(ra.arch);
	curNode = coalesce(dir, &arch);
	fsop->setWatch(curDir);
	return true;
}

void Browser::cleanupArchive() {
	cleanupEvent(SDL_USEREVENT_THREAD_ARCHIVE, [](SDL_UserEvent& event) {
		if (ThreadEvent(event.code) == ThreadEvent::finished)
			delete static_cast<BrowserResultAsync*>(event.data1);
	});
}

void Browser::startPreview(const vector<string>& files, const vector<string>& dirs, int maxHeight) {
	stopThread();
	curThread = ThreadType::preview;
	thread = curNode
		? std::jthread(&Browser::previewArchThread, fsop, curDir, sliceArchiveFiles(*curNode), curNode->path(), fromPath(World::fileSys()->dirIcons()), maxHeight)
		: std::jthread(&Browser::previewDirThread, fsop, curDir, files, dirs, fromPath(World::fileSys()->dirIcons()), World::sets()->showHidden, maxHeight);
}

void Browser::cleanupPreview() {
	cleanupEvent(SDL_USEREVENT_THREAD_PREVIEW, [](SDL_UserEvent& event) {
		if (ThreadEvent(event.code) == ThreadEvent::progress) {
			delete[] static_cast<char*>(event.data1);
			SDL_FreeSurface(static_cast<SDL_Surface*>(event.data2));
		}
	});
}

void Browser::previewDirThread(std::stop_token stoken, FileOps* fsop, string curDir, vector<string> files, vector<string> dirs, string iconPath, bool showHidden, int maxHeight) {
	if (uptr<SDL_Surface> dicon(!dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / "folder.svg").c_str(), maxHeight), false) : nullptr); dicon)
		for (const string& it : dirs) {
			if (stoken.stop_requested())
				return;

			string dpath = curDir / it;
			for (const string& sit : fsop->listDirectory(dpath, true, false, showHidden))
				if (SDL_Surface* img = combineIcons(dicon.get(), fsop->loadPicture(dpath / sit))) {
					pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it, false), img);
					break;
				}
		}
	for (const string& it : files) {
		if (stoken.stop_requested())
			return;

		if (string fpath = curDir / it; SDL_Surface* img = scaleDown(fsop->loadPicture(fpath), maxHeight))
			pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it, true), img);
		else if (vector<string> entries = fsop->listArchiveFiles(fpath); !entries.empty())
			for (const string& ei : entries)
				if (img = scaleDown(fsop->loadArchivePicture(fpath, ei), maxHeight); img) {
					pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it, true), img);
					break;
				}
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

void Browser::previewArchThread(std::stop_token stoken, FileOps* fsop, string curDir, ArchiveDir root, string dir, string iconPath, int maxHeight) {
	uptr<SDL_Surface> dicon(!root.dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / "folder.svg").c_str(), maxHeight), false) : nullptr);
	umap<string, bool> entries;
	entries.reserve(dicon ? root.dirs.size() + root.files.size() : root.files.size());
	if (dicon)
		for (const ArchiveDir& it : root.dirs)
			if (vector<ArchiveFile>::const_iterator fit = rng::find_if(it.files, [](const ArchiveFile& af) -> bool { return af.size; }); fit != it.files.end())
				entries.emplace(dir + it.name + '/' + fit->name, false);
	for (const ArchiveFile& it : root.files)
		if (it.size)
			entries.emplace(dir + it.name, true);

	if (archive* arch = fsop->openArchive(curDir)) {
		for (archive_entry* entry; !entries.empty() && !archive_read_next_header(arch, &entry);) {
			if (stoken.stop_requested()) {
				archive_read_free(arch);
				return;
			}

			if (umap<string, bool>::iterator pit = entries.find(archive_entry_pathname_utf8(entry)); pit != entries.end()) {
				if (SDL_Surface* img = pit->second ? scaleDown(FileOps::loadArchivePicture(arch, entry), maxHeight) : combineIcons(dicon.get(), FileOps::loadArchivePicture(arch, entry)))
					pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(pit->first, pit->second), img);
				entries.erase(pit);
			}
		}
		archive_read_free(arch);
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

ArchiveDir Browser::sliceArchiveFiles(const ArchiveDir& node) {
	ArchiveDir root;
	root.dirs.resize(node.dirs.size());
	for (size_t i = 0; i < root.dirs.size(); ++i) {
		root.dirs[i].name = node.dirs[i].name;
		root.dirs[i].files = node.dirs[i].files;
	}
	root.files = node.files;
	return root;
}

SDL_Surface* Browser::combineIcons(SDL_Surface* dir, SDL_Surface* img) {
	if (!img)
		return img;

	ivec2 asiz(dir->w / 4 * 3, dir->h / 3 * 2);
	ivec2 size = img->h - asiz.y >= img->w - asiz.x ? ivec2(float(img->w * asiz.y) / float(img->h), asiz.y) : ivec2(asiz.x, float(img->h * asiz.x) / float(img->w));
	SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, dir->w, dir->h, dir->format->BitsPerPixel, dir->format->format);
	if (dst) {
		SDL_Rect rect = { dir->w - size.x, dir->h - size.y, size.x, size.y };
		SDL_BlitSurface(dir, nullptr, dst, nullptr);
		SDL_FillRect(dst, &rect, SDL_MapRGBA(dst->format, 0, 0, 0, 255));
		SDL_BlitScaled(img, nullptr, dst, &rect);
	}
	SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* Browser::scaleDown(SDL_Surface* img, int maxHeight) {
	if (img && img->h > maxHeight)
		if (SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, int(float(img->w * maxHeight) / float(img->h)), maxHeight, img->format->BitsPerPixel, img->format->format)) {
			SDL_BlitScaled(img, nullptr, dst, nullptr);
			SDL_FreeSurface(img);
			img = dst;
		}
	return World::drawSys()->getRenderer()->makeCompatible(img, false);
}

char* Browser::allocatePreviewName(string_view name, bool file) {
	auto str = new char[name.length() + 2];
	str[0] = file;
	std::copy_n(name.data(), name.length() + 1, str + 1);
	return str;
}

void Browser::startLoadPictures(BrowserResultPicture* rp, bool fwd) {
	stopThread();
	curThread = ThreadType::reader;
	uint compress = World::sets()->compression != Settings::Compression::b16 ? 1 : 2;
	if (rp->archive) {
		umap<string, uintptr_t> files = mapArchiveFiles(curNode->path(), curNode->files, World::sets()->picLim, compress, filename(rp->file), fwd);
		thread = std::jthread(&Browser::loadPicturesArchThread, rp, fsop, std::move(files), World::sets()->picLim);
	} else
		thread = std::jthread(&Browser::loadPicturesDirThread, rp, fsop, World::sets()->picLim, compress, fwd, World::sets()->showHidden);
}

void Browser::finishLoadPictures(BrowserResultPicture& rp) {
	if (!rp.rootDir.empty()) {
		rootDir = std::move(rp.rootDir);
		curDir = std::move(rp.curDir);
		arch.clear();
		curNode = nullptr;
		fsop->setWatch(curDir);
		if (rp.archive) {
			arch = std::move(rp.arch);
			auto [dir, fil] = arch.find(rp.file);
			curNode = coalesce(dir, &arch);
		}
	}
}

void Browser::cleanupLoadPictures() {
	cleanupEvent(SDL_USEREVENT_THREAD_READER, [](SDL_UserEvent& event) {
		switch (ThreadEvent(event.code)) {
		using enum ThreadEvent;
		case progress: {
			auto pp = static_cast<BrowserPictureProgress*>(event.data1);
			SDL_FreeSurface(pp->img);
			delete pp;
			delete[] static_cast<char*>(event.data2);
			break; }
		case finished:
			if (auto rp = static_cast<BrowserResultPicture*>(event.data1)) {
				for (auto& [name, tex] : rp->pics)
					if (tex)
						World::drawSys()->getRenderer()->freeTexture(tex);
				delete rp;
			}
		}
	});
}

void Browser::loadPicturesDirThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint compress, bool fwd, bool showHidden) {
	vector<string> files = fsop->listDirectory(rp->curDir, true, false, showHidden);
	size_t start = fwd ? 0 : files.size() - 1;
	if (picLim.type != PicLim::Type::none)
		if (vector<string>::iterator it = rng::lower_bound(files, rp->file, StrNatCmp()); it != files.end() && *it == rp->file)
			start = it - files.begin();
	auto [lim, mem, sizMag] = initLoadLimits(picLim, fwd ? files.size() - start : start + 1);	// picture count limit, picture size limit, magnitude index
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	uintptr_t m = 0;

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	for (size_t mov = btom<uintptr_t>(fwd), i = start, c = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (stoken.stop_requested())
			break;

		if (SDL_Surface* img = World::drawSys()->getRenderer()->makeCompatible(fsop->loadPicture(rp->curDir / files[i]), true)) {
			rp->mpic.lock();
			rp->pics.emplace_back(std::move(files[i]), nullptr);
			rp->mpic.unlock();
			m += uintptr_t(img->w) * uintptr_t(img->h) * 4 / compress;
			++c;
			pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(rp, img, c - 1), progressText(limitToStr(picLim, c, m, sizMag), progLim));
		}
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp, std::bit_cast<void*>(uintptr_t(fwd)));
}

void Browser::loadPicturesArchThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, umap<string, uintptr_t> files, PicLim picLim) {
	auto [lim, mem, sizMag] = initLoadLimits(picLim, files.size());
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	size_t c = 0;
	uintptr_t m = 0;
	archive* arch = fsop->openArchive(rp->curDir);
	if (!arch) {
		delete rp;
		pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished);
		return;
	}

	for (archive_entry* entry; !files.empty() && !archive_read_next_header(arch, &entry);) {
		if (stoken.stop_requested())
			break;

		if (umap<string, uintptr_t>::iterator fit = files.find(archive_entry_pathname_utf8(entry)); fit != files.end()) {
			if (SDL_Surface* img = World::drawSys()->getRenderer()->makeCompatible(FileOps::loadArchivePicture(arch, entry), true)) {
				rp->mpic.lock();
				rp->pics.emplace_back(filename(fit->first), nullptr);
				rp->mpic.unlock();
				m += fit->second;
				++c;
				pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(rp, img, c - 1), progressText(limitToStr(picLim, c, m, sizMag), progLim));
			}
			files.erase(fit);
		}
	}
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp);
}

umap<string, uintptr_t> Browser::mapArchiveFiles(string_view dir, const vector<ArchiveFile>& files, const PicLim& picLim, uint compress, string_view firstPic, bool fwd) {
	size_t start = 0;
	if (picLim.type != PicLim::Type::none)
		if (vector<ArchiveFile>::const_iterator it = std::lower_bound(files.begin(), files.end(), firstPic, [](const ArchiveFile& a, string_view b) -> bool { return StrNatCmp::less(a.name, b); }); it != files.end() && it->name == firstPic)
			start = it - files.begin();

	vector<pair<string, uintptr_t>> stage;
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		stage.reserve(files.size());
		for (size_t i = 0; i < files.size(); ++i)
			if (files[i].size)
				stage.emplace_back(dir + files[i].name, files[i].size);
		break;
	case count:
		stage.reserve(picLim.getCount());
		for (size_t i = start, mov = btom<size_t>(fwd); i < files.size() && stage.size() < picLim.getCount(); i += mov)
			if (files[i].size)
				stage.emplace_back(dir + files[i].name, files[i].size);
		break;
	case size: {
		stage.reserve(files.size());
		uintptr_t m = 0;
		for (size_t i = start, mov = btom<size_t>(fwd); i < files.size() && m < picLim.getSize(); i += mov)
			if (files[i].size) {
				stage.emplace_back(dir + files[i].name, files[i].size);
				m += files[i].size / compress;
			}
	} }
	return umap<string, uintptr_t>(std::make_move_iterator(stage.begin()), std::make_move_iterator(stage.end()));
}

tuple<size_t, uintptr_t, uint8> Browser::initLoadLimits(const PicLim& picLim, size_t max) {
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		return tuple(max, UINTPTR_MAX, 0);
	case count:
		return tuple(std::min(picLim.getCount(), max), UINTPTR_MAX, 0);
	case size:
		return tuple(max, picLim.getSize(), PicLim::memSizeMag(picLim.getSize()));
	}
	return tuple(0, 0, 0);
}

string Browser::limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, uint8 mag) {
	switch (picLim.type) {
	using enum PicLim::Type;
	case none: case count:
		return toStr(c);
	case size:
		return PicLim::memoryString(m, mag);
	}
	return string();
}

char* Browser::progressText(string_view val, string_view lim) {
	auto text = new char[val.length() + lim.length() + 2];
	rng::copy(val, text);
	text[val.length()] = '/';
	rng::copy(lim, text + val.length() + 1);
	text[val.length() + 1 + lim.length()] = '\0';
	return text;
}
