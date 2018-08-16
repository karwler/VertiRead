#pragma once

#include "engine/filer.h"

// logic for browsing files
class Browser {
public:
	enum class FileType : uint8 {
		none,
		picture,
		archive
	};

	Browser(string rootDirectory="", string curDirectory="", PCall exitCall=nullptr);

	bool goIn(const string& dirname);
	bool goUp();			// go to parent direcory if possible
	void goNext(bool fwd);	// go to the next/previous archive or directory from the viewpoint of the parent directory
	bool selectFile(const string& filename) { return selectFile(curDir, filename); }

	const string& getRootDir() const { return rootDir; }
	const string& getCurDir() const { return curDir; }
	const string& getCurFile() const { return curFile; }
	string curFilepath() const { return appendDsep(curDir) + curFile; }
	FileType getCurType() const { return curType; }

	PCall exCall;	// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;	// the top directory one can visit
	string curDir;	// directory in which one currently is
	string curFile;	// currently selected or temporarily saved file (name) in curDir
	FileType curType;

	void shiftDir(bool fwd);
	void shiftArchive(bool fwd);
	bool selectFile(const string& drc, const string& fname);
	void clearCurFile();
};
