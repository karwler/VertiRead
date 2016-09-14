#pragma once

#include "utils/types.h"

class Browser {
public:
	Browser(const string& RD=string(), const string& CD=string());

	vector<string> ListFiles();
	vector<string> ListDirs();
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
