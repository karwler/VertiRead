#include "browser.h"
#include "engine/filer.h"

Browser::Browser(fs::path RD, fs::path CD) :
	rootDir(RD),
	curDir(CD)
{
	if (rootDir.empty())
		rootDir = Filer::execDir();
	if (curDir.empty())
		curDir = rootDir;
}

vector<fs::path> Browser::ListFiles() {
	return Filer::ListDir(curDir, FILTER_FILE);
}

vector<fs::path> Browser::ListDirs() {
	return Filer::ListDir(curDir, FILTER_DIR);
}

bool Browser::GoTo(string dirname) {
	fs::path newPath = curDir.string() + dsep + dirname;
	if (!fs::exists(newPath))
		return false;
	curDir = newPath;
	return true;
}

bool Browser::GoUp() {
	if (curDir == rootDir)
		return false;
	curDir = curDir.parent_path();
	return true;
}

fs::path Browser::RootDir() const {
	return rootDir;
}

fs::path Browser::CurDir() const {
	return curDir;
}
