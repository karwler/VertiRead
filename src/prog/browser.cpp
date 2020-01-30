#include "engine/world.h"

Browser::Browser(string rootDirectory, string curDirectory, PCall exitCall) :
	exCall(exitCall),
	rootDir(std::move(rootDirectory)),
	curDir(curDirectory.empty() ? rootDir : std::move(curDirectory)),
	inArchive(false)
{
	if (FileSys::fileType(rootDir) != FTYPE_DIR || FileSys::fileType(curDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid file browser arguments");
}

Browser::Browser(string rootDirectory, string container, string file, PCall exitCall, bool checkFile) :
	exCall(exitCall),
	rootDir(std::move(rootDirectory)),
	curDir(std::move(container)),
	curFile(std::move(file))
{
	if (FileSys::fileType(rootDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid archive browser arguments");

	if (inArchive = FileSys::fileType(curDir) != FTYPE_DIR) {
		if (!FileSys::isArchive(curDir))
			throw std::runtime_error(curDir + " isn't a directory or archive");
		if (checkFile && !FileSys::isArchivePicture(curDir, curFile))
			throw std::runtime_error(curFile + " isn't a valid picture");
	} else if (string path = childPath(curDir, curFile); checkFile && !FileSys::isPicture(path))
		throw std::runtime_error(path + " isn't a valid picture or archive");
}

FileType Browser::goTo(const string& path) {
	if (!isSubpath(path, rootDir))
		return FTYPE_NON;

	if (FileSys::fileType(path) == FTYPE_DIR) {
		curDir = path;
		inArchive = false;
		return FTYPE_DIR;
	}
	if (FileSys::isArchive(path)) {
		curDir = path;
		inArchive = true;
		return FTYPE_DIR;
	}
	if (FileSys::isPicture(path)) {
		curDir = parentPath(path);
		inArchive = false;
		return FTYPE_REG;
	}
	return FTYPE_NON;
}

bool Browser::goIn(const string& dname) {
	if (string newPath = childPath(curDir, dname); !dname.empty() && FileSys::fileType(newPath) == FTYPE_DIR) {
		curDir = newPath;
		return true;
	}
	return false;
}

FileType Browser::goFile(const string& fname) {
	if (inArchive) {
		if (FileSys::isArchivePicture(curDir, fname)) {
			curFile = fname;
			return FTYPE_REG;
		}
	} else if (string path = childPath(curDir, fname); FileSys::isPicture(path)) {
		curFile = fname;
		return FTYPE_REG;
	} else if (FileSys::isArchive(path)) {
		curDir = path;
		inArchive = true;
		return FTYPE_DIR;
	}
	return FTYPE_NON;
}

bool Browser::goUp() {
	if (pathCmp(curDir, rootDir))
		return false;

	curDir = parentPath(curDir);
	inArchive = false;
	return true;
}

void Browser::goNext(bool fwd) {
	if (inArchive)
		shiftArchive(fwd);
	else if (!pathCmp(curDir, rootDir))
		shiftDir(fwd);
}

void Browser::shiftDir(bool fwd) {
	// find id of current directory and set it to the path of the next valid directory in the parent directory
	string dir = parentPath(curDir);
	vector<string> dirs = FileSys::listDir(dir, FTYPE_DIR, World::sets()->showHidden);
	if (vector<string>::iterator di = std::find_if(dirs.begin(), dirs.end(), [this, &dir](const string& it) -> bool { return pathCmp(childPath(dir, it), curDir); }); di != dirs.end())
		foreachAround(dirs, di, fwd, this, &Browser::nextDir, dir);
	curFile.clear();
}

bool Browser::nextDir(const string& dit, const string& pdir) {
	string idir = childPath(pdir, dit);
	vector<string> files = FileSys::listDir(idir, FTYPE_REG, World::sets()->showHidden);
	if (vector<string>::iterator fi = std::find_if(files.begin(), files.end(), [&idir](const string& it) -> bool { return FileSys::isPicture(childPath(idir, it));}); fi != files.end()) {
		curDir = idir;
		return true;
	}
	return false;
}

void Browser::shiftArchive(bool fwd) {
	// get list of archive files in the same directory and find id of the current file and select the next one
	string dir = parentPath(curDir);
	vector<string> files = FileSys::listDir(dir, FTYPE_REG, World::sets()->showHidden);
	if (vector<string>::iterator fi = std::find_if(files.begin(), files.end(), [this, &dir](const string& it) -> bool { return curDir == childPath(dir, it); }); fi != files.end())
		foreachAround(files, fi, fwd, this, &Browser::nextArchive, dir);
	curFile.clear();
}

bool Browser::nextArchive(const string& ait, const string& pdir) {
	if (string path = childPath(pdir, ait); FileSys::isPictureArchive(path)) {
		curDir = path;
		return true;
	}
	return false;
}

string Browser::nextDirFile(const string& file, bool fwd) const {
	if (!file.empty()) {
		vector<string> files = FileSys::listDir(curDir, FTYPE_REG, World::sets()->showHidden);
		for (sizet mov = btom<sizet>(fwd), i = sizet(std::find(files.begin(), files.end(), file) - files.begin()) + mov; i < files.size(); i += mov)
			if (FileSys::isPicture(childPath(curDir, files[i])))
				return files[i];
	}
	return "";
}

string Browser::nextArchiveFile(const string& file, bool fwd) const {
	if (file.empty())
		return string();
	archive* arch = FileSys::openArchive(curDir);
	if (!arch)
		return "";

	// list loadable pictures in archive
	vector<string> pics;
	for (archive_entry* entry; !archive_read_next_header(arch, &entry);)
		if (SDL_Surface* pic = FileSys::loadArchivePicture(arch, entry)) {
			SDL_FreeSurface(pic);
			pics.emplace_back(archive_entry_pathname(entry));
		}
	archive_read_free(arch);
	std::sort(pics.begin(), pics.end(), strnatless);

	// get next picture if there's one
	sizet i = sizet(std::find(pics.begin(), pics.end(), file) - pics.begin()) + btom<sizet>(fwd);
	return i < pics.size() ? pics[i] : "";
}

pair<vector<string>, vector<string>> Browser::listCurDir() const {
	return inArchive ? pair(FileSys::listArchive(curDir), vector<string>()) : FileSys::listDirSep(curDir, FTYPE_REG, World::sets()->showHidden);
}
