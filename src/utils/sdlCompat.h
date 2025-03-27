#pragma once

#include "sthandle.h"
#ifdef WITH_SDL3
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_surface.h>
#else
#define SDL_MAIN_HANDLED
#include <SDL_surface.h>
#endif
#include <memory>

#ifdef WITH_SDL3

#undef SDL_ConvertSurfaceFormat
#undef SDL_RWread
#undef SDL_RWwrite

#define SDL_ConvertSurfaceFormat(s, t, f) SDL_ConvertSurface(s, t)
#define SDL_GL_GetDrawableSize SDL_GetWindowSizeInPixels
#define SDL_PixelFormatEnum SDL_PixelFormat
#define SDL_RWread(c, p, s, n) SDL_ReadIO(c, p, (s) * (n))
#define SDL_RWwrite(c, p, s, n) SDL_WriteIO(c, p, (s) * (n))
#define SDL_TICKS_PASSED(a, b) (int64((b) - (a)) <= 0)
#define SDL_Vulkan_GetDrawableSize SDL_GetWindowSizeInPixels
#define SDL_WINDOW_FULLSCREEN_DESKTOP SDL_WINDOW_FULLSCREEN

#define IMG_Load_RW IMG_Load_IO
#define IMG_LoadSizedSVG_RW IMG_LoadSizedSVG_IO
#define IMG_LoadTGA_RW IMG_LoadTGA_IO

#define mpvec2 vec2
#define keyFromScancode(k) SDL_GetKeyFromScancode(k, SDL_KMOD_NONE, true)
#define scancodeFromKey(k) SDL_GetScancodeFromKey(k, nullptr)
#define sdlFailed(r) !(r)
#define sdlSucceeded(r) (r)
#define surfaceBytesPpx(s) SDL_BYTESPERPIXEL((s)->format)
#define surfaceFormat(s) (s)->format
#define surfacePalette(s) SDL_GetSurfacePalette(s)
#define surfaceScaleNearest(si, sr, di, dr) SDL_BlitSurfaceScaled(si, sr, di, dr, SDL_SCALEMODE_NEAREST)
#define surfaceScaleLinear(si, sr, di, dr) SDL_BlitSurfaceScaled(si, sr, di, dr, SDL_SCALEMODE_LINEAR)
#define tick_t uint64

#else

#define SDL_CreateSurface(w, h, t) SDL_CreateRGBSurfaceWithFormat(0, w, h, SDL_BITSPERPIXEL(t), t)
#define SDL_CreateSurfaceFrom(w, h, t, x, p) SDL_CreateRGBSurfaceWithFormatFrom(x, w, h, SDL_BITSPERPIXEL(t), p, t)
#define SDL_DisplayID int
#define SDL_IOWhence int
#define SDL_MapSurfaceRGBA(s, r, g, b, a) SDL_MapRGBA((s)->format, r, g, b, a)
#define SDL_WindowID uint32

#define mpvec2 ivec2
#define keyFromScancode(k) SDL_GetKeyFromScancode(k)
#define scancodeFromKey(k) SDL_GetScancodeFromKey(k)
#define sdlFailed(r) r
#define sdlSucceeded(r) !(r)
#define surfaceBytesPpx(s) (s)->format->BytesPerPixel
#define surfaceFormat(s) SDL_PixelFormatEnum((s)->format->format)
#define surfacePalette(s) (s)->format->palette
#define surfaceScaleNearest(si, sr, di, dr) SDL_BlitScaled(si, sr, di, dr)
#define surfaceScaleLinear(si, sr, di, dr) SDL_BlitScaled(si, sr, di, dr)
#define tick_t uint32
#endif

namespace std {

template <>
struct default_delete<SDL_Surface> {
	void operator()(SDL_Surface* ptr) const noexcept { SDL_FreeSurface(ptr); }
};

template <>
struct default_delete<SDL_RWops> {
	void operator()(SDL_RWops* ptr) const noexcept { SDL_RWclose(ptr); }
};

}

struct SdlFreePtr {
	void operator()(void* ptr) const noexcept { SDL_free(ptr); }
};

#ifdef WITH_SDL3
template <>
struct DefaultHandleClose<SDL_PropertiesID> {
	void operator()(SDL_PropertiesID hnd) const noexcept { SDL_DestroyProperties(hnd); }
};
#endif
