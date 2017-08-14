#include "browser.h"
#include "engine/filer.h"

Browser::Browser(const string& RD, const string& CD)
{
	rootDir = RD.empty() ? Filer::dirExec : appendDsep(RD);
	curDir = CD.empty() ? rootDir : appendDsep(CD);
}

vector<string> Browser::listFiles() const {
#ifdef _WIN32
	if (curDir == "\\")	// there shouldn't be any files. only drives
		return {};
#endif
	return Filer::listDir(curDir, FTYPE_FILE);
}

vector<string> Browser::listDirs() const {
#ifdef _WIN32
	if (curDir == "\\") {
		// get drive letters and present them as directories
		vector<char> letters = Filer::listDrives();
		vector<string> drives(letters.size());
		for (size_t i=0; i!=drives.size(); i++)
			drives[i] = letters[i] + string(":");
		return drives;
	}
#endif
	return Filer::listDir(curDir, FTYPE_DIR);
}

bool Browser::goTo(const string& dirname) {
	if (dirname.empty())
		return false;

	string newPath;
#ifdef _WIN32
	if (curDir == "\\") {
		newPath = appendDsep(dirname);
		bool isOk = false;
		for (char c : Filer::listDrives())
			if (c == dirname[0]) {
				isOk = true;
				break;
			}
		
		if (!isOk || !isDriveLetter(dirname))
			return false;
	} else {
		newPath = curDir + appendDsep(dirname);
		if (Filer::fileType(newPath) != FTYPE_DIR)
			return false;
	}
#else
	newPath = curDir + appendDsep(dirname);
	if (Filer::fileType(newPath) != FTYPE_DIR)
		return false;
#endif
	curDir = newPath;
	return true;
}

bool Browser::goUp() {
	if (curDir == rootDir)
		return false;

#ifdef _WIN32
	curDir = isDriveLetter(curDir) ? "\\" : parentPath(curDir);
#else
	curDir = parentPath(curDir);
#endif
	return true;
}

string Browser::goNext() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		vector<char> letters = Filer::listDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++)
			if (*it == curDir[0]) {
				curDir[0] = (it == letters.end()-1) ? *letters.begin() : *++it;
				break;
			}
	} else {
		string parent = parentPath(curDir);
		vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);
		for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (parent+*it+dsep == curDir) {
				curDir = (it == dirs.end()-1) ? parent + *dirs.begin() + dsep : parent + *++it + dsep;
				break;
			}
	}
#else
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);
	for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.vend(); it++)
		if (parent+*it+dsep == curDir) {
			curDir = (it == dirs.vend()-1) ? parent + *dirs.begin() + dsep : parent + *++it + dsep;
			break;
		}
#endif
	return curDir;
}

string Browser::goPrev() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		vector<char> letters = Filer::listDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++)
			if (*it == curDir[0]) {
				curDir[0] = (it == letters.begin()) ? *(letters.end()-1) : *--it;
				break;
			}
	} else {
		string parent = parentPath(curDir);
		vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);
		for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (parent+*it+dsep == curDir) {
				curDir = (it == dirs.begin()) ? parent + *(dirs.end()-1) + dsep : parent + *--it + dsep;
				break;
			}
	}
#else
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);
	for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.vend(); it++)
		if (parent+*it+dsep == curDir) {
			curDir = (it == dirs.begin()) ? parent + *(dirs.vend()-1) + dsep : parent + *--it + dsep;
			break;
		}
#endif
	return curDir;
}

string Browser::getRootDir() const {
	return rootDir;
}

string Browser::getCurDir() const {
	return curDir;
}
