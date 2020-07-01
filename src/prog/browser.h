#pragma once

#include "engine/fileSys.h"

// logic for browsing files
class Browser {
public:
	PCall exCall;	// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	fs::path rootDir;	// the top directory one can visit
	fs::path curDir;	// directory in which one currently is
	fs::path curFile;	// currently selected or temporarily saved file (name) in curDir
	bool inArchive;		// whether curDir is an archive

public:
	Browser(fs::path rootDirectory, fs::path curDirectory, PCall exitCall);
	Browser(fs::path rootDirectory, fs::path container, fs::path file, PCall exitCall, bool checkFile);

	fs::file_type goTo(const fs::path& path);
	bool goIn(const fs::path& dname);
	fs::file_type goFile(const fs::path& fname);
	bool goUp();			// go to parent directory if possible
	void goNext(bool fwd, bool showHidden);	// go to the next/previous archive or directory from the viewpoint of the parent directory

	const fs::path& getRootDir() const;
	const fs::path& getCurDir() const;
	const fs::path& getCurFile() const;
	bool getInArchive() const;
	string nextFile(const string& file, bool fwd, bool showHidden) const;
	pair<vector<string>, vector<string>> listCurDir(bool showHidden) const;

private:
	void shiftDir(bool fwd, bool showHidden);
	void shiftArchive(bool fwd, bool showHidden);
	bool nextDir(const fs::path& dit, const fs::path& pdir, bool showHidden);
	bool nextArchive(const fs::path& ait, const fs::path& pdir);
	string nextDirFile(const string& file, bool fwd, bool showHidde) const;
	string nextArchiveFile(const string& file, bool fwd) const;

	template <class T, class P, class F, class... A> static bool foreachFAround(const vector<T>& vec, typename vector<T>::const_iterator start, P* parent, F func, A... args);
	template <class T, class P, class F, class... A> static bool foreachRAround(const vector<T>& vec, typename vector<T>::const_reverse_iterator start, P* parent, F func, A... args);
	template <class T, class P, class F, class... A> static bool foreachAround(const vector<T>& vec, typename vector<T>::const_iterator start, bool fwd, P* parent, F func, A... args);
};

inline const fs::path& Browser::getRootDir() const {
	return rootDir;
}

inline const fs::path& Browser::getCurDir() const {
	return curDir;
}

inline const fs::path& Browser::getCurFile() const {
	return curFile;
}

inline bool Browser::getInArchive() const {
	return inArchive;
}

inline string Browser::nextFile(const string& file, bool fwd, bool showHidden) const {
	return inArchive ? nextArchiveFile(file, fwd) : nextDirFile(file, fwd, showHidden);
}
