#pragma once

#include "types.h"
#include <thread>

struct _PopplerDocument;
struct _PopplerPage;
struct _cairo_surface;

// logic for browsing files
class Browser {
public:
	static constexpr char dotStr[] = ".";
private:
	static constexpr uint previewSpeedyStopCheckInterval = 32;

	enum class ThreadType : uint8 {
		none,
		preview,
		reader,
		archive
	};

public:
	void (Program::*exCall)();		// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;		// the top directory one can visit
	string curDir;		// directory or PDF in which one currently is
	ArchiveData arch;	// current archive directory tree root and info

	FileOps* fsop = nullptr;
	std::jthread thread;
	ThreadType curThread = ThreadType::none;

public:
	~Browser();

	string prepareNavigationPath(string_view path) const;
	uptr<RemoteLocation> prepareFileOps(string_view path);	// returns a location if a new connection is needed
	void start(string&& root, const RemoteLocation& location, vector<string>&& passwords = vector<string>());
	void start(string&& root, string&& path);
	bool goTo(const RemoteLocation& location, vector<string>&& passwords = vector<string>());	// returns whether to wait
	bool goTo(const string& path);	// ^
	bool openPicture(string&& rootDir, vector<string>&& paths);
	bool goIn(string_view dname);
	bool goFile(string_view fname);
	bool goUp();
	bool goNext(bool fwd, string_view picname);
	void exitFile();

	const string& getCurDir() const { return curDir; }
	string locationForDisplay() const;
	vector<string> locationForStore(string_view pname) const;
	pair<vector<Cstring>, vector<Cstring>> listCurDir();
	vector<Cstring> listDirDirs(const string& path) const;
	bool deleteEntry(string_view ename);
	bool renameEntry(string_view oldName, string_view newName);
	bool directoryUpdate(vector<FileChange>& files);

	bool finishArchive(BrowserResultArchive&& ra);
	void startPreview(const vector<Cstring>& files, const vector<Cstring>& dirs, int maxHeight);
	void startReloadPictures(string&& first);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread();
	void requestStop();

private:
	bool inArchive() const;
	bool inPdf();
	template <Invocable<FileOps*, const RemoteLocation&> F> auto beginRemoteOps(const RemoteLocation& location, vector<string>&& passwords, F func);

	bool nextDir(const Cstring& dit, bool fwd, const string& pdir);
	bool nextArchiveDir(const ArchiveDir* dit, bool fwd, const string& pdir);
	bool nextPdf(const Cstring& fit, bool fwd, const string& pdir);
	bool nextArchivePdf(const ArchiveFile* fit, bool fwd, const string& pdir);
	string nextDirFile(string_view file, bool fwd);
	string nextArchiveFile(string_view file, bool fwd);
	string nextPdfPage(string_view file, bool fwd);
	template <class T, MemberFunction F, class... A> bool foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F func, A&&... args);

	void startArchive(BrowserResultArchive* ra);
	void cleanupArchive();

	void cleanupPreview();
	static void previewDirThread(std::stop_token stoken, FileOps* fsop, string curDir, vector<Cstring> files, vector<Cstring> dirs, string iconPath, bool showHidden, int maxHeight);
	static void previewArchThread(std::stop_token stoken, FileOps* fsop, ArchiveData slice, string curDir, string iconPath, int maxHeight);
#ifdef CAN_PDF
	static void previewPdf(std::stop_token stoken, _PopplerDocument* doc, int maxHeight, string_view fname);
#endif
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img);
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight);
	static char* allocatePreviewName(string_view name, bool file);

	void startLoadPictures(BrowserResultPicture* rp, bool fwd = true);
	void cleanupLoadPictures();
	umap<string, uintptr_t> prepareArchiveDirPicLoad(BrowserResultPicture* rp, const PicLim& picLim, uint compress, bool fwd);
	static void loadPicturesDirThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint compress, bool fwd, bool showHidden);
	static void loadPicturesArchThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, umap<string, uintptr_t> files, PicLim picLim, bool fwd);
#ifdef CAN_PDF
	static void loadPicturesPdfThread(std::stop_token stoken, BrowserResultPicture* rp, FileOps* fsop, PicLim picLim, uint compress, bool imageOnly, bool fwd);
	static vector<pair<dvec2, int>> getPdfPageImagesInfo(_PopplerPage* page);
	static SDL_Surface* cairoImageToSdl(_cairo_surface* img);
#endif
	static tuple<size_t, uintptr_t, pair<uint8, uint8>> initLoadLimits(const PicLim& picLim, size_t max);
	static string limitToStr(const PicLim& picLim, uintptr_t c, uintptr_t m, pair<uint8, uint8> mag);
	static char* progressText(string_view val, string_view lim);
};

inline void Browser::startReloadPictures(string&& first) {
	startLoadPictures(new BrowserResultPicture((inPdf() ? BRS_PDF : BRS_NONE), std::nullopt, valcp(curDir), std::move(first), arch.copyLight()));
}

inline void Browser::requestStop() {
	thread.request_stop();
}

inline bool Browser::inArchive() const {
	return !arch.name.empty();
}
