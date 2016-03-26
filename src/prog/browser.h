#pragma once

#include "utils/types.h"

class Browser {
public:
	Browser(fs::path RD=fs::path(), fs::path CD=fs::path());

	vector<fs::path> ListFiles();
	vector<fs::path> ListDirs();
	bool GoTo(string dirname);
	bool GoUp();
	boost::filesystem::path GoNext();
	boost::filesystem::path GoPrev();

	fs::path RootDir() const;
	fs::path CurDir() const;
	
private:
	fs::path rootDir;
	fs::path curDir;
};
