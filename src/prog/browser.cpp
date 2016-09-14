#include "browser.h"
#include "engine/filer.h"

Browser::Browser(const string& RD, const string& CD)
{
	rootDir = RD.empty() ? Filer::dirExec : appendDsep(RD);
	curDir = CD.empty() ? rootDir : appendDsep(CD);
}

vector<string> Browser::ListFiles() {
#ifdef _WIN32
	if (curDir == "\\")
		return {};
#endif
	return Filer::ListDir(curDir, FILTER_FILE);
}

vector<string> Browser::ListDirs() {
#ifdef _WIN32
	if (curDir == "\\") {
		vector<char> letters = Filer::ListDrives();
		vector<string> drives(letters.size());
		for (size_t i=0; i!=drives.size(); i++)
			drives[i] = letters[i] + string(":");
		return drives;
	}
#endif
	return Filer::ListDir(curDir, FILTER_DIR);
}

bool Browser::GoTo(const string& dirname) {
	if (dirname.empty())
		return false;

	string newPath;
#ifdef _WIN32
	if (curDir == "\\") {
		newPath = appendDsep(dirname);
		bool isOk = false;
		for (char c : Filer::ListDrives())
			if (c == dirname[0]) {
				isOk = true;
				break;
			}
		
		if (!isOk || !isDriveLetter(dirname))
			return false;
	}
	else {
		newPath = curDir + appendDsep(dirname);
		if (Filer::FileType(newPath) != EFileType::dir)
			return false;
	}
#else
	newPath = curDir + appendDsep(dirname);
	if (Filer::FileType(newPath) != EFileType::dir)
		return false;
#endif
	curDir = newPath;
	return true;
}

bool Browser::GoUp() {
	if (curDir == rootDir)
		return false;

#ifdef _WIN32
	curDir = isDriveLetter(curDir) ? "\\" : parentPath(curDir);
#else
	curDir = parentPath(curDir);
#endif
	return true;
}

string Browser::GoNext() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		vector<char> letters = Filer::ListDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++)
			if (*it == curDir[0]) {
				curDir[0] = (it == letters.end()-1) ? *letters.begin() : *++it;
				break;
			}
	}
	else {
		string parent = parentPath(curDir);
		vector<string> dirs = Filer::ListDir(parent, FILTER_DIR);
		for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (parent+*it+dsep == curDir) {
				curDir = (it == dirs.end()-1) ? parent + *dirs.begin() + dsep : parent + *++it + dsep;
				break;
			}
	}
#else
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::ListDir(parent, FILTER_DIR);
	for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
		if (*it == curDir) {
			curDir = (it == dirs.end()-1) ? parent + *dirs.begin() : parent + *++it;
			break;
	}
#endif
	return curDir;
}

string Browser::GoPrev() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		vector<char> letters = Filer::ListDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++)
			if (*it == curDir[0]) {
				curDir[0] = (it == letters.begin()) ? *(letters.end()-1) : *--it;
				break;
			}
	}
	else {
		string parent = parentPath(curDir);
		vector<string> dirs = Filer::ListDir(parent, FILTER_DIR);
		for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (parent+*it+dsep == curDir) {
				curDir = (it == dirs.begin()) ? parent + *(dirs.end()-1) + dsep : parent + *--it + dsep;
				break;
			}
	}
#else
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::ListDir(parent, FILTER_DIR);
	for (vector<string>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
		if (*it == curDir) {
			curDir = (it == dirs.begin()) ? parent + *(dirs.end()-1) : parent + *--it;
			break;
	}
#endif
	return curDir;
}

string Browser::RootDir() const {
	return rootDir;
}

string Browser::CurDir() const {
	return curDir;
}
