#pragma once

#include "engine/filer.h"

// logic for browsing files
class Browser {
public:
	Browser(const string& RD="", const string& CD="");

	vector<string> listFiles() const;	// list current directory's files
	vector<string> listDirs() const;	// list current directory's directories
	bool goIn(const string& dirname);
	bool goUp();		// go to parent direcory if possible
	void goNext();	// go to the next directory from the viewpoint of the parent directory
	void goPrev();	// go to the previous directory from the viewpoint of the parent directory
	bool selectPicture(const string& filename);
	void selectFirstPicture();

	const string& getRootDir() const { return rootDir; }
	const string& getCurDir() const { return curDir; }
	const string& getCurFile() const { return curFile; }
	string curFilepath() const { return curDir + curFile; }

private:
	string rootDir;	// the top directory one can visit
	string curDir;	// directory in which one currently is
	string curFile;	// currently selected or temporarily saved file (name) in curDir

	bool goInDir(const string& dirname);
#ifdef _WIN32
	void shiftLetter(int ofs);
#endif
	void shiftDir(int ofs);

	bool isPicture(const string& file);
};
