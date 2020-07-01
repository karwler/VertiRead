#include "browser.h"

Browser::Browser(fs::path rootDirectory, fs::path curDirectory, PCall exitCall) :
	exCall(exitCall),
	rootDir(std::move(rootDirectory)),
	curDir(curDirectory.empty() ? rootDir : std::move(curDirectory)),
	inArchive(false)
{
	if ((rootDir != topDir && !fs::is_directory(rootDir)) || (curDir != topDir && !fs::is_directory(curDir)) || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid file browser arguments");
}

Browser::Browser(fs::path rootDirectory, fs::path container, fs::path file, PCall exitCall, bool checkFile) :
	exCall(exitCall),
	rootDir(std::move(rootDirectory)),
	curDir(std::move(container)),
	curFile(std::move(file))
{
	if ((rootDir != topDir && !fs::is_directory(rootDir)) || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid archive browser arguments");

	if (inArchive = !fs::is_directory(curDir)) {
		if (!FileSys::isArchive(curDir))
			throw std::runtime_error(curDir.u8string() + " isn't a directory or archive");
		if (checkFile && !FileSys::isArchivePicture(curDir, curFile.u8string()))
			throw std::runtime_error(curFile.u8string() + " isn't a valid picture");
	} else if (fs::path path = curDir / curFile; checkFile && !FileSys::isPicture(path))
		throw std::runtime_error(path.u8string() + " isn't a valid picture or archive");
}

fs::file_type Browser::goTo(const fs::path& path) {
	if (!isSubpath(path, rootDir))
		return fs::file_type::none;

	try {
		if (fs::is_directory(path)) {
			curDir = path;
			inArchive = false;
			return fs::file_type::directory;
		}
	} catch (...) {}
	if (FileSys::isArchive(path)) {
		curDir = path;
		inArchive = true;
		return fs::file_type::directory;
	}
	if (FileSys::isPicture(path)) {
		curDir = parentPath(path);
		inArchive = false;
		return fs::file_type::regular;
	}
	return fs::file_type::none;
}

bool Browser::goIn(const fs::path& dname) {
	try {
		if (fs::path newPath = curDir / dname; fs::is_directory(newPath)) {
			curDir = std::move(newPath);
			return true;
		}
	} catch (...) {}
	return false;
}

fs::file_type Browser::goFile(const fs::path& fname) {
	if (inArchive) {
		if (FileSys::isArchivePicture(curDir, fname.u8string())) {
			curFile = fname;
			return fs::file_type::regular;
		}
	} else if (fs::path path = curDir / fname; FileSys::isPicture(path)) {
		curFile = fname;
		return fs::file_type::regular;
	} else if (FileSys::isArchive(path)) {
		curDir = std::move(path);
		inArchive = true;
		return fs::file_type::directory;
	}
	return fs::file_type::none;
}

bool Browser::goUp() {
	if (curDir == rootDir)
		return false;

	inArchive = false;
	try {
		if (curDir = parentPath(curDir); !fs::is_directory(curDir))
			throw 0;
	} catch (...) {
		curDir = rootDir;
	}
	return true;
}

void Browser::goNext(bool fwd, bool showHidden) {
	try {
		if (inArchive)
			shiftArchive(fwd, showHidden);
		else if (curDir != rootDir)
			shiftDir(fwd, showHidden);
	} catch (...) {}
}

void Browser::shiftDir(bool fwd, bool showHidden) {
	// find id of current directory and set it to the path of the next valid directory in the parent directory
	fs::path dir = parentPath(curDir);
	vector<fs::path> dirs = FileSys::listDir(dir, false, true, showHidden);
	if (vector<fs::path>::iterator di = std::find_if(dirs.begin(), dirs.end(), [this, &dir](const fs::path& it) -> bool { return dir / it == curDir; }); di != dirs.end())
		foreachAround(dirs, di, fwd, this, &Browser::nextDir, dir, showHidden);
	curFile.clear();
}

bool Browser::nextDir(const fs::path& dit, const fs::path& pdir, bool showHidden) {
	fs::path idir = pdir / dit;
	vector<fs::path> files = FileSys::listDir(idir, true, false, showHidden);
	if (vector<fs::path>::iterator fi = std::find_if(files.begin(), files.end(), [&idir](const fs::path& it) -> bool { return FileSys::isPicture(idir / it);}); fi != files.end()) {
		curDir = std::move(idir);
		return true;
	}
	return false;
}

void Browser::shiftArchive(bool fwd, bool showHidden) {
	// get list of archive files in the same directory and find id of the current file and select the next one
	fs::path dir = parentPath(curDir);
	vector<fs::path> files = FileSys::listDir(dir, true, false, showHidden);
	if (vector<fs::path>::iterator fi = std::find_if(files.begin(), files.end(), [this, &dir](const fs::path& it) -> bool { return curDir == dir / it; }); fi != files.end())
		foreachAround(files, fi, fwd, this, &Browser::nextArchive, dir);
	curFile.clear();
}

bool Browser::nextArchive(const fs::path& ait, const fs::path& pdir) {
	if (fs::path path = pdir / ait; FileSys::isPictureArchive(path)) {
		curDir = std::move(path);
		return true;
	}
	return false;
}

string Browser::nextDirFile(const string& file, bool fwd, bool showHidden) const {
	if (!file.empty()) {
		vector<fs::path> files = FileSys::listDir(curDir, true, false, showHidden);
		for (sizet mov = btom<sizet>(fwd), i = sizet(std::find(files.begin(), files.end(), fs::u8path(file)) - files.begin()) + mov; i < files.size(); i += mov)
			if (FileSys::isPicture(curDir / files[i]))
				return files[i].u8string();
	}
	return string();
}

string Browser::nextArchiveFile(const string& file, bool fwd) const {
	if (vector<string> pics; !file.empty()) {
		FileSys::listArchivePictures(curDir, pics);
		if (sizet i = sizet(std::find(pics.begin(), pics.end(), file) - pics.begin()) + btom<sizet>(fwd); i < pics.size())
			return pics[i];
	}
	return string();
}

pair<vector<string>, vector<string>> Browser::listCurDir(bool showHidden) const {
	if (inArchive)
		return pair(FileSys::listArchive(curDir), vector<string>());

	pair<vector<fs::path>, vector<fs::path>> in = FileSys::listDirSep(curDir, showHidden);
	pair<vector<string>, vector<string>> out(in.first.size(), in.second.size());
	std::transform(in.first.begin(), in.first.end(), out.first.begin(), [](const fs::path& it) -> string { return it.u8string(); });
	std::transform(in.second.begin(), in.second.end(), out.second.begin(), [](const fs::path& it) -> string { return it.u8string(); });
	return out;
}

template <class T, class P, class F, class... A>
bool Browser::foreachFAround(const vector<T>& vec, typename vector<T>::const_iterator start, P* parent, F func, A... args) {
	if (std::find_if(start + 1, vec.end(), [parent, func, args...](const T& it) -> bool { return (parent->*func)(it, args...); }) != vec.end())
		return true;
	return std::find_if(vec.begin(), start, [parent, func, args...](const T& it) -> bool { return (parent->*func)(it, args...); }) != start;
}

template <class T, class P, class F, class... A>
bool Browser::foreachRAround(const vector<T>& vec, typename vector<T>::const_reverse_iterator start, P* parent, F func, A... args) {
	if (std::find_if(start + 1, vec.rend(), [parent, func, args...](const T& it) -> bool { return (parent->*func)(it, args...); }) != vec.rend())
		return true;
	return !vec.empty() && std::find_if(vec.rbegin(), start, [parent, func, args...](const T& it) -> bool { return (parent->*func)(it, args...); }) != start;
}

template <class T, class P, class F, class... A>
bool Browser::foreachAround(const vector<T>& vec, typename vector<T>::const_iterator start, bool fwd, P* parent, F func, A... args) {
	return fwd ? foreachFAround(vec, start, parent, func, args...) : foreachRAround(vec, std::make_reverse_iterator(start + 1), parent, func, args...);
}
