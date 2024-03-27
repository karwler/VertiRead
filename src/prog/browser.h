#pragma once

#include "types.h"
#include <thread>

// logic for browsing files
class Browser {
public:
	static constexpr char dotStr[] = ".";
private:
	static constexpr uint previewSpeedyStopCheckInterval = 32;

	enum class ThreadType : uint8 {
		none,
		misc,
		list,
		preview,
		reader,
		next,
		archive
	};

public:
	void (Program::*exCall)();		// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;		// the top directory one can visit
	string curDir;		// directory or PDF in which one currently is
	ArchiveData arch;	// current archive directory tree root and info
	PdfFile pdf;		// current PDF file

	FileOps* fsop = nullptr;
	std::jthread thread;
	ThreadType curThread = ThreadType::none;

public:
	~Browser();

	string prepareNavigationPath(string_view path) const;
	uptr<RemoteLocation> prepareFileOps(string_view path);	// returns a location if a new connection is needed
	void beginFs(string&& root, const RemoteLocation& location, vector<string>&& passwords = vector<string>());
	void beginFs(string&& root, string&& path);
	bool goTo(const RemoteLocation& location, vector<string>&& passwords = vector<string>());	// returns whether to wait
	bool goTo(const string& path);	// ^
	bool openPicture(string&& rootDir, vector<string>&& paths);
	bool goIn(string_view dname);
	bool goFile(string_view fname);
	bool goUp();
	void startGoNext(string&& picname, bool fwd);
	void exitFile();

	const string& getCurDir() const { return curDir; }
	string locationForDisplay() const;
	vector<string> locationForStore(string_view pname) const;
	void startListCurDir();
	void startListDirDirs(string&& path);
	bool startDeleteEntry(string_view ename);
	bool renameEntry(string_view oldName, string_view newName);
	void setDirectoryWatch();
	bool directoryUpdate(vector<FileChange>& files);

	bool finishArchive(BrowserResultArchive&& ra);
	void startPreview(int maxHeight);
	void startLoadPictures(BrowserResultPicture* rp);
	void startReloadPictures(string&& first);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread();
	void requestStop();

private:
	template <Invocable<FileOps*, const RemoteLocation&> F> auto beginRemoteOps(const RemoteLocation& location, vector<string>&& passwords, F func);
	template <class T, class F> vector<T>::iterator foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F check);

	static void listDirFsThread(std::stop_token stoken, FileOps* fsop, string path, bool showHidden);
	static void listDirArchThread(std::stop_token stoken, ArchiveDir slice);
	static void listDirDirsThread(std::stop_token stoken, FileOps* fsop, string path, bool showHidden);
	void goNextThread(std::stop_token stoken, string picname, bool fwd);
	void startArchive(BrowserResultArchive* ra);

	static void previewDirThread(std::stop_token stoken, FileOps* fsop, string curDir, string iconPath, bool showHidden, int maxHeight);
	static void previewArchThread(std::stop_token stoken, FileOps* fsop, ArchiveData slice, string curDir, string iconPath, int maxHeight);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void previewPdf(std::stop_token stoken, PdfFile& pdfFile, int maxHeight, string_view fname);
#endif
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img);
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight);
	static char* allocatePreviewName(string_view name, bool file);

	umap<string, uintptr_t> prepareArchiveDirPicLoad(BrowserResultPicture* rp, const PicLim& picLim, uint8 compress);
	static void loadPicturesDirThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint8 compress, bool showHidden);
	static void loadPicturesArchThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, umap<string, uintptr_t> files, PicLim picLim);
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void loadPicturesPdfThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, float dpi, PicLim picLim, uint8 compress);
#endif
	static tuple<size_t, uintptr_t, pair<uint8, uint8>> initLoadLimits(const PicLim& picLim, size_t max);
	static string limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, pair<uint8, uint8> mag);
	static char* progressText(string_view val, string_view lim);
};

inline void Browser::startReloadPictures(string&& first) {
	startLoadPictures(new BrowserResultPicture(BRS_FWD, std::nullopt, valcp(curDir), std::move(first), arch.copyLight(), pdf.copyLight()));
}

inline void Browser::requestStop() {
	thread.request_stop();
}
