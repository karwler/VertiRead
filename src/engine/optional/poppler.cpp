#ifdef CAN_PDF
#include "glib.h"
#include "poppler.h"
#include "internal.h"

static LibType libp = nullptr;
static LibType libc = nullptr;
static bool failed = false;
decltype(poppler_document_new_from_bytes)* popplerDocumentNewFromBytes = nullptr;
decltype(poppler_document_get_n_pages)* popplerDocumentGetNPages = nullptr;
decltype(poppler_document_get_page)* popplerDocumentGetPage = nullptr;
decltype(poppler_page_get_image_mapping)* popplerPageGetImageMapping = nullptr;
decltype(poppler_page_free_image_mapping)* popplerPageFreeImageMapping = nullptr;
decltype(poppler_page_get_image)* popplerPageGetImage = nullptr;
decltype(poppler_page_get_size)* popplerPageGetSize = nullptr;
decltype(poppler_page_render)* popplerPageRender = nullptr;
decltype(cairo_create)* cairoCreate = nullptr;
decltype(cairo_destroy)* cairoDestroy = nullptr;
decltype(cairo_pdf_surface_create)* cairoPdfSurfaceCreate = nullptr;
decltype(cairo_surface_map_to_image)* cairoSurfaceMapToImage = nullptr;
decltype(cairo_image_surface_get_format)* cairoImageSurfaceGetFormat = nullptr;
decltype(cairo_image_surface_get_data)* cairoImageSurfaceGetData = nullptr;
decltype(cairo_image_surface_get_width)* cairoImageSurfaceGetWidth = nullptr;
decltype(cairo_image_surface_get_height)* cairoImageSurfaceGetHeight = nullptr;
decltype(cairo_image_surface_get_stride)* cairoImageSurfaceGetStride = nullptr;
decltype(cairo_surface_unmap_image)* cairoSurfaceUnmapImage = nullptr;
decltype(cairo_surface_destroy)* cairoSurfaceDestroy = nullptr;

bool symPoppler() {
	if (!(libp || failed || (symGlib() && (libc = libOpen("libcairo" LIB_EXT)) && (libp = libOpen("libpoppler-glib" LIB_EXT))
		&& (cairoCreate = libSym<decltype(cairoCreate)>(libc, "cairo_create"))
		&& (cairoDestroy = libSym<decltype(cairoDestroy)>(libc, "cairo_destroy"))
		&& (cairoPdfSurfaceCreate = libSym<decltype(cairoPdfSurfaceCreate)>(libc, "cairo_pdf_surface_create"))
		&& (cairoSurfaceMapToImage = libSym<decltype(cairoSurfaceMapToImage)>(libc, "cairo_surface_map_to_image"))
		&& (cairoImageSurfaceGetFormat = libSym<decltype(cairoImageSurfaceGetFormat)>(libc, "cairo_image_surface_get_format"))
		&& (cairoImageSurfaceGetData = libSym<decltype(cairoImageSurfaceGetData)>(libc, "cairo_image_surface_get_data"))
		&& (cairoImageSurfaceGetWidth = libSym<decltype(cairoImageSurfaceGetWidth)>(libc, "cairo_image_surface_get_width"))
		&& (cairoImageSurfaceGetHeight = libSym<decltype(cairoImageSurfaceGetHeight)>(libc, "cairo_image_surface_get_height"))
		&& (cairoImageSurfaceGetStride = libSym<decltype(cairoImageSurfaceGetStride)>(libc, "cairo_image_surface_get_stride"))
		&& (cairoSurfaceUnmapImage = libSym<decltype(cairoSurfaceUnmapImage)>(libc, "cairo_surface_unmap_image"))
		&& (cairoSurfaceDestroy = libSym<decltype(cairoSurfaceDestroy)>(libc, "cairo_surface_destroy"))
		&& (popplerDocumentNewFromBytes = libSym<decltype(popplerDocumentNewFromBytes)>(libp, "poppler_document_new_from_bytes"))
		&& (popplerDocumentGetNPages = libSym<decltype(popplerDocumentGetNPages)>(libp, "poppler_document_get_n_pages"))
		&& (popplerDocumentGetPage = libSym<decltype(popplerDocumentGetPage)>(libp, "poppler_document_get_page"))
		&& (popplerPageGetImageMapping = libSym<decltype(popplerPageGetImageMapping)>(libp, "poppler_page_get_image_mapping"))
		&& (popplerPageFreeImageMapping = libSym<decltype(popplerPageFreeImageMapping)>(libp, "poppler_page_free_image_mapping"))
		&& (popplerPageGetImage = libSym<decltype(popplerPageGetImage)>(libp, "poppler_page_get_image"))
		&& (popplerPageGetSize = libSym<decltype(popplerPageGetSize)>(libp, "poppler_page_get_size"))
		&& (popplerPageRender = libSym<decltype(popplerPageRender)>(libp, "poppler_page_render"))
	))) {
		libClose(libp);
		libClose(libc);
		failed = true;
	}
	return libp;
}

void closePoppler() {
	libClose(libp);
	libClose(libc);
	failed = false;
}
#endif
