#pragma once

#include "types.h"
#include "utils/settings.h"
#include "utils/stvector.h"
#include <thread>

// logic for browsing files
class Browser {
public:
	static constexpr char dotStr[] = ".";
private:
	static constexpr uint stopCheckInterval = 32;

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

		ListArchData(BrowserListOption options) noexcept : opts(options) {}
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
		bool showHidden;

		LoadPicturesDirData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, bool hidden) noexcept;
	};

#ifdef WITH_ARCHIVE
	struct LoadPicturesArchData {
		FileOps* fsop;
		uptr<BrowserResultPicture> rp;
		ArchiveDir slice;
		PicLim picLim;

		LoadPicturesArchData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim) noexcept;
	};
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	struct LoadPicturesPdfData {
		FileOps* fsop;
		uptr<BrowserResultPicture> rp;
		PicLim picLim;
		float scale;

		LoadPicturesPdfData(FileOps* fs, uptr<BrowserResultPicture>&& res, const PicLim& plim, float scl) noexcept;
	};
#endif

	class LoadProgress {
	private:
		string suffix;		// text with the total number
		size_t lim;			// entry count limit
		uintptr_t m = 0;	// memory progress
		uintptr_t mem;		// memory limit
		uint8 dmag;			// memory unit display x1000
		uint8 smag;			// memory unit display x1024
	public:
		uint8 cbpp;			// last recorded image bytes per pixel

		LoadProgress(const PicLim& picLim, size_t max);

		bool ok(const BrowserResultPicture* rp) const noexcept { return rp->cnt < lim && m < mem; }
		void pushImage(BrowserResultPicture* rp, Cstring&& name, uptr<SDL_Surface>& img, const PicLim& picLim);
	private:
		string numStr(const PicLim& picLim, size_t ci, uintptr_t mi) const;
	};

public:
	void (Program::*exCall)();		// gets called when goUp() fails, aka stepping out of rootDir into the previous menu
private:
	string rootDir;		// the top directory one can visit
	string curDir;		// directory or PDF in which one currently is	// TODO: how to handle this with a possible prefix?
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
	bool openPicture(string&& rootDir, stvector<string, Settings::maxPageElements>&& paths);
	bool goIn(string_view dname);
	bool goFile(string_view fname);
	bool goUp();
	void startGoNext(string&& picname, bool fwd);
	void exitFile();

	const string& getCurDir() const noexcept { return curDir; }
	string locationForDisplay() const;
	stvector<string, Settings::maxPageElements> locationForStore(string_view pname) const;
	bool isLocal() const noexcept;
	void startListCurDir(bool files = true);
	void startListDir(string&& path, bool files = true);
	bool startDeleteEntry(string_view ename);
	bool renameEntry(string_view oldName, string_view newName);
	void setDirectoryWatch() noexcept;
	bool directoryUpdate(vector<FileChange>& files) noexcept;

#ifdef WITH_ARCHIVE
	bool finishArchive(BrowserResultArchive&& ra);
#endif
	void startPreview(int maxHeight);
	void startLoadPictures(uptr<BrowserResultPicture>&& rp);
	void startReloadPictures(string&& first);
	void finishLoadPictures(BrowserResultPicture& rp);
	void stopThread() noexcept;
	void requestStop() noexcept;

private:
	template <Invocable<FileOps*, const RemoteLocation&> F> auto beginRemoteOps(const RemoteLocation& location, vector<string>&& passwords, F func);
	template <class T, class F> vector<T>::iterator foreachAround(vector<T>& vec, vector<T>::iterator start, bool found, bool fwd, F check);

	static void listDirFsThread(std::stop_token stoken, uptr<ListDirData> ld) noexcept;
#ifdef WITH_ARCHIVE
	static void listDirArchThread(std::stop_token stoken, uptr<ListArchData> ld) noexcept;
#endif
	template <InvocableR<uptr<BrowserResultList>> F> static void listDirThread(const std::stop_token& stoken, F func) noexcept;
	void goNextThread(std::stop_token stoken, uptr<GoNextData> gd) noexcept;
#ifdef WITH_ARCHIVE
	void startArchive(uptr<BrowserResultArchive>&& ra);
#endif

	static void previewDirThread(std::stop_token stoken, uptr<PreviewDirData> pd) noexcept;
#ifdef WITH_ARCHIVE
	static void previewArchThread(std::stop_token stoken, uptr<PreviewArchData> pd) noexcept;
	static SDL_Surface* findArchiveDirectoryThumbnail(const std::stop_token stoken, FileOps* fsop, ArchiveData& ad, vector<Cstring>& entries, CountedStopReq& csr);
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void previewPdf(const std::stop_token& stoken, PdfFile& pdfFile, int maxHeight, string_view fname);
#endif
	static SDL_Surface* combineIcons(SDL_Surface* dir, SDL_Surface* img) noexcept;
	static SDL_Surface* scaleDown(SDL_Surface* img, int maxHeight) noexcept;
	static void pushPreviewPicture(uptr<SDL_Surface>& img, string_view name, bool file);

	static void loadPicturesDirThread(std::stop_token stoken, uptr<LoadPicturesDirData> ld) noexcept;
#ifdef WITH_ARCHIVE
	static void loadPicturesArchThread(std::stop_token stoken, uptr<LoadPicturesArchData> ld) noexcept;
#endif
#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
	static void loadPicturesPdfThread(std::stop_token stoken, uptr<LoadPicturesPdfData> ld) noexcept;
#endif
};

inline void Browser::startReloadPictures(string&& first) {
	startLoadPictures(std::make_unique<BrowserResultPicture>(BRS_FWD, std::nullopt, valcp(curDir), std::move(first), arch.copyLight(), pdf.copyLight()));
}

inline void Browser::requestStop() noexcept {
	thread.request_stop();
}
