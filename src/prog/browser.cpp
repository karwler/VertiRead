#include "browser.h"

Browser::Browser(const string& RD, const string& CD, void (Program::*XC)(Button*)) :
	exCall(XC)
{
	rootDir = RD.empty() ? Filer::dirExec : RD;
	curDir = CD.empty() ? rootDir : CD;
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
	if (curDir == "\\") {	// if in "root" directory, get drive letters and present them as directories
		vector<char> letters = Filer::listDrives();
		vector<string> drives(letters.size());
		for (sizt i=0; i<drives.size(); i++)
			drives[i] = letters[i] + string(":");
		return drives;
	}
#endif
	return Filer::listDir(curDir, FTYPE_DIR);
}

bool Browser::goIn(const string& dirname) {
	if (dirname.empty())
		return false;

	curFile.clear();
#ifdef _WIN32
	if (curDir == "\\") {
		vector<char> letters = Filer::listDrives();
		if (isDriveLetter(dirname) && dirname[0] >= letters[0] && dirname[0] <= letters.back()) {
			curDir = dirname;
			return true;
		}
		return false;
	}
#endif
	return goInDir(dirname);
}

bool Browser::goInDir(const string& dirname) {
	string newPath = appendDsep(curDir) + dirname;
	if (Filer::fileType(newPath) != FTYPE_DIR)
		return false;

	curDir = newPath;
	return true;
}

bool Browser::goUp() {
	if (appendDsep(curDir) == appendDsep(rootDir))
		return false;

	curFile.clear();
#ifdef _WIN32
	curDir = isDriveLetter(curDir) ? "\\" : parentPath(curDir);
#else
	curDir = parentPath(curDir);
#endif
	return true;
}

void Browser::goNext() {
	if (appendDsep(curDir) == appendDsep(rootDir))
		return;

	curFile.clear();
#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		shiftLetter(1);
		return;
	}
#endif
	shiftDir(1);
}

void Browser::goPrev() {
	if (appendDsep(curDir) == appendDsep(rootDir))
		return;

	curFile.clear();
#ifdef _WIN32
	if (isDriveLetter(curDir)) {
		shiftLetter(-1);
		return;
	}
#endif
	shiftDir(-1);
}

#ifdef _WIN32
void Browser::shiftLetter(int ofs) {
	vector<char> letters = Filer::listDrives();
	for (sizt i=0; i<letters.size(); i++)
		if (letters[i] == curDir[0]) {
			curDir[0] = letters[(i + ofs) % letters.size()];
			break;
		}
}
#endif

void Browser::shiftDir(int ofs) {
	string cd = appendDsep(curDir);
	string parent = appendDsep(parentPath(curDir));
	vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);

	for (sizt i=0; i<dirs.size(); i++)
		if (appendDsep(parent + dirs[i]) == cd) {
			curDir = parent + dirs[(i + ofs) % dirs.size()];
			break;
		}
}

bool Browser::selectPicture(const string& filename) {
	if (Filer::isPicture(appendDsep(curDir) + filename)) {
		curFile = filename;
		return true;
	}
	return false;
}

void Browser::selectFirstPicture() {
	string cd = appendDsep(curDir);
	for (string& it : listFiles())
		if (Filer::isPicture(cd + it)) {
			curFile = it;
			return;
		}
	curFile.clear();
}
