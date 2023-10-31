#include "types.h"
#include "utils/compare.h"

void pushEvent(UserEvent code, void* data1, void* data2) {
	SDL_Event event = { .user = {
		.type = code,
		.timestamp = SDL_GetTicks(),
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

// FILE CHANGE

FileChange::FileChange(string&& entry, Type change) :
	name(std::move(entry)),
	type(change)
{}

// ARCHIVE NODES

ArchiveFile::ArchiveFile(string&& filename, uintptr_t mem) :
	name(std::move(filename)),
	size(mem)
{}

ArchiveDir::ArchiveDir(ArchiveDir* daddy, string&& dirname) :
	parent(daddy),
	name(std::move(dirname))
{}

void ArchiveDir::sort() {
	rng::sort(dirs, [](const ArchiveDir& a, const ArchiveDir& b) -> bool { return StrNatCmp::less(a.name, b.name); });
	rng::sort(files, [](const ArchiveFile& a, const ArchiveFile& b) -> bool { return StrNatCmp::less(a.name, b.name); });
	for (ArchiveDir& it : dirs)
		it.sort();
}

void ArchiveDir::clear() {
	name.clear();
	dirs.clear();
	files.clear();
}

string ArchiveDir::path() const {
	size_t len = 0;
	for (const ArchiveDir* it = this; it->parent; it = it->parent)
		len += it->name.length() + 1;
	string str;
	str.resize(len);

	for (const ArchiveDir* it = this; it->parent; it = it->parent) {
		str[--len] = '/';
		len -= it->name.length();
		rng::copy(it->name, str.begin() + len);
	}
	return str;
}

pair<ArchiveDir*, ArchiveFile*> ArchiveDir::find(string_view path) {
	if (path.empty())
		return pair(nullptr, nullptr);

	ArchiveDir* node = this;
	size_t p = path.find_first_not_of('/');
	for (size_t e; (e = path.find('/', p)) != string::npos; p = path.find_first_not_of('/', p)) {
		vector<ArchiveDir>::iterator dit = node->findDir(path.substr(p, e - p));
		if (dit == dirs.end())
			return pair(nullptr, nullptr);
		node = std::to_address(dit);
	}
	if (p < path.length()) {
		string_view sname = path.substr(p);
		if (vector<ArchiveDir>::iterator dit = node->findDir(sname); dit != dirs.end())
			node = std::to_address(dit);
		else if (vector<ArchiveFile>::iterator fit = node->findFile(sname); fit != files.end())
			return pair(node, std::to_address(fit));
		else
			return pair(nullptr, nullptr);
	}
	return pair(node, nullptr);
}

vector<ArchiveDir>::iterator ArchiveDir::findDir(string_view dname) {
	vector<ArchiveDir>::iterator dit = std::lower_bound(dirs.begin(), dirs.end(), dname, [](const ArchiveDir& a, string_view b) -> bool { return StrNatCmp::less(a.name, b); });
	return dit != dirs.end() && dit->name == dname ? dit : dirs.end();
}

vector<ArchiveFile>::iterator ArchiveDir::findFile(string_view fname) {
	vector<ArchiveFile>::iterator fit = std::lower_bound(files.begin(), files.end(), fname, [](const ArchiveFile& a, string_view b) -> bool { return StrNatCmp::less(a.name, b); });
	return fit != files.end() && fit->name == fname ? fit : files.end();
}

// RESULT ASYNC

BrowserResultAsync::BrowserResultAsync(string&& root, string&& directory, string&& pname, ArchiveDir&& aroot) :
	rootDir(std::move(root)),
	curDir(std::move(directory)),
	file(std::move(pname)),
	arch(std::move(aroot))
{}

BrowserResultPicture::BrowserResultPicture(bool inArch, string&& root, string&& directory, string&& pname, ArchiveDir&& aroot) :
	BrowserResultAsync(std::move(root), std::move(directory), std::move(pname), std::move(aroot)),
	archive(inArch)
{}

BrowserPictureProgress::BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index) :
	pnt(rp),
	img(pic),
	id(index)
{}

FontListResult::FontListResult(vector<string>&& fa, uptr<string[]>&& fl, size_t id, string&& msg) :
	families(std::move(fa)),
	files(std::move(fl)),
	select(id),
	error(std::move(msg))
{}

