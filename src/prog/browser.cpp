#include "browser.h"

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
			curDir = appendDsep(dirname);
			return true;
		}
		return false;
	}
#endif
	return goInDir(dirname);
}

bool Browser::goInDir(const string& dirname) {
	string newPath = curDir + appendDsep(dirname);
	if (Filer::fileType(newPath) != FTYPE_DIR)
		return false;

	curDir = newPath;
	return true;
}

bool Browser::goUp() {
	if (curDir == rootDir)
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
	if (curDir == rootDir)
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
	if (curDir == rootDir)
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
	string parent = parentPath(curDir);
	vector<string> dirs = Filer::listDir(parent, FTYPE_DIR);

	for (sizt i=0; i<dirs.size(); i++)
		if (parent + dirs[i] + dsep == curDir) {
			curDir = parent + dirs[(i + ofs) % dirs.size()] + dsep;
			break;
		}
}

bool Browser::selectPicture(const string& filename) {
	if (isPicture(curDir + filename)) {
		curFile = filename;
		return true;
	}
	return false;
}

void Browser::selectFirstPicture() {
	for (string& it : listFiles())
		if (isPicture(curDir + it)) {
			curFile = it;
			return;
		}
	curFile.clear();
}

bool Browser::isPicture(const string& file) {
	if (SDL_Surface* img = IMG_Load(file.c_str())) {
		SDL_FreeSurface(img);
		return true;
	}
	return false;
}
