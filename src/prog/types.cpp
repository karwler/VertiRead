#include "types.h"
#include "engine/optional/glib.h"
#include "engine/optional/mupdf.h"
#include "engine/optional/poppler.h"
#include "utils/compare.h"
#include <SDL_timer.h>

void pushEvent(EventId id, void* data1, void* data2) {
	if (id)
		pushEvent(id.type, id.code, data1, data2);
}

void pushEvent(UserEvent type, int32 code, void* data1, void* data2) {
	SDL_Event event = { .user = {
		.type = type,
#if SDL_VERSION_ATLEAST(3, 0, 0)
		.timestamp = SDL_GetTicksNS(),
#else
		.timestamp = SDL_GetTicks(),
#endif
		.code = code,
		.data1 = data1,
		.data2 = data2
	} };
	if (int rc = SDL_PushEvent(&event); rc <= 0)
		throw std::runtime_error(rc ? SDL_GetError() : "Event queue full");
}

// REMOTE LOCATION

Protocol RemoteLocation::getProtocol(string_view str) noexcept {
	if (str.length() < 6)	// at least 3 for the protocol and 3 for ://
		return Protocol::none;
	size_t i;
	for (i = 0; i < str.length() && str[i] != ':' && notDsep(str[i]); ++i);
	if (i < str.length() - 3 && str[i] == ':' && isDsep(str[i + 1]) && isDsep(str[i + 2])) {
#ifdef WITH_FTP
		if (strciequal(string_view(str.data(), i), protocolNames[eint(Protocol::ftp)]))
			return Protocol::ftp;
#endif
#ifdef CAN_SFTP
		if (strciequal(string_view(str.data(), i), protocolNames[eint(Protocol::sftp)]))
			return Protocol::sftp;
#endif
#ifdef CAN_SMB
		if (strciequal(string_view(str.data(), i), protocolNames[eint(Protocol::smb)]))
			return Protocol::smb;
#endif
	}
	return Protocol::none;
}

RemoteLocation RemoteLocation::fromPath(string_view str, Protocol proto) {
	RemoteLocation rl = { .port = protocolPorts[eint(proto)], .protocol = proto };
	str = str.substr(str.find(':') + 3);	// skip ://

	size_t i;
	for (i = 0; i < str.length() && notDsep(str[i]); ++i);
	if (i < str.length()) {
		rl.path = str.substr(i);
		str = str.substr(0, i);
	}

	if (i = str.find('@'); i != string_view::npos) {
		string_view iden = str.substr(0, i);
		size_t si = iden.find(':');
		rl.user = iden.substr(0, si);
		if (si != string_view::npos)
			rl.password = iden.substr(si + 1);
		str = str.substr(i + 1);
	}

	if (!str.empty())
		if (str[0] == '[')
			if (i = str.find(']', 1); i != string_view::npos)
				if (bool ate = i + 1 >= str.length(); ate || str[i + 1] == ':') {
					rl.server = str.substr(1, i - 1);
					if (!ate)
						if (uint16 pnum = toNum<uint16>(str.substr(i + 2)))
							rl.port = pnum;
					return rl;
				}

	i = str.find(':');
	rl.server = str.substr(0, i);
	if (i != string_view::npos)
		if (uint16 pnum = toNum<uint16>(str.substr(i + 1)))
			rl.port = pnum;
	return rl;
}

// ARCHIVE NODES

#ifdef WITH_ARCHIVE
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

void ArchiveDir::finalize() noexcept {
	dirs.sort([](const ArchiveDir& a, const ArchiveDir& b) -> bool { return strcmp(a.name.data(), b.name.data()) < 0; });
	files.sort([](const ArchiveFile& a, const ArchiveFile& b) -> bool { return strcmp(a.name.data(), b.name.data()) < 0; });
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

ArchiveDir* ArchiveDir::findDir(string_view dname) noexcept {
	std::forward_list<ArchiveDir>::iterator dit = std::lower_bound(dirs.begin(), dirs.end(), dname, [](const ArchiveDir& a, string_view b) -> bool { return strncmp(a.name.data(), b.data(), b.length()) < 0; });
	return dit != dirs.end() && dit->name == dname ? std::to_address(dit) : nullptr;
}

ArchiveFile* ArchiveDir::findFile(string_view fname) noexcept {
	std::forward_list<ArchiveFile>::iterator fit = std::lower_bound(files.begin(), files.end(), fname, [](const ArchiveFile& a, string_view b) -> bool { return strncmp(a.name.data(), b.data(), b.length()) < 0; });
	return fit != files.end() && fit->name == fname ? std::to_address(fit) : nullptr;
}

void ArchiveDir::copySlicedDentsFrom(const ArchiveDir& src, bool copyHidden) {
	dirs.clear();
#ifndef _WIN32
	if (!copyHidden) {
		for (const ArchiveDir& it : src.dirs)
			if (it.name[0] != '.')
				dirs.emplace_front(valcp(it.name)).files = it.files;
		for (const ArchiveFile& it : src.files)
			if (it.name[0] != '.')
				files.push_front(it);
		return;
	}
#endif
	for (const ArchiveDir& it : src.dirs)
		dirs.emplace_front(valcp(it.name)).files = it.files;
	files = src.files;
}

vector<Cstring> ArchiveDir::copySortedFiles(bool copyHidden) const {
	return copySortedDents(files, copyHidden);
}

vector<Cstring> ArchiveDir::copySortedDirs(bool copyHidden) const {
	return copySortedDents(dirs, copyHidden);
}

template <Class T>
vector<Cstring> ArchiveDir::copySortedDents(const std::forward_list<T>& dents, bool copyHidden) {
	vector<Cstring> names;
	if (copyHidden) {
		size_t cnt = 0;
		for (typename std::forward_list<T>::const_iterator it = dents.begin(); it != dents.end(); ++it, ++cnt);
		names.resize(cnt);
		rng::transform(dents, names.begin(), [](const T& it) -> Cstring { return it.name; });
	} else
		for (const T& it : dents)
			if (it.name[0] != '.')
				names.push_back(it.name);
	rng::sort(names, [](const Cstring& a, const Cstring& b) -> bool { return Strcomp::less(a.data(), b.data()); });
	return names;
}

ArchiveData ArchiveData::copyLight() const {
	ArchiveData ad;
	ad.name = name;
	ad.dref = &data;
	ad.passphrase = passphrase;
	ad.pc = pc;
	return ad;
}
#endif

// PDF FILE

#if defined(CAN_MUPDF) || defined(CAN_POPPLER)
PdfFile::PdfFile(PdfFile&& pdf) noexcept :
	Data(std::move(pdf.ptr), pdf.len),
	mctx(pdf.mctx),
	mdoc(pdf.mdoc),
	pdoc(pdf.pdoc),
	owner(pdf.owner)
{
	pdf.owner = false;
}

PdfFile::PdfFile(SDL_RWops* ops, Cstring* error) {
	if (ops) {
		if (int64 esiz = SDL_RWsize(ops); esiz > signatureLen) {
			byte_t sig[signatureLen];
			if (size_t blen = SDL_RWread(ops, sig, 1, sizeof(sig)); blen == sizeof(sig) && !memcmp(signature, sig, sizeof(sig))) {
				resize(esiz);
				rng::copy(sig, ptr.get());
				size_t toRead = esiz - signatureLen;
				if (blen = SDL_RWread(ops, ptr.get() + signatureLen, sizeof(byte_t), toRead); blen) {
					if (blen < toRead)
						resize(signatureLen + blen);
#ifdef CAN_MUPDF
					if (symMupdf() && (mctx = fzNewContextImp(nullptr, nullptr, FZ_STORE_UNLIMITED, FZ_VERSION))) {
						fz_buffer* bytes = nullptr;
						fzVar(bytes);
						fzTry(mctx) {
							fzRegisterDocumentHandlers(mctx);
							bytes = fzNewBufferFromSharedData(mctx, reinterpret_cast<uchar*>(ptr.get()), len);
							mdoc = fzOpenDocumentWithBuffer(mctx, signature, bytes);
						} fzAlways(mctx) {
							fzDropBuffer(mctx, bytes);
						} fzCatch(mctx) {
							fzDropContext(mctx);
						}
					}
#endif
#ifdef CAN_POPPLER
					if (!mdoc && symPoppler()) {
						GError* gerr = nullptr;
						GBytes* bytes = gBytesNewStatic(ptr.get(), len);
						if (pdoc = popplerDocumentNewFromBytes(bytes, nullptr, &gerr); !pdoc) {
							if (error)
								*error = gerr->message;
							gErrorFree(gerr);
						}
						gBytesUnref(bytes);
					}
#endif
				}
			} else if (error)
				*error = "Missing PDF signature";
		}
		SDL_RWclose(ops);
	}
	if (owner = mdoc || pdoc; !owner) {
		clear();
		if (error && error->empty())
			*error = "Failed to read PDF data";
	}
}

void PdfFile::freeDoc() noexcept {
	if (owner) {
#ifdef CAN_MUPDF
		if (mdoc) {
			fzDropDocument(mctx, mdoc);
			fzDropContext(mctx);
		}
#endif
#ifdef CAN_POPPLER
		if (pdoc)
			gObjectUnref(pdoc);
#endif
	}
}

PdfFile& PdfFile::operator=(PdfFile&& pdf) noexcept {
	freeDoc();
	ptr = std::move(pdf.ptr);
	len = pdf.len;
	mctx = pdf.mctx;
	mdoc = pdf.mdoc;
	pdoc = pdf.pdoc;
	owner = pdf.owner;
	pdf.owner = false;
	return *this;
}

int PdfFile::numPages() const noexcept {
	int pcnt = 0;
#ifdef CAN_MUPDF
	if (mdoc) {
		fzTry(mctx) {
			pcnt = fzCountPages(mctx, mdoc);
		} fzCatch(mctx) {}
	}
#endif
#ifdef CAN_POPPLER
	if (pdoc)
		pcnt = popplerDocumentGetNPages(pdoc);
#endif
	return pcnt;
}

SDL_Surface* PdfFile::renderPage(int pid, double dpi) noexcept {
	SDL_Surface* pic = nullptr;
#ifdef CAN_MUPDF
	if (mdoc) {
		float scale = dpi / defaultDpi;
		fz_matrix ctm = fzScale(scale, scale);
		fz_pixmap* pm;
		fzTry(mctx) {
			pm = fzNewPixmapFromPageNumber(mctx, mdoc, pid, ctm, fzDeviceRgb(mctx), 0);
			if (pm->n == 3 + pm->alpha)
				if (pic = SDL_CreateSurface(pm->w, pm->h, pm->alpha ? SDL_PIXELFORMAT_ABGR8888 : SDL_PIXELFORMAT_RGB24); pic)
					copyPixels(pic->pixels, pm->samples, pic->pitch, pm->stride, pic->w * surfaceBytesPpx(pic), pic->h);
			fzDropPixmap(mctx, pm);
		} fzCatch(mctx) {}
	}
#endif
#ifdef CAN_POPPLER
	if (pdoc)
		if (PopplerPage* page = popplerDocumentGetPage(pdoc, pid)) {
			double scale = dpi / defaultDpi;
			dvec2 size;
			popplerPageGetSize(page, &size.x, &size.y);
			size = glm::floor(size * scale);
			cairo_surface_t* tgt = cairoPdfSurfaceCreate(nullptr, size.x, size.y);
			cairo_t* ctx = cairoCreate(tgt);
			cairoScale(ctx, scale, scale);
			popplerPageRender(page, ctx);
			cairo_surface_t* img = cairoSurfaceMapToImage(tgt, nullptr);
			switch (cairoImageSurfaceGetFormat(img)) {
			case CAIRO_FORMAT_ARGB32:
				pic = SDL_CreateSurface(cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), SDL_PIXELFORMAT_ARGB8888);
				break;
			case CAIRO_FORMAT_RGB24:
				pic = SDL_CreateSurface(cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), SDL_PIXELFORMAT_RGB24);
				break;
			case CAIRO_FORMAT_RGB16_565:
				pic = SDL_CreateSurface(cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), SDL_PIXELFORMAT_RGB565);
			}
			if (pic)
				copyPixels(pic->pixels, cairoImageSurfaceGetData(img), pic->pitch, cairoImageSurfaceGetStride(img), pic->w * surfaceBytesPpx(pic), pic->h);
			cairoSurfaceUnmapImage(tgt, img);
			cairoDestroy(ctx);
			cairoSurfaceDestroy(tgt);
			gObjectUnref(page);
		}
#endif
	return pic;
}

bool PdfFile::canOpen(SDL_RWops* ops) noexcept {
	if (ops) {
		byte_t sig[signatureLen];
		bool ok = SDL_RWread(ops, sig, 1, sizeof(sig)) == sizeof(sig) && !memcmp(signature, sig, sizeof(sig));
		SDL_RWclose(ops);
#if defined(CAN_MUPDF) && defined(CAN_POPPLER)
		return ok && (symMupdf() || symPoppler());
#elif defined(CAN_MUPDF)
		return ok && symMupdf();
#else
		return ok && symPoppler();
#endif
	}
	return false;
}

PdfFile PdfFile::copyLight() const noexcept {
	PdfFile pf;
	pf.mctx = mctx;
	pf.mdoc = mdoc;
	pf.pdoc = pdoc;
	return pf;
}
#endif

// RESULT ASYNC

BrowserResultList::BrowserResultList(vector<Cstring>&& fent, vector<Cstring>&& dent) noexcept :
	files(std::move(fent)),
	dirs(std::move(dent))
{}

#ifdef WITH_ARCHIVE
BrowserResultArchive::BrowserResultArchive(optional<string>&& root, ArchiveData&& aroot, string&& fpath, string&& ppage) noexcept :
	rootDir(root ? std::move(*root) : string()),
	opath(std::move(fpath)),
	page(std::move(ppage)),
	arch(std::move(aroot)),
	hasRootDir(root)
{}
#endif

BrowserResultPicture::BrowserResultPicture(BrowserResultState brs, optional<string>&& root, string&& container, string&& pname, ArchiveData&& aroot, PdfFile&& pdfFile) noexcept :
	rootDir(root ? std::move(*root) : string()),
	curDir(std::move(container)),
	picname(std::move(pname)),
	arch(std::move(aroot)),
	pdf(std::move(pdfFile)),
	hasRootDir(root),
	newCurDir(brs & BRS_LOC),
	newArchive(brs & BRS_ARCH),
	newPdf(brs & BRS_PDF),
	fwd(brs & BRS_FWD)
{}

BrowserPictureProgress::BrowserPictureProgress(SDL_Surface* pic, Texture*& ref, Cstring&& msg) noexcept :
	img(pic),
	tex(ref),
	text(std::move(msg))
{}

FontListResult::FontListResult(vector<Cstring>&& fa, uptr<Cstring[]>&& fl, size_t id, string&& msg) noexcept :
	families(std::move(fa)),
	files(std::move(fl)),
	select(id),
	error(std::move(msg))
{}

// COUNTED STOP REQ

bool CountedStopReq::stopReq(std::stop_token stoken) noexcept {
	if (++cnt >= lim) {
		cnt = 0;
		return stoken.stop_requested();
	}
	return false;
}
