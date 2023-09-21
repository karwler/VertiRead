#include "browser.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/world.h"
#include "utils/compare.h"
#ifdef _WIN32
#include <SDL_image.h>
#else
#include <SDL2/SDL_image.h>
#endif
#include <archive.h>
#include <archive_entry.h>

// BROWSER

Browser::Browser(string&& root, string&& directory, PCall exit) :
	exCall(exit),
	rootDir(std::move(root)),
	curDir(std::move(directory)),
	fwatch(curDir.c_str())
{}

uptr<Browser> Browser::openExplorer(string&& rootDir, string&& curDir, PCall exitCall) {
	try {
		if (rootDir != topDir && !fs::is_directory(toPath(rootDir)))
			throw std::runtime_error(std::format("Browser root '{}' isn't a directory", rootDir));
		if (curDir.empty())
			curDir = rootDir;
		if (curDir != topDir && !fs::is_directory(toPath(curDir)))
			throw std::runtime_error(std::format("Browser directory '{}' isn't a directory", curDir));
		if (!isSubpath(curDir, rootDir))
			throw std::runtime_error(std::format("Browser directory '{}' isn't a subpath of root '{}'", curDir, rootDir));
		return std::make_unique<Browser>(std::move(rootDir), std::move(curDir), exitCall);
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
	return nullptr;
}

bool Browser::openPicture(string&& rdir, string&& dirc, string&& file) {
	try {
		if (rdir != topDir && (!fs::is_directory(toPath(rdir)) || !isSubpath(file, rdir)))
			rdir = topDir;

		switch (fs::status(toPath(dirc)).type()) {
		using enum fs::file_type;
		case regular:
			if (FileSys::isArchive(dirc.c_str())) {
				startArchive(BrowserResultAsync(std::move(rdir), std::move(dirc), std::move(file)));
				return true;
			}
			break;
		case directory:
			startLoadPictures(new BrowserResultPicture(false, std::move(rdir), std::move(dirc), std::move(file)));
			return true;
		}
		logError(dirc, " isn't a valid path");
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
	}
	return false;
}

Browser::Response Browser::openFile(string&& file) {
	try {
		switch (fs::status(toPath(file)).type()) {
		using enum fs::file_type;
		case regular:
			if (FileSys::isPicture(file.c_str())) {
				startLoadPictures(new BrowserResultPicture(false, isSubpath(file, rootDir) ? string() : topDir, string(parentPath(file)), string(filename(file))));
				return RWAIT;
			}
			if (FileSys::isArchive(file.c_str())) {
				startArchive(BrowserResultAsync(isSubpath(file, rootDir) ? string() : topDir, string(file)));
				return RWAIT;
			}
			if (string_view dirc = parentPath(file); fs::is_directory(toPath(dirc))) {
				rootDir = topDir;
				curDir = dirc;
				fwatch.set(curDir.c_str());
				return REXPLORER;
			}
			break;
		case directory:
			rootDir = topDir;
			curDir = std::move(file);
			fwatch.set(curDir.c_str());
			return REXPLORER;
		}
		logError(file, " isn't a valid path");
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
	}
	return RERROR;
}

Browser::Response Browser::goTo(const char* path) {
	try {
		switch (fs::status(toPath(path)).type()) {
		using enum fs::file_type;
		case regular:
			if (FileSys::isPicture(path)) {
				startLoadPictures(new BrowserResultPicture(false, isSubpath(path, rootDir) ? string() : topDir, string(parentPath(path)), string(filename(path))));
				return RWAIT | REXPLORER;
			}
			if (FileSys::isArchive(path)) {
				bool sub = isSubpath(path, rootDir);
				startArchive(BrowserResultAsync(sub ? string() : topDir, path));
				return RWAIT;
			}
			break;
		case directory:
			if (!isSubpath(path, rootDir))
				rootDir = topDir;
			curDir = path;
			arch.clear();
			curNode = nullptr;
			fwatch.set(curDir.c_str());
			return REXPLORER;
		}
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
	}
	return RERROR;
}

bool Browser::goIn(string_view dname) {
	try {
		if (curNode) {
			if (vector<ArchiveDir>::iterator dit = curNode->findDir(dname); dit != curNode->dirs.end()) {
				curNode = std::to_address(dit);
				return true;
			}
		} else if (string newPath = curDir / dname; fs::is_directory(toPath(newPath))) {
			curDir = std::move(newPath);
			fwatch.set(curDir.c_str());
			return true;
		}
	} catch (const fs::filesystem_error& err) {
		logError(err.what());
	}
	return false;
}

bool Browser::goFile(string_view fname) {
	if (curNode) {
		if (vector<ArchiveFile>::iterator fit = curNode->findFile(fname); fit != curNode->files.end() && fit->size) {
			startLoadPictures(curNode->path() + fname);
			return true;
		}
	} else if (string path = curDir / fname; FileSys::isPicture(path.c_str())) {
		startLoadPictures(string(fname));
		return true;
	} else if (FileSys::isArchive(path.c_str())) {
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
			fwatch.set(curDir.c_str());
		}
	} else {
		curDir = parentPath(curDir);
		fwatch.set(curDir.c_str());
	}
	return true;
}

bool Browser::goNext(bool fwd, string_view picname) {
	try {
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
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
	return false;
}

void Browser::shiftDir(bool fwd) {
	if (curDir != rootDir) {
		// find current directory and set it to the path of the next valid directory in the parent directory
		string dir(parentPath(curDir));
		string_view dname = filename(curDir);
		vector<string> dirs = FileSys::listDir(dir.c_str(), false, true, World::sets()->showHidden);
		if (vector<string>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), dname, StrNatCmp()); di != dirs.end() && *di == dname)
			foreachAround(dirs, di, fwd, &Browser::nextDir, dir);
	}
}

bool Browser::nextDir(string_view dit, const string& pdir) {
	string idir = pdir / dit;
	vector<string> files = FileSys::listDir(idir.c_str(), true, false, World::sets()->showHidden);
	if (vector<string>::iterator fi = rng::find_if(files, [&idir](const string& it) -> bool { return FileSys::isPicture((idir / it).c_str()); }); fi != files.end()) {
		curDir = std::move(idir);
		fwatch.set(curDir.c_str());
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
		vector<string> files = FileSys::listDir(pdir.c_str(), true, false, World::sets()->showHidden);
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
	if (string path = pdir / ait; FileSys::isPictureArchive(path.c_str())) {
		curDir = std::move(path);
		fwatch.set(curDir.c_str());
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
		vector<string> files = FileSys::listDir(curDir.c_str(), true, false, World::sets()->showHidden);
		vector<string>::iterator fit = std::lower_bound(files.begin(), files.end(), file, StrNatCmp());
		for (size_t mov = btom<size_t>(fwd), i = fit - files.begin() + (fit != files.end() && (*fit == file || !fwd) ? mov : 0); i < files.size(); i += mov)
			if (FileSys::isPicture((curDir / files[i]).c_str()))
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
	return FileSys::listDirSep(curDir.c_str(), World::sets()->showHidden);
}

pair<vector<pair<bool, string>>, bool> Browser::directoryUpdate() {
	pair<vector<pair<bool, string>>, bool> res = fwatch.changed();
	if (curNode && res.second) {
		arch.clear();
		curNode = &arch;
	}
	return res;
}

void Browser::stopThread() {
	if (thread.joinable()) {
		ThreadType handling = threadType;
		threadType = ThreadType::none;
		thread.join();

		switch (handling) {
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
	}
}

void Browser::startArchive(BrowserResultAsync&& ra) {
	stopThread();
	threadType = ThreadType::archive;
	thread = std::thread(FileSys::makeArchiveTreeThread, std::ref(threadType), std::move(ra), World::sets()->maxPicRes);
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
	fwatch.set(curDir.c_str());
	return true;
}

void Browser::cleanupArchive() {
	SDL_FlushEvent(SDL_USEREVENT_ARCHIVE_PROGRESS);
	cleanupEvent(SDL_USEREVENT_ARCHIVE_FINISHED, [](SDL_UserEvent& user) { delete static_cast<BrowserResultAsync*>(user.data1); });
}

void Browser::startPreview(const vector<string>& files, const vector<string>& dirs, int maxHeight) {
	stopThread();
	threadType = ThreadType::preview;
	thread = curNode
		? std::thread(&Browser::previewArchThread, std::ref(threadType), curDir, sliceArchiveFiles(*curNode), curNode->path(), fromPath(World::fileSys()->dirIcons()), maxHeight)
		: std::thread(&Browser::previewDirThread, std::ref(threadType), curDir, files, dirs, fromPath(World::fileSys()->dirIcons()), World::sets()->showHidden, maxHeight);
}

void Browser::cleanupPreview() {
	cleanupEvent(SDL_USEREVENT_PREVIEW_PROGRESS, [](SDL_UserEvent& user) {
		delete[] static_cast<char*>(user.data1);
		SDL_FreeSurface(static_cast<SDL_Surface*>(user.data2));
	});
	SDL_FlushEvent(SDL_USEREVENT_PREVIEW_FINISHED);
}

void Browser::previewDirThread(std::atomic<ThreadType>& mode, string curDir, vector<string> files, vector<string> dirs, string iconPath, bool showHidden, int maxHeight) {
	if (uptr<SDL_Surface> dicon(!dirs.empty() ? World::renderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / "folder.svg").c_str(), maxHeight), false) : nullptr); dicon)
		for (const string& it : dirs) {
			if (mode != ThreadType::preview)
				return;

			string dpath = curDir / it;
			for (const string& sit : FileSys::listDir(dpath.c_str(), true, false, showHidden))
				if (SDL_Surface* img = combineIcons(dicon.get(), IMG_Load((dpath / sit).c_str()))) {
					pushEvent(SDL_USEREVENT_PREVIEW_PROGRESS, allocatePreviewName(it, false), img);
					break;
				}
		}
	for (const string& it : files) {
		if (mode != ThreadType::preview)
			return;

		if (string fpath = curDir / it; SDL_Surface* img = scaleDown(IMG_Load(fpath.c_str()), maxHeight))
			pushEvent(SDL_USEREVENT_PREVIEW_PROGRESS, allocatePreviewName(it, true), img);
		else if (vector<string> entries = FileSys::listArchiveFiles(fpath.c_str()); !entries.empty())
			for (const string& ei : entries)
				if (img = scaleDown(FileSys::loadArchivePicture(fpath.c_str(), ei), maxHeight); img) {
					pushEvent(SDL_USEREVENT_PREVIEW_PROGRESS, allocatePreviewName(it, true), img);
					break;
				}
	}
	pushEvent(SDL_USEREVENT_PREVIEW_FINISHED);
}

void Browser::previewArchThread(std::atomic<ThreadType>& mode, string curDir, ArchiveDir root, string dir, string iconPath, int maxHeight) {
	uptr<SDL_Surface> dicon(!root.dirs.empty() ? World::renderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / "folder.svg").c_str(), maxHeight), false) : nullptr);
	umap<string, bool> entries;
	entries.reserve(dicon ? root.dirs.size() + root.files.size() : root.files.size());
	if (dicon)
		for (const ArchiveDir& it : root.dirs)
			if (vector<ArchiveFile>::const_iterator fit = rng::find_if(it.files, [](const ArchiveFile& af) -> bool { return af.size; }); fit != it.files.end())
				entries.emplace(dir + it.name + '/' + fit->name, false);
	for (const ArchiveFile& it : root.files)
		if (it.size)
			entries.emplace(dir + it.name, true);

	if (archive* arch = FileSys::openArchive(curDir.c_str())) {
		for (archive_entry* entry; !entries.empty() && !archive_read_next_header(arch, &entry);) {
			if (mode != ThreadType::preview) {
				archive_read_free(arch);
				return;
			}

			if (umap<string, bool>::iterator pit = entries.find(archive_entry_pathname_utf8(entry)); pit != entries.end()) {
				if (SDL_Surface* img = pit->second ? scaleDown(FileSys::loadArchivePicture(arch, entry), maxHeight) : combineIcons(dicon.get(), FileSys::loadArchivePicture(arch, entry)))
					pushEvent(SDL_USEREVENT_PREVIEW_PROGRESS, allocatePreviewName(pit->first, pit->second), img);
				entries.erase(pit);
			}
		}
		archive_read_free(arch);
	}
	pushEvent(SDL_USEREVENT_PREVIEW_FINISHED);
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
	return World::renderer()->makeCompatible(img, false);
}

char* Browser::allocatePreviewName(string_view name, bool file) {
	char* str = new char[name.length() + 2];
	str[0] = file;
	std::copy_n(name.data(), name.length() + 1, str + 1);
	return str;
}

void Browser::startLoadPictures(BrowserResultPicture* rp, bool fwd) {
	try {
		stopThread();
		threadType = ThreadType::reader;
		uint compress = World::sets()->compression != Settings::Compression::b16 ? 1 : 2;
		if (rp->archive) {
			umap<string, uintptr_t> files = mapArchiveFiles(curNode->path(), curNode->files, World::sets()->picLim, compress, filename(rp->file), fwd);
			thread = std::thread(&Browser::loadPicturesArchThread, std::ref(threadType), rp, std::move(files), World::sets()->picLim);
		} else
			thread = std::thread(&Browser::loadPicturesDirThread, std::ref(threadType), rp, World::sets()->picLim, compress, fwd, World::sets()->showHidden);
	} catch (const std::system_error&) {
		delete rp;
		throw;
	}
}

void Browser::finishLoadPictures(BrowserResultPicture& rp) {
	if (!rp.rootDir.empty()) {
		rootDir = std::move(rp.rootDir);
		curDir = std::move(rp.curDir);
		arch.clear();
		curNode = nullptr;
		fwatch.set(curDir.c_str());
		if (rp.archive) {
			arch = std::move(rp.arch);
			auto [dir, fil] = arch.find(rp.file);
			curNode = coalesce(dir, &arch);
		}
	}
}

void Browser::cleanupLoadPictures() {
	cleanupEvent(SDL_USEREVENT_READER_PROGRESS, [](SDL_UserEvent& user) {
		BrowserPictureProgress* pp = static_cast<BrowserPictureProgress*>(user.data1);
		SDL_FreeSurface(pp->img);
		delete pp;
		delete[] static_cast<char*>(user.data2);
	});
	cleanupEvent(SDL_USEREVENT_READER_FINISHED, [](SDL_UserEvent& user) {
		if (BrowserResultPicture* rp = static_cast<BrowserResultPicture*>(user.data1)) {
			for (auto& [name, tex] : rp->pics)
				if (tex)
					World::renderer()->freeTexture(tex);
			delete rp;
		}
	});
}

void Browser::loadPicturesDirThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, PicLim picLim, uint compress, bool fwd, bool showHidden) {
	vector<string> files = FileSys::listDir(rp->curDir.c_str(), true, false, showHidden);
	size_t start = fwd ? 0 : files.size() - 1;
	if (picLim.type != PicLim::Type::none)
		if (vector<string>::iterator it = rng::lower_bound(files, rp->file, StrNatCmp()); it != files.end() && *it == rp->file)
			start = it - files.begin();
	auto [lim, mem, sizMag] = initLoadLimits(picLim, fwd ? files.size() - start : start + 1);	// picture count limit, picture size limit, magnitude index
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	uintptr_t m = 0;

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	for (size_t mov = btom<uintptr_t>(fwd), i = start, c = 0; i < files.size() && c < lim && m < mem; i += mov) {
		if (mode != ThreadType::reader)
			break;

		if (SDL_Surface* img = World::renderer()->makeCompatible(IMG_Load((rp->curDir / files[i]).c_str()), true)) {
			rp->mpic.lock();
			rp->pics.emplace_back(std::move(files[i]), nullptr);
			rp->mpic.unlock();
			m += uintptr_t(img->w) * uintptr_t(img->h) * 4 / compress;
			++c;
			pushEvent(SDL_USEREVENT_READER_PROGRESS, new BrowserPictureProgress(rp, img, c - 1), progressText(limitToStr(picLim, c, m, sizMag), progLim));
		}
	}
	pushEvent(SDL_USEREVENT_READER_FINISHED, rp, std::bit_cast<void*>(uintptr_t(fwd)));
}

void Browser::loadPicturesArchThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, umap<string, uintptr_t> files, PicLim picLim) {
	auto [lim, mem, sizMag] = initLoadLimits(picLim, files.size());
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	size_t c = 0;
	uintptr_t m = 0;
	archive* arch = FileSys::openArchive(rp->curDir.c_str());
	if (!arch) {
		delete rp;
		pushEvent(SDL_USEREVENT_READER_FINISHED);
		return;
	}

	for (archive_entry* entry; !files.empty() && !archive_read_next_header(arch, &entry);) {
		if (mode != ThreadType::reader)
			break;

		if (umap<string, uintptr_t>::iterator fit = files.find(archive_entry_pathname_utf8(entry)); fit != files.end()) {
			if (SDL_Surface* img = World::renderer()->makeCompatible(FileSys::loadArchivePicture(arch, entry), true)) {
				rp->mpic.lock();
				rp->pics.emplace_back(filename(fit->first), nullptr);
				rp->mpic.unlock();
				m += fit->second;
				++c;
				pushEvent(SDL_USEREVENT_READER_PROGRESS, new BrowserPictureProgress(rp, img, c - 1), progressText(limitToStr(picLim, c, m, sizMag), progLim));
			}
			files.erase(fit);
		}
	}
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_READER_FINISHED, rp);
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
	char* text = new char[val.length() + lim.length() + 2];
	rng::copy(val, text);
	text[val.length()] = '/';
	rng::copy(lim, text + val.length() + 1);
	text[val.length() + 1 + lim.length()] = '\0';
	return text;
}
