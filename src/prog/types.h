#pragma once

#include "utils/utils.h"

enum UserEvent : uint32 {
	SDL_USEREVENT_GENERAL = SDL_USEREVENT,
	SDL_USEREVENT_PROG_BOOKS,
	SDL_USEREVENT_PROG_FILE_EXPLORER,
	SDL_USEREVENT_PROG_PAGE_BROWSER,
	SDL_USEREVENT_PROG_READER,
	SDL_USEREVENT_PROG_SETTINGS,
	SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_PROG_MAX = SDL_USEREVENT_PROG_SEARCH_DIR,
	SDL_USEREVENT_THREAD_ARCHIVE,
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
	setPortrait,
	setLandscape,
	setSquare,
	setFill,
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
	string name;
	uintptr_t size;

	ArchiveFile() = default;
	ArchiveFile(const ArchiveFile& af) = default;
	ArchiveFile(ArchiveFile&& af) = default;
	ArchiveFile(string&& filename, uintptr_t mem) : name(std::move(filename)), size(mem) {}

	ArchiveFile& operator=(const ArchiveFile& af) = default;
	ArchiveFile& operator=(ArchiveFile&& af) = default;
};

// archive directory node
struct ArchiveDir {
	ArchiveDir* parent = nullptr;
	string name;
	vector<ArchiveDir> dirs;
	vector<ArchiveFile> files;

	ArchiveDir() = default;
	ArchiveDir(const ArchiveDir& ad) = default;
	ArchiveDir(ArchiveDir&& ad) = default;
	ArchiveDir(ArchiveDir* daddy, string&& dirname) : parent(daddy), name(std::move(dirname)) {}

	ArchiveDir& operator=(const ArchiveDir& ad) = default;
	ArchiveDir& operator=(ArchiveDir&& ad) = default;

	void sort();
	void clear();
	string path() const;
	pair<ArchiveDir*, ArchiveFile*> find(string_view path);
	vector<ArchiveDir>::iterator findDir(string_view dname);
	vector<ArchiveFile>::iterator findFile(string_view fname);
};

// archive/picture info
struct BrowserResultAsync {
	string rootDir;
	string curDir;
	string file;
	ArchiveDir arch;

	BrowserResultAsync(string&& root, string&& directory, string&& pname = string(), ArchiveDir&& aroot = ArchiveDir());
};

// picture load info
struct BrowserResultPicture : public BrowserResultAsync {
	vector<pair<string, Texture*>> pics;
	std::mutex mpic;
	const bool archive;

	BrowserResultPicture(bool inArch, string&& root, string&& directory, string&& pname = string(), ArchiveDir&& aroot = ArchiveDir());
};

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
