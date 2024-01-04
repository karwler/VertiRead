#include "browser.h"
#include "fileOps.h"
#include "engine/drawSys.h"
#include "engine/fileSys.h"
#include "engine/world.h"
#include "utils/compare.h"
#ifdef CAN_PDF
#include "engine/optional/glib.h"
#include "engine/optional/poppler.h"
using namespace LibGlib;
using namespace LibPoppler;
#endif
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

string Browser::prepareNavigationPath(string_view path) const {
	if (!path.empty() && RemoteLocation::getProtocol(path) == Protocol::none && !isAbsolute(path)) {
		if (rootDir == fsop->prefix()) {
			std::error_code ec;
			if (fs::path cwd = fs::current_path(ec); !ec)
				return  fromPath(cwd) / path;
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
			startLoadPictures(new BrowserResultPicture(BRS_LOC, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), string(parentPath(path)), string(filename(path))));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(new BrowserResultPicture(BRS_LOC | BRS_PDF, isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), string(path)));
			return true;
		}
		if (fsop->isArchive(path)) {
			startArchive(new BrowserResultArchive(isSubpath(path, rootDir) ? std::nullopt : optional(fsop->prefix()), ArchiveDir(path)));
			return true;
		}
		if (path = parentPath(path); !fsop->isDirectory(path))
			break;
	case directory:
		if (!isSubpath(path, rootDir))
			rootDir = fsop->prefix();
		curDir = path;
		arch.clear();
		fsop->setWatch(curDir);
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
			startLoadPictures(new BrowserResultPicture(BRS_LOC | BRS_PDF, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
			return true;
		}
		if (fsop->isArchive(paths[0])) {
			startArchive(new BrowserResultArchive(std::move(rdir), ArchiveDir(std::move(paths[0])), paths.size() > 1 ? std::move(paths[1]) : string(), paths.size() > 2 ? std::move(paths[2]) : string()));
			return true;
		}
		break;
	case directory:
		startLoadPictures(new BrowserResultPicture(BRS_LOC, std::move(rdir), std::move(paths[0]), paths.size() > 1 ? std::move(paths[1]) : string()));
		return true;
	}
	return false;
}

bool Browser::goIn(string_view dname) {
	string path = curDir / dname;
	if (inArchive()) {
		if (auto [dir, fil] = arch.find(path); dir && !fil) {
			curDir = std::move(path);
			return true;
		}
	} else if (fsop->isDirectory(path)) {
		curDir = std::move(path);
		fsop->setWatch(curDir);
		return true;
	}
	return false;
}

bool Browser::goFile(string_view fname) {
	string path = curDir / fname;
	if (inArchive()) {
		if (auto [dir, fil] = arch.find(path); fil && (fil->size || fil->isPdf)) {
			startLoadPictures(fil->isPdf
				? new BrowserResultPicture(BRS_LOC | BRS_PDF, std::nullopt, std::move(path), string(), valcp(arch.name))
				: new BrowserResultPicture(BRS_NONE, std::nullopt, valcp(curDir), string(fname), valcp(arch.name)));
			return true;
		}
	} else if (fsop->isRegular(path)) {
		if (fsop->isPicture(path)) {
			startLoadPictures(new BrowserResultPicture(BRS_NONE, std::nullopt, valcp(curDir), string(fname)));
			return true;
		}
		if (fsop->isPdf(path)) {
			startLoadPictures(new BrowserResultPicture(BRS_LOC | BRS_PDF, std::nullopt, std::move(path)));
			return true;
		}
		if (fsop->isArchive(path)) {
			startArchive(new BrowserResultArchive(std::nullopt, ArchiveDir(path)));
			return true;
		}
	}
	return false;
}

bool Browser::goUp() {
	if (inArchive()) {
		if (!curDir.empty())
			curDir = parentPath(curDir);
		else {
			curDir = parentPath(arch.name.data());
			arch.clear();
			fsop->setWatch(curDir);
		}
	} else {
		if (pathEqual(curDir, rootDir))
			return false;

		curDir = parentPath(curDir);
		fsop->setWatch(curDir);
	}
	return true;
}

bool Browser::goNext(bool fwd, string_view picname) {
	bool curArch = inArchive();
	bool curPdf = inPdf();
	if (!picname.empty())	// move to the next batch of pictures if the given picture name can be found
		if (string file = curPdf ? nextPdfPage(picname, fwd) : curArch ? nextArchiveFile(picname, fwd) : nextDirFile(picname, fwd); !file.empty()) {
			startLoadPictures(new BrowserResultPicture((curPdf ? BRS_PDF : BRS_NONE), std::nullopt, valcp(curDir), std::move(file), curArch ? ArchiveDir(valcp(arch.name)) : ArchiveDir()), fwd);
			return true;
		}

	// move to the next container if either no picture name was given or the given one wasn't found
	string pdir(parentPath(curDir));
	const char* cname = filenamePtr(curDir);
	if (curPdf) {
		if (curArch) {
			if (auto [dir, fil] = arch.find(curDir); fil) {
				vector<ArchiveFile*> files = dir->listFiles();
				if (vector<ArchiveFile*>::iterator fi = std::lower_bound(files.begin(), files.end(), cname, [](const ArchiveFile* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); fi != files.end() && (*fi)->name == cname)
					return foreachAround(files, fi, fwd, &Browser::nextArchivePdf, pdir);
			}
		} else {
			vector<Cstring> files = fsop->listDirectory(pdir, true, false, World::sets()->showHidden);
			if (vector<Cstring>::iterator fi = std::lower_bound(files.begin(), files.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); fi != files.end() && *fi == cname)
				return foreachAround(files, fi, fwd, &Browser::nextPdf, pdir);
		}
	} else if (curArch) {
		if (ArchiveDir* dir = arch.find(pdir).first; dir && dir != &arch) {
			vector<ArchiveDir*> dirs = dir->listDirs();
			if (vector<ArchiveDir*>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const ArchiveDir* a, const char* b) -> bool { return Strcomp::less(a->name.data(), b); }); di != dirs.end() && (*di)->name == cname)
				return foreachAround(dirs, di, fwd, &Browser::nextArchiveDir, pdir);
		}
	} else if (!pathEqual(curDir, rootDir)) {
		vector<Cstring> dirs = fsop->listDirectory(pdir, false, true, World::sets()->showHidden);
		if (vector<Cstring>::iterator di = std::lower_bound(dirs.begin(), dirs.end(), cname, [](const Cstring& a, const char* b) -> bool { return Strcomp::less(a.data(), b); }); di != dirs.end() && *di == cname)
			return foreachAround(dirs, di, fwd, &Browser::nextDir, pdir);
	}
	return false;
}

bool Browser::nextDir(const Cstring& dit, bool fwd, const string& pdir) {
	string idir = pdir / dit.data();
	if (rng::any_of(fsop->listDirectory(idir, true, false, World::sets()->showHidden), [this, &idir](const Cstring& it) -> bool { return fsop->isPicture(idir / it.data()); })) {
		startLoadPictures(new BrowserResultPicture(BRS_LOC, std::nullopt, std::move(idir)), fwd);
		return true;
	}
	return false;
}

bool Browser::nextArchiveDir(const ArchiveDir* dit, bool fwd, const string& pdir) {
	if (rng::any_of(dit->files, [](const ArchiveFile& it) -> bool { return it.size; })) {
		startLoadPictures(new BrowserResultPicture(BRS_LOC, std::nullopt, pdir / dit->name.data(), string(), valcp(arch.name)), fwd);
		return true;
	}
	return false;
}

bool Browser::nextPdf(const Cstring& fit, bool fwd, const string& pdir) {
	if (string path = pdir / fit.data(); fsop->isPdf(path)) {
		startLoadPictures(new BrowserResultPicture(BRS_LOC | BRS_PDF, std::nullopt, std::move(path)), fwd);
		return true;
	}
	return false;
}

bool Browser::nextArchivePdf(const ArchiveFile* fit, bool fwd, const string& pdir) {
	if (fit->isPdf) {
		startLoadPictures(new BrowserResultPicture(BRS_LOC | BRS_PDF, std::nullopt, pdir / fit->name.data(), string(), valcp(arch.name)), fwd);
		return true;
	}
	return false;
}

template <class T, MemberFunction F, class... A>
bool Browser::foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F func, A&&... args) {
	if (fwd) {
		return std::find_if(start + 1, vec.end(), [this, func, &args...](T& it) -> bool { return (this->*func)(it, true, args...); }) != vec.end()
			|| std::find_if(vec.begin(), start, [this, func, &args...](T& it) -> bool { return (this->*func)(it, true, args...); }) != start;
	}
	typename vector<T>::reverse_iterator rstart = std::make_reverse_iterator(start + 1);
	return std::find_if(rstart + 1, vec.rend(), [this, func, &args...](T& it) -> bool { return (this->*func)(it, false, args...); }) != vec.rend()
		|| (!vec.empty() && std::find_if(vec.rbegin(), rstart, [this, func, &args...](T& it) -> bool { return (this->*func)(it, false, args...); }) != rstart);
}

string Browser::nextDirFile(string_view file, bool fwd) {
	vector<Cstring> files = fsop->listDirectory(curDir, true, false, World::sets()->showHidden);
	if (vector<Cstring>::iterator fit = std::lower_bound(files.begin(), files.end(), file, [](const Cstring& a, string_view b) -> bool { return Strcomp::less(a.data(), b); }); fit != files.end())
		for (size_t mov = btom<size_t>(fwd), i = fit - files.begin() + (*fit == file || fwd ? mov : 0); i < files.size(); i += mov)
			if (fsop->isPicture(curDir / files[i].data()))
				return files[i].data();
	return string();
}

string Browser::nextArchiveFile(string_view file, bool fwd) {
	if (ArchiveDir* dir = arch.find(curDir).first) {
		vector<ArchiveFile*> files = dir->listFiles();
		if (vector<ArchiveFile*>::iterator fit = std::lower_bound(files.begin(), files.end(), file, [](const ArchiveFile* a, string_view b) -> bool { return Strcomp::less(a->name.data(), b); }); fit != files.end())
			for (size_t mov = btom<size_t>(fwd), i = fit - files.begin() + ((*fit)->name == file || fwd ? mov : 0); i < files.size(); i += mov)
				if (files[i]->size)
					return files[i]->name.data();
	}
	return string();
}

string Browser::nextPdfPage(string_view file, bool fwd) {
#ifdef CAN_PDF
	PopplerDocument* doc = nullptr;
	Data fdat;
	string error;
	if (inArchive()) {
		if (archive* ar = fsop->openArchive(arch.name.data(), &error)) {
			int rc;
			for (archive_entry* entry; (rc = archive_read_next_header(ar, &entry)) == ARCHIVE_OK;)
				if (pathEqual(archive_entry_pathname_utf8(entry), curDir.c_str())) {
					std::tie(doc, fdat) = FileOps::loadArchivePdf(ar, entry);
					break;
				}
			if (rc != ARCHIVE_OK && rc != ARCHIVE_EOF)
				error = archive_error_string(ar);
			archive_read_free(ar);
		}
	} else
		std::tie(doc, fdat) = fsop->loadPdf(curDir, &error);

	string ret;
	if (doc) {
		if (int pcnt = popplerDocumentGetNPages(doc); pcnt > 0)
			if (uint npage = toNum<uint>(file) + (fwd ? 1 : -1); npage < uint(pcnt))
				ret = toStr(npage);
		gObjectUnref(doc);
	} else if (!error.empty())
		logError(error);
	return ret;
#else
	return string();
#endif
}

void Browser::exitFile() {
	if (inPdf())
		curDir = parentPath(curDir);
}

bool Browser::inPdf() {
	if (inArchive()) {
		auto [dir, fil] = arch.find(curDir);
		return fil && fil->isPdf;
	}
	return fsop->isRegular(curDir) && fsop->isPdf(curDir);
}

string Browser::locationForDisplay() const {
	string path = inArchive() ? !curDir.empty() ? arch.name.data() / curDir : arch.name.data() : curDir;
	if (rootDir != fsop->prefix())
		if (string_view rpath = relativePath(path, parentPath(rootDir)); !rpath.empty())
			return string(rpath);
	return path;
}

vector<string> Browser::locationForStore(string_view pname) const {
	vector<string> paths = { dotStr };
	if (inArchive())
		paths.emplace_back(arch.name.data());
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

pair<vector<Cstring>, vector<Cstring>> Browser::listCurDir() {
	if (inArchive()) {
		ArchiveDir* dir = arch.find(curDir).first;
		if (!dir)
			return pair<vector<Cstring>, vector<Cstring>>();

		pair<vector<Cstring>, vector<Cstring>> out;
		rng::transform(dir->files, std::back_inserter(out.first), [](const ArchiveFile& it) -> Cstring { return it.name; });
		rng::sort(out.first, [](const Cstring& a, const Cstring& b) -> bool { return Strcomp::less(a.data(), b.data()); });
		rng::transform(dir->dirs, std::back_inserter(out.second), [](const ArchiveDir& it) -> Cstring { return it.name; });
		rng::sort(out.second, [](const Cstring& a, const Cstring& b) -> bool { return Strcomp::less(a.data(), b.data()); });
		return out;
	}
	return fsop->listDirectorySep(curDir, World::sets()->showHidden);
}

vector<Cstring> Browser::listDirDirs(string_view path) const {
	return fsop->listDirectory(path, false, true, World::sets()->showHidden);
}

bool Browser::deleteEntry(string_view ename) {
	return !inArchive() && fsop->deleteEntry(curDir / ename);
}

bool Browser::renameEntry(string_view oldName, string_view newName) {
	return !inArchive() && fsop->renameEntry(curDir / oldName, curDir / newName);
}

FileOpCapabilities Browser::fileOpCapabilities() const {
	return !inArchive() ? fsop->capabilities() : FileOpCapabilities::none;
}

bool Browser::directoryUpdate(vector<FileChange>& files) {
	bool gone = fsop->pollWatch(files);
	if (inArchive() && gone) {
		curDir.clear();
		arch.clear();
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

void Browser::startArchive(BrowserResultArchive* ra) {
	stopThread();
	curThread = ThreadType::archive;
	thread = std::jthread(std::bind_front(&FileOps::makeArchiveTreeThread, fsop), ra, World::sets()->maxPicRes);
}

bool Browser::finishArchive(BrowserResultArchive&& ra) {
	auto [dir, fil] = ra.arch.find(ra.opath);
	if (fil) {
		startLoadPictures(fil->isPdf
			? new BrowserResultPicture(BRS_LOC | BRS_PDF | BRS_ARCH, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, std::move(ra.opath), std::move(ra.page), std::move(ra.arch))
			: new BrowserResultPicture(BRS_LOC | BRS_ARCH, ra.hasRootDir ? optional(std::move(ra.rootDir)) : std::nullopt, string(parentPath(ra.opath)), fil->name.data(), std::move(ra.arch)));
		return false;
	}
	if (ra.hasRootDir)
		rootDir = std::move(ra.rootDir);
	curDir = parentPath(ra.opath);
	arch = std::move(ra.arch);
	fsop->setWatch(arch.name.data());
	return true;
}

void Browser::cleanupArchive() {
	cleanupEvent(SDL_USEREVENT_THREAD_ARCHIVE_FINISHED, [](SDL_UserEvent& event) { delete static_cast<BrowserResultArchive*>(event.data1); });
}

void Browser::startPreview(const vector<Cstring>& files, const vector<Cstring>& dirs, int maxHeight) {
	stopThread();
	curThread = ThreadType::preview;
	thread = inArchive()
		? std::jthread(&Browser::previewArchThread, fsop, sliceCurrentArchiveDir(), curDir, fromPath(World::fileSys()->dirIcons()), maxHeight)
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

void Browser::previewDirThread(std::stop_token stoken, FileOps* fsop, string curDir, vector<Cstring> files, vector<Cstring> dirs, string iconPath, bool showHidden, int maxHeight) {
	if (uptr<SDL_Surface> dicon(!dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / DrawSys::iconName(DrawSys::Tex::folder)).c_str(), maxHeight), false) : nullptr); dicon)
		for (const Cstring& it : dirs) {
			if (stoken.stop_requested())
				return;

			string dpath = curDir / it.data();
			for (const Cstring& sit : fsop->listDirectory(dpath, true, false, showHidden))
				if (SDL_Surface* img = combineIcons(dicon.get(), fsop->loadPicture(dpath / sit.data()))) {
					pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it.data(), false), img);
					break;
				}
		}
	for (const Cstring& it : files) {
		if (stoken.stop_requested())
			return;

		if (string fpath = curDir / it.data(); SDL_Surface* img = scaleDown(fsop->loadPicture(fpath), maxHeight))
			pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(it.data(), true), img);
		else if (archive* arch = fsop->openArchive(fpath)) {
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

			for (size_t i = 0; i < entries.size() && (arch = fsop->openArchive(fpath));) {
				size_t orig = i;
				for (archive_entry* entry; i < entries.size() && archive_read_next_header(arch, &entry) == ARCHIVE_OK;) {
					if (csr.stopReq(stoken)) {
						archive_read_free(arch);
						return;
					}
					if (archive_entry_pathname_utf8(entry) == entries[i]) {
						if (img = scaleDown(FileOps::loadArchivePicture(arch, entry), maxHeight); img) {
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
#ifdef CAN_PDF
		else if (auto [doc, fdat] = fsop->loadPdf(fpath); doc)
			previewPdf(stoken, doc, maxHeight, it.data());
#endif
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

void Browser::previewArchThread(std::stop_token stoken, FileOps* fsop, ArchiveDir slice, string curDir, string iconPath, int maxHeight) {
	CountedStopReq csr(previewSpeedyStopCheckInterval);
	vector<ArchiveDir*> dirs = slice.listDirs();
	vector<ArchiveFile*> files = slice.listFiles();
	uptr<SDL_Surface> dicon(!dirs.empty() ? World::drawSys()->getRenderer()->makeCompatible(World::drawSys()->loadIcon((iconPath / DrawSys::iconName(DrawSys::Tex::folder)).c_str(), maxHeight), false) : nullptr);
	umap<string, pair<bool, bool>> entries;	// path, pair(isFile, isPdf)
	entries.reserve(dicon ? dirs.size() + files.size() : files.size());
	if (dicon)
		for (ArchiveDir* it : dirs) {
			if (csr.stopReq(stoken))
				return;
			vector<ArchiveFile*> subfiles = it->listFiles();
			if (vector<ArchiveFile*>::iterator fit = rng::find_if(subfiles, [](const ArchiveFile* af) -> bool { return af->size; }); fit != subfiles.end())
				entries.emplace(curDir / it->name.data() + '/' + (*fit)->name.data(), pair(false, false));
		}
	for (const ArchiveFile* it : files) {
		if (csr.stopReq(stoken))
			return;
		if (it->size || it->isPdf)
			entries.emplace(curDir / it->name.data(), pair(true, it->isPdf));
	}

	if (archive* arch = fsop->openArchive(slice.name.data())) {
		for (archive_entry* entry; !entries.empty() && archive_read_next_header(arch, &entry) == ARCHIVE_OK;) {
			if (stoken.stop_requested()) {
				archive_read_free(arch);
				return;
			}

			if (umap<string, pair<bool, bool>>::iterator pit = entries.find(archive_entry_pathname_utf8(entry)); pit != entries.end()) {
				if (!pit->second.second) {
					if (pit->second.first) {
						if (SDL_Surface* img = scaleDown(FileOps::loadArchivePicture(arch, entry), maxHeight))
							pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(filename(pit->first), true), img);
					} else if (SDL_Surface* img = combineIcons(dicon.get(), FileOps::loadArchivePicture(arch, entry)))
						pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(filename(parentPath(pit->first)), false), img);
				}
#ifdef CAN_PDF
				else if (auto [doc, fdat] = FileOps::loadArchivePdf(arch, entry); doc)
					previewPdf(stoken, doc, maxHeight, filename(pit->first));
#endif
				entries.erase(pit);
			}
		}
		archive_read_free(arch);
	}
	pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::finished);
}

#ifdef CAN_PDF
void Browser::previewPdf(std::stop_token stoken, PopplerDocument* doc, int maxHeight, string_view fname) {
	for (int i = 0, pcnt = popplerDocumentGetNPages(doc); i < pcnt; ++i) {
		PopplerPage* page = popplerDocumentGetPage(doc, i);
		for (auto& [pos, id] : getPdfPageImagesInfo(page)) {
			if (stoken.stop_requested()) {
				gObjectUnref(page);
				gObjectUnref(doc);
				return;
			}

			cairo_surface_t* img = popplerPageGetImage(page, id);
			SDL_Surface* pic = scaleDown(cairoImageToSdl(img), maxHeight);
			cairoSurfaceDestroy(img);
			if (pic) {
				pushEvent(SDL_USEREVENT_THREAD_PREVIEW, ThreadEvent::progress, allocatePreviewName(fname, true), pic);
				i = pcnt - 1;
				break;
			}
		}
		gObjectUnref(page);
	}
	gObjectUnref(doc);
}
#endif

ArchiveDir Browser::sliceCurrentArchiveDir() {
	ArchiveDir slice;
	if (auto [dir, fil] = arch.find(curDir); dir) {
		slice.name = arch.name;
		slice.copySlicedDentsFrom(*dir);
	}
	return slice;
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
	rng::copy(name, str + 1);
	str[name.length() + 1] = '\0';
	return str;
}

void Browser::startLoadPictures(BrowserResultPicture* rp, bool fwd) {
	stopThread();
	curThread = ThreadType::reader;
	uint compress = World::sets()->compression != Settings::Compression::b16 ? 1 : 2;
#ifdef CAN_PDF
	if (rp->pdf)
		thread = std::jthread(&Browser::loadPicturesPdfThread, rp, fsop, World::sets()->picLim, compress, World::sets()->pdfImages, fwd);
	else
#endif
	if (rp->hasArchive())
		thread = std::jthread(&Browser::loadPicturesArchThread, rp, fsop, prepareArchiveDirPicLoad(rp, World::sets()->picLim, compress, fwd), World::sets()->picLim, fwd);
	else
		thread = std::jthread(&Browser::loadPicturesDirThread, rp, fsop, World::sets()->picLim, compress, fwd, World::sets()->showHidden);
}

void Browser::finishLoadPictures(BrowserResultPicture& rp) {
	if (rp.hasRootDir)
		rootDir = std::move(rp.rootDir);
	if (rp.hasArchive()) {
		if (rp.newArchive) {
			curDir.clear();
			arch = std::move(rp.arch);
			fsop->setWatch(arch.name.data());
		}
		if (rp.newCurDir)
			curDir = std::move(rp.curDir);
	} else {
		arch.clear();
		if (rp.newCurDir) {
			curDir = std::move(rp.curDir);
			fsop->setWatch(curDir);
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
		case finished: {
			auto rp = static_cast<BrowserResultPicture*>(event.data1);
			for (auto& [name, tex] : rp->pics)
				if (tex)
					World::drawSys()->getRenderer()->freeTexture(tex);
			delete rp;
		} }
	});
}

void Browser::loadPicturesDirThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint compress, bool fwd, bool showHidden) {
	vector<Cstring> files = fsop->listDirectory(rp->curDir, true, false, showHidden);
	size_t start = fwd ? 0 : files.size() - 1;
	if (picLim.type != PicLim::Type::none && !rp->picname.empty())
		if (vector<Cstring>::iterator it = std::lower_bound(files.begin(), files.end(), rp->picname, [](const Cstring& a, const string& b) -> bool { return Strcomp::less(a.data(), b.c_str()); }); it != files.end() && *it == rp->picname)
			start = it - files.begin();
	auto [lim, mem, sizMag] = initLoadLimits(picLim, fwd ? files.size() - start : start + 1);	// picture count limit, picture size limit, magnitude index
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	uintptr_t m = 0;

	// iterate over files until one of the limits is hit (it should be the one associated with the setting)
	for (size_t mov = btom<size_t>(fwd), i = start, c = 0; i < files.size() && c < lim && m < mem && !stoken.stop_requested(); i += mov)
		if (SDL_Surface* img = World::drawSys()->getRenderer()->makeCompatible(fsop->loadPicture(rp->curDir / files[i].data()), true)) {
			rp->mpic.lock();
			rp->pics.emplace_back(std::move(files[i]), nullptr);
			rp->mpic.unlock();
			m += uintptr_t(img->w) * uintptr_t(img->h) * 4 / compress;
			++c;
			pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(rp, img, c - 1), progressText(limitToStr(picLim, c, m, sizMag), progLim));
		}
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp, std::bit_cast<void*>(uintptr_t(fwd)));	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}

void Browser::loadPicturesArchThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, umap<string, uintptr_t> files, PicLim picLim, bool fwd) {
	auto [lim, mem, sizMag] = initLoadLimits(picLim, files.size());
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	size_t c = 0;
	uintptr_t m = 0;
	archive* arch = fsop->openArchive(rp->arch.name.data(), &rp->error);
	if (!arch) {
		if (rp->error.empty())
			rp->error = "Failed to load archive";
		pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp);
		return;
	}

	int rc = ARCHIVE_OK;
	for (archive_entry* entry; !files.empty() && (rc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK && !stoken.stop_requested();)
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
	if (rc != ARCHIVE_EOF && rc != ARCHIVE_OK)
		rp->error = archive_error_string(arch);
	archive_read_free(arch);
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp, std::bit_cast<void*>(uintptr_t(fwd)));	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}

#ifdef CAN_PDF
void Browser::loadPicturesPdfThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint compress, bool imageOnly, bool fwd) {
	PopplerDocument* doc = nullptr;
	Data fdata;
	if (rp->hasArchive()) {
		if (archive* arch = fsop->openArchive(rp->arch.name.data(), &rp->error)) {
			int rc;
			for (archive_entry* entry; (rc = archive_read_next_header(arch, &entry)) == ARCHIVE_OK;)
				if (pathEqual(archive_entry_pathname_utf8(entry), rp->curDir.c_str())) {
					std::tie(doc, fdata) = FileOps::loadArchivePdf(arch, entry, &rp->error);
					break;
				}
			if (rc != ARCHIVE_OK)
				rp->error = rc == ARCHIVE_EOF ? "Failed to find PDF file" : archive_error_string(arch);
			archive_read_free(arch);
		}
	} else
		std::tie(doc, fdata) = fsop->loadPdf(rp->curDir, &rp->error);

	int pcnt;
	if (!doc || (pcnt = popplerDocumentGetNPages(doc)) <= 0) {
		if (rp->error.empty())
			rp->error = doc ? "No pages" : "Failed to load PDF file";
		pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp);
		return;
	}
	uint start = fwd ? 0 : pcnt - 1;
	if (picLim.type != PicLim::Type::none && !rp->picname.empty())
		start = toNum<uint>(rp->picname);
	auto [lim, mem, sizMag] = initLoadLimits(picLim, fwd ? pcnt - start : start + 1);	// picture count limit, picture size limit, magnitude index
	string progLim = limitToStr(picLim, lim, mem, sizMag);
	uintptr_t m = 0;
	size_t c = 0;
	size_t texCnt = 0;

	auto processImage = [rp, &c, &m, &texCnt, &picLim, &progLim, sizMag, compress](cairo_surface_t* img, uint pid) -> bool {
		if (SDL_Surface* pic = World::drawSys()->getRenderer()->makeCompatible(cairoImageToSdl(img), true)) {
			rp->mpic.lock();
			rp->pics.emplace_back(toStr(pid), nullptr);
			rp->mpic.unlock();
			m += uintptr_t(pic->w) * uintptr_t(pic->h) * 4 / compress;
			pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::progress, new BrowserPictureProgress(rp, pic, texCnt++), progressText(limitToStr(picLim, c + 1, m, sizMag), progLim));
			return true;
		}
		return false;
	};

	for (uint mov = btom<uint>(fwd), i = start; i < uint(pcnt) && c < lim && m < mem && !stoken.stop_requested(); i += mov) {
		PopplerPage* page = popplerDocumentGetPage(doc, i);
		if (imageOnly) {
			bool hit = false;
			for (auto& [pos, id] : getPdfPageImagesInfo(page)) {
				cairo_surface_t* img = popplerPageGetImage(page, id);
				hit |= processImage(img, i);
				cairoSurfaceDestroy(img);
			}
			c += hit;
		} else {
			dvec2 size;
			popplerPageGetSize(page, &size.x, &size.y);
			cairo_surface_t* tgt = cairoPdfSurfaceCreate(nullptr, size.x, size.y);
			cairo_t* ctx = cairoCreate(tgt);
			cairoSurfaceDestroy(tgt);
			popplerPageRender(page, ctx);
			cairo_surface_t* img = cairoSurfaceMapToImage(tgt, nullptr);
			c += processImage(img, i);
			cairoSurfaceUnmapImage(tgt, img);
			cairoDestroy(ctx);
		}
		gObjectUnref(page);
	}
	gObjectUnref(doc);
	pushEvent(SDL_USEREVENT_THREAD_READER, ThreadEvent::finished, rp, std::bit_cast<void*>(uintptr_t(fwd)));	// even if the thread was cancelled push this event so the textures can be freed in the main thread
}

vector<pair<dvec2, int>> Browser::getPdfPageImagesInfo(PopplerPage* page) {
	vector<pair<dvec2, int>> positions;
	GList* imaps = popplerPageGetImageMapping(page);
	for (GList* it = imaps; it; it = it->next) {
		auto im = static_cast<PopplerImageMapping*>(it->data);
		positions.emplace_back(dvec2(im->area.x1, im->area.y1), im->image_id);
	}
	popplerPageFreeImageMapping(imaps);
	rng::sort(positions, [](const pair<const dvec2, int>& a, const pair<const dvec2, int>& b) -> bool { return a.first.y < b.first.y || (a.first.y == b.first.y && a.first.x < b.first.x); });
	return positions;
}

SDL_Surface* Browser::cairoImageToSdl(cairo_surface_t* img) {
	if (cairo_format_t fmt = cairoImageSurfaceGetFormat(img); fmt == CAIRO_FORMAT_ARGB32 || fmt == CAIRO_FORMAT_RGB24)
		if (SDL_Surface* pic = SDL_CreateRGBSurfaceWithFormat(0, cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), 32, SDL_PIXELFORMAT_BGRA32)) {
			uchar* src = cairoImageSurfaceGetData(img);
			auto dst = static_cast<uint8*>(pic->pixels);
			uint rlen = pic->w * 4;
			int spitch = cairoImageSurfaceGetStride(img);
			if (fmt == CAIRO_FORMAT_ARGB32) {
				for (int y = 0; y < pic->h; ++y, src += spitch, dst += pic->pitch)
					std::copy_n(src, rlen, dst);
			} else
				for (int y = 0; y < pic->h; ++y, src += spitch, dst += pic->pitch) {
					std::copy_n(src, rlen, dst);
					for (int x = 0; x < pic->w; ++x) {
						if constexpr (std::endian::native == std::endian::little)
							dst[x * 4 + 3] = UINT8_MAX;
						else
							dst[x * 4] = UINT8_MAX;
					}
				}
			return pic;
		}
	return nullptr;
}
#endif

umap<string, uintptr_t> Browser::prepareArchiveDirPicLoad(BrowserResultPicture* rp, const PicLim& picLim, uint compress, bool fwd) {
	vector<ArchiveFile*> files = (rp->newArchive ? rp->arch.find(rp->curDir).first : arch.find(rp->curDir).first)->listFiles();	// rp->curDir must be guaranteed to be a valid path
	size_t start = fwd ? 0 : files.size() - 1;
	if (picLim.type != PicLim::Type::none && !rp->picname.empty())
		if (vector<ArchiveFile*>::const_iterator it = std::lower_bound(files.begin(), files.end(), rp->picname, [](const ArchiveFile* a, const string& b) -> bool { return Strcomp::less(a->name.data(), b.c_str()); }); it != files.end() && (*it)->name == rp->picname)
			start = it - files.begin();

	vector<pair<string, uintptr_t>> stage;
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		stage.reserve(files.size());
		for (size_t i = 0; i < files.size(); ++i)
			if (files[i]->size)
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
		break;
	case count:
		stage.reserve(picLim.count);
		for (size_t i = start, mov = btom<size_t>(fwd); i < files.size() && stage.size() < picLim.count; i += mov)
			if (files[i]->size)
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
		break;
	case size: {
		stage.reserve(files.size());
		uintptr_t m = 0;
		for (size_t i = start, mov = btom<size_t>(fwd); i < files.size() && m < picLim.size; i += mov)
			if (files[i]->size) {
				stage.emplace_back(rp->curDir / files[i]->name.data(), uintptr_t(files[i]->size));
				m += files[i]->size / compress;
			}
	} }
	return umap<string, uintptr_t>(std::make_move_iterator(stage.begin()), std::make_move_iterator(stage.end()));
}

tuple<size_t, uintptr_t, pair<uint8, uint8>> Browser::initLoadLimits(const PicLim& picLim, size_t max) {
	switch (picLim.type) {
	using enum PicLim::Type;
	case none:
		return tuple(max, UINTPTR_MAX, pair(0, 0));
	case count:
		return tuple(std::min(picLim.count, max), UINTPTR_MAX, pair(0, 0));
	case size:
		return tuple(max, picLim.size, PicLim::memSizeMag(picLim.size));
	}
	return tuple(0, 0, pair(0, 0));
}

string Browser::limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, pair<uint8, uint8> mag) {
	return picLim.type != PicLim::Type::size ? toStr(c) : PicLim::memoryString(m, mag.first, mag.second);
}

char* Browser::progressText(string_view val, string_view lim) {
	auto text = new char[val.length() + lim.length() + 2];
	rng::copy(val, text);
	text[val.length()] = '/';
	rng::copy(lim, text + val.length() + 1);
	text[val.length() + 1 + lim.length()] = '\0';
	return text;
}
