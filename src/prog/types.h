#pragma once

#include "utils/utils.h"
#include <forward_list>
#include <mutex>
#include <stop_token>
#include <SDL_events.h>

struct fz_context;
struct fz_document;
struct _PopplerDocument;

enum UserEvent : uint32 {
	SDL_USEREVENT_GENERAL = SDL_USEREVENT,
	SDL_USEREVENT_PROG_BOOKS,
	SDL_USEREVENT_PROG_FILE_EXPLORER,
	SDL_USEREVENT_PROG_PAGE_BROWSER,
	SDL_USEREVENT_PROG_READER,
	SDL_USEREVENT_PROG_SETTINGS,
	SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_PROG_MAX = SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_THREAD_LIST_FINISHED,
	SDL_USEREVENT_THREAD_DELETE_FINISHED,
	SDL_USEREVENT_THREAD_ARCHIVE_FINISHED,
	SDL_USEREVENT_THREAD_PREVIEW,
	SDL_USEREVENT_THREAD_READER,
	SDL_USEREVENT_THREAD_GO_NEXT_FINISHED,
	SDL_USEREVENT_THREAD_MOVE,
	SDL_USEREVENT_THREAD_FONTS_FINISHED,
	SDL_USEREVENT_MAX
};

enum class GeneralEvent : int32 {
	closePopup,
	closeContext,
	confirmComboBox,
	resizeComboContext,
	startRequestPassphrase,
	requestPassphrase,
	confirmPassphrase,
	cancelPassphrase,
	exit
};

enum class ProgBooksEvent : int32 {
	openBookList,
	openBookListLogin,
	openPageBrowser,
	openPageBrowserGeneral,
	openLastPage,
	openLastPageGeneral,
	askDeleteBook,
	deleteBook,
	queryRenameBook,
	renameBook,
	openFileLogin,
	openSettings
};

enum class ProgFileExplorerEvent : int32 {
	goUp,
	goIn,
	goTo,
	goToLogin,
	exit
};

enum class ProgPageBrowserEvent : int32 {
	fileLoadingCancelled,
	goFile
};

enum class ProgReaderEvent : int32 {
	zoomIn,
	zoomOut,
	zoomReset,
	zoomFit,
	centerView,
	nextDir,
	prevDir,
	exit
};

enum class ProgSettingsEvent : int32 {
	setDirection,
	setZoomType,
	setZoom,
	setSpacing,
	setLibraryDirLe,
	openLibDirBrowser,
	moveBooks,
	moveCancelled,
	setScreenMode,
	setRenderer,
	setDevice,
	setCompression,
	setVsync,
	setGpuSelecting,
	setMultiFullscreen,
	setPreview,
	setHide,
	setTooltips,
	setTheme,
	setFontCmb,
	setFontLe,
	setFontHinting,
	setScrollSpeed,
	setDeadzoneSl,
	setDeadzoneLe,
	setPicLimitType,
	setPicLimitCount,
	setPicLimitSize,
	setMaxPicResSl,
	setMaxPicResLe,
	reset
};

enum class ProgSearchDirEvent : int32 {
	goIn,
	setLibraryDirBw
};

enum class ThreadEvent : int32 {
	progress,
	finished
};

struct EventId {
	UserEvent type;
	int32 code;	// should not exceed 16 bits for widget events because of the packing in Button

	constexpr EventId(UserEvent t, int32 c) : type(t), code(c) {}
	constexpr EventId(GeneralEvent e) : type(SDL_USEREVENT_GENERAL), code(int32(e)) {}
	constexpr EventId(ProgBooksEvent e) : type(SDL_USEREVENT_PROG_BOOKS), code(int32(e)) {}
	constexpr EventId(ProgFileExplorerEvent e) : type(SDL_USEREVENT_PROG_FILE_EXPLORER), code(int32(e)) {}
	constexpr EventId(ProgPageBrowserEvent e) : type(SDL_USEREVENT_PROG_PAGE_BROWSER), code(int32(e)) {}
	constexpr EventId(ProgReaderEvent e) : type(SDL_USEREVENT_PROG_READER), code(int32(e)) {}
	constexpr EventId(ProgSettingsEvent e) : type(SDL_USEREVENT_PROG_SETTINGS), code(int32(e)) {}
	constexpr EventId(ProgSearchDirEvent e) : type(SDL_USEREVENT_PROG_SEARCH_DIR), code(int32(e)) {}

	constexpr operator bool() const { return bool(type); }
};

inline constexpr EventId nullEvent = EventId(UserEvent(0), 0);

void pushEvent(EventId id, void* data1 = nullptr, void* data2 = nullptr);
void pushEvent(UserEvent type, int32 code, void* data1 = nullptr, void* data2 = nullptr);

template <Enumeration T>
void pushEvent(UserEvent type, T code, void* data1 = nullptr, void* data2 = nullptr) {
	pushEvent(type, int32(code), data1, data2);
}

template <Invocable<SDL_UserEvent&> F>
void cleanupEvent(UserEvent type, F dealloc) {
	array<SDL_Event, 16> events;
	while (int num = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, type, type)) {
		if (num < 0)
			throw std::runtime_error(SDL_GetError());
		for (int i = 0; i < num; ++i)
			dealloc(events[i].user);
	}
}

enum class Protocol : uint8 {
	none,
	smb,
	sftp
};
inline constexpr array protocolNames = {
	"",
	"smb",
	"sftp"
};
inline constexpr array<uint16, protocolNames.size()> protocolPorts = {
	0,
	445,
	22
};

enum class Family : uint8 {
	any,
	v4,
	v6
};
inline constexpr array familyNames = {
	"any",
	"IPv4",
	"IPv6"
};

// connection information about a network location
struct RemoteLocation {
	string server;
	string path;
	string user;
	string workgroup;
	string password;
	uint16 port;
	Protocol protocol = Protocol::none;
	Family family = Family::any;

	static Protocol getProtocol(string_view str) noexcept;
	static RemoteLocation fromPath(string_view str, Protocol proto);
private:
	static uint16 sanitizePort(string_view port, Protocol protocol);
};

// a new or deleted directory entry
struct FileChange {
	enum Type {
		deleteEntry,
		addFile,
		addDirectory
	};

	Cstring name;
	Type type;

	FileChange(Cstring&& entry, Type change) : name(std::move(entry)), type(change) {}
};

#ifdef WITH_ARCHIVE
// archive file with image size
struct ArchiveFile {
	Cstring name;
	bool isPdf : 1 = false;
	uint64 size : 63 = 0;

	ArchiveFile() = default;
	ArchiveFile(Cstring&& filename, uint64 mem) : name(std::move(filename)), size(mem) {}
	ArchiveFile(Cstring&& pdfName) : name(std::move(pdfName)), isPdf(true) {}
};

// archive directory node
class ArchiveDir {
public:
	Cstring name;	// if this is a root node then the name is the path to the associated archive file
	std::forward_list<ArchiveDir> dirs;
	std::forward_list<ArchiveFile> files;

	ArchiveDir() = default;
	ArchiveDir(Cstring&& dirname) : name(std::move(dirname)) {}

	vector<ArchiveDir*> listDirs();
	vector<ArchiveFile*> listFiles();
	void finalize() noexcept;
	pair<ArchiveDir*, ArchiveFile*> find(string_view path);
	ArchiveDir* findDir(string_view dname) noexcept;
	ArchiveFile* findFile(string_view fname) noexcept;
	void copySlicedDentsFrom(const ArchiveDir& src, bool copyHidden);
	vector<Cstring> copySortedFiles(bool copyHidden) const;
	vector<Cstring> copySortedDirs(bool copyHidden) const;
private:
	template <Class T> static vector<Cstring> copySortedDents(const std::forward_list<T>& dents, bool copyHidden);
};

// archive file tree with optional passphrase and its loaded memory
struct ArchiveData : public ArchiveDir {
	enum class PassCode : uint8 {
		none,
		set,
		ignore,
		attempt
	};

	Data data;
	const Data* dref = nullptr;	// if this is null then the data member is used unless reading from a file
	string passphrase;
	PassCode pc = PassCode::none;

	ArchiveData() = default;
	ArchiveData(Cstring&& file, PassCode pass = PassCode::none) : ArchiveDir(std::move(file)), pc(pass) {}

	operator bool() const;
	ArchiveData copyLight() const;
};

inline ArchiveData::operator bool() const {
	return name.filled();
}

#else
class ArchiveData {
public:
	operator bool() const { return false; }
	ArchiveData copyLight() const { return ArchiveData(); }
};
#endif

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
// mupdf/poppler wrapper
class PdfFile : private Data {
public:
	static constexpr char signature[] = "%PDF-";
	static constexpr uint signatureLen = sizeof(signature) - sizeof(char);
private:
	static constexpr double defaultDpi = 72.0;

	fz_context* mctx;
	fz_document* mdoc = nullptr;
	_PopplerDocument* pdoc = nullptr;
	bool owner = false;

public:
	PdfFile() = default;
	PdfFile(PdfFile&& pdf) noexcept;
	PdfFile(SDL_RWops* ops, Cstring* error);
	~PdfFile() { freeDoc(); }

	PdfFile& operator=(PdfFile&& pdf) noexcept;
	operator bool() const noexcept;
	int numPages() const noexcept;
	SDL_Surface* renderPage(int pid, double dpi) noexcept;
	PdfFile copyLight() const noexcept;

	static bool canOpen(SDL_RWops* ops) noexcept;	// closes ops if it's not a nullptr

private:
	void freeDoc() noexcept;
};

inline PdfFile::operator bool() const noexcept {
	return mdoc || pdoc;
}

#else
class PdfFile {
public:
	constexpr operator bool() const { return false; }
	constexpr PdfFile copyLight() const { return PdfFile(); }
};
#endif

enum class ResultCode : uint8 {
	ok,
	stop,
	error
};

enum BrowserListOption : uint8 {
	BLO_NONE = 0x0,
	BLO_FILES = 0x1,
	BLO_DIRS = 0x2,
	BLO_HIDDEN = 0x4
};

enum BrowserResultState : uint8 {
	BRS_NONE = 0x0,
	BRS_LOC = 0x1,
	BRS_PDF = 0x2,
	BRS_ARCH = 0x4,
	BRS_FWD = 0x8
};

// files and directories info
struct BrowserResultList {
	vector<Cstring> files, dirs;
	Cstring error;

	BrowserResultList() = default;
	BrowserResultList(vector<Cstring>&& fent, vector<Cstring>&& dent) noexcept;
};

#ifdef WITH_ARCHIVE
// archive load info
struct BrowserResultArchive {
	string rootDir;
	string opath;	// path to the file or directory to open
	string page;	// for PDF only
	ArchiveData arch;
	Cstring error;
	const bool hasRootDir;
	ResultCode rc = ResultCode::ok;

	BrowserResultArchive(optional<string>&& root, ArchiveData&& aroot, string&& fpath = string(), string&& ppage = string()) noexcept;
};
#endif

// picture load info
struct BrowserResultPicture {
	string rootDir;
	string curDir;
	string picname;
	ArchiveData arch;
	PdfFile pdf;
	vector<pair<Cstring, Texture*>> pics;
	std::mutex mpic;
	Cstring error;
	const bool hasRootDir;
	const bool newCurDir;
	const bool newArchive;
	const bool newPdf;
	const bool fwd;

	BrowserResultPicture(BrowserResultState brs, optional<string>&& root, string&& container, string&& pname = string(), ArchiveData&& aroot = ArchiveData(), PdfFile&& pdfFile = PdfFile()) noexcept;
};

// intermediate picture load buffer
struct BrowserPictureProgress {
	BrowserResultPicture* pnt;	// needed to access the texture vector and mutex (mustn't be deleted)
	SDL_Surface* img;
	size_t id;
	Cstring text;

	BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index, Cstring&& msg) noexcept;
};

// list of font families, files and which to select
struct FontListResult {
	vector<Cstring> families;
	uptr<Cstring[]> files;
	size_t select;
	string error;

	FontListResult(vector<Cstring>&& fa, uptr<Cstring[]>&& fl, size_t id, string&& msg) noexcept;
};

// check a stop token every n iterations
class CountedStopReq {
private:
	uint cnt = 0;
	uint lim;

public:
	CountedStopReq(uint steps) : lim(steps) {}

	bool stopReq(std::stop_token stoken) noexcept;
};
