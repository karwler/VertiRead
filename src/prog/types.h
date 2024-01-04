#pragma once

#include "utils/utils.h"
#include <forward_list>
#include <mutex>
#include <stop_token>
#include <SDL2/SDL_events.h>

enum UserEvent : uint32 {
	SDL_USEREVENT_GENERAL = SDL_USEREVENT,
	SDL_USEREVENT_PROG_BOOKS,
	SDL_USEREVENT_PROG_FILE_EXPLORER,
	SDL_USEREVENT_PROG_PAGE_BROWSER,
	SDL_USEREVENT_PROG_READER,
	SDL_USEREVENT_PROG_SETTINGS,
	SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_PROG_MAX = SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_THREAD_ARCHIVE_FINISHED,
	SDL_USEREVENT_THREAD_PREVIEW,
	SDL_USEREVENT_THREAD_READER,
	SDL_USEREVENT_THREAD_MOVE,
	SDL_USEREVENT_THREAD_FONTS_FINISHED,
	SDL_USEREVENT_MAX
};

enum class GeneralEvent : int32 {
	closePopup,
	closeContext,
	confirmComboBox,
	resizeComboContext,
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
	setPdfImages,
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

#define constructorEventId(etype, clazz) constexpr EventId(clazz e) : type(etype), code(int32(e)) {}

struct EventId {
	UserEvent type;
	int32 code;	// should not exceed 16 bits for widget events because of the packing in Button

	constexpr EventId(UserEvent t, int32 c) : type(t), code(c) {}
	constructorEventId(SDL_USEREVENT_GENERAL, GeneralEvent)
	constructorEventId(SDL_USEREVENT_PROG_BOOKS, ProgBooksEvent)
	constructorEventId(SDL_USEREVENT_PROG_FILE_EXPLORER, ProgFileExplorerEvent)
	constructorEventId(SDL_USEREVENT_PROG_PAGE_BROWSER, ProgPageBrowserEvent)
	constructorEventId(SDL_USEREVENT_PROG_READER, ProgReaderEvent)
	constructorEventId(SDL_USEREVENT_PROG_SETTINGS, ProgSettingsEvent)
	constructorEventId(SDL_USEREVENT_PROG_SEARCH_DIR, ProgSearchDirEvent)

	constexpr operator bool() const { return bool(type); }
};

constexpr EventId nullEvent = EventId(UserEvent(0), 0);

void pushEvent(UserEvent type, int32 code, void* data1 = nullptr, void* data2 = nullptr);

template <Enumeration T>
void pushEvent(UserEvent type, T code, void* data1 = nullptr, void* data2 = nullptr) {
	pushEvent(type, int32(code), data1, data2);
}

inline void pushEvent(EventId id, void* data1 = nullptr, void* data2 = nullptr) {
	if (id)
		pushEvent(id.type, id.code, data1, data2);
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

enum class FileOpCapabilities : uint8 {
	none = 0x0,
	remove = 0x1,
	rename = 0x2,
	watch = 0x4
};

enum class Protocol : uint8 {
	none,
	smb,
	sftp
};
constexpr array protocolNames = {
	"",
	"smb",
	"sftp"
};
constexpr array<uint16, protocolNames.size()> protocolPorts = {
	0,
	445,
	22
};

enum class Family : uint8 {
	any,
	v4,
	v6
};
constexpr array familyNames = {
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

	static Protocol getProtocol(string_view str);
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

	string name;
	Type type;

	FileChange(string&& entry, Type change) : name(std::move(entry)), type(change) {}
};

// archive file with image size
struct ArchiveFile {
	Cstring name;
	bool isPdf : 1 = false;
	uint64 size : 63 = 0;

	ArchiveFile() = default;
	ArchiveFile(const ArchiveFile& af) = default;
	ArchiveFile(ArchiveFile&& af) = default;
	ArchiveFile(Cstring&& filename, uint64 mem) : name(std::move(filename)), size(mem) {}
	ArchiveFile(Cstring&& pdfName) : name(std::move(pdfName)), isPdf(true) {}

	ArchiveFile& operator=(const ArchiveFile& af) = default;
	ArchiveFile& operator=(ArchiveFile&& af) = default;
};

// archive directory node
struct ArchiveDir {
	Cstring name;	// if this is a root node then the name is the path to the associated archive file
	std::forward_list<ArchiveDir> dirs;
	std::forward_list<ArchiveFile> files;

	ArchiveDir() = default;
	ArchiveDir(Cstring&& dirname) : name(std::move(dirname)) {}

	vector<ArchiveDir*> listDirs();
	vector<ArchiveFile*> listFiles();
	void finalize();
	void clear();
	pair<ArchiveDir*, ArchiveFile*> find(string_view path);
	ArchiveDir* findDir(string_view dname);
	ArchiveFile* findFile(string_view fname);
	void copySlicedDentsFrom(const ArchiveDir& src);
};

enum BrowserResultState : uint8 {
	BRS_NONE = 0x0,
	BRS_LOC = 0x1,
	BRS_PDF = 0x2,
	BRS_ARCH = 0x4
};

// archive load info
struct BrowserResultArchive {
	string rootDir;
	string opath;	// path to the file or directory to open
	string page;	// for PDF only
	ArchiveDir arch;
	string error;
	const bool hasRootDir;

	BrowserResultArchive(optional<string>&& root, ArchiveDir&& aroot, string&& fpath = string(), string&& ppage = string());
};

// picture load info
struct BrowserResultPicture {
	string rootDir;
	string curDir;
	string picname;
	ArchiveDir arch;
	vector<pair<Cstring, Texture*>> pics;
	std::mutex mpic;
	string error;
	const bool hasRootDir;
	const bool newCurDir;
	const bool newArchive;
	const bool pdf;

	BrowserResultPicture(BrowserResultState brs, optional<string>&& root, string&& container, string&& pname = string(), ArchiveDir&& aroot = ArchiveDir());

	bool hasArchive() const;
};

inline bool BrowserResultPicture::hasArchive() const {
	return !arch.name.empty();
}

// intermediate picture load buffer
struct BrowserPictureProgress {
	BrowserResultPicture* pnt;
	SDL_Surface* img;
	size_t id;

	BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index);
};

// list of font families, files and which to select
struct FontListResult {
	vector<Cstring> families;
	uptr<Cstring[]> files;
	size_t select;
	string error;

	FontListResult(vector<Cstring>&& fa, uptr<Cstring[]>&& fl, size_t id, string&& msg);
};

// check a stop token every n iterations
class CountedStopReq {
private:
	uint cnt = 0;
	uint lim;

public:
	CountedStopReq(uint steps) : lim(steps) {}

	bool stopReq(std::stop_token stoken);
};
