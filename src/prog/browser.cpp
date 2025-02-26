#include "browser.h"
#include "fileOps.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/world.h"
#include "engine/optional/mupdf.h"
#include "engine/optional/poppler.h"
#include "utils/compare.h"
#ifdef WITH_ARCHIVE
#include <archive_entry.h>
#endif
#include <functional>

Browser::ListDirData::ListDirData(FileOps* fs, string loc, BrowserListOption options) noexcept :
	fsop(fs),
	path(std::move(loc)),
	opts(options)
{}

Browser::GoNextData::GoNextData(string&& pname, bool forward) noexcept :
	picname(std::move(pname)),
	fwd(forward)
{}

Browser::PreviewDirData::PreviewDirData(FileOps* fs, string&& cdir, string&& iloc, int isize, bool hidden) noexcept :
	fsop(fs),
	curDir(std::move(cdir)),
	iconPath(std::move(iloc)),
	maxHeight(isize),
	showHidden(hidden)
{}

#ifdef WITH_ARCHIVE
Browser::PreviewArchData::PreviewArchData(FileOps* fs, ArchiveData&& as, string&& cdir, string&& iloc, int isize) noexcept :
	fsop(fs),
	slice(std::move(as)),
	curDir(std::move(cdir)),
	iconPath(std::move(iloc)),
	maxHeight(isize)
{}
#endif

Browser::LoadPicturesDirData::LoadPicturesDirData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, bool hidden) noexcept :
	fsop(fs),
	rp(std::move(res)),
	picLim(plim),
	showHidden(hidden)
{}

#ifdef WITH_ARCHIVE
Browser::LoadPicturesArchData::LoadPicturesArchData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim) noexcept :
	fsop(fs),
	rp(std::move(res)),
	picLim(plim)
{}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
Browser::LoadPicturesPdfData::LoadPicturesPdfData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, float ddpi) noexcept :
	fsop(fs),
	rp(std::move(res)),
	picLim(plim),
	dpi(ddpi)
{}
#endif

Browser::~Browser() {
	stopThread();
	delete fsop;
#ifdef CAN_MUPDF
	closeMupdf();
#endif
#ifdef CAN_POPPLER
	closePoppler();
#endif
}

bool Browser::isLocal() const noexcept {
	return dynamic_cast<FileOpsLocal*>(fsop);
}

string Browser::prepareNavigationPath(string_view path) const {
	if (!path.empty() && RemoteLocation::getProtocol(path) == Protocol::none && !isAbsolute(path)) {
		if (rootDir == fsop->prefix()) {
			if (string cwd = FileSys::currentDirectory(); !cwd.empty())
				return cwd / path;
		} else if (auto sep = rng::find_if(path, isDsep); filename(rootDir) == string_view(path.begin(), sep))
			return rootDir / string_view(std::find_if(sep, path.end(), notDsep), path.end());
	}
	return string(path);
}

uptr<RemoteLocation> Browser::prepareFileOps(string_view path) {
	Protocol proto = RemoteLocation::getProtocol(path);
	if (proto == Protocol::none) {
		if (!isLocal()) {	// if remote to local, else local to local
			delete fsop;
			fsop = nullptr;
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

void Browser::beginFs(string&& root, const RemoteLocation& location, vector<string>&& passwords) {
	beginRemoteOps(location, std::move(passwords), [this, &root](FileOps* backup, const RemoteLocation& rl) {
		beginFs(std::move(root), valcp(rl.path));
		delete backup;
	});
}

void Browser::beginFs(string&& root, string&& path) {
	if (root.empty())
		root = fsop->prefix();
	else if (!fsop->isDirectory(root))
		throw std::runtime_error(fmt::format("Browser root '{}' isn't a directory", root));

	if (!path.empty()) {
		if (!fsop->isDirectory(path))
			throw std::runtime_error(fmt::format("Browser directory '{}' isn't a directory", path));
		if (!isSubpath(path, root))
			throw std::runtime_error(fmt::format("Browser directory '{}' isn't a subpath of root '{}'", path, root));
		curDir = std::move(path);
	} else
		curDir = root;
	rootDir = std::move(root);
	arch = ArchiveData();
	fsop->unsetWatch();
}

bool Browser::goTo(const RemoteLocation& location, vector<string>&& passwords) {
	return beginRemoteOps(location, std::move(passwords), [this](FileOps* backup, const RemoteLocation& rl) -> bool {
		bool wait = goTo(rl.path);
		delete backup;
		return wait;
	});
}

bool Browser::goTo(const string& path) {
	switch (fsop->fileType(path)) {
	using enum FileOps::FileType;
	case regular:
		if (fsop->isPicture(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_FWD, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), string(parentPath(path)), string(filename(path))));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_FWD, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), valcp(path)));
			return true;
		}
#ifdef WITH_ARCHIVE
		if (ArchiveData ad(path); fsop->isArchive(ad)) {
			startArchive(std::make_unique<BrowserResultArchive>(isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), std::move(ad)));
			return true;
		}
#endif
		break;
	case directory:
		if (!isSubpath(path, rootDir))
			rootDir = fsop->prefix();
		curDir = path;
		arch = ArchiveData();
		fsop->unsetWatch();
		return false;
	}
	throw std::runtime_error(fmt::format("Invalid path '{}'", path));
}

bool Browser::openPicture(string&& rdir, stvector<string, Settings::maxPageElements>&& paths) {
	if (string prefix = fsop->prefix(); rdir != prefix && !isSubpath(paths[0], rdir))
		rdir = fsop->prefix();

	switch (fsop->fileType(paths[0])) {
	using enum FileOps::FileType;
	case regular:
		if (fsop->isPdf(paths[0])) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_FWD, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
			return true;
		}
#ifdef WITH_ARCHIVE
		if (ArchiveData ad(paths[0]); fsop->isArchive(ad)) {
			startArchive(std::make_unique<BrowserResultArchive>(std::move(rdir), std::move(ad), paths.size() > 1 ? std::move(paths[1]) : string(), paths.size() > 2 ? std::move(paths[2]) : string()));
			return true;
		}
#endif
		break;
	case directory:
		startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_FWD, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
		return true;
	}
	return false;
}

bool Browser::goIn(string_view dname) {
	string path = curDir / dname;
#ifdef WITH_ARCHIVE
	if (arch) {
		if (auto [dir, fil] = arch.find(path); dir && !fil) {
			curDir = std::move(path);
			return true;
		}
	} else
#endif
	if (fsop->isDirectory(path)) {
		curDir = std::move(path);
		fsop->unsetWatch();
		return true;
	}
	return false;
}

bool Browser::goFile(string_view fname) {
	string path = curDir / fname;
#ifdef WITH_ARCHIVE
	if (arch) {
		if (auto [dir, fil] = arch.find(path); fil && (fil->isPic || fil->isPdf)) {
			startLoadPictures(fil->isPdf
				? std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_FWD, std::nullopt, std::move(path), string(), arch.copyLight())
				: std::make_unique<BrowserResultPicture>(BRS_FWD, std::nullopt, valcp(curDir), string(fname), arch.copyLight()));
			return true;
		}
	} else
#endif
	if (fsop->isRegular(path)) {
		if (fsop->isPicture(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_FWD, std::nullopt, valcp(curDir), string(fname)));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_FWD, std::nullopt, std::move(path)));
			return true;
		}
#ifdef WITH_ARCHIVE
		if (ArchiveData ad(path); fsop->isArchive(ad)) {
			startArchive(std::make_unique<BrowserResultArchive>(std::nullopt, std::move(ad)));
			return true;
		}
#endif
	}
	return false;
}

bool Browser::goUp() {
#ifdef WITH_ARCHIVE
	if (arch) {
		if (!curDir.empty())
			curDir = parentPath(curDir);
		else {
			curDir = parentPath(arch.name.data());
			arch = ArchiveData();
			fsop->unsetWatch();
		}
		return true;
	}
#endif
	if (pathEqual(curDir, rootDir))
		return false;

	curDir = parentPath(curDir);
	fsop->unsetWatch();
	return true;
}

void Browser::startGoNext(string&& picname, bool fwd) {
	stopThread();
	curThread = ThreadType::next;
	thread = std::jthread(std::bind_front(&Browser::goNextThread, this), std::make_unique<GoNextData>(std::move(picname), fwd));
}

void Browser::goNextThread(std::stop_token stoken, uptr<GoNextData> gd) noexcept {
	uptr<BrowserResultPicture> rp;
	string file;
	try {
		if (!gd->picname.empty()) {	// move to the next batch of pictures if the given picture name can be found
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
			if (pdf) {
				if (int pcnt = pdf.numPages(); pcnt > 0)
					if (uint npage = toNum<uint>(gd->picname) + btom<int>(gd->fwd); npage < uint(pcnt))
						file = toStr(npage);
			} else
#endif
#ifdef WITH_ARCHIVE
			if (arch) {
				if (ArchiveDir* dir = arch.find(curDir).first) {
					vector<ArchiveFile*> files = dir->listFiles();
					if (auto fit = std::lower_bound(files.begin(), files.end(), gd->picname, [](const ArchiveFile* a, string_view b) -> bool { return Strcomp::less(a->name.data(), b); }); fit != files.end())
						for (size_t mov = btom<size_t>(gd->fwd), i = fit - files.begin() + ((*fit)->name == gd->picname ? mov : 0); i < files.size(); i += mov)
							if (files[i]->isPic) {
								file = files[i]->name.data();
								break;
							}
				}
			} else
#endif
			{
				vector<Cstring> files = fsop->listDirectory(stoken, curDir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
				if (auto fit = std::lower_bound(files.begin(), files.end(), gd->picname, [](const Cstring& a, string_view b) -> bool { return Strcomp::less(a.data(), b); }); fit != files.end())
					for (size_t mov = btom<size_t>(gd->fwd), i = fit - files.begin() + (*fit == gd->picname ? mov : 0); i < files.size(); i += mov)
						if (fsop->isPicture(curDir / files[i].data())) {
							file = files[i].data();
							break;
						}
			}
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	try {
		if (!file.empty())
			rp = std::make_unique<BrowserResultPicture>(gd->fwd ? BRS_FWD : BRS_NONE, std::nullopt, valcp(curDir), std::move(file), arch.copyLight(), pdf.copyLight());
		else {	// move to the next container if either no picture name was given or the given one wasn't found
			string pdir(parentPath(curDir));
			const char* cname = filenamePtr(curDir);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
			if (pdf) {
#ifdef WITH_ARCHIVE
				if (arch) {
					if (auto [dir, fil] = arch.find(curDir); fil) {
						vector<ArchiveFile*> files = dir->listFiles();
						if (auto fi = std::lower_bound(files.begin(), files.end(), cname, [](const ArchiveFile* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); fi != files.end())
							if (fi = foreachAround(files, fi, (*fi)->name == cname, gd->fwd, [](const ArchiveFile* af) -> bool { return af->isPdf; }); fi != files.end())
								rp = std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / (*fi)->name.data(), string(), arch.copyLight());
					}
				} else
#endif
				{
					vector<Cstring> files = fsop->listDirectory(stoken, pdir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
					if (auto fi = std::lower_bound(files.begin(), files.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); fi != files.end())
						if (fi = foreachAround(files, fi, *fi == cname, gd->fwd, [this, &pdir](const Cstring& fn) -> bool { return fsop->isPdf(pdir / fn.data()); }); fi != files.end())
							rp = std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / fi->data());
				}
			} else
#endif
#ifdef WITH_ARCHIVE
			if (arch) {
				if (ArchiveDir* dir = arch.find(pdir).first; dir && dir != &arch) {
					vector<ArchiveDir*> dirs = dir->listDirs();
					if (auto di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const ArchiveDir* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); di != dirs.end())
						if (di = foreachAround(dirs, di, (*di)->name == cname, gd->fwd, [](const ArchiveDir* ad) -> bool { return rng::any_of(ad->files, [](const ArchiveFile& it) -> bool { return it.isPic; }); }); di != dirs.end())
							rp = std::make_unique<BrowserResultPicture>(BRS_LOC | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / (*di)->name.data(), string(), arch.copyLight());
				}
			} else
#endif
			if (!pathEqual(curDir, rootDir)) {
				vector<Cstring> dirs = fsop->listDirectory(stoken, pdir, BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).dirs;
				if (auto di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); di != dirs.end()) {
					if (di = foreachAround(dirs, di, *di == cname, gd->fwd, [this, stoken, &pdir](const Cstring& fn) -> bool {
						string idir = pdir / fn.data();
						return rng::any_of(fsop->listDirectory(stoken, idir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files, [this, &idir](const Cstring& it) -> bool { return fsop->isPicture(idir / it.data()); });
					}); di != dirs.end())
						rp = std::make_unique<BrowserResultPicture>(BRS_LOC | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / di->data());
				}
			}
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	ResultCode rc = stoken.stop_requested() ? ResultCode::stop : rp ? ResultCode::ok : ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_GO_NEXT_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)), rp.release());
}

template <class T, class F>
vector<T>::iterator Browser::foreachAround(vector<T>& vec, vector<T>::iterator start, bool found, bool fwd, F check) {
	if (vec.empty())
		return vec.end();

	if (fwd) {
		typename vector<T>::iterator rs;
		if (rs = std::find_if(start + found, vec.end(), check); rs == vec.end())
			if (rs = std::find_if(vec.begin(), start, check); rs == start)
				return vec.end();
		return rs;
	}
	typename vector<T>::reverse_iterator rv, rstart = std::make_reverse_iterator(start + !found);
	if (rv = std::find_if(rstart, vec.rend(), check); rv == vec.rend())
		if (rv = std::find_if(vec.rbegin(), rstart, check); rv == rstart)
			return vec.end();
	return rv.base() - 1;
}

void Browser::exitFile() {
	if (pdf) {
		curDir = parentPath(curDir);
		pdf = PdfFile();
	}
}

string Browser::locationForDisplay() const {
#ifdef WITH_ARCHIVE
	string path = arch ? !curDir.empty() ? arch.name.data() / curDir : arch.name.data() : curDir;
#else
	string path = curDir;
#endif
	if (rootDir != fsop->prefix())
		if (string_view rpath = relativePath(path, parentPath(rootDir)); !rpath.empty())
			return string(rpath);
	return path;
}

stvector<string, Settings::maxPageElements> Browser::locationForStore(string_view pname) const {
	stvector<string, Settings::maxPageElements> paths = { dotStr };
#ifdef WITH_ARCHIVE
	if (arch)
		paths.emplace_back(arch.name.data());
#endif
	paths.push_back(curDir);
	if (!pname.empty())
		paths.emplace_back(pname);

	if (rootDir != fsop->prefix())
		if (string_view rpath = relativePath(paths[1], parentPath(rootDir)); !rpath.empty()) {
			auto sep = rng::find_if(rpath, isDsep);
			paths[0].assign(rpath.begin(), sep);
			paths[1].assign(std::find_if(sep, rpath.end(), notDsep), rpath.end());
		}
	return paths;
}

void Browser::startListCurDir(bool files) {
	stopThread();
	curThread = ThreadType::list;
#ifdef WITH_ARCHIVE
	if (arch) {
		uptr<ListArchData> ld = std::make_unique<ListArchData>((files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE));
		if (ArchiveDir* dir = arch.find(curDir).first)
			ld->slice.copySlicedDentsFrom(*dir, World::sets()->showHidden);
		thread = std::jthread(&Browser::listDirArchThread, std::move(ld));
	} else
#endif
	{
		thread = std::jthread(&Browser::listDirFsThread, std::make_unique<ListDirData>(fsop, valcp(curDir), (files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)));
	}
}

void Browser::startListDir(string&& path, bool files) {
	stopThread();
	curThread = ThreadType::list;
	thread = std::jthread(&Browser::listDirFsThread, std::make_unique<ListDirData>(fsop, std::move(path), (files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)));
}

void Browser::listDirFsThread(std::stop_token stoken, uptr<ListDirData> ld) noexcept {
	listDirThread(stoken, [&stoken, &ld]() -> uptr<BrowserResultList> { return std::make_unique<BrowserResultList>(ld->fsop->listDirectory(stoken, ld->path, ld->opts)); });
}

#ifdef WITH_ARCHIVE
void Browser::listDirArchThread(std::stop_token stoken, uptr<ListArchData> ld) noexcept {
	listDirThread(stoken, [&ld]() -> uptr<BrowserResultList> { return std::make_unique<BrowserResultList>(ld->opts & BLO_FILES ? ld->slice.copySortedFiles(ld->opts & BLO_HIDDEN) : vector<Cstring>(), ld->opts & BLO_DIRS ? ld->slice.copySortedDirs(ld->opts & BLO_HIDDEN) : vector<Cstring>()); });
}
#endif

template <InvocableR<uptr<BrowserResultList>> F>
void Browser::listDirThread(const std::stop_token& stoken, F func) noexcept {
	uptr<BrowserResultList> rl;
	try {
		rl = func();
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	ResultCode rc = stoken.stop_requested() ? ResultCode::stop : rl ? ResultCode::ok : ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_LIST_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)), rl.release());
}

bool Browser::startDeleteEntry(string_view ename) {
	bool ok = !arch;
	if (ok) {
		stopThread();
		curThread = ThreadType::misc;
		thread = std::jthread(std::bind_front(&FileOps::deleteEntryThread, fsop), std::make_unique<string>(curDir / ename));
	}
	return ok;
}

bool Browser::renameEntry(string_view oldName, string_view newName) {
	return !arch && fsop->renameEntry(curDir / oldName, curDir / newName);
}

void Browser::setDirectoryWatch() noexcept {
	if (!arch)	// should be already set if in an archive
		fsop->setWatch(curDir);
}

bool Browser::directoryUpdate(vector<FileChange>& files) noexcept {
	bool gone = fsop->pollWatch(files);
	if (arch && gone) {
		curDir.clear();
		arch = ArchiveData();
	}
	return gone;
}

void Browser::stopThread() noexcept {
	if (thread.joinable()) {
		thread = std::jthread();
		switch (curThread) {
		using enum ThreadType;
		case list:
			cleanupEvent(SDL_USEREVENT_THREAD_LIST_FINISHED);
			break;
		case preview:
			cleanupEvent(SDL_USEREVENT_THREAD_PREVIEW);
			break;
		case reader:
			cleanupEvent(SDL_USEREVENT_THREAD_READER);
			break;
		case next:
			cleanupEvent(SDL_USEREVENT_THREAD_GO_NEXT_FINISHED);
			break;
#ifdef WITH_ARCHIVE
		case archive:
			cleanupEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED);
#endif
		}
		curThread = ThreadType::none;
	}
}

#ifdef WITH_ARCHIVE
void Browser::startArchive(uptr<BrowserResultArchive>&& ra) {
	stopThread();
	curThread = ThreadType::archive;
	thread = std::jthread(std::bind_front(&FileOps::makeArchiveTreeThread, fsop), std::make_unique<FileOps::MakeArchiveTreeData>(std::move(ra), World::sets()->maxPicRes));
}

bool Browser::finishArchive(BrowserResultArchive&& ra) {
	auto [dir, fil] = ra.arch.find(ra.opath);
	if (fil) {
		startLoadPictures(fil->isPdf
			? std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_ARCH | BRS_FWD, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, std::move(ra.opath), std::move(ra.page), std::move(ra.arch))
			: std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_ARCH | BRS_FWD, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, string(parentPath(ra.opath)), fil->name.data(), std::move(ra.arch)));
		return false;
	}
	if (ra.hasRootDir)
		rootDir = std::move(ra.rootDir);
	curDir = parentPath(ra.opath);
	arch = std::move(ra.arch);
	fsop->setWatch(arch.name.data());
	return true;
}
#endif

void Browser::startPreview(int maxHeight) {
	stopThread();
	curThread = ThreadType::preview;
#ifdef WITH_ARCHIVE
	if (arch) {
		ArchiveData ad = arch.copyLight();
		if (auto [dir, fil] = arch.find(curDir); dir)
			ad.copySlicedDentsFrom(*dir, World::sets()->showHidden);
		thread = std::jthread(&Browser::previewArchThread, std::make_unique<PreviewArchData>(fsop, std::move(ad), valcp(curDir), valcp(World::fileSys()->dirIcons()), maxHeight));
	} else
#endif
	{
		thread = std::jthread(&Browser::previewDirThread, std::make_unique<PreviewDirData>(fsop, valcp(curDir), valcp(World::fileSys()->dirIcons()), maxHeight, World::sets()->showHidden));
	}
}

void Browser::previewDirThread(std::stop_token stoken, uptr<PreviewDirData> pd) noexcept {
	try {
		BrowserResultList rl = pd->fsop->listDirectory(stoken, pd->curDir, BLO_FILES | BLO_DIRS | (pd->showHidden ? BLO_HIDDEN : BLO_NONE));
		if (uptr<SDL_Surface> dicon(!rl.dirs.empty() ? World::drawSys()->getRenderer()->prepareImage(World::drawSys()->loadIcon((pd->iconPath / DrawSys::iconName(DrawSys::Tex::folder)).data(), pd->maxHeight)) : nullptr); dicon)
			for (const Cstring& it : rl.dirs) {
				if (stoken.stop_requested())
					return;

				string dpath = pd->curDir / it.data();
				for (const Cstring& sit : pd->fsop->listDirectory(stoken, dpath, BLO_FILES | (pd->showHidden ? BLO_HIDDEN : BLO_NONE)).files)
					if (uptr<SDL_Surface> img(combineIcons(dicon.get(), pd->fsop->loadPicture(dpath / sit.data()))); img) {
						pushPreviewPicture(img, it.data(), false);
						break;
					}
			}
		for (const Cstring& it : rl.files) {
			if (stoken.stop_requested())
				return;

			string fpath = pd->curDir / it.data();
			if (uptr<SDL_Surface> img(scaleDown(pd->fsop->loadPicture(fpath), pd->maxHeight)); img)
				pushPreviewPicture(img, it.data(), true);
#ifdef WITH_ARCHIVE
			else if (ArchiveData ad(fpath, ArchiveData::PassCode::ignore); archive* ap = pd->fsop->openArchive(ad, false)) {
				uptr<archive> arch(ap);
				CountedStopReq csr(stopCheckInterval);
				vector<Cstring> entries;	// list all available files first to get a sorted list
				for (archive_entry* entry; archive_read_next_header(arch.get(), &entry) == ARCHIVE_OK;) {
					if (csr.stopReq(stoken))
						return;
					if (archive_entry_filetype(entry) == AE_IFREG)
						entries.emplace_back(archive_entry_pathname_utf8(entry));
				}
				arch.reset();
				if (img.reset(scaleDown(findArchiveDirectoryThumbnail(stoken, pd->fsop, ad, entries, csr), pd->maxHeight)); img)
					pushPreviewPicture(img, it.data(), true);
			}
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
			else if (PdfFile pdf = pd->fsop->loadPdf(fpath, false))
				previewPdf(stoken, pdf, pd->maxHeight, it.data());
#endif
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

#ifdef WITH_ARCHIVE
void Browser::previewArchThread(std::stop_token stoken, uptr<PreviewArchData> pd) noexcept {
	try {
		CountedStopReq csr(stopCheckInterval);
		uptr<SDL_Surface> dicon(!pd->slice.dirs.empty() ? World::drawSys()->getRenderer()->prepareImage(World::drawSys()->loadIcon((pd->iconPath / DrawSys::iconName(DrawSys::Tex::folder)).data(), pd->maxHeight)) : nullptr);
		pd->slice.pc = ArchiveData::PassCode::attempt;
		if (uptr<archive> arch(pd->fsop->openArchive(pd->slice, false)); arch) {
			umap<ArchiveDir*, vector<Cstring>> dentries;	// available sorted files of each directory
			for (archive_entry* entry; archive_read_next_header(arch.get(), &entry) == ARCHIVE_OK;) {
				if (csr.stopReq(stoken))
					return;

				if (archive_entry_filetype(entry) == AE_IFREG) {
					string_view aepath = archive_entry_pathname_utf8(entry);
					if (auto fit = rng::find_if(pd->slice.files, [&pd, aepath](const ArchiveFile& af) -> bool { return pathEqual(aepath, pd->curDir / af.name.data()); }); fit != pd->slice.files.end()) {
						if (uptr<SDL_Surface> img(scaleDown(FileOps::loadArchivePicture(arch.get(), entry), pd->maxHeight)); img)
							pushPreviewPicture(img, fit->name.data(), true);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
						else if (PdfFile pdf = FileOps::loadArchivePdf(arch.get(), entry, false))
							previewPdf(stoken, pdf, pd->maxHeight, fit->name.data());
#endif
					} else {
						string_view aeparent = parentPath(aepath);
						if (auto dit = rng::find_if(pd->slice.dirs, [&pd, aeparent](const ArchiveDir& ad) -> bool { return pathEqual(aeparent, pd->curDir / ad.name.data()); }); dit != pd->slice.dirs.end())
							dentries[std::to_address(dit)].emplace_back(aepath);
					}
				}
			}
			if (dicon)
				for (ArchiveDir& cdir : pd->slice.dirs) {
					if (stoken.stop_requested())
						return;
					if (auto dfiles = dentries.find(&cdir); dfiles != dentries.end())
						if (uptr<SDL_Surface> img(combineIcons(dicon.get(), findArchiveDirectoryThumbnail(stoken, pd->fsop, pd->slice, dfiles->second, csr))); img)
							pushPreviewPicture(img, dfiles->first->name.data(), false);
				}
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

SDL_Surface* Browser::findArchiveDirectoryThumbnail(const std::stop_token stoken, FileOps* fsop, ArchiveData& ad, vector<Cstring>& entries, CountedStopReq& csr) {
	rng::sort(entries, Strcomp());
	for (size_t i = 0; i < entries.size();) {
		uptr<archive> arch(fsop->openArchive(ad, false));
		if (!arch)
			return nullptr;

		size_t orig = i;
		for (archive_entry* entry; i < entries.size() && archive_read_next_header(arch.get(), &entry) == ARCHIVE_OK;) {
			if (csr.stopReq(stoken))
				return nullptr;
			if (archive_entry_pathname_utf8(entry) == entries[i]) {
				if (SDL_Surface* img = FileOps::loadArchivePicture(arch.get(), entry))
					return img;
				++i;
			}
		}
		i += i == orig;	// unlikely to happen but increment in case there was no name match in a whole archive iteration
	}
	return nullptr;
}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
void Browser::previewPdf(const std::stop_token& stoken, PdfFile& pdfFile, int maxHeight, string_view fname) {
	for (int i = 0, pcnt = pdfFile.numPages(); i < pcnt && !stoken.stop_requested(); ++i)
		if (uptr<SDL_Surface> pic(scaleDown(pdfFile.renderPage(i, 0.2), maxHeight)); pic) {
			pushPreviewPicture(pic, fname, true);
			break;
		}
}
#endif

SDL_Surface* Browser::combineIcons(SDL_Surface* dir, SDL_Surface* img) noexcept {
	if (!img)
		return nullptr;

	ivec2 asiz(dir->w / 4 * 3, dir->h / 3 * 2);
	ivec2 size = img->h - asiz.y >= img->w - asiz.x ? ivec2(float(img->w * asiz.y) / float(img->h), asiz.y) : ivec2(asiz.x, float(img->h * asiz.x) / float(img->w));
	SDL_Surface* dst = SDL_CreateSurface(dir->w, dir->h, surfaceFormat(dir));
	if (dst) {
#ifdef WITH_SDL3
		Renderer::copyPalette(dst, img);
#endif
		SDL_Rect rect = { dir->w - size.x, dir->h - size.y, size.x, size.y };
		if (sdlFailed(SDL_BlitSurface(dir, nullptr, dst, nullptr))
			|| sdlFailed(SDL_FillRect(dst, &rect, SDL_MapSurfaceRGBA(dst, 0, 0, 0, 255)))
			|| sdlFailed(surfaceScaleNearest(img, nullptr, dst, &rect))
		) {
			SDL_FreeSurface(dst);
			dst = nullptr;
		}
	}
	SDL_FreeSurface(img);
	return dst;
}

SDL_Surface* Browser::scaleDown(SDL_Surface* img, int maxHeight) noexcept {
	if (img && img->h > maxHeight)
		if (SDL_Surface* dst = SDL_CreateSurface(float(img->w * maxHeight) / float(img->h), maxHeight, surfaceFormat(img))) {
#ifdef WITH_SDL3
			Renderer::copyPalette(dst, img);
#endif
			if (sdlFailed(surfaceScaleNearest(img, nullptr, dst, nullptr))) {
				SDL_FreeSurface(dst);
				dst = nullptr;
			}
			SDL_FreeSurface(img);
			img = dst;
		}
	return World::drawSys()->getRenderer()->prepareImage(img);
}

void Browser::pushPreviewPicture(uptr<SDL_Surface>& img, string_view name, bool file) {
	auto str = new char[name.length() + 2];
	str[0] = file;
	rng::copy(name, str + 1);
	str[name.length() + 1] = '\0';
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, str, img.release());
}

void Browser::startLoadPictures(uptr<BrowserResultPicture>&& rp) {
	stopThread();
	curThread = ThreadType::reader;
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	if (rp->newPdf || rp->pdf)
		thread = std::jthread(&Browser::loadPicturesPdfThread, std::make_unique<LoadPicturesPdfData>(fsop, std::move(rp), World::sets()->picLim, World::drawSys()->getWinDpi()));
	else
#endif
#ifdef WITH_ARCHIVE
	if (rp->arch) {
		uptr<LoadPicturesArchData> ld = std::make_unique<LoadPicturesArchData>(fsop, std::move(rp), World::sets()->picLim);
		ld->slice.copySlicedDentsFrom(*(ld->rp->newArchive ? ld->rp->arch.find(ld->rp->curDir).first : arch.find(ld->rp->curDir).first), World::sets()->showHidden);	// rp->curDir must be guaranteed to be a valid path
		thread = std::jthread(&Browser::loadPicturesArchThread, std::move(ld));
	} else
#endif
	{
		thread = std::jthread(&Browser::loadPicturesDirThread, std::make_unique<LoadPicturesDirData>(fsop, std::move(rp), World::sets()->picLim, World::sets()->showHidden));
	}
}

void Browser::finishLoadPictures(BrowserResultPicture& rp) {
	if (rp.hasRootDir)
		rootDir = std::move(rp.rootDir);
	if (rp.newCurDir)
		curDir = std::move(rp.curDir);
#ifdef WITH_ARCHIVE
	if (rp.arch) {
		if (rp.newArchive) {
			arch = std::move(rp.arch);
			fsop->setWatch(arch.name.data());
		} else {
			arch.passphrase = std::move(rp.arch.passphrase);
			arch.pc = rp.arch.pc;
		}
	} else
#endif
	{
		arch = ArchiveData();
		if (rp.newCurDir)
			fsop->setWatch(curDir);
	}
	if (rp.newPdf)
		pdf = std::move(rp.pdf);
	if (rp.fwd)
		rp.pics.reverse();	// pictures are gonna be sorted backwards
}

void Browser::loadPicturesDirThread(std::stop_token stoken, uptr<LoadPicturesDirData> ld) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		vector<Cstring> files = ld->fsop->listDirectory(stoken, ld->rp->curDir, BLO_FILES | (ld->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
		size_t start = ld->rp->fwd ? 0 : files.size() - 1;
		if (ld->picLim.type != PicLim::Type::none && !ld->rp->picname.empty())
			if (auto it = std::lower_bound(files.begin(), files.end(), ld->rp->picname, [](const Cstring& a, const string& b) -> bool { return Strcomp::less(a.data(), b.data()); }); it != files.end() && *it == ld->rp->picname)
				start = it - files.begin();

		LoadProgress prg(ld->picLim, ld->rp->fwd ? files.size() - start : start + 1);
		for (size_t mov = btom<size_t>(ld->rp->fwd), i = start; i < files.size() && prg.ok(ld->rp.get()); i += mov) {
			if (stoken.stop_requested()) {
				rc = ResultCode::stop;
				break;
			}
			if (uptr<SDL_Surface> pic(World::drawSys()->getRenderer()->prepareImage(ld->fsop->loadPicture(ld->rp->curDir / files[i].data()), &prg.cbpp)); pic)
				prg.pushImage(ld->rp.get(), std::move(files[i]), pic, ld->picLim);
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}

#ifdef WITH_ARCHIVE
void Browser::loadPicturesArchThread(std::stop_token stoken, uptr<LoadPicturesArchData> ld) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		vector<ArchiveFile*> files = ld->slice.listFiles();
		size_t start = ld->rp->fwd ? 0 : files.size() - 1;
		if (ld->picLim.type != PicLim::Type::none && !ld->rp->picname.empty())
			if (auto it = std::lower_bound(files.begin(), files.end(), ld->rp->picname, [](const ArchiveFile* a, const string& b) -> bool { return Strcomp::less(a->name.data(), b.data()); }); it != files.end() && (*it)->name == ld->rp->picname)
				start = it - files.begin();

		CountedStopReq csr(stopCheckInterval);
		LoadProgress prg(ld->picLim, files.size());
		for (size_t mov = btom<size_t>(ld->rp->fwd), i = start; i < files.size() && prg.ok(ld->rp.get());) {
			size_t orig = i;
			uptr<archive> arch(ld->fsop->openArchive(ld->rp->arch, true));
			int arrc = ARCHIVE_OK;
			for (archive_entry* entry; i < files.size() && prg.ok(ld->rp.get()) && (arrc = archive_read_next_header(arch.get(), &entry)) == ARCHIVE_OK;) {
				if (csr.stopReq(stoken)) {
					rc = ResultCode::stop;
					i = files.size();
					break;
				}

				if (pathEqual(archive_entry_pathname_utf8(entry), ld->rp->curDir / files[i]->name.data())) {
					if (uptr<SDL_Surface> pic(World::drawSys()->getRenderer()->prepareImage(FileOps::loadArchivePicture(arch.get(), entry), &prg.cbpp)); pic)
						prg.pushImage(ld->rp.get(), files[i]->name.data(), pic, ld->picLim);
					i += mov;
				}
			}
			if (arrc != ARCHIVE_EOF && arrc != ARCHIVE_OK) {
				if (const char* msg = archive_error_string(arch.get()); strncmp(msg, "Passphrase", 10))
					throw std::runtime_error(msg);
				rc = ResultCode::stop;
				break;
			}
			if (i == orig)	// unlikely to happen but increment in case there was no name match in a whole archive iteration
				i += mov;
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
void Browser::loadPicturesPdfThread(std::stop_token stoken, uptr<LoadPicturesPdfData> ld) noexcept {
	ResultCode rc = ResultCode::ok;
	try {
		if (ld->rp->newPdf) {
#ifdef WITH_ARCHIVE
			if (ld->rp->arch) {
				uptr<archive> arch(ld->fsop->openArchive(ld->rp->arch, true));
				int arrc;
				for (archive_entry* entry; (arrc = archive_read_next_header(arch.get(), &entry)) == ARCHIVE_OK;)
					if (pathEqual(archive_entry_pathname_utf8(entry), ld->rp->curDir.data())) {
						ld->rp->pdf = FileOps::loadArchivePdf(arch.get(), entry, true);
						break;
					}
				if (arrc != ARCHIVE_OK)
					throw std::runtime_error(arrc == ARCHIVE_EOF ? "Failed to find PDF file" : archive_error_string(arch.get()));
			} else
#endif
			{
				ld->rp->pdf = ld->fsop->loadPdf(ld->rp->curDir, true);
			}
		}

		int pcnt = ld->rp->pdf.numPages();
		if (pcnt <= 0)
			throw std::runtime_error(ld->rp->pdf ? "No pages" : "Failed to load PDF file");
		uint start = ld->rp->fwd ? 0 : pcnt - 1;
		if (ld->picLim.type != PicLim::Type::none && !ld->rp->picname.empty())
			start = toNum<uint>(ld->rp->picname);

		LoadProgress prg(ld->picLim, ld->rp->fwd ? pcnt - start : start + 1);
		for (uint mov = btom<uint>(ld->rp->fwd), i = start; i < uint(pcnt) && prg.ok(ld->rp.get()); i += mov) {
			if (stoken.stop_requested()) {
				rc = ResultCode::stop;
				break;
			}
			if (uptr<SDL_Surface> pic(World::drawSys()->getRenderer()->prepareImage(ld->rp->pdf.renderPage(i, ld->dpi), &prg.cbpp)); pic)
				prg.pushImage(ld->rp.get(), toStr(i), pic, ld->picLim);
		}
	} catch (const std::exception& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}
#endif

// PROGRESS COUNTERS

Browser::LoadProgress::LoadProgress(const PicLim& picLim, size_t max) {
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		suffix = '/';
		lim = max;
		mem = UINTPTR_MAX;
		break;
	case count:
		suffix = '/';
		lim = std::min(picLim.count, max);
		mem = UINTPTR_MAX;
		break;
	case size:
		suffix = " / ";
		lim = max;
		mem = picLim.size;
		std::tie(dmag, smag) = PicLim::memSizeMag(picLim.size);
	}
	suffix += numStr(picLim, lim, mem);
}

void Browser::LoadProgress::pushImage(BrowserResultPicture* rp, Cstring&& name, uptr<SDL_Surface>& img, const PicLim& picLim) {
	++rp->cnt;
	m += uintptr_t(img->w) * uintptr_t(img->h) * uintptr_t(cbpp);
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(img, rp->pics.emplace_front(std::move(name), nullptr).second, fmt::format("Loading {}{}", numStr(picLim, rp->cnt, m), suffix)));
}

string Browser::LoadProgress::numStr(const PicLim& picLim, size_t ci, uintptr_t mi) const {
	return picLim.type != PicLim::Type::size ? toStr(ci) : PicLim::memoryString(mi, dmag, smag);
}
