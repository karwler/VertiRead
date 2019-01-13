#include "engine/world.h"

Browser::Browser(const string& rootDirectory, const string& curDirectory, PCall exitCall) :
	exCall(exitCall),
	rootDir(rootDirectory),
	inArchive(false)
{
	curDir = curDirectory.empty() ? rootDir : curDirectory;
	if (FileSys::fileType(rootDir) != FTYPE_DIR || FileSys::fileType(curDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid file browser arguments");
}

Browser::Browser(const string& rootDirectory, const string& container, const string& file, PCall exitCall, bool checkFile) :
	exCall(exitCall),
	rootDir(rootDirectory),
	curDir(container),
	curFile(file)
{
	if (FileSys::fileType(rootDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("invalid archive browser arguments");

	if (FileSys::fileType(curDir) == FTYPE_DIR) {
		inArchive = false;
		if (string path = childPath(curDir, curFile); checkFile && !FileSys::isPicture(path))
			throw std::runtime_error(path + " isn't a valid picture or archive");
	} else {
		archive* arch = FileSys::openArchive(curDir);
		if (!arch)
			throw std::runtime_error(curDir + " isn't a directory or archive");

		inArchive = true;
		if (checkFile) {
			bool found = false;
			for (archive_entry* entry; !archive_read_next_header(arch, &entry);) {
				if (archive_entry_pathname(entry) != curFile)
					archive_read_data_skip(arch);
				else {
					if (SDL_Surface* pic = FileSys::loadArchivePicture(arch, entry)) {
						SDL_FreeSurface(pic);
						found = true;
					}
					break;
				}
			}
			archive_read_free(arch);
			if (!found)
				throw std::runtime_error(curFile + " isn't a valid picture");
		} else
			archive_read_free(arch);
	}
}

bool Browser::goTo(const string& path) {
	if (isSubpath(path, rootDir))
		switch (FileSys::fileType(path)) {
		case FTYPE_REG:
			curDir = parentPath(path);
			return true;
		case FTYPE_DIR:
			curDir = path;
			return true;
		}
	return false;
}

bool Browser::goIn(const string& dirname) {
	if (string newPath = childPath(curDir, dirname); !dirname.empty() && FileSys::fileType(newPath) == FTYPE_DIR) {
		curDir = newPath;
		return true;
	}
	return false;
}

bool Browser::goUp() {
	if (pathCmp(curDir, rootDir))
		return false;

	curDir = parentPath(curDir);
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
	if (string path = childPath(pdir, ait); FileSys::isArchive(path)) {
		curDir = path;
		return true;
	}
	return false;
}

bool Browser::selectFile(const string& fname) {
	if (string path = childPath(curDir, fname); FileSys::isPicture(path))
		curFile = fname;
	else if (FileSys::isArchive(path)) {
		curDir = path;
		curFile.clear();
		inArchive = true;
	} else
		return false;
	return true;
}

void Browser::clearCurFile() {
	curFile.clear();
	if (inArchive) {
		curDir = parentPath(curDir);
		inArchive = false;
	}
}
