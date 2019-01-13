#pragma once

#include "engine/fileSys.h"

// logic for browsing files
class Browser {
public:
	PCall exCall;	// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;	// the top directory one can visit
	string curDir;	// directory in which one currently is
	string curFile;	// currently selected or temporarily saved file (name) in curDir
	bool inArchive;	// whether curDir is an archive

public:
	Browser(const string& rootDirectory, const string& curDirectory, PCall exitCall);
	Browser(const string& rootDirectory, const string& container, const string& file, PCall exitCall, bool checkFile);

	bool goTo(const string& path);
	bool goIn(const string& dirname);
	bool goUp();			// go to parent direcory if possible
	void goNext(bool fwd);	// go to the next/previous archive or directory from the viewpoint of the parent directory
	bool selectFile(const string& fname);
	void clearCurFile();

	const string& getRootDir() const;
	const string& getCurDir() const;
	const string& getCurFile() const;
	string curFilepath() const;
	bool getInArchive() const;

private:
	void shiftDir(bool fwd);
	void shiftArchive(bool fwd);
	bool nextDir(const string& dit, const string& pdir);
	bool nextArchive(const string& ait, const string& pdir);
};

inline const string& Browser::getRootDir() const {
	return rootDir;
}

inline const string& Browser::getCurDir() const {
	return curDir;
}

inline const string& Browser::getCurFile() const {
	return curFile;
}

inline string Browser::curFilepath() const {
	return childPath(curDir, curFile);
}

inline bool Browser::getInArchive() const {
	return inArchive;
}
