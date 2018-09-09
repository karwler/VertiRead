#pragma once

#include "engine/fileSys.h"

// logic for browsing files
class Browser {
public:
	Browser(const string& rootDirectory, const string& curDirectory, PCall exitCall);
	Browser(const string& rootDirectory, const string& container, const string& file, PCall exitCall, bool checkFile);

	bool goTo(const string& path);
	bool goIn(const string& dirname);
	bool goUp();			// go to parent direcory if possible
	void goNext(bool fwd);	// go to the next/previous archive or directory from the viewpoint of the parent directory
	bool selectFile(const string& fname);
	void clearCurFile();

	const string& getRootDir() const { return rootDir; }
	const string& getCurDir() const { return curDir; }
	const string& getCurFile() const { return curFile; }
	string curFilepath() const { return childPath(curDir, curFile); }
	bool getInArchive() const { return inArchive; }

	PCall exCall;	// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;	// the top directory one can visit
	string curDir;	// directory in which one currently is
	string curFile;	// currently selected or temporarily saved file (name) in curDir
	bool inArchive;	// whether curDir is an archive

	void shiftDir(bool fwd);
	void shiftArchive(bool fwd);
	bool nextDir(const string& dit, const string& pdir);
	bool nextArchive(const string& ait, const string& pdir);
};
