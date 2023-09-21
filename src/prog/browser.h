#pragma once

#include "types.h"
#include <atomic>
#include <thread>

// logic for browsing files
class Browser {
public:
#ifdef _WIN32
	static constexpr char topDir[] = "";
#else
	static constexpr char topDir[] = "/";
#endif

	enum Response : uint8 {
		RNONE = 0x0,
		RWAIT = 0x1,
		REXPLORER = 0x2,
		RERROR = 0x4
	};

	PCall exCall;	// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;	// the top directory one can visit
	string curDir;	// directory in which one currently is
	ArchiveDir arch;	// archive directory tree root
	ArchiveDir* curNode = nullptr;

	FileWatch fwatch;
	std::thread thread;
	std::atomic<ThreadType> threadType;

public:
	Browser(string&& root, string&& directory, PCall exit = nullptr);
	~Browser();

	static uptr<Browser> openExplorer(string&& rootDir, string&& curDir, PCall exitCall);
	bool openPicture(string&& rootDir, string&& dirc, string&& file);
	Response openFile(string&& file);

	Response goTo(const char* path);
	bool goIn(string_view dname);
	bool goFile(string_view fname);
	bool goUp();
	bool goNext(bool fwd, string_view picname);

	const string& getRootDir() const;
	const string& getCurDir() const;
	string currentLocation() const;
	string curDirSuffix() const;
	pair<vector<string>, vector<string>> listCurDir() const;
	pair<vector<pair<bool, string>>, bool> directoryUpdate();

	bool finishArchive(BrowserResultAsync&& ra);
	void startPreview(const vector<string>& files, const vector<string>& dirs, int maxHeight);
	void startLoadPictures(string&& first, bool fwd = true);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread();

private:
	void shiftDir(bool fwd);
	void shiftArchive(bool fwd);
	bool nextDir(string_view dit, const string& pdir);
	bool nextArchiveDir(ArchiveDir& dit);
	bool nextArchive(string_view ait, const string& pdir);
	string nextDirFile(string_view file, bool fwd) const;
	string nextArchiveFile(string_view file, bool fwd) const;

	template <Class T, MemberFunction F, class... A> bool foreachFAround(vector<T>& vec, vector<T>::iterator start, F func, A&&... args);
	template <Class T, MemberFunction F, class... A> bool foreachRAround(vector<T>& vec, vector<T>::reverse_iterator start, F func, A&&... args);
	template <Class T, MemberFunction F, class... A> bool foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F func, A&&... args);

	void startArchive(BrowserResultAsync&& ra);
	void cleanupArchive();

	void cleanupPreview();
	static void previewDirThread(std::atomic<ThreadType>& mode, string curDir, vector<string> files, vector<string> dirs, string iconPath, bool showHidden, int maxHeight);
	static void previewArchThread(std::atomic<ThreadType>& mode, string curDir, ArchiveDir root, string dir, string iconPath, int maxHeight);
	static ArchiveDir sliceArchiveFiles(const ArchiveDir& node);
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img);
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight);
	static char* allocatePreviewName(string_view name, bool file);

	void startLoadPictures(BrowserResultPicture* rp, bool fwd = true);
	void cleanupLoadPictures();
	static umap<string, uintptr_t> mapArchiveFiles(string_view dir, const vector<ArchiveFile>& files, const PicLim& picLim, uint compress, string_view firstPic, bool fwd);
	static void loadPicturesDirThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, PicLim picLim, uint compress, bool fwd, bool showHidden);
	static void loadPicturesArchThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, umap<string, uintptr_t> files, PicLim picLim);
	static tuple<size_t, uintptr_t, uint8> initLoadLimits(const PicLim& picLim, size_t max);
	static string limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, uint8 mag);
	static char* progressText(string_view val, string_view lim);
};

inline Browser::~Browser() {
	stopThread();
}

inline const string& Browser::getRootDir() const {
	return rootDir;
}

inline const string& Browser::getCurDir() const {
	return curDir;
}

inline void Browser::startLoadPictures(string&& first, bool fwd) {
	startLoadPictures(new BrowserResultPicture(curNode, string(), valcp(curDir), std::move(first)), fwd);
}
