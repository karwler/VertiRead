#pragma once

#include "types.h"
#include <atomic>
#include <thread>

// logic for browsing files
class Browser {
public:
#ifdef _WIN32
	static constexpr wchar topDir[] = L"";
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
	fs::path rootDir;	// the top directory one can visit
	fs::path curDir;	// directory in which one currently is
	ArchiveDir arch;	// archive directory tree root
	ArchiveDir* curNode = nullptr;

	FileWatch fwatch;
	std::thread thread;
	std::atomic<ThreadType> threadType;

public:
	Browser(fs::path root, fs::path directory, PCall exit = nullptr);
	~Browser();

	static uptr<Browser> openExplorer(fs::path rootDir, fs::path curDir, PCall exitCall);
	bool openPicture(fs::path rootDir, fs::path dirc, fs::path file);
	Response openFile(const fs::path& file);

	Response goTo(const fs::path& path);
	bool goIn(const fs::path& dname);
	bool goFile(const fs::path& fname);
	bool goUp();
	bool goNext(bool fwd, string_view picname);

	const fs::path& getRootDir() const;
	const fs::path& getCurDir() const;
	fs::path currentLocation() const;
	string curDirSuffix() const;
	pair<vector<string>, vector<string>> listCurDir() const;
	pair<vector<pair<bool, string>>, bool> directoryUpdate();

	bool finishArchive(BrowserResultAsync&& ra);
	void startPreview(const vector<string>& files, const vector<string>& dirs, int maxHeight);
	void startLoadPictures(const fs::path& first, bool fwd = true);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread();

private:
	void shiftDir(bool fwd);
	void shiftArchive(bool fwd);
	bool nextDir(const fs::path& dit, const fs::path& pdir);
	bool nextArchiveDir(ArchiveDir& dit);
	bool nextArchive(const fs::path& ait, const fs::path& pdir);
	string nextDirFile(string_view file, bool fwd) const;
	string nextArchiveFile(string_view file, bool fwd) const;

	template <class T, class F, class... A> bool foreachFAround(vector<T>& vec, typename vector<T>::iterator start, F func, A&&... args);
	template <class T, class F, class... A> bool foreachRAround(vector<T>& vec, typename vector<T>::reverse_iterator start, F func, A&&... args);
	template <class T, class F, class... A> bool foreachAround(vector<T>& vec, typename vector<T>::iterator start, bool fwd, F func, A&&... args);

	void startArchive(BrowserResultAsync&& ra);
	void cleanupArchive();

	void cleanupPreview();
	static void previewDirThread(std::atomic<ThreadType>& mode, fs::path curDir, vector<string> files, vector<string> dirs, fs::path iconPath, bool showHidden, int maxHeight);
	static void previewArchThread(std::atomic<ThreadType>& mode, fs::path curDir, ArchiveDir root, string dir, fs::path iconPath, int maxHeight);
	static ArchiveDir sliceArchiveFiles(const ArchiveDir& node);
	static SDL_Surface* loadDirectoryIcon(fs::path drc, int height);
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img);
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight);
	static char* allocatePreviewName(string_view name, bool file);

	void startLoadPictures(BrowserResultPicture* rp, bool fwd = true);
	void cleanupLoadPictures();
	static umap<string, uintptr_t> mapArchiveFiles(string_view dir, const vector<ArchiveFile>& files, const PicLim& picLim, const string& firstPic, bool fwd);
	static void loadPicturesDirThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, PicLim picLim, bool fwd, bool showHidden);
	static void loadPicturesArchThread(std::atomic<ThreadType>& mode, BrowserResultPicture* rp, umap<string, uintptr_t> files, PicLim picLim);
	static tuple<size_t, uintptr_t, uint8> initLoadLimits(const PicLim& picLim, size_t max);
	static string limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, uint8 mag);
	static char* progressText(string_view val, string_view lim);
};

inline Browser::~Browser() {
	stopThread();
}

inline const fs::path& Browser::getRootDir() const {
	return rootDir;
}

inline const fs::path& Browser::getCurDir() const {
	return curDir;
}

inline void Browser::startLoadPictures(const fs::path& first, bool fwd) {
	startLoadPictures(new BrowserResultPicture(curNode, fs::path(), valcp(curDir), fs::path(first)), fwd);
}
