#include "types.h"
#include "utils/compare.h"

void pushEvent(UserEvent type, int32 code, void* data1, void* data2) {
	SDL_Event event = { .user = {
		.type = type,
		.timestamp = SDL_GetTicks(),
		.code = code,
		.data1 = data1,
		.data2 = data2
	} };
	if (int rc = SDL_PushEvent(&event); rc <= 0)
		throw std::runtime_error(rc ? SDL_GetError() : "Event queue full");
}

// REMOTE LOCATION

Protocol RemoteLocation::getProtocol(string_view str) {
	if (str.length() < 6)	// at least 3 for the protocol and 3 for ://
		return Protocol::none;
	size_t i;
	for (i = 0; i < str.length() && str[i] != ':' && notDsep(str[i]); ++i);
	if (i < str.length() - 3 && str[i] == ':' && isDsep(str[i + 1]) && isDsep(str[i + 2])) {
#ifdef CAN_SMB
		if (strciequal(string_view(str.data(), i), protocolNames[eint(Protocol::smb)]))
			return Protocol::smb;
#endif
#ifdef CAN_SFTP
		if (strciequal(string_view(str.data(), i), protocolNames[eint(Protocol::sftp)]))
			return Protocol::sftp;
#endif
	}
	return Protocol::none;
}

RemoteLocation RemoteLocation::fromPath(string_view str, Protocol proto) {
	size_t i = str.find(':') + 3;	// skip ://
	size_t beg = i;
	for (; i < str.length() && str[i] != '@' && str[i] != ':' && notDsep(str[i]); ++i);
	string_view pl(str.begin() + beg, str.begin() + i);
	if (i >= str.length() || isDsep(str[i]))
		return {
			.server = string(pl),
			.path = string(str.begin() + i, str.end()),
			.port = protocolPorts[eint(proto)],
			.protocol = proto
		};

	string_view pr;
	if (str[i] == ':') {
		size_t cl = ++i;
		for (; i < str.length() && str[i] != '@' && notDsep(str[i]); ++i);
		pr = string_view(str.begin() + cl, str.begin() + i);
		if (i >= str.length() || isDsep(str[i]))
			return {
				.server = string(pl),
				.path = string(str.begin() + i, str.end()),
				.port = sanitizePort(pr, proto),
				.protocol = proto
			};
	}

	size_t at = ++i;
	for (; i < str.length() && str[i] != ':' && notDsep(str[i]); ++i);
	string_view sl(str.begin() + at, str.begin() + i);
	if (i >= str.length() || isDsep(str[i]))
		return {
			.server = string(sl),
			.path = string(str.begin() + i, str.end()),
			.user = string(pl),
			.password = string(pr),
			.port = protocolPorts[eint(proto)],
			.protocol = proto
		};

	size_t cl = ++i;
	for (; i < str.length() && notDsep(str[i]); ++i);
	return {
		.server = string(sl),
		.path = string(str.begin() + i, str.end()),
		.user = string(pl),
		.password = string(pr),
		.port = sanitizePort(string_view(str.begin() + cl, str.begin() + i), proto),
		.protocol = proto
	};
}

uint16 RemoteLocation::sanitizePort(string_view port, Protocol protocol) {
	uint16 pnum = toNum<uint16>(port);
	return pnum ? pnum : protocolPorts[eint(protocol)];
}

// ARCHIVE NODES

vector<ArchiveDir*> ArchiveDir::listDirs() {
	vector<ArchiveDir*> entries;
	rng::transform(dirs, std::back_inserter(entries), [](ArchiveDir& it) -> ArchiveDir* { return &it; });
	rng::sort(entries, [](const ArchiveDir* a, const ArchiveDir* b) -> bool { return Strcomp::less(a->name.data(), b->name.data()); });
	return entries;
}

vector<ArchiveFile*> ArchiveDir::listFiles() {
	vector<ArchiveFile*> entries;
	rng::transform(files, std::back_inserter(entries), [](ArchiveFile& it) -> ArchiveFile* { return &it; });
	rng::sort(entries, [](const ArchiveFile* a, const ArchiveFile* b) -> bool { return Strcomp::less(a->name.data(), b->name.data()); });
	return entries;
}

void ArchiveDir::finalize() {
	dirs.sort([](const ArchiveDir& a, const ArchiveDir& b) -> bool { return strcmp(a.name.data(), b.name.data()) < 0; });
	files.sort([](const ArchiveFile& a, const ArchiveFile& b) -> bool { return strcmp(a.name.data(), b.name.data()) < 0; });
}

void ArchiveDir::clear() {
	name.clear();
	dirs.clear();
	files.clear();
}

pair<ArchiveDir*, ArchiveFile*> ArchiveDir::find(string_view path) {
	ArchiveDir* node = this;
	size_t p = path.find_first_not_of('/');
	for (size_t e; (e = path.find('/', p)) != string::npos; p = path.find_first_not_of('/', e))
		if (node = node->findDir(path.substr(p, e - p)); !node)
			return pair(nullptr, nullptr);
	if (p == string::npos)
		return pair(node, nullptr);

	string_view sname = path.substr(p);
	if (ArchiveDir* dit = node->findDir(sname); dit)
		return pair(dit, nullptr);
	ArchiveFile* fit = node->findFile(sname);
	return fit ? pair(node, fit) : pair(nullptr, nullptr);
}

ArchiveDir* ArchiveDir::findDir(string_view dname) {
	std::forward_list<ArchiveDir>::iterator dit = std::lower_bound(dirs.begin(), dirs.end(), dname, [](const ArchiveDir& a, string_view b) -> bool { return strncmp(a.name.data(), b.data(), b.length()) < 0; });
	return dit != dirs.end() && dit->name == dname ? std::to_address(dit) : nullptr;
}

ArchiveFile* ArchiveDir::findFile(string_view fname) {
	std::forward_list<ArchiveFile>::iterator fit = std::lower_bound(files.begin(), files.end(), fname, [](const ArchiveFile& a, string_view b) -> bool { return strncmp(a.name.data(), b.data(), b.length()) < 0; });
	return fit != files.end() && fit->name == fname ? std::to_address(fit) : nullptr;
}

void ArchiveDir::copySlicedDentsFrom(const ArchiveDir& src) {
	dirs.clear();
	for (const ArchiveDir& it : src.dirs)
		dirs.emplace_front(valcp(it.name)).files = it.files;
	files = src.files;
}

// RESULT ASYNC

BrowserResultArchive::BrowserResultArchive(optional<string>&& root, ArchiveDir&& aroot, string&& fpath, string&& ppage) :
	rootDir(root ? std::move(*root) : string()),
	opath(std::move(fpath)),
	page(std::move(ppage)),
	arch(std::move(aroot)),
	hasRootDir(root)
{}

BrowserResultPicture::BrowserResultPicture(BrowserResultState brs, optional<string>&& root, string&& container, string&& pname, ArchiveDir&& aroot) :
	rootDir(root ? std::move(*root) : string()),
	curDir(std::move(container)),
	picname(std::move(pname)),
	arch(std::move(aroot)),
	hasRootDir(root),
	newCurDir(brs & BRS_LOC),
	newArchive(brs & BRS_ARCH),
	pdf(brs & BRS_PDF)
{}

BrowserPictureProgress::BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index) :
	pnt(rp),
	img(pic),
	id(index)
{}

FontListResult::FontListResult(vector<Cstring>&& fa, uptr<Cstring[]>&& fl, size_t id, string&& msg) :
	families(std::move(fa)),
	files(std::move(fl)),
	select(id),
	error(std::move(msg))
{}

// COUNTED STOP REQ

bool CountedStopReq::stopReq(std::stop_token stoken) {
	if (++cnt >= lim) {
		cnt = 0;
		return stoken.stop_requested();
	}
	return false;
}
