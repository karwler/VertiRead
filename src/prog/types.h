#pragma once

#include "utils/utils.h"
#ifdef WITH_SDL3
#include <SDL3/SDL_events.h>
#else
#include <SDL_events.h>
#endif
#include <forward_list>
#include <stop_token>

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
	setLoginPopupProtocol,
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
	openLogin,
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
	setGammaType,
	setGammaStepSl,
	setGammaStepLe,
	setCompression,
	setVsync,
	setMultiFullscreen,
	setPreview,
	setHide,
	setTooltips,
	setTheme,
	setFontCmb,
	setFontLe,
	setMonoFont,
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

	constexpr EventId(UserEvent t, int32 c) noexcept : type(t), code(c) {}
	constexpr EventId(GeneralEvent e) noexcept : type(SDL_USEREVENT_GENERAL), code(eint(e)) {}
	constexpr EventId(ProgBooksEvent e) noexcept : type(SDL_USEREVENT_PROG_BOOKS), code(eint(e)) {}
	constexpr EventId(ProgFileExplorerEvent e) noexcept : type(SDL_USEREVENT_PROG_FILE_EXPLORER), code(eint(e)) {}
	constexpr EventId(ProgPageBrowserEvent e) noexcept : type(SDL_USEREVENT_PROG_PAGE_BROWSER), code(eint(e)) {}
	constexpr EventId(ProgReaderEvent e) noexcept : type(SDL_USEREVENT_PROG_READER), code(eint(e)) {}
	constexpr EventId(ProgSettingsEvent e) noexcept : type(SDL_USEREVENT_PROG_SETTINGS), code(eint(e)) {}
	constexpr EventId(ProgSearchDirEvent e) noexcept : type(SDL_USEREVENT_PROG_SEARCH_DIR), code(eint(e)) {}

	constexpr operator bool() const noexcept { return bool(type); }
};

inline constexpr EventId nullEvent = EventId(UserEvent(0), 0);

bool pushEvent(UserEvent type, int32 code, void* data1 = nullptr, void* data2 = nullptr) noexcept;
void cleanupEvents(UserEvent first, UserEvent last) noexcept;

inline bool pushEvent(EventId id, void* data1 = nullptr, void* data2 = nullptr) noexcept {	// data1 and data2 can't be newly allocated memory if id is invalid
	return !id || pushEvent(id.type, id.code, data1, data2);
}

template <Enumeration T>
bool pushEvent(UserEvent type, T code, void* data1 = nullptr, void* data2 = nullptr) noexcept {
	return pushEvent(type, eint(code), data1, data2);
}

inline void cleanupEvent(UserEvent type) noexcept {
	cleanupEvents(type, type);
}

enum class Protocol : uint8 {
	none,
	ftp,
	sftp,
	smb
};
inline constexpr array protocolNames = {
	"",
	"ftp",
	"sftp",
	"smb"
};
inline constexpr array<uint16, protocolNames.size()> protocolPorts = {
	0,
	21,
	22,
	445
};

// connection information about a network location
struct RemoteLocation {
	enum class Family : uint8 {
		any,
		v4,
		v6
	};
	static constexpr array familyNames = {
		"any",
		"IPv4",
		"IPv6"
	};

	enum class Encrypt : uint8 {
		off,
		on,
		force
	};
	static constexpr array encryptNames = {
		"off",
		"on",
		"force"
	};

	string server;
	string path;
	string user;
	string workgroup;
	string password;
	uint16 port;
	Protocol protocol = Protocol::none;
	Family family = Family::any;
	Encrypt encrypt = Encrypt::on;

	static Protocol getProtocol(string_view str) noexcept;
	static RemoteLocation fromPath(string_view str, Protocol proto);
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

	FileChange(Cstring&& entry, Type change) noexcept : name(std::move(entry)), type(change) {}
};

#ifdef WITH_ARCHIVE
// archive file with image size
struct ArchiveFile {
	Cstring name;
	bool isPic = false;
	bool isPdf = false;

	ArchiveFile() = default;
	ArchiveFile(Cstring&& filename, bool pic, bool pdf) noexcept : name(std::move(filename)), isPic(pic), isPdf(pdf) {}
};

// archive directory node
class ArchiveDir {
public:
	Cstring name;	// if this is a root node then the name is the path to the associated archive file
	std::forward_list<ArchiveDir> dirs;
	std::forward_list<ArchiveFile> files;

	ArchiveDir() = default;
	ArchiveDir(Cstring&& dirname) noexcept : name(std::move(dirname)) {}

	vector<ArchiveDir*> listDirs();
	vector<ArchiveFile*> listFiles();
	void finalize() noexcept;
	pair<ArchiveDir*, ArchiveFile*> find(string_view path) noexcept;
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
	ArchiveData(Cstring&& file, PassCode pass = PassCode::none) noexcept : ArchiveDir(std::move(file)), pc(pass) {}

	operator bool() const noexcept;
	ArchiveData copyLight() const;
};

inline ArchiveData::operator bool() const noexcept {
	return name.filled();
}

#else
class ArchiveData {
public:
	operator bool() const noexcept { return false; }
	ArchiveData copyLight() const noexcept { return ArchiveData(); }
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
	PdfFile(uptr<SDL_RWops>&& ops, bool force);
	~PdfFile() { freeDoc(); }

	PdfFile& operator=(PdfFile&& pdf) noexcept;
	operator bool() const noexcept;
	int numPages() const noexcept;
	SDL_Surface* renderPage(int pid, double scale) noexcept;
	PdfFile copyLight() const noexcept;

	static bool canOpen(uptr<SDL_RWops>&& ops) noexcept;	// closes ops if it's not a nullptr

private:
	void freeDoc() noexcept;
};

inline PdfFile::operator bool() const noexcept {
	return mdoc || pdoc;
}

#else
class PdfFile {
public:
	constexpr operator bool() const noexcept { return false; }
	constexpr PdfFile copyLight() const noexcept { return PdfFile(); }
};
#endif

enum class ResultCode : uint8 {
	ok,
	stop,
	error
};

enum BrowserListOption : uint8 {
	BLO_NONE	= 0x00,
	BLO_FILES	= 0x01,
	BLO_DIRS	= 0x02,
	BLO_HIDDEN	= 0x04
};

enum BrowserResultState : uint8 {
	BRS_NONE	= 0x00,
	BRS_LOC		= 0x01,
	BRS_PDF		= 0x02,
	BRS_ARCH	= 0x04,
	BRS_FWD		= 0x08
};

// files and directories info
struct BrowserResultList {
	vector<Cstring> files, dirs;
};

#ifdef WITH_ARCHIVE
// archive load info
struct BrowserResultArchive {
	string rootDir;
	string opath;	// path to the file or directory to open
	string page;	// for PDF only
	ArchiveData arch;
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
	std::forward_list<pair<Cstring, Texture*>> pics;
	uint cnt = 0;
	const bool hasRootDir;
	const bool newCurDir;
	const bool newArchive;
	const bool newPdf;
	const bool fwd;

	BrowserResultPicture(BrowserResultState brs, optional<string>&& root, string&& container, string&& pname = string(), ArchiveData&& aroot = ArchiveData(), PdfFile&& pdfFile = PdfFile()) noexcept;
};

// intermediate picture load buffer
struct BrowserPictureProgress {
	SDL_Surface* img;
	Texture*& tex;
	Cstring text;

	BrowserPictureProgress(uptr<SDL_Surface>& pic, Texture*& ref, Cstring&& msg) noexcept;
};

// list of font families, files and which to select
struct FontListResult {
	vector<Cstring> families;
	uptr<Cstring[]> files;
	size_t select;

	FontListResult(size_t cnt);
};

// check a stop token every n iterations
class CountedStopReq {
private:
	uint cnt = 0;
	uint lim;

public:
	CountedStopReq(uint steps) noexcept : lim(steps) {}

	bool stopReq(std::stop_token stoken) noexcept;
};

template <class T>
size_t countListElements(const std::forward_list<T>& list) noexcept {
	size_t i = 0;
	for (auto it = list.begin(); it != list.end(); ++it, ++i);
	return i;
}
