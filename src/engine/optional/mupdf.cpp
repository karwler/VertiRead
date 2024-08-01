#ifdef CAN_MUPDF
#include "mupdf.h"
#include "internal.h"

static void* lib = nullptr;
static bool failed = false;
decltype(fz_new_context_imp)* fzNewContextImp = nullptr;
decltype(fz_drop_context)* fzDropContext = nullptr;
decltype(fz_var_imp)* fzVarImp = nullptr;
decltype(fz_push_try)* fzPushTry = nullptr;
decltype(fz_do_try)* fzDoTry = nullptr;
decltype(fz_do_always)* fzDoAlways = nullptr;
decltype(fz_do_catch)* fzDoCatch = nullptr;
decltype(fz_new_buffer_from_shared_data)* fzNewBufferFromSharedData = nullptr;
decltype(fz_drop_buffer)* fzDropBuffer = nullptr;
decltype(fz_register_document_handlers)* fzRegisterDocumentHandlers = nullptr;
decltype(fz_open_document_with_buffer)* fzOpenDocumentWithBuffer = nullptr;
decltype(fz_drop_document)* fzDropDocument = nullptr;
decltype(fz_count_pages)* fzCountPages = nullptr;
decltype(fz_new_pixmap_from_page_number)* fzNewPixmapFromPageNumber = nullptr;
decltype(fz_drop_pixmap)* fzDropPixmap = nullptr;
decltype(fz_scale)* fzScale = nullptr;
decltype(fz_device_rgb)* fzDeviceRgb = nullptr;

bool symMupdf() noexcept {
	if (!(lib || failed || ((lib = libOpen("libmupdf" LIB_EXT))
		&& (fzNewContextImp = reinterpret_cast<decltype(fzNewContextImp)>(dlsym(lib, "fz_new_context_imp")))
		&& (fzDropContext = reinterpret_cast<decltype(fzDropContext)>(dlsym(lib, "fz_drop_context")))
		&& (fzVarImp = reinterpret_cast<decltype(fzVarImp)>(dlsym(lib, "fz_var_imp")))
		&& (fzPushTry = reinterpret_cast<decltype(fzPushTry)>(dlsym(lib, "fz_push_try")))
		&& (fzDoTry = reinterpret_cast<decltype(fzDoTry)>(dlsym(lib, "fz_do_try")))
		&& (fzDoAlways = reinterpret_cast<decltype(fzDoAlways)>(dlsym(lib, "fz_do_always")))
		&& (fzDoCatch = reinterpret_cast<decltype(fzDoCatch)>(dlsym(lib, "fz_do_catch")))
		&& (fzNewBufferFromSharedData = reinterpret_cast<decltype(fzNewBufferFromSharedData)>(dlsym(lib, "fz_new_buffer_from_shared_data")))
		&& (fzDropBuffer = reinterpret_cast<decltype(fzDropBuffer)>(dlsym(lib, "fz_drop_buffer")))
		&& (fzRegisterDocumentHandlers = reinterpret_cast<decltype(fzRegisterDocumentHandlers)>(dlsym(lib, "fz_register_document_handlers")))
		&& (fzOpenDocumentWithBuffer = reinterpret_cast<decltype(fzOpenDocumentWithBuffer)>(dlsym(lib, "fz_open_document_with_buffer")))
		&& (fzDropDocument = reinterpret_cast<decltype(fzDropDocument)>(dlsym(lib, "fz_drop_document")))
		&& (fzCountPages = reinterpret_cast<decltype(fzCountPages)>(dlsym(lib, "fz_count_pages")))
		&& (fzNewPixmapFromPageNumber = reinterpret_cast<decltype(fzNewPixmapFromPageNumber)>(dlsym(lib, "fz_new_pixmap_from_page_number")))
		&& (fzDropPixmap = reinterpret_cast<decltype(fzDropPixmap)>(dlsym(lib, "fz_drop_pixmap")))
		&& (fzScale = reinterpret_cast<decltype(fzScale)>(dlsym(lib, "fz_scale")))
		&& (fzDeviceRgb = reinterpret_cast<decltype(fzDeviceRgb)>(dlsym(lib, "fz_device_rgb")))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeMupdf() noexcept {
	libClose(lib);
	failed = false;
}
#endif
