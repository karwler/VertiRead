#pragma once

#include "utils/types.h"

class Browser {
public:
	Browser(const fs::path& RD=fs::path(), const fs::path& CD=fs::path());

	vector<fs::path> ListFiles();
	vector<fs::path> ListDirs();
	bool GoTo(const string& dirname);
	bool GoUp();
	fs::path GoNext();
	fs::path GoPrev();

	fs::path RootDir() const;
	fs::path CurDir() const;
	
private:
	fs::path rootDir;
	fs::path curDir;
};
