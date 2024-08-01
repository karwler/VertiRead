#pragma once

#include <SDL_version.h>
#if SDL_VERSION_ATLEAST(3, 0, 0)
#define SDL_ENABLE_OLD_NAMES
#include <SDL_oldnames.h>

#undef SDL_CONTROLLER_AXIS_MAX
#undef SDL_CONTROLLER_BUTTON_MAX
#undef SDL_ConvertSurfaceFormat
#undef SDL_RWread
#undef SDL_RWwrite

#define SDL_CONTROLLER_AXIS_MAX SDL_GAMEPAD_AXIS_COUNT
#define SDL_CONTROLLER_BUTTON_MAX SDL_GAMEPAD_BUTTON_COUNT
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
#define sdlFailed(r) !(r)
#define surfaceBytesPpx(s) SDL_BYTESPERPIXEL((s)->format)
#define surfaceFormat(s) (s)->format
#define surfacePalette(s) SDL_GetSurfacePalette(s)
#define surfaceScaleNearest(si, sr, di, dr) SDL_BlitSurfaceScaled(si, sr, di, dr, SDL_SCALEMODE_NEAREST)
#define surfaceScaleLinear(si, sr, di, dr) SDL_BlitSurfaceScaled(si, sr, di, dr, SDL_SCALEMODE_LINEAR)
#define tick_t uint64
#else
#define SDL_MAIN_HANDLED

#define SDL_CreateSurface(w, h, t) SDL_CreateRGBSurfaceWithFormat(0, w, h, SDL_BITSPERPIXEL(t), t)
#define SDL_DisplayID int
#define SDL_IOWhence int
#define SDL_MapSurfaceRGBA(s, r, g, b, a) SDL_MapRGBA((s)->format, r, g, b, a)

#define mpvec2 ivec2
#define sdlFailed(r) r
#define surfaceBytesPpx(s) (s)->format->BytesPerPixel
#define surfaceFormat(s) SDL_PixelFormatEnum((s)->format->format)
#define surfacePalette(s) (s)->format->palette
#define surfaceScaleNearest(si, sr, di, dr) SDL_BlitScaled(si, sr, di, dr)
#define surfaceScaleLinear(si, sr, di, dr) SDL_BlitScaled(si, sr, di, dr)
#define tick_t uint32
#endif
