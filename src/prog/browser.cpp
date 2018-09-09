#include "browser.h"

Browser::Browser(const string& rootDirectory, const string& curDirectory, PCall exitCall) :
	exCall(exitCall),
	rootDir(rootDirectory),
	inArchive(false)
{
	curDir = curDirectory.empty() ? rootDir : curDirectory;
	if (FileSys::fileType(rootDir) != FTYPE_DIR || FileSys::fileType(curDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("Invalid file browser arguments");
}

Browser::Browser(const string& rootDirectory, const string& container, const string& file, PCall exitCall, bool checkFile) :
	exCall(exitCall),
	rootDir(rootDirectory),
	curDir(container),
	curFile(file)
{
	if (FileSys::fileType(rootDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("Invalid archive browser arguments");

	if (FileSys::fileType(curDir) == FTYPE_DIR) {
		inArchive = false;
		string path = childPath(curDir, curFile);
		if (checkFile && !FileSys::isPicture(path))
			throw std::runtime_error(path + " isn't a valid picture or archive");
	} else {
		archive* arch = openArchive(curDir);
		if (!arch)
			throw std::runtime_error(curDir + " isn't a directory or archive");

		inArchive = true;
		if (checkFile) {
			bool found = false;
			archive_entry* entry;
			while (!archive_read_next_header(arch, &entry)) {
				if (archive_entry_pathname(entry) != curFile)
					archive_read_data_skip(arch);
				else {
					if (SDL_RWops* io = readArchiveEntry(arch, entry))
						if (SDL_Surface* pic = IMG_Load_RW(io, SDL_TRUE)) {
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
		case FTYPE_FILE:
			curDir = parentPath(path);
			return true;
		case FTYPE_DIR:
			curDir = path;
			return true;
		}
	return false;
}

bool Browser::goIn(const string& dirname) {
	string newPath = childPath(curDir, dirname);
	if (dirname.size() && FileSys::fileType(newPath) == FTYPE_DIR) {
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
	vector<string> dirs = FileSys::listDir(dir, FTYPE_DIR);
	for (sizt i = 0; i < dirs.size(); i++)
		if (pathCmp(childPath(dir, dirs[i]), curDir)) {
			foreachAround(dirs, i, fwd, this, &Browser::nextDir, dir);
			break;
		}
	curFile.clear();
}

bool Browser::nextDir(const string& dit, const string& pdir) {
	string idir = childPath(pdir, dit);
	for (string& it : FileSys::listDir(idir, FTYPE_FILE))
		if (FileSys::isPicture(childPath(idir, it))) {
			curDir = idir;
			return true;
		}
	return false;
}

void Browser::shiftArchive(bool fwd) {
	// get list of archive files in the same directory and find id of the current file and select the next one
	string dir = parentPath(curDir);
	vector<string> files = FileSys::listDir(dir, FTYPE_FILE);
	for (sizt i = 0; i < files.size(); i++)
		if (curDir == childPath(dir, files[i])) {
			foreachAround(files, i, fwd, this, &Browser::nextArchive, dir);
			break;
		}
	curFile.clear();
}

bool Browser::nextArchive(const string& ait, const string& pdir) {
	string path = childPath(pdir, ait);
	if (FileSys::isArchive(path)) {
		curDir = path;
		return true;
	}
	return false;
}

bool Browser::selectFile(const string& fname) {
	string path = childPath(curDir, fname);
	if (FileSys::isPicture(path))
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
