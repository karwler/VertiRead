#pragma once

#include "types.h"
#include "utils/settings.h"
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

	struct ListDirData {
		FileOps* fsop;
		string path;
		BrowserListOption opts;

		ListDirData(FileOps* fs, string loc, BrowserListOption options) noexcept;
	};

#ifdef WITH_ARCHIVE
	struct ListArchData {
		ArchiveDir slice;
		BrowserListOption opts;

		ListArchData(BrowserListOption options) : opts(options) {}
	};
#endif

	struct GoNextData {
		string picname;
		bool fwd;

		GoNextData(string&& pname, bool forward) noexcept;
	};

	struct PreviewDirData {
		FileOps* fsop;
		string curDir;
		string iconPath;
		int maxHeight;
		bool showHidden;

		PreviewDirData(FileOps* fs, string&& cdir, string&& iloc, int isize, bool hidden) noexcept;
	};

#ifdef WITH_ARCHIVE
	struct PreviewArchData {
		FileOps* fsop;
		ArchiveData slice;
		string curDir;
		string iconPath;
		int maxHeight;

		PreviewArchData(FileOps* fs, ArchiveData&& as, string&& cdir, string&& iloc, int isize) noexcept;
	};
#endif

	struct LoadPicturesDirData {
		FileOps* fsop;
		uptr<BrowserResultPicture> rp;
		PicLim picLim;
		uint8 compress;
		bool showHidden;

		LoadPicturesDirData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, uint8 cprs, bool hidden) noexcept;
	};

#ifdef WITH_ARCHIVE
	struct LoadPicturesArchData {
		FileOps* fsop;
		uptr<BrowserResultPicture> rp;
		umap<string, uintptr_t> files;
		PicLim picLim;

		LoadPicturesArchData(FileOps* fs, uptr<BrowserResultPicture>&& res, umap<string, uintptr_t>&& fmap, const PicLim& plim) noexcept;
	};
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	struct LoadPicturesPdfData {
		FileOps* fsop;
		uptr<BrowserResultPicture> rp;
		PicLim picLim;
		float dpi;
		uint8 compress;

		LoadPicturesPdfData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, float ddpi, uint8 cprs) noexcept;
	};
#endif

	class LoadProgress {
	private:
		string suffix;		// text with the total number
		size_t c = 0;		// entry progress
		size_t lim = 0;		// entry count limit
		uintptr_t m = 0;	// memory progress
		uintptr_t mem = 0;	// memory limit
		uint8 dmag = 0;		// memory unit display x1000
		uint8 smag = 0;		// memory unit display x1024

	public:
		LoadProgress(const PicLim& picLim, size_t max);

		bool ok() const { return c < lim && m < mem; }
		void pushImage(BrowserResultPicture* rp, Cstring&& name, SDL_Surface* img, const PicLim& picLim, uintptr_t imgSize);
	private:
		string numStr(const PicLim& picLim, size_t ci, uintptr_t mi) const;
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
	void startListCurDir(bool files = true);
	void startListDir(string&& path, bool files = true);
	bool startDeleteEntry(string_view ename);
	bool renameEntry(string_view oldName, string_view newName);
	void setDirectoryWatch();
	bool directoryUpdate(vector<FileChange>& files);

#ifdef WITH_ARCHIVE
	bool finishArchive(BrowserResultArchive&& ra);
#endif
	void startPreview(int maxHeight);
	void startLoadPictures(uptr<BrowserResultPicture>&& rp);
	void startReloadPictures(string&& first);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread();
	void requestStop();

private:
	template <Invocable<FileOps*, const RemoteLocation&> F> auto beginRemoteOps(const RemoteLocation& location, vector<string>&& passwords, F func);
	template <class T, class F> vector<T>::iterator foreachAround(vector<T>& vec, vector<T>::iterator start, bool fwd, F check);

	static void listDirFsThread(std::stop_token stoken, uptr<ListDirData> ld);
#ifdef WITH_ARCHIVE
	static void listDirArchThread(std::stop_token stoken, uptr<ListArchData> ld);
#endif
	void goNextThread(std::stop_token stoken, uptr<GoNextData> gd);
#ifdef WITH_ARCHIVE
	void startArchive(uptr<BrowserResultArchive>&& ra);
#endif

	static void previewDirThread(std::stop_token stoken, uptr<PreviewDirData> pd);
#ifdef WITH_ARCHIVE
	static void previewArchThread(std::stop_token stoken, uptr<PreviewArchData> pd);
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void previewPdf(std::stop_token stoken, PdfFile& pdfFile, int maxHeight, string_view fname);
#endif
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img);
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight);
	static char* allocatePreviewName(string_view name, bool file);

	umap<string, uintptr_t> prepareArchiveDirPicLoad(BrowserResultPicture* rp, const PicLim& picLim, uint8 compress, bool showHidden);
	static void loadPicturesDirThread(std::stop_token stoken, uptr<LoadPicturesDirData> ld);
#ifdef WITH_ARCHIVE
	static void loadPicturesArchThread(std::stop_token stoken, uptr<LoadPicturesArchData> ld);
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void loadPicturesPdfThread(std::stop_token stoken, uptr<LoadPicturesPdfData> ld);
#endif
};

inline void Browser::startReloadPictures(string&& first) {
	startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_FWD, std::nullopt, valcp(curDir), std::move(first), arch.copyLight(), pdf.copyLight()));
}

inline void Browser::requestStop() {
	thread.request_stop();
}
