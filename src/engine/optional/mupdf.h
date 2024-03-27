#pragma once

#ifdef CAN_MUPDF
#include <mupdf/fitz.h>

#define fzVar(var) fzVarImp(&(var))
#ifdef HAVE_SIGSETJMP
#define fzTry(ctx) if (!sigsetjmp(*fzPushTry(ctx), 0) && fzDoTry(ctx)) do
#else
#define fzTry(ctx) if (!setjmp(*fzPushTry(ctx)) && fzDoTry(ctx)) do
#endif
#define fzAlways(ctx) while (0); if (fzDoAlways(ctx)) do
#define fzCatch(ctx) while (0); if (fzDoCatch(ctx))

extern decltype(fz_new_context_imp)* fzNewContextImp;
extern decltype(fz_drop_context)* fzDropContext;
extern decltype(fz_var_imp)* fzVarImp;
extern decltype(fz_push_try)* fzPushTry;
extern decltype(fz_do_try)* fzDoTry;
extern decltype(fz_do_always)* fzDoAlways;
extern decltype(fz_do_catch)* fzDoCatch;
extern decltype(fz_new_buffer_from_shared_data)* fzNewBufferFromSharedData;
extern decltype(fz_drop_buffer)* fzDropBuffer;
extern decltype(fz_register_document_handlers)* fzRegisterDocumentHandlers;
extern decltype(fz_open_document_with_buffer)* fzOpenDocumentWithBuffer;
extern decltype(fz_drop_document)* fzDropDocument;
extern decltype(fz_count_pages)* fzCountPages;
extern decltype(fz_new_pixmap_from_page_number)* fzNewPixmapFromPageNumber;
extern decltype(fz_drop_pixmap)* fzDropPixmap;
extern decltype(fz_scale)* fzScale;
extern decltype(fz_device_rgb)* fzDeviceRgb;

bool symMupdf() noexcept;
void closeMupdf() noexcept;
#endif
