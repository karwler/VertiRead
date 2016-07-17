#include "browser.h"
#include "engine/filer.h"

Browser::Browser(const fs::path& RD, const fs::path& CD)
{
	rootDir = RD.empty() ? Filer::execDir() : RD;
	curDir = CD.empty() ? rootDir : CD;
}

vector<fs::path> Browser::ListFiles() {
#ifdef _WIN32
	if (curDir == "\\")
		return {};
	return Filer::ListDir(curDir, FILTER_FILE);
#else
	return Filer::ListDir(curDir, FILTER_FILE);
#endif
}

vector<fs::path> Browser::ListDirs() {
#ifdef _WIN32
	if (curDir == "\\") {
		vector<char> letters = Filer::ListDrives();
		vector<fs::path> drives(letters.size());
		for (size_t i=0; i!=drives.size(); i++)
			drives[i] = letters[i] + string(":");
		return drives;
	}
	else
		return Filer::ListDir(curDir, FILTER_DIR);
#else
	return Filer::ListDir(curDir, FILTER_DIR);
#endif
}

bool Browser::GoTo(const string& dirname) {
	if (dirname.empty())
		return false;

	fs::path newPath;
#ifdef _WIN32
	if (curDir == "\\") {
		newPath = (dirname[dirname.length()-1] == dsep) ? dirname : dirname + dsep;
		bool isOk = false;
		for (char c : Filer::ListDrives())
			if (c == dirname[0]) {
				isOk = true;
				break;
			}
		
		if (!isOk || !Filer::isDriveLetter(dirname))
			return false;
		cout << newPath << endl;
	}
	else {
		newPath = curDir.string() + dsep + dirname;
		if (!fs::is_directory(newPath))
			return false;
	}
#else
	newPath = curDir.string() + dsep + dirname;
	if (!fs::is_directory(newPath))
		return false;
#endif
	curDir = newPath;
	return true;
}

bool Browser::GoUp() {
	if (curDir == rootDir)
		return false;

#ifdef _WIN32
	curDir = Filer::isDriveLetter(curDir.string()) ? "\\" : curDir.parent_path();
#else
	curDir = curDir.parent_path();
#endif
	return true;
}

fs::path Browser::GoNext() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (Filer::isDriveLetter(curDir.string())) {
		vector<char> letters = Filer::ListDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++) {
			string cdCopy = curDir.string();
			if (*it == cdCopy[0]) {
				cdCopy[0] = (it == letters.end()-1) ? *letters.begin() : *++it;
				curDir = cdCopy;
				break;
			}
		}
	}
	else {
		vector<fs::path> dirs = Filer::ListDir(curDir.parent_path(), FILTER_DIR);
		for (vector<fs::path>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (*it == curDir) {
				curDir = (it == dirs.end()-1) ? *dirs.begin() : *++it;
				break;
			}
	}
#else
	vector<fs::path> dirs = Filer::ListDir(curDir.parent_path(), FILTER_DIR);
	for (vector<fs::path>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
		if (*it == curDir) {
			curDir = (it == dirs.end()-1) ? *dirs.begin() : *++it;
			break;
		}
#endif
	return curDir;
}

fs::path Browser::GoPrev() {
	if (curDir == rootDir)
		return curDir;

#ifdef _WIN32
	if (Filer::isDriveLetter(curDir.string())) {
		vector<char> letters = Filer::ListDrives();
		for (vector<char>::const_iterator it=letters.begin(); it!=letters.end(); it++) {
			string cdCopy = curDir.string();
			if (*it == cdCopy[0]) {
				cdCopy[0] = (it == letters.begin()) ? *(letters.end()-1) : *--it;
				curDir = cdCopy;
				break;
			}
		}
	}
	else {
		vector<fs::path> dirs = Filer::ListDir(curDir.parent_path(), FILTER_DIR);
		for (vector<fs::path>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
			if (*it == curDir) {
				curDir = (it == dirs.begin()) ? *(dirs.end()-1) : *--it;
				break;
			}
	}
#else
	vector<fs::path> dirs = Filer::ListDir(curDir.parent_path(), FILTER_DIR);
	for (vector<fs::path>::const_iterator it=dirs.begin(); it!=dirs.end(); it++)
		if (*it == curDir) {
			curDir = (it == dirs.begin()) ? *(dirs.end()-1) : *--it;
			break;
		}
#endif
	return curDir;
}

fs::path Browser::RootDir() const {
	return rootDir;
}

fs::path Browser::CurDir() const {
	return curDir;
}
