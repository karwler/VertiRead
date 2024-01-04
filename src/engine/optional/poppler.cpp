#ifdef CAN_PDF
#include "glib.h"
#include "poppler.h"
#include <iostream>
#include <dlfcn.h>

#define PLIB_NAME "poppler-glib"
#define CLIB_NAME "cairo"

namespace LibPoppler {

static void* libp = nullptr;
static void* libc = nullptr;
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
	if (libp || failed)
		return libp;
	if (!LibGlib::symGlib()) {
		failed = true;
		return false;
	}
	try {
		if (libc = dlopen("lib" CLIB_NAME ".so", RTLD_LAZY | RTLD_GLOBAL); !libc)
			throw "Failed to open " CLIB_NAME;
		if (libp = dlopen("lib" PLIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !libp)
			throw "Failed to open " PLIB_NAME;
	} catch (const char* msg) {
		const char* err = dlerror();
		std::cerr << (err ? err : msg) << std::endl;
		if (libp)
			dlclose(libp);
		if (libc)
			dlclose(libc);
		libp = libc = nullptr;
		failed = true;
		return false;
	}

	try {
		if (!((cairoCreate = reinterpret_cast<decltype(cairoCreate)>(dlsym(libc, "cairo_create")))
			&& (cairoDestroy = reinterpret_cast<decltype(cairoDestroy)>(dlsym(libc, "cairo_destroy")))
			&& (cairoPdfSurfaceCreate = reinterpret_cast<decltype(cairoPdfSurfaceCreate)>(dlsym(libc, "cairo_pdf_surface_create")))
			&& (cairoSurfaceMapToImage = reinterpret_cast<decltype(cairoSurfaceMapToImage)>(dlsym(libc, "cairo_surface_map_to_image")))
			&& (cairoImageSurfaceGetFormat = reinterpret_cast<decltype(cairoImageSurfaceGetFormat)>(dlsym(libc, "cairo_image_surface_get_format")))
			&& (cairoImageSurfaceGetData = reinterpret_cast<decltype(cairoImageSurfaceGetData)>(dlsym(libc, "cairo_image_surface_get_data")))
			&& (cairoImageSurfaceGetWidth = reinterpret_cast<decltype(cairoImageSurfaceGetWidth)>(dlsym(libc, "cairo_image_surface_get_width")))
			&& (cairoImageSurfaceGetHeight = reinterpret_cast<decltype(cairoImageSurfaceGetHeight)>(dlsym(libc, "cairo_image_surface_get_height")))
			&& (cairoImageSurfaceGetStride = reinterpret_cast<decltype(cairoImageSurfaceGetStride)>(dlsym(libc, "cairo_image_surface_get_stride")))
			&& (cairoSurfaceUnmapImage = reinterpret_cast<decltype(cairoSurfaceUnmapImage)>(dlsym(libc, "cairo_surface_unmap_image")))
			&& (cairoSurfaceDestroy = reinterpret_cast<decltype(cairoSurfaceDestroy)>(dlsym(libc, "cairo_surface_destroy")))
		))
			throw "Failed to find " CLIB_NAME " functions";

		if (!((popplerDocumentNewFromBytes = reinterpret_cast<decltype(popplerDocumentNewFromBytes)>(dlsym(libp, "poppler_document_new_from_bytes")))
			&& (popplerDocumentGetNPages = reinterpret_cast<decltype(popplerDocumentGetNPages)>(dlsym(libp, "poppler_document_get_n_pages")))
			&& (popplerDocumentGetPage = reinterpret_cast<decltype(popplerDocumentGetPage)>(dlsym(libp, "poppler_document_get_page")))
			&& (popplerPageGetImageMapping = reinterpret_cast<decltype(popplerPageGetImageMapping)>(dlsym(libp, "poppler_page_get_image_mapping")))
			&& (popplerPageFreeImageMapping = reinterpret_cast<decltype(popplerPageFreeImageMapping)>(dlsym(libp, "poppler_page_free_image_mapping")))
			&& (popplerPageGetImage = reinterpret_cast<decltype(popplerPageGetImage)>(dlsym(libp, "poppler_page_get_image")))
			&& (popplerPageGetSize = reinterpret_cast<decltype(popplerPageGetSize)>(dlsym(libp, "poppler_page_get_size")))
			&& (popplerPageRender = reinterpret_cast<decltype(popplerPageRender)>(dlsym(libp, "poppler_page_render")))
		))
			throw "Failed to find " PLIB_NAME " functions";
	} catch (const char* msg) {
		std::cerr << msg << std::endl;
		dlclose(libp);
		dlclose(libc);
		libp = libc = nullptr;
		failed = true;
		return false;
	}
	return true;
}

void closePoppler() {
	if (libp) {
		dlclose(libp);
		dlclose(libc);
		libp = libc = nullptr;
	}
	failed = false;
}

}
#endif
