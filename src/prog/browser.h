#pragma once

#include "utils/types.h"

class Browser {
public:
	Browser(const string& RD=string(), const string& CD=string());

	vector<string> ListFiles() const;
	vector<string> ListDirs() const;
	bool GoTo(const string& dirname);
	bool GoUp();
	string GoNext();
	string GoPrev();

	string RootDir() const;
	string CurDir() const;
	
private:
	string rootDir;
	string curDir;
};
