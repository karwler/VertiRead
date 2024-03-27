#pragma once

#ifdef CAN_POPPLER
#include <cairo-pdf.h>
#include <poppler-document.h>
#include <poppler-page.h>

extern decltype(poppler_document_new_from_bytes)* popplerDocumentNewFromBytes;
extern decltype(poppler_document_get_n_pages)* popplerDocumentGetNPages;
extern decltype(poppler_document_get_page)* popplerDocumentGetPage;
extern decltype(poppler_page_get_image_mapping)* popplerPageGetImageMapping;
extern decltype(poppler_page_free_image_mapping)* popplerPageFreeImageMapping;
extern decltype(poppler_page_get_image)* popplerPageGetImage;
extern decltype(poppler_page_get_size)* popplerPageGetSize;
extern decltype(poppler_page_render)* popplerPageRender;

extern decltype(cairo_create)* cairoCreate;
extern decltype(cairo_destroy)* cairoDestroy;
extern decltype(cairo_scale)* cairoScale;
extern decltype(cairo_pdf_surface_create)* cairoPdfSurfaceCreate;
extern decltype(cairo_surface_map_to_image)* cairoSurfaceMapToImage;
extern decltype(cairo_image_surface_get_format)* cairoImageSurfaceGetFormat;
extern decltype(cairo_image_surface_get_data)* cairoImageSurfaceGetData;
extern decltype(cairo_image_surface_get_width)* cairoImageSurfaceGetWidth;
extern decltype(cairo_image_surface_get_height)* cairoImageSurfaceGetHeight;
extern decltype(cairo_image_surface_get_stride)* cairoImageSurfaceGetStride;
extern decltype(cairo_surface_unmap_image)* cairoSurfaceUnmapImage;
extern decltype(cairo_surface_destroy)* cairoSurfaceDestroy;

bool symPoppler() noexcept;	// loads glib because of dependency
void closePoppler() noexcept;
#endif
