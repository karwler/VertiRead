#include "types.h"
#ifdef CAN_MUPDF
#include "engine/optional/mupdf.h"
#endif
#ifdef CAN_POPPLER
#include "engine/optional/glib.h"
#include "engine/optional/poppler.h"
#endif
#include "utils/compare.h"
#include <SDL_timer.h>

void pushEvent(EventId id, void* data1, void* data2) {
	if (id)
		pushEvent(id.type, id.code, data1, data2);
}

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

Protocol RemoteLocation::getProtocol(string_view str) noexcept {
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

void ArchiveDir::copySlicedDentsFrom(const ArchiveDir& src) {
	dirs.clear();
	for (const ArchiveDir& it : src.dirs)
		dirs.emplace_front(valcp(it.name)).files = it.files;
	files = src.files;
}

ArchiveData ArchiveData::copyLight() const {
	ArchiveData ad;
	ad.name = name;
	ad.dref = &data;
	ad.passphrase = passphrase;
	ad.pc = pc;
	return ad;
}

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

PdfFile::PdfFile(SDL_RWops* ops, string* error) {
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
				if (pic = SDL_CreateRGBSurfaceWithFormat(0, pm->w, pm->h, pm->n * 8, pm->alpha ? SDL_PIXELFORMAT_ABGR8888 : SDL_PIXELFORMAT_RGB24); pic)
					copyPixels(pic->pixels, pm->samples, pic->pitch, pm->stride, pic->w * pic->format->BytesPerPixel, pic->h);
			fzDropPixmap(mctx, pm);
		} fzCatch(mctx) {}
	}
#endif
#ifdef CAN_POPPLER
	if (pdoc && !pic)
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
				pic = SDL_CreateRGBSurfaceWithFormat(0, cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), 32, SDL_PIXELFORMAT_ARGB8888);
				break;
			case CAIRO_FORMAT_RGB24:
				pic = SDL_CreateRGBSurfaceWithFormat(0, cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), 24, SDL_PIXELFORMAT_RGB24);
				break;
			case CAIRO_FORMAT_RGB16_565:
				pic = SDL_CreateRGBSurfaceWithFormat(0, cairoImageSurfaceGetWidth(img), cairoImageSurfaceGetHeight(img), 16, SDL_PIXELFORMAT_RGB565);
			}
			if (pic)
				copyPixels(pic->pixels, cairoImageSurfaceGetData(img), pic->pitch, cairoImageSurfaceGetStride(img), pic->w * pic->format->BytesPerPixel, pic->h);
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

BrowserResultArchive::BrowserResultArchive(optional<string>&& root, ArchiveData&& aroot, string&& fpath, string&& ppage) noexcept :
	rootDir(root ? std::move(*root) : string()),
	opath(std::move(fpath)),
	page(std::move(ppage)),
	arch(std::move(aroot)),
	hasRootDir(root)
{}

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

BrowserPictureProgress::BrowserPictureProgress(BrowserResultPicture* rp, SDL_Surface* pic, size_t index) noexcept :
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

bool CountedStopReq::stopReq(std::stop_token stoken) noexcept {
	if (++cnt >= lim) {
		cnt = 0;
		return stoken.stop_requested();
	}
	return false;
}
