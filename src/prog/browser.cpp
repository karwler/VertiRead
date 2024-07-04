#include "browser.h"
#include "fileOps.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/world.h"
#include "utils/compare.h"
#include <functional>
#ifdef WITH_ARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace std {

template <>
struct default_delete<SDL_Surface> {
	void operator()(SDL_Surface* ptr) const {
		SDL_FreeSurface(ptr);
	}
};

}

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

Browser::LoadPicturesDirData::LoadPicturesDirData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, uint8 cprs, bool hidden) noexcept :
	fsop(fs),
	rp(std::move(res)),
	picLim(plim),
	compress(cprs),
	showHidden(hidden)
{}

#ifdef WITH_ARCHIVE
Browser::LoadPicturesArchData::LoadPicturesArchData(FileOps* fs, uptr<BrowserResultPicture>&& res, umap<string, uintptr_t>&& fmap, const PicLim& plim) noexcept :
	fsop(fs),
	rp(std::move(res)),
	files(std::move(fmap)),
	picLim(plim)
{}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
Browser::LoadPicturesPdfData::LoadPicturesPdfData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, float ddpi, uint8 cprs) noexcept :
	fsop(fs),
	rp(std::move(res)),
	picLim(plim),
	dpi(ddpi),
	compress(cprs)
{}
#endif

Browser::~Browser() {
	try {
		stopThread();
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
	delete fsop;
}

string Browser::prepareNavigationPath(string_view path) const {
	if (!path.empty() && RemoteLocation::getProtocol(path) == Protocol::none && !isAbsolute(path)) {
		if (rootDir == fsop->prefix()) {
			std::error_code ec;
			if (fs::path cwd = fs::current_path(ec); !ec)
				return fromPath(cwd) / path;
		} else if (string_view::iterator sep = rng::find_if(path, isDsep); filename(rootDir) == string_view(path.begin(), sep))
			return rootDir / string_view(std::find_if(sep, path.end(), notDsep), path.end());
	}
	return string(path);
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
	arch = ArchiveData();
	fsop->unsetWatch();
}

bool Browser::goTo(const RemoteLocation& location, vector<string>&& passwords) {
	return beginRemoteOps(location, std::move(passwords), [this](FileOps* backup, const RemoteLocation& rl) -> bool {
		bool wait = goTo(fsop->prefix() / rl.path);
		delete backup;
		return wait;
	});
}

bool Browser::goTo(const string& path) {
	switch (fsop->fileType(path)) {
	using enum fs::file_type;
	case regular:
		if (fsop->isPicture(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), string(parentPath(path)), string(filename(path))));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), valcp(path)));
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
	throw std::runtime_error(std::format("Invalid path '{}'", path));
}

bool Browser::openPicture(string&& rdir, vector<string>&& paths) {
	if (string prefix = fsop->prefix(); rdir != prefix && !isSubpath(paths[0], rdir))
		rdir = fsop->prefix();

	switch (fsop->fileType(paths[0])) {
	using enum fs::file_type;
	case regular:
		if (fsop->isPdf(paths[0])) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
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
		startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
		return true;
	}
	return false;
}

bool Browser::goIn(string_view dname) {
	string path = curDir / dname;
	if (arch) {
#ifdef WITH_ARCHIVE
		if (auto [dir, fil] = arch.find(path); dir && !fil) {
			curDir = std::move(path);
			return true;
		}
#endif
	} else if (fsop->isDirectory(path)) {
		curDir = std::move(path);
		fsop->unsetWatch();
		return true;
	}
	return false;
}

bool Browser::goFile(string_view fname) {
	string path = curDir / fname;
	if (arch) {
#ifdef WITH_ARCHIVE
		if (auto [dir, fil] = arch.find(path); fil && (fil->size || fil->isPdf)) {
			startLoadPictures(fil->isPdf
				? std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF, std::nullopt, std::move(path), string(), arch.copyLight())
				: std::make_unique<BrowserResultPicture>(BRS_NONE, std::nullopt, valcp(curDir), string(fname), arch.copyLight()));
			return true;
		}
#endif
	} else if (fsop->isRegular(path)) {
		if (fsop->isPicture(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_NONE, std::nullopt, valcp(curDir), string(fname)));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF, std::nullopt, std::move(path)));
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
	if (arch) {
#ifdef WITH_ARCHIVE
		if (!curDir.empty())
			curDir = parentPath(curDir);
		else {
			curDir = parentPath(arch.name.data());
			arch = ArchiveData();
			fsop->unsetWatch();
		}
#endif
	} else {
		if (pathEqual(curDir, rootDir))
			return false;

		curDir = parentPath(curDir);
		fsop->unsetWatch();
	}
	return true;
}

void Browser::startGoNext(string&& picname, bool fwd) {
	stopThread();
	curThread = ThreadType::next;
	thread = std::jthread(std::bind_front(&Browser::goNextThread, this), std::make_unique<GoNextData>(std::move(picname), fwd));
}

void Browser::goNextThread(std::stop_token stoken, uptr<GoNextData> gd) {
	uptr<BrowserResultPicture> rp;
	if (!gd->picname.empty()) {	// move to the next batch of pictures if the given picture name can be found
		string file;
		if (pdf) {
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
			if (int pcnt = pdf.numPages(); pcnt > 0)
				if (uint npage = toNum<uint>(gd->picname) + (gd->fwd ? 1 : -1); npage < uint(pcnt))
					file = toStr(npage);
#endif
		} else if (arch) {
#ifdef WITH_ARCHIVE
			if (ArchiveDir* dir = arch.find(curDir).first) {
				vector<ArchiveFile*> files = dir->listFiles();
				if (vector<ArchiveFile*>::iterator fit = std::lower_bound(files.begin(), files.end(), gd->picname, [](const ArchiveFile* a, string_view b) -> bool { return Strcomp::less(a->name.data(), b); }); fit != files.end())
					for (size_t mov = btom<size_t>(gd->fwd), i = fit - files.begin() + ((*fit)->name == gd->picname || gd->fwd ? mov : 0); i < files.size(); i += mov)
						if (files[i]->size) {
							file = files[i]->name.data();
							break;
						}
			}
#endif
		} else {
			vector<Cstring> files = fsop->listDirectory(stoken, curDir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
			if (vector<Cstring>::iterator fit = std::lower_bound(files.begin(), files.end(), gd->picname, [](const Cstring& a, string_view b) -> bool { return Strcomp::less(a.data(), b); }); fit != files.end())
				for (size_t mov = btom<size_t>(gd->fwd), i = fit - files.begin() + (*fit == gd->picname || gd->fwd ? mov : 0); i < files.size(); i += mov)
					if (fsop->isPicture(curDir / files[i].data())) {
						file = files[i].data();
						break;
					}
		}
		if (!file.empty())
			rp = std::make_unique<BrowserResultPicture>(gd->fwd ? BRS_FWD : BRS_NONE, std::nullopt, valcp(curDir), std::move(file), arch.copyLight(), pdf.copyLight());
	} else {	// move to the next container if either no picture name was given or the given one wasn't found
		string pdir(parentPath(curDir));
		const char* cname = filenamePtr(curDir);
		if (pdf) {
			if (arch) {
#ifdef WITH_ARCHIVE
				if (auto [dir, fil] = arch.find(curDir); fil) {
					vector<ArchiveFile*> files = dir->listFiles();
					if (vector<ArchiveFile*>::iterator fi = std::lower_bound(files.begin(), files.end(), cname, [](const ArchiveFile* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); fi != files.end() && (*fi)->name == cname)
						if (fi = foreachAround(files, fi, gd->fwd, [](const ArchiveFile* af) -> bool { return af->isPdf; }); fi != files.end())
							rp = std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / (*fi)->name.data(), string(), arch.copyLight());
				}
#endif
			} else {
				vector<Cstring> files = fsop->listDirectory(stoken, pdir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
				if (vector<Cstring>::iterator fi = std::lower_bound(files.begin(), files.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); fi != files.end() && *fi == cname)
					if (fi = foreachAround(files, fi, gd->fwd, [this, &pdir](const Cstring& fn) -> bool { return fsop->isPdf(pdir / fn.data()); }); fi != files.end())
						rp = std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / fi->data());
			}
		} else if (arch) {
#ifdef WITH_ARCHIVE
			if (ArchiveDir* dir = arch.find(pdir).first; dir && dir != &arch) {
				vector<ArchiveDir*> dirs = dir->listDirs();
				if (vector<ArchiveDir*>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const ArchiveDir* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); di != dirs.end() && (*di)->name == cname)
					if (di = foreachAround(dirs, di, gd->fwd, [](const ArchiveDir* ad) -> bool { return rng::any_of(ad->files, [](const ArchiveFile& it) -> bool { return it.size; }); }); di != dirs.end())
						rp = std::make_unique<BrowserResultPicture>(BRS_LOC | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / (*di)->name.data(), string(), arch.copyLight());
			}
#endif
		} else if (!pathEqual(curDir, rootDir)) {
			vector<Cstring> dirs = fsop->listDirectory(stoken, pdir, BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).dirs;
			if (vector<Cstring>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); di != dirs.end() && *di == cname) {
				if (di = foreachAround(dirs, di, gd->fwd, [this, stoken, &pdir](const Cstring& fn) -> bool {
					string idir = pdir / fn.data();
					return rng::any_of(fsop->listDirectory(stoken, idir, BLO_FILES | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)).files, [this, &idir](const Cstring& it) -> bool { return fsop->isPicture(idir / it.data()); });
				}); di != dirs.end())
					rp = std::make_unique<BrowserResultPicture>(BRS_LOC | (gd->fwd ? BRS_FWD : BRS_NONE), std::nullopt, pdir / di->data());
			}
		}
	}
	ResultCode rc = stoken.stop_requested() ? ResultCode::stop : rp ? ResultCode::ok : ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_GO_NEXT_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)), rp.release());
}

template <class T, class F>
vector<T>::iterator Browser::foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F check) {
	if (vec.empty())
		return vec.end();

	if (fwd) {
		typename vector<T>::iterator rs;
		if (rs = std::find_if(start + 1, vec.end(), check); rs == vec.end())
			if (rs = std::find_if(vec.begin(), start, check); rs == start)
				return vec.end();
		return rs;
	}
	typename vector<T>::reverse_iterator rv, rstart = std::make_reverse_iterator(start + 1);
	if (rv = std::find_if(rstart + 1, vec.rend(), check); rv == vec.rend())
		if (rv = std::find_if(vec.rbegin(), rstart, check); rv == rstart)
			return vec.end();
	return rv.base();
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

vector<string> Browser::locationForStore(string_view pname) const {
	vector<string> paths = { dotStr };
	if (arch)
#ifdef WITH_ARCHIVE
		paths.emplace_back(arch.name.data());
#endif
	paths.push_back(curDir);
	if (!pname.empty())
		paths.emplace_back(pname);

	if (rootDir != fsop->prefix())
		if (string_view rpath = relativePath(paths[1], parentPath(rootDir)); !rpath.empty()) {
			string_view::iterator sep = rng::find_if(rpath, isDsep);
			paths[0].assign(rpath.begin(), sep);
			paths[1].assign(std::find_if(sep, rpath.end(), notDsep), rpath.end());
		}
	return paths;
}

void Browser::startListCurDir(bool files) {
	stopThread();
	curThread = ThreadType::list;
	if (arch) {
#ifdef WITH_ARCHIVE
		uptr<ListArchData> ld = std::make_unique<ListArchData>((files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE));
		if (ArchiveDir* dir = arch.find(curDir).first)
#ifdef _WIN32
			ld->slice.copySlicedDentsFrom(*dir, true);
#else
			ld->slice.copySlicedDentsFrom(*dir, World::sets()->showHidden);
#endif
		thread = std::jthread(&Browser::listDirArchThread, std::move(ld));
#endif
	} else
		thread = std::jthread(&Browser::listDirFsThread, std::make_unique<ListDirData>(fsop, valcp(curDir), (files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)));
}

void Browser::startListDir(string&& path, bool files) {
	stopThread();
	curThread = ThreadType::list;
	thread = std::jthread(&Browser::listDirFsThread, std::make_unique<ListDirData>(fsop, std::move(path), (files ? BLO_FILES : BLO_NONE) | BLO_DIRS | (World::sets()->showHidden ? BLO_HIDDEN : BLO_NONE)));
}

void Browser::listDirFsThread(std::stop_token stoken, uptr<ListDirData> ld) {
	uptr<BrowserResultList> rl = std::make_unique<BrowserResultList>(ld->fsop->listDirectory(stoken, ld->path, ld->opts));
	ResultCode rc = stoken.stop_requested() ? ResultCode::stop : rl->error.empty() ? ResultCode::ok : ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_LIST_FINISHED, 0, std::bit_cast<void*>(uintptr_t(rc)), rl.release());
}

#ifdef WITH_ARCHIVE
void Browser::listDirArchThread(std::stop_token stoken, uptr<ListArchData> ld) {
	uptr<BrowserResultList> rl = std::make_unique<BrowserResultList>(ld->opts & BLO_FILES ? ld->slice.copySortedFiles(ld->opts & BLO_HIDDEN) : vector<Cstring>(), ld->opts & BLO_DIRS ? ld->slice.copySortedDirs(ld->opts & BLO_HIDDEN) : vector<Cstring>());
	pushEvent(SDL_USEREVENT_THREAD_LIST_FINISHED, 0, std::bit_cast<void*>(uintptr_t(!stoken.stop_requested() ? ResultCode::ok : ResultCode::stop)), rl.release());
}
#endif

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

void Browser::setDirectoryWatch() {
	if (!arch)	// should be already set if in an archive
		fsop->setWatch(curDir);
}

bool Browser::directoryUpdate(vector<FileChange>& files) {
	bool gone = fsop->pollWatch(files);
	if (arch && gone) {
		curDir.clear();
		arch = ArchiveData();
	}
	return gone;
}

void Browser::stopThread() {
	if (thread.joinable()) {
		thread = std::jthread();
		switch (curThread) {
		using enum ThreadType;
		case list:
			cleanupEvent(SDL_USEREVENT_THREAD_LIST_FINISHED, [](SDL_UserEvent& event) { delete static_cast<BrowserResultList*>(event.data2); });
			break;
		case preview:
			cleanupEvent(SDL_USEREVENT_THREAD_PREVIEW, [](SDL_UserEvent& event) {
				if (ThreadEvent(event.code) == ThreadEvent::progress) {
					delete[] static_cast<char*>(event.data1);
					SDL_FreeSurface(static_cast<SDL_Surface*>(event.data2));
				}
			});
			break;
		case reader:
			cleanupEvent(SDL_USEREVENT_THREAD_READER, [](SDL_UserEvent& event) {
				switch (ThreadEvent(event.code)) {
				using enum ThreadEvent;
				case progress: {
					auto pp = static_cast<BrowserPictureProgress*>(event.data1);
					SDL_FreeSurface(pp->img);
					delete pp;
					break; }
				case finished: {
					auto rp = static_cast<BrowserResultPicture*>(event.data2);
					for (auto& [name, tex] : rp->pics)
						World::drawSys()->getRenderer()->freeTexture(tex);
					delete rp;
				} }
			});
			break;
		case next:
			cleanupEvent(SDL_USEREVENT_THREAD_GO_NEXT_FINISHED, [](SDL_UserEvent& event) { delete static_cast<BrowserResultPicture*>(event.data2); });
			break;
#ifdef WITH_ARCHIVE
		case archive:
			cleanupEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, [](SDL_UserEvent& event) { delete static_cast<BrowserResultArchive*>(event.data1); });
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
			? std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_PDF | BRS_ARCH, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, std::move(ra.opath), std::move(ra.page), std::move(ra.arch))
			: std::make_unique<BrowserResultPicture>(BRS_LOC | BRS_ARCH, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, string(parentPath(ra.opath)), fil->name.data(), std::move(ra.arch)));
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
	if (arch) {
#ifdef WITH_ARCHIVE
		ArchiveData ad = arch.copyLight();
		if (auto [dir, fil] = arch.find(curDir); dir)
#ifdef _WIN32
			ad.copySlicedDentsFrom(*dir, true);
#else
			ad.copySlicedDentsFrom(*dir, World::sets()->showHidden);
#endif
		thread = std::jthread(&Browser::previewArchThread, std::make_unique<PreviewArchData>(fsop, std::move(ad), valcp(curDir), valcp(fromPath(World::fileSys()->dirIcons())), maxHeight));
#endif
	} else
		thread = std::jthread(&Browser::previewDirThread, std::make_unique<PreviewDirData>(fsop, valcp(curDir), valcp(fromPath(World::fileSys()->dirIcons())), maxHeight, World::sets()->showHidden));
}

void Browser::previewDirThread(std::stop_token stoken, uptr<PreviewDirData> pd) {
	BrowserResultList rl = pd->fsop->listDirectory(stoken, pd->curDir, BLO_FILES | BLO_DIRS | (pd->showHidden ? BLO_HIDDEN : BLO_NONE));
	if (uptr<SDL_Surface> dicon(!rl.dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((pd->iconPath / DrawSys::iconName(DrawSys::Tex::folder)).data(), pd->maxHeight), false) : nullptr); dicon)
		for (const Cstring& it : rl.dirs) {
			if (stoken.stop_requested())
				return;

			string dpath = pd->curDir / it.data();
			for (const Cstring& sit : pd->fsop->listDirectory(stoken, dpath, BLO_FILES | (pd->showHidden ? BLO_HIDDEN : BLO_NONE)).files)
				if (SDL_Surface* img = combineIcons(dicon.get(), pd->fsop->loadPicture(dpath / sit.data()))) {
					pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it.data(), false), img);
					break;
				}
		}
	for (const Cstring& it : rl.files) {
		if (stoken.stop_requested())
			return;

		if (string fpath = pd->curDir / it.data(); SDL_Surface* img = scaleDown(pd->fsop->loadPicture(fpath), pd->maxHeight))
			pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it.data(), true), img);
#ifdef WITH_ARCHIVE
		else if (ArchiveData ad(fpath, ArchiveData::PassCode::ignore); archive* arch = pd->fsop->openArchive(ad)) {
			CountedStopReq csr(previewSpeedyStopCheckInterval);
			vector<Cstring> entries;	// list all available files first to get a sorted list
			for (archive_entry* entry; archive_read_next_header(arch, &entry) == ARCHIVE_OK;) {
				if (csr.stopReq(stoken)) {
					archive_read_free(arch);
					return;
				}
				if (archive_entry_size(entry) > 0)
					entries.emplace_back(archive_entry_pathname_utf8(entry));
			}
			archive_read_free(arch);
			rng::sort(entries, Strcomp());

			for (size_t i = 0; i < entries.size() && (arch = pd->fsop->openArchive(ad));) {
				size_t orig = i;
				for (archive_entry* entry; i < entries.size() && archive_read_next_header(arch, &entry) == ARCHIVE_OK;) {
					if (csr.stopReq(stoken)) {
						archive_read_free(arch);
						return;
					}
					if (archive_entry_pathname_utf8(entry) == entries[i]) {
						if (img = scaleDown(FileOps::loadArchivePicture(arch, entry), pd->maxHeight); img) {
							pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it.data(), true), img);
							i = entries.size();
							break;
						}
						++i;
					}
				}
				i += i == orig;	// unlikely to happen but increment in case there was no name match in a whole archive iteration
				archive_read_free(arch);
			}
		}
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
		else if (PdfFile pdf = pd->fsop->loadPdf(fpath))
			previewPdf(stoken, pdf, pd->maxHeight, it.data());
#endif
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

#ifdef WITH_ARCHIVE
void Browser::previewArchThread(std::stop_token stoken, uptr<PreviewArchData> pd) {
	CountedStopReq csr(previewSpeedyStopCheckInterval);
	vector<ArchiveDir*> dirs = pd->slice.listDirs();
	vector<ArchiveFile*> files = pd->slice.listFiles();
	uptr<SDL_Surface> dicon(!dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((pd->iconPath / DrawSys::iconName(DrawSys::Tex::folder)).data(), pd->maxHeight), false) : nullptr);
	umap<string, pair<bool, bool>> entries;	// path, pair(isFile, isPdf)
	entries.reserve(dicon ? dirs.size() + files.size() : files.size());
	if (dicon)
		for (ArchiveDir* it : dirs) {
			if (csr.stopReq(stoken))
				return;
			vector<ArchiveFile*> subfiles = it->listFiles();
			if (vector<ArchiveFile*>::iterator fit = rng::find_if(subfiles, [](const ArchiveFile* af) -> bool { return af->size; }); fit != subfiles.end())
				entries.emplace(pd->curDir / it->name.data() + '/' + (*fit)->name.data(), pair(false, false));
		}
	for (const ArchiveFile* it : files) {
		if (csr.stopReq(stoken))
			return;
		if (it->size || it->isPdf)
			entries.emplace(pd->curDir / it->name.data(), pair(true, it->isPdf));
	}

	pd->slice.pc = ArchiveData::PassCode::attempt;
	if (archive* arch = pd->fsop->openArchive(pd->slice)) {
		for (archive_entry* entry; !entries.empty() && archive_read_next_header(arch, &entry) == ARCHIVE_OK;) {
			if (stoken.stop_requested()) {
				archive_read_free(arch);
				return;
			}

			if (umap<string, pair<bool, bool>>::iterator pit = entries.find(archive_entry_pathname_utf8(entry)); pit != entries.end()) {
				if (!pit->second.second) {
					if (pit->second.first) {
						if (SDL_Surface* img = scaleDown(FileOps::loadArchivePicture(arch, entry), pd->maxHeight))
							pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(filename(pit->first), true), img);
					} else if (SDL_Surface* img = combineIcons(dicon.get(), FileOps::loadArchivePicture(arch, entry)))
						pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(filename(parentPath(pit->first)), false), img);
				}
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
				else if (PdfFile pdf = FileOps::loadArchivePdf(arch, entry))
					previewPdf(stoken, pdf, pd->maxHeight, filename(pit->first));
#endif
				entries.erase(pit);
			}
		}
		archive_read_free(arch);
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
void Browser::previewPdf(std::stop_token stoken, PdfFile& pdfFile, int maxHeight, string_view fname) {
	int pcnt = pdfFile.numPages();
	for (int i = 0; i < pcnt && !stoken.stop_requested(); ++i)
		if (SDL_Surface* pic = scaleDown(pdfFile.renderPage(i, 0.2), maxHeight)) {
			pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(fname, true), pic);
			break;
		}
}
#endif

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
	rng::copy(name, str + 1);
	str[name.length() + 1] = '\0';
	return str;
}

void Browser::startLoadPictures(uptr<BrowserResultPicture>&& rp) {
	stopThread();
	curThread = ThreadType::reader;
	uint8 compress = World::sets()->compression == Settings::Compression::b16 ? 1 : World::sets()->compression == Settings::Compression::b8 ? 2 : 0;
	if (rp->newPdf || rp->pdf) {
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
		thread = std::jthread(&Browser::loadPicturesPdfThread, std::make_unique<LoadPicturesPdfData>(fsop, std::move(rp), World::sets()->picLim, World::drawSys()->getWinDpi(), compress));
#endif
	} else if (rp->arch) {
#ifdef WITH_ARCHIVE
#ifdef _WIN32
		umap<string, uintptr_t> files = prepareArchiveDirPicLoad(rp.get(), World::sets()->picLim, compress, true);
#else
		umap<string, uintptr_t> files = prepareArchiveDirPicLoad(rp.get(), World::sets()->picLim, compress, World::sets()->showHidden);
#endif
		thread = std::jthread(&Browser::loadPicturesArchThread, std::make_unique<LoadPicturesArchData>(fsop, std::move(rp), std::move(files), World::sets()->picLim));
#endif
	} else
		thread = std::jthread(&Browser::loadPicturesDirThread, std::make_unique<LoadPicturesDirData>(fsop, std::move(rp), World::sets()->picLim, compress, World::sets()->showHidden));
}

void Browser::finishLoadPictures(BrowserResultPicture& rp) {
	if (rp.hasRootDir)
		rootDir = std::move(rp.rootDir);
	if (rp.newCurDir)
		curDir = std::move(rp.curDir);
	if (rp.arch) {
#ifdef WITH_ARCHIVE
		if (rp.newArchive) {
			arch = std::move(rp.arch);
			fsop->setWatch(arch.name.data());
		} else {
			arch.passphrase = std::move(rp.arch.passphrase);
			arch.pc = rp.arch.pc;
		}
#endif
	} else {
		arch = ArchiveData();
		if (rp.newCurDir)
			fsop->setWatch(curDir);
	}
	if (rp.newPdf)
		pdf = std::move(rp.pdf);
}

void Browser::loadPicturesDirThread(std::stop_token stoken, uptr<LoadPicturesDirData> ld) {
	ResultCode rc = ResultCode::ok;
	vector<Cstring> files = ld->fsop->listDirectory(stoken, ld->rp->curDir, BLO_FILES | (ld->showHidden ? BLO_HIDDEN : BLO_NONE)).files;
	size_t start = ld->rp->fwd ? 0 : files.size() - 1;
	if (ld->picLim.type != PicLim::Type::none && !ld->rp->picname.empty())
		if (vector<Cstring>::iterator it = std::lower_bound(files.begin(), files.end(), ld->rp->picname, [](const Cstring& a, const string& b) -> bool { return Strcomp::less(a.data(), b.data()); }); it != files.end() && *it == ld->rp->picname)
			start = it - files.begin();

	LoadProgress prg(ld->picLim, ld->rp->fwd ? files.size() - start : start + 1);
	for (size_t mov = btom<size_t>(ld->rp->fwd), i = start; i < files.size() && prg.ok(); i += mov) {
		if (stoken.stop_requested()) {
			rc = ResultCode::stop;
			break;
		}
		if (SDL_Surface* pic = World::drawSys()->getRenderer()->makeCompatible(ld->fsop->loadPicture(ld->rp->curDir / files[i].data()), true))
			prg.pushImage(ld->rp.get(), std::move(files[i]), pic, ld->picLim, (uintptr_t(pic->w) * uintptr_t(pic->h) * 4) >> ld->compress);
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}

#ifdef WITH_ARCHIVE
void Browser::loadPicturesArchThread(std::stop_token stoken, uptr<LoadPicturesArchData> ld) {
	ResultCode rc = ResultCode::ok;
	if (archive* arch = ld->fsop->openArchive(ld->rp->arch, &ld->rp->error)) {
		LoadProgress prg(ld->picLim, ld->files.size());
		int arrc = ARCHIVE_OK;
		for (archive_entry* entry; !ld->files.empty() && (arrc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK;) {
			if (stoken.stop_requested()) {
				rc = ResultCode::stop;
				break;
			}

			if (umap<string, uintptr_t>::iterator fit = ld->files.find(archive_entry_pathname_utf8(entry)); fit != ld->files.end()) {
				if (SDL_Surface* pic = World::drawSys()->getRenderer()->makeCompatible(FileOps::loadArchivePicture(arch, entry), true))
					prg.pushImage(ld->rp.get(), filename(fit->first), pic, ld->picLim, fit->second);
				ld->files.erase(fit);
			}
		}
		if (arrc != ARCHIVE_EOF && arrc != ARCHIVE_OK) {
			string_view err(archive_error_string(arch));
			ld->rp->error = err;
			rc = err.starts_with("Passphrase") ? ResultCode::stop : ResultCode::error;
		}
		archive_read_free(arch);
	} else
		rc = ResultCode::error;
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
void Browser::loadPicturesPdfThread(std::stop_token stoken, uptr<LoadPicturesPdfData> ld) {
	if (ld->rp->newPdf) {
		if (ld->rp->arch) {
#ifdef WITH_ARCHIVE
			if (archive* arch = ld->fsop->openArchive(ld->rp->arch, &ld->rp->error)) {
				int arrc;
				for (archive_entry* entry; (arrc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK;)
					if (pathEqual(archive_entry_pathname_utf8(entry), ld->rp->curDir.data())) {
						ld->rp->pdf = FileOps::loadArchivePdf(arch, entry, &ld->rp->error);
						break;
					}
				if (arrc != ARCHIVE_OK)
					ld->rp->error = arrc == ARCHIVE_EOF ? "Failed to find PDF file" : archive_error_string(arch);
				archive_read_free(arch);
			}
#endif
		} else
			ld->rp->pdf = ld->fsop->loadPdf(ld->rp->curDir, &ld->rp->error);
	}

	ResultCode rc = ResultCode::ok;
	if (int pcnt = ld->rp->pdf.numPages(); pcnt > 0) {
		uint start = ld->rp->fwd ? 0 : pcnt - 1;
		if (ld->picLim.type != PicLim::Type::none && !ld->rp->picname.empty())
			start = toNum<uint>(ld->rp->picname);

		LoadProgress prg(ld->picLim, ld->rp->fwd ? pcnt - start : start + 1);
		for (uint mov = btom<uint>(ld->rp->fwd), i = start; i < uint(pcnt) && prg.ok(); i += mov) {
			if (stoken.stop_requested()) {
				rc = ResultCode::stop;
				break;
			}
			if (SDL_Surface* pic = World::drawSys()->getRenderer()->makeCompatible(ld->rp->pdf.renderPage(i, ld->dpi), true))
				prg.pushImage(ld->rp.get(), toStr(i), pic, ld->picLim, (uintptr_t(pic->w) * uintptr_t(pic->h) * 4) >> ld->compress);
		}
	} else {
		if (ld->rp->error.empty())
			ld->rp->error = ld->rp->pdf ? "No pages" : "Failed to load PDF file";
		rc = ResultCode::error;
	}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, std::bit_cast<void*>(uintptr_t(rc)), ld->rp.release());	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}
#endif

#ifdef WITH_ARCHIVE
umap<string, uintptr_t> Browser::prepareArchiveDirPicLoad(BrowserResultPicture* rp, const PicLim& picLim, uint8 compress, bool showHidden) {
	vector<ArchiveFile*> files = (rp->newArchive ? rp->arch.find(rp->curDir).first : arch.find(rp->curDir).first)->listFiles();	// rp->curDir must be guaranteed to be a valid path
	size_t start = rp->fwd ? 0 : files.size() - 1;
	if (picLim.type != PicLim::Type::none && !rp->picname.empty())
		if (vector<ArchiveFile*>::const_iterator it = std::lower_bound(files.begin(), files.end(), rp->picname, [](const ArchiveFile* a, const string& b) -> bool { return Strcomp::less(a->name.data(), b.data()); }); it != files.end() && (*it)->name == rp->picname)
			start = it - files.begin();

	vector<pair<string, uintptr_t>> stage;
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		stage.reserve(files.size());
		for (size_t i = 0; i < files.size(); ++i)
			if (files[i]->size && (showHidden || files[i]->name[0] != '.'))
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
		break;
	case count:
		stage.reserve(picLim.count);
		for (size_t i = start, mov = btom<size_t>(rp->fwd); i < files.size() && stage.size() < picLim.count; i += mov)
			if (files[i]->size && (showHidden || files[i]->name[0] != '.'))
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
		break;
	case size: {
		stage.reserve(files.size());
		uintptr_t m = 0;
		for (size_t i = start, mov = btom<size_t>(rp->fwd); i < files.size() && m < picLim.size; i += mov)
			if (files[i]->size && (showHidden || files[i]->name[0] != '.')) {
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
				m += files[i]->size >> compress;
			}
	} }
	return umap<string, uintptr_t>(std::make_move_iterator(stage.begin()), std::make_move_iterator(stage.end()));
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

void Browser::LoadProgress::pushImage(BrowserResultPicture* rp, Cstring&& name, SDL_Surface* img, const PicLim& picLim, uintptr_t imgSize) {
	rp->mpic.lock();
	rp->pics.emplace_back(std::move(name), nullptr);
	rp->mpic.unlock();
	++c;
	m += imgSize;
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(rp, img, c - 1, std::format("Loading {}{}", numStr(picLim, c, m), suffix)));
}

string Browser::LoadProgress::numStr(const PicLim& picLim, size_t ci, uintptr_t mi) const {
	return picLim.type != PicLim::Type::size ? toStr(ci) : PicLim::memoryString(mi, dmag, smag);
}
