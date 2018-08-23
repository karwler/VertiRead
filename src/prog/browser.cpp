#include "browser.h"

Browser::Browser(string rootDirectory, string curDirectory, PCall exitCall) :
	exCall(exitCall),
	curType(FileType::none)
{
	rootDirectory = absolutePath(rootDirectory);
	rootDir = Filer::fileType(rootDirectory) == FTYPE_DIR ? rootDirectory : dseps;

	if (curDirectory.empty())
		curDir = rootDir;
	else {
		curDirectory = absolutePath(curDirectory);
		curDir = Filer::fileType(curDirectory) == FTYPE_DIR && isSubpath(curDirectory, rootDir) ? curDirectory : rootDir;
	}
}

bool Browser::goIn(const string& dirname) {
	if (dirname.empty())
		return false;

	string newPath = childPath(curDir, dirname);
	if (Filer::fileType(newPath) == FTYPE_DIR) {
		curDir = newPath;
		clearCurFile();
		return true;
	}
	return false;
}

bool Browser::goUp() {
	if (!directoryCmp(curDir, rootDir)) {
		curDir = parentPath(curDir);
		clearCurFile();
		return true;
	}
	return false;
}

void Browser::goNext(bool fwd) {
	if (curType == FileType::archive)
		shiftArchive(fwd);
	else if (!directoryCmp(curDir, rootDir))
		shiftDir(fwd);
}

void Browser::shiftDir(bool fwd) {
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);

	// find id of current directory and set it to the path of the next directory in the parent directory
	for (sizt i = 0; i < dirs.size(); i++)
		if (directoryCmp(childPath(parent, dirs[i]), curDir)) {
			curDir = parent + dirs[nextIndex(i, dirs.size(), fwd)];
			break;
		}
	
	// loop through current directory's files and select the first one that's openable
	for (string& it : Filer::listDir(curDir, FTYPE_FILE))
		if (selectFile(curDir, it))
			return;
	clearCurFile();
}

void Browser::shiftArchive(bool fwd) {
	// get list of archive files in the same directory
	vector<string> files; 
	for (string& it : Filer::listDir(curDir, FTYPE_FILE))
		if (Filer::isArchive(childPath(curDir, it)))
			files.push_back(it);

	// find id of the current file and select the next one
	for (sizt i = 0; i < files.size(); i++)
		if (curFile == files[i]) {
			curFile = files[nextIndex(i, files.size(), fwd)];
			break;
		}
}

bool Browser::selectFile(const string& drc, const string& fname) {
	string path = childPath(drc, fname);
	if (Filer::isPicture(path))
		curType = FileType::picture;
	else if (Filer::isArchive(path))
		curType = FileType::archive;
	else
		return false;

	curFile = fname;
	return true;
}

void Browser::clearCurFile() {
	curFile.clear();
	curType = FileType::none;
}
