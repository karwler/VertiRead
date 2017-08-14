#pragma once

#include "utils/types.h"

// logic for browsing files
class Browser {
public:
	Browser(const string& DR=string(), const string& DC=string());

	vector<string> listFiles() const;	// list current directory's files
	vector<string> listDirs() const;	// list current directory's directories
	bool goTo(const string& dirname);
	bool goUp();		// go to parent direcory if possible
	string goNext();	// go to the next directory from the viewpoint of the parent directory
	string goPrev();	// go to the previous directory from the viewpoint of the parent directory

	string getRootDir() const;
	string getCurDir() const;
	
private:
	string rootDir;	// the top directory we can visit
	string curDir;	// directory in which we currently are
};
