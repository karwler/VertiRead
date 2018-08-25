#include "browser.h"

Browser::Browser(const string& rootDirectory, const string& curDirectory, PCall exitCall) :
	exCall(exitCall),
	rootDir(absolutePath(rootDirectory)),
	inArchive(false)
{
	curDir = curDirectory.empty() ? rootDir : absolutePath(curDirectory);
	if (Filer::fileType(rootDirectory) != FTYPE_DIR || Filer::fileType(curDir) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("Invalid file browser arguments");
}

Browser::Browser(const string& rootDirectory, const string& container, const string& file, PCall exitCall, bool checkFile) :
	exCall(exitCall),
	rootDir(absolutePath(rootDirectory)),
	curDir(absolutePath(container)),
	curFile(file)
{
	if (Filer::fileType(rootDirectory) != FTYPE_DIR || !isSubpath(curDir, rootDir))
		throw std::runtime_error("Invalid archive browser arguments");

	if (Filer::fileType(curDir) == FTYPE_DIR) {
		inArchive = false;
		string path = childPath(curDir, curFile);
		if (checkFile && !Filer::isPicture(path))
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

bool Browser::goIn(const string& dirname) {
	if (dirname.empty())
		return false;

	string newPath = childPath(curDir, dirname);
	if (Filer::fileType(newPath) == FTYPE_DIR) {
		curDir = newPath;
		return true;
	}
	return false;
}

bool Browser::goUp() {
	if (dirCmp(curDir, rootDir))
		return false;

	curDir = parentPath(curDir);
	return true;
}

void Browser::goNext(bool fwd) {
	if (inArchive)
		shiftArchive(fwd);
	else if (!dirCmp(curDir, rootDir))
		shiftDir(fwd);
}

void Browser::shiftDir(bool fwd) {
	// find id of current directory and set it to the path of the next directory in the parent directory
	string dir = parentPath(curDir);
	vector<string> dirs = Filer::listDir(dir, FTYPE_DIR);
	for (sizt i = 0; i < dirs.size(); i++)
		if (dirCmp(childPath(dir, dirs[i]), curDir)) {
			curDir = childPath(dir, dirs[nextIndex(i, dirs.size(), fwd)]);
			break;
		}
	curFile.clear();
}

void Browser::shiftArchive(bool fwd) {
	// get list of archive files in the same directory
	string dir = parentPath(curDir);
	vector<string> files; 
	for (string& it : Filer::listDir(dir, FTYPE_FILE))
		if (Filer::isArchive(childPath(dir, it)))
			files.push_back(it);

	// find id of the current file and select the next one
	for (sizt i = 0; i < files.size(); i++)
		if (curDir == childPath(dir, files[i])) {
			curDir = childPath(dir, files[nextIndex(i, files.size(), fwd)]);
			break;
		}
	curFile.clear();
}

bool Browser::selectFile(const string& fname) {
	string path = childPath(curDir, fname);
	if (Filer::isPicture(path))
		curFile = fname;
	else if (Filer::isArchive(path)) {
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
