#ifdef WITH_OPENGL
#include "rendererGl.h"
#include "shaders/glDefs.h"
#include <SDL_log.h>
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include <regex>

#ifdef _WIN32
#define gfget FunctionsGl& gl = static_cast<ViewGl*>(cvw)->gl;
#define gf1get FunctionsGl& gl = static_cast<ViewGl*>(cvw)->gl; FunctionsGl1& gl1 = static_cast<ViewGl1*>(cvw)->gl1;
#define gf3get FunctionsGl& gl = static_cast<ViewGl*>(cvw)->gl; FunctionsGl3& gl3 = static_cast<ViewGl3*>(cvw)->gl3;
#define gf3set gl = static_cast<ViewGl*>(cvw)->gl; gl3 = static_cast<ViewGl3*>(cvw)->gl3;
#else
#define gfget
#define gf1get
#define gf3get
#define gf3set
#endif

// FUNCTIONS

void FunctionsGl::initFunctions() {
	if (!((bindTexture = reinterpret_cast<decltype(bindTexture)>(SDL_GL_GetProcAddress("glBindTexture")))
		&& (blendFunc = reinterpret_cast<decltype(blendFunc)>(SDL_GL_GetProcAddress("glBlendFunc")))
		&& (clear = reinterpret_cast<decltype(clear)>(SDL_GL_GetProcAddress("glClear")))
		&& (clearColor = reinterpret_cast<decltype(clearColor)>(SDL_GL_GetProcAddress("glClearColor")))
		&& (deleteTextures = reinterpret_cast<decltype(deleteTextures)>(SDL_GL_GetProcAddress("glDeleteTextures")))
		&& (disable = reinterpret_cast<decltype(disable)>(SDL_GL_GetProcAddress("glDisable")))
		&& (drawArrays = reinterpret_cast<decltype(drawArrays)>(SDL_GL_GetProcAddress("glDrawArrays")))
		&& (enable = reinterpret_cast<decltype(enable)>(SDL_GL_GetProcAddress("glEnable")))
		&& (frontFace = reinterpret_cast<decltype(frontFace)>(SDL_GL_GetProcAddress("glFrontFace")))
		&& (genTextures = reinterpret_cast<decltype(genTextures)>(SDL_GL_GetProcAddress("glGenTextures")))
		&& (getIntegerv = reinterpret_cast<decltype(getIntegerv)>(SDL_GL_GetProcAddress("glGetIntegerv")))
		&& (pixelStorei = reinterpret_cast<decltype(pixelStorei)>(SDL_GL_GetProcAddress("glPixelStorei")))
		&& (texImage2D = reinterpret_cast<decltype(texImage2D)>(SDL_GL_GetProcAddress("glTexImage2D")))
		&& (texParameteri = reinterpret_cast<decltype(texParameteri)>(SDL_GL_GetProcAddress("glTexParameteri")))
		&& (viewport = reinterpret_cast<decltype(viewport)>(SDL_GL_GetProcAddress("glViewport")))
	))
		throw std::runtime_error("Failed to find core OpenGL functions");
}

void FunctionsGl1::initFunctions() {
	if (!((color4fv = reinterpret_cast<decltype(color4fv)>(SDL_GL_GetProcAddress("glColor4fv")))
		&& (enableClientState = reinterpret_cast<decltype(enableClientState)>(SDL_GL_GetProcAddress("glEnableClientState")))
		&& (loadMatrixf = reinterpret_cast<decltype(loadMatrixf)>(SDL_GL_GetProcAddress("glLoadMatrixf")))
		&& (matrixMode = reinterpret_cast<decltype(matrixMode)>(SDL_GL_GetProcAddress("glMatrixMode")))
		&& (texCoordPointer = reinterpret_cast<decltype(texCoordPointer)>(SDL_GL_GetProcAddress("glTexCoordPointer")))
		&& (vertexPointer = reinterpret_cast<decltype(vertexPointer)>(SDL_GL_GetProcAddress("glVertexPointer")))
	))
		throw std::runtime_error("Failed to find legacy OpenGL functions");
}

void FunctionsGl3::initFunctions() {
	if (!((attachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(SDL_GL_GetProcAddress("glAttachShader")))
		&& (bindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer")))
		&& (bindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(SDL_GL_GetProcAddress("glBindVertexArray")))
		&& (bufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData")))
		&& (compileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(SDL_GL_GetProcAddress("glCompileShader")))
		&& (createProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(SDL_GL_GetProcAddress("glCreateProgram")))
		&& (createShader = reinterpret_cast<PFNGLCREATESHADERPROC>(SDL_GL_GetProcAddress("glCreateShader")))
		&& (deleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers")))
		&& (deleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(SDL_GL_GetProcAddress("glDeleteShader")))
		&& (deleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(SDL_GL_GetProcAddress("glDeleteProgram")))
		&& (deleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glDeleteVertexArrays")))
		&& (detachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(SDL_GL_GetProcAddress("glDetachShader")))
		&& (enableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(SDL_GL_GetProcAddress("glEnableVertexAttribArray")))
		&& (genBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers")))
		&& (genVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glGenVertexArrays")))
		&& (getAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(SDL_GL_GetProcAddress("glGetAttribLocation")))
		&& (getProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(SDL_GL_GetProcAddress("glGetProgramInfoLog")))
		&& (getProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(SDL_GL_GetProcAddress("glGetProgramiv")))
		&& (getShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(SDL_GL_GetProcAddress("glGetShaderInfoLog")))
		&& (getShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(SDL_GL_GetProcAddress("glGetShaderiv")))
		&& (getUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(SDL_GL_GetProcAddress("glGetUniformLocation")))
		&& (linkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(SDL_GL_GetProcAddress("glLinkProgram")))
		&& (shaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(SDL_GL_GetProcAddress("glShaderSource")))
		&& (uniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(SDL_GL_GetProcAddress("glUniform1i")))
		&& (uniform4f = reinterpret_cast<PFNGLUNIFORM4FPROC>(SDL_GL_GetProcAddress("glUniform4f")))
		&& (uniform4fv = reinterpret_cast<PFNGLUNIFORM4FVPROC>(SDL_GL_GetProcAddress("glUniform4fv")))
		&& (uniform4iv = reinterpret_cast<PFNGLUNIFORM4IVPROC>(SDL_GL_GetProcAddress("glUniform4iv")))
		&& (useProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(SDL_GL_GetProcAddress("glUseProgram")))
		&& (vertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(SDL_GL_GetProcAddress("glVertexAttribPointer")))
	))
		throw std::runtime_error("Failed to find OpenGL extension functions");
}

// RENDERER GL

RendererGl::SurfaceInfo::SurfaceInfo(SDL_Surface* surface, uint16 internal, uint16 format, uint16 texel) noexcept :
	img(surface),
	ifmt(internal),
	pfmt(format),
	type(texel)
{
	for (align = 8; align > 1 && (uintptr_t(img->pixels) % align || uint(img->pitch) % align); align /= 2);
}

RendererGl::RendererGl(size_t numViews) :
	Renderer(numViews, UINT_MAX)
{
	int profile;
	if (sdlFailed(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile)))
		throw std::runtime_error(SDL_GetError());
	core = profile != SDL_GL_CONTEXT_PROFILE_ES;
	hasTextureCompression = core;
}

void RendererGl::setContext(View* view) {
	auto vw = static_cast<ViewGl*>(view);
#ifdef _WIN32
	cvw = vw;
#endif
	if (sdlFailed(SDL_GL_MakeCurrent(vw->win, vw->ctx)))
		throw std::runtime_error(SDL_GetError());
}

template <Class T, class F>
void RendererGl::initContexts(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, F initGl) {
	if (!vofs) {
		SDL_GL_GetDrawableSize(windows[0], &viewRes.x, &viewRes.y);
		auto vw = static_cast<T*>(views[0] = new T(windows[0], Recti(ivec2(0), viewRes)));
		if (vw->ctx = SDL_GL_CreateContext(windows[0]); !vw->ctx)
			throw std::runtime_error(SDL_GetError());
		setContext(vw);
		initGl(vw);
	} else
		for (size_t i = 0; i < views.size(); ++i) {
			Recti wrect;
			wrect.pos() = vofs[i] - vofs[views.size()];
			SDL_GL_GetDrawableSize(windows[i], &wrect.w, &wrect.h);
			viewRes = glm::max(viewRes, wrect.end());
			auto vw = static_cast<T*>(views[i] = new T(windows[i], wrect));
			if (vw->ctx = SDL_GL_CreateContext(windows[i]); !vw->ctx)
				throw std::runtime_error(SDL_GetError());
			setContext(vw);
			initGl(vw);
		}
}

void RendererGl::initGlCommon(ViewGl* view, bool vsync, const vec4& bgcolor, uintptr_t& availableMemory) noexcept {
	gfget
	setSwapInterval(vsync);

	int gval[4];
	if (gl.getIntegerv(GL_MAX_TEXTURE_SIZE, gval); uint(gval[0]) < maxTextureSize)
		maxTextureSize = gval[0];
	if (SDL_GL_ExtensionSupported("GL_NVX_gpu_memory_info")) {
		if (gl.getIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, gval); uint(gval[0]) > availableMemory)
			availableMemory = gval[0];
	} else if (SDL_GL_ExtensionSupported("GL_ATI_meminfo"))
		if (gl.getIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, gval); uint(gval[0]) > availableMemory)
			availableMemory = gval[0];

#ifndef NDEBUG
	if (bool khr = SDL_GL_ExtensionSupported("GL_KHR_debug"); khr || SDL_GL_ExtensionSupported("GL_ARB_debug_output")) {
		PFNGLDEBUGMESSAGECALLBACKPROC debugMessageCallback = nullptr;
		PFNGLDEBUGMESSAGECONTROLPROC debugMessageControl = nullptr;
		if (khr) {
			if (gl.getIntegerv(GL_CONTEXT_FLAGS, gval); gval[0] & GL_CONTEXT_FLAG_DEBUG_BIT) {
				debugMessageCallback = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKPROC>(SDL_GL_GetProcAddress("glDebugMessageCallback"));
				debugMessageControl = reinterpret_cast<PFNGLDEBUGMESSAGECONTROLPROC>(SDL_GL_GetProcAddress("glDebugMessageControl"));
			}
		} else {
			debugMessageCallback = reinterpret_cast<PFNGLDEBUGMESSAGECALLBACKPROC>(SDL_GL_GetProcAddress("glDebugMessageCallbackARB"));
			debugMessageControl = reinterpret_cast<PFNGLDEBUGMESSAGECONTROLPROC>(SDL_GL_GetProcAddress("glDebugMessageControlARB"));
		}
		if (debugMessageCallback && debugMessageControl) {
			if (khr)
				gl.enable(GL_DEBUG_OUTPUT);
			gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			debugMessageCallback(debugMessage, view);
			debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}
	}
#endif
	gl.viewport(0, 0, view->rect.w, view->rect.h);
	gl.clearColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	gl.disable(GL_MULTISAMPLE);
	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RendererGl::finalizeConstruction(Settings* sets, uintptr_t availableMemory) noexcept {
	setCompression(sets->compression);
	setMaxPicRes(sets->maxPicRes);
	if (!sets->picLim.size) {
		sets->picLim.size = availableMemory * 1024 / 2;
		recommendPicRamLimit(sets->picLim.size);
	}
}

#ifndef NDEBUG
void APIENTRY RendererGl::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam) noexcept {
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;

	const char* ssrc;
	switch (source) {
	case GL_DEBUG_SOURCE_API:
		ssrc = "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		ssrc = "window system";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		ssrc = "shader compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		ssrc = "third party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		ssrc = "application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		ssrc = "other";
		break;
	default:
		ssrc = "unknown";
	}

	const char* stype;
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		stype = "error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		stype = "deprecated behavior";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		stype = "undefined behavior";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		stype = "portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		stype = "performance";
		break;
	case GL_DEBUG_TYPE_OTHER:
		stype = "other";
		break;
	case GL_DEBUG_TYPE_MARKER:
		stype = "marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		stype = "push group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		stype = "pop group";
		break;
	default:
		stype = "unknown";
	}

	const char* ssever;
	SDL_LogPriority prio = SDL_LOG_PRIORITY_ERROR;
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		ssever = "high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		ssever = "medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		ssever = "low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		ssever = "notification";
		prio = SDL_LOG_PRIORITY_WARN;
		break;
	default:
		ssever = "unknown";
	}

	auto view = static_cast<const ViewGl*>(userParam);
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, prio, "OpenGL: %u, Context: %d %d %d %d, Source: %s, Type: %s, Severity: %s, Message: %s", id, view->rect.x, view->rect.y, view->rect.w, view->rect.h, ssrc, stype, ssever, string(trim(string_view(message, length))).data());
}
#endif

void RendererGl::setClearColor(const vec4& color) {
	for (View* it : views) {
		setContext(it);
		gfget
		gl.clearColor(color.r, color.g, color.b, color.a);
	}
}

void RendererGl::setVsync(bool vsync) {
	for (View* it : views) {
		setContext(it);
		setSwapInterval(vsync);
	}
}

void RendererGl::setSwapInterval(bool vsync) noexcept {
	if (!vsync || (SDL_GL_SetSwapInterval(-1) && SDL_GL_SetSwapInterval(1)))
		SDL_GL_SetSwapInterval(0);
}

void RendererGl::finishDraw(View* view) {
	SDL_GL_SwapWindow(static_cast<ViewGl*>(view)->win);
}

Texture* RendererGl::texFromEmpty() {
	return new TextureGl(uvec2(0), initTexture(GL_NEAREST));
}

Texture* RendererGl::texFromIcon(SDL_Surface* img) noexcept {
	if (SurfaceInfo si = pickPixFormat<true>(limitSize(img, maxTextureSize)); si.img) {
		gfget
		auto tex = new TextureGl(uvec2(si.img->w, si.img->h), initTexture(GL_LINEAR));
		uploadTexture(tex, si);
		return tex;
	}
	return nullptr;
}

bool RendererGl::texFromIcon(Texture* tex, SDL_Surface* img) noexcept {
	if (SurfaceInfo si = pickPixFormat<true>(limitSize(img, maxTextureSize)); si.img) {
		gfget
		auto gtx = static_cast<TextureGl*>(tex);
		gtx->res = uvec2(si.img->w, si.img->h);
		gl.bindTexture(texType, gtx->id);
		uploadTexture(gtx, si);
		return true;
	}
	return false;
}

Texture* RendererGl::texFromRpic(SDL_Surface* img) noexcept {
	if (SurfaceInfo si = pickPixFormat<false>(img); si.img) {
		gfget
		auto tex = new TextureGl(uvec2(si.img->w, si.img->h), initTexture(GL_LINEAR));
		uploadTexture(tex, si);
		return tex;
	}
	return nullptr;
}

Texture* RendererGl::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x) {
		gfget
		auto tex = new TextureGl(glm::min(pm.res, uvec2(maxTextureSize)), initTexture(GL_NEAREST));
		uploadTexture(tex, pm);
		return tex;
	}
	return nullptr;
}

bool RendererGl::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (pm.res.x) {
		gfget
		auto gtx = static_cast<TextureGl*>(tex);
		gtx->res = glm::min(pm.res, uvec2(maxTextureSize));
		gl.bindTexture(texType, gtx->id);
		uploadTexture(gtx, pm);
		return true;
	}
	return false;
}

void RendererGl::freeTexture(Texture* tex) noexcept {
	if (auto gtx = static_cast<TextureGl*>(tex)) {
		gfget
		gl.deleteTextures(1, &gtx->id);
		delete gtx;
	}
}

GLuint RendererGl::initTexture(GLint filter) noexcept {
	gfget
	GLuint id;
	gl.genTextures(1, &id);
	gl.bindTexture(texType, id);
	gl.texParameteri(texType, GL_TEXTURE_MIN_FILTER, filter);
	gl.texParameteri(texType, GL_TEXTURE_MAG_FILTER, filter);
	gl.texParameteri(texType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(texType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return id;
}

void RendererGl::uploadTexture(TextureGl* tex, SurfaceInfo& si) noexcept {
	gfget
	gl.pixelStorei(GL_UNPACK_ROW_LENGTH, uint(si.img->pitch) / surfaceBytesPpx(si.img));
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, si.align);
	if (si.ifmt == GL_R8) {
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_R, GL_RED);	// TODO: why don't these stay bound?
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_G, GL_RED);
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_B, GL_RED);
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_A, GL_ONE);
	}
	gl.texImage2D(texType, 0, si.ifmt, tex->res.x, tex->res.y, 0, si.pfmt, si.type, si.img->pixels);
	SDL_FreeSurface(si.img);
}

void RendererGl::uploadTexture(TextureGl* tex, const Pixmap& pm) noexcept {
	gfget
	gl.pixelStorei(GL_UNPACK_ROW_LENGTH, pm.res.x);
	if (hasSwizzle) {
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(uint8));
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_R, GL_ONE);	// TODO: why don't these stay bound?
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_G, GL_ONE);
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_B, GL_ONE);
		gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_A, GL_RED);
		gl.texImage2D(texType, 0, GL_R8, tex->res.x, tex->res.y, 0, GL_RED, GL_UNSIGNED_BYTE, pm.pix.get());
	} else {
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(uint32));
		uptr<uint32[]> pix = std::make_unique_for_overwrite<uint32[]>(tex->res.x * tex->res.y);
		copyTextPixels(pix.get(), pm, tex->res, tex->res.x * sizeof(uint32));
		gl.texImage2D(texType, 0, GL_RGBA8, tex->res.x, tex->res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix.get());
	}
}

template <bool keep>
RendererGl::SurfaceInfo RendererGl::pickPixFormat(SDL_Surface* img) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (surfaceFormat(img)) {
	case SDL_PIXELFORMAT_ABGR8888:
		if constexpr (keep)
			return SurfaceInfo(img, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		else
			return SurfaceInfo(img, iformRgba8, GL_RGBA, GL_UNSIGNED_BYTE);
	case SDL_PIXELFORMAT_ARGB8888:
		if (hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE);
			else
				return SurfaceInfo(img, iformRgba8, GL_BGRA, GL_UNSIGNED_BYTE);
		}
		break;
	case SDL_PIXELFORMAT_RGB24:
		if constexpr (keep)
			return SurfaceInfo(img, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
		else
			return SurfaceInfo(img, iformRgb8, GL_RGB, GL_UNSIGNED_BYTE);
	case SDL_PIXELFORMAT_BGR24:
		if (hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE);
			else
				return SurfaceInfo(img, iformRgb8, GL_BGR, GL_UNSIGNED_BYTE);
		}
		break;
#if SDL_VERSION_ATLEAST(3, 0, 0)
	case SDL_PIXELFORMAT_ABGR2101010:
		if (hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
			else
				return SurfaceInfo(img, iformRgba10, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
		}
		break;
#endif
	case SDL_PIXELFORMAT_ARGB2101010:
		if (hasPackedPixels && hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV);
			else
				return SurfaceInfo(img, iformRgba10, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV);
		}
		break;
	case SDL_PIXELFORMAT_BGR565:
		if (core && hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV);
			else
				return SurfaceInfo(img, iformRgb5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV);
		}
		break;
	case SDL_PIXELFORMAT_RGB565:
		if (hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, core ? GL_RGB5 : GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
			else
				return SurfaceInfo(img, iformRgb5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
		}
		break;
	case SDL_PIXELFORMAT_ABGR1555:
		if (core && hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
			else
				return SurfaceInfo(img, iformRgba5, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		}
		break;
	case SDL_PIXELFORMAT_ARGB1555:
		if (core && hasPackedPixels && hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
			else
				return SurfaceInfo(img, iformRgba5, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		}
		break;
	case SDL_PIXELFORMAT_BGRA5551:
		if (hasPackedPixels && hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1);
			else
				return SurfaceInfo(img, iformRgba5, GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1);
		}
		break;
	case SDL_PIXELFORMAT_RGBA5551:
		if (hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
			else
				return SurfaceInfo(img, iformRgba5, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
		}
		break;
	case SDL_PIXELFORMAT_ABGR4444:
		if (core && hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV);
			else
				return SurfaceInfo(img, iformRgba4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV);
		}
		break;
	case SDL_PIXELFORMAT_ARGB4444:
		if (core && hasPackedPixels && hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV);
			else
				return SurfaceInfo(img, iformRgba4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV);
		}
		break;
	case SDL_PIXELFORMAT_BGRA4444:
		if (hasPackedPixels && hasBgra) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
			else
				return SurfaceInfo(img, iformRgba4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
		}
		break;
	case SDL_PIXELFORMAT_RGBA4444:
		if (hasPackedPixels) {
			if constexpr (keep)
				return SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
			else
				return SurfaceInfo(img, iformRgba4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
		}
		break;
	case SDL_PIXELFORMAT_RGB332:
		if (core && hasPackedPixels)
			return SurfaceInfo(img, GL_R3_G3_B2, GL_RGB, GL_UNSIGNED_BYTE_3_3_2);
		break;
	case SDL_PIXELFORMAT_INDEX8:
		if (hasSwizzle && isIndexedGrayscale(img))
			return SurfaceInfo(img, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
	}
	if constexpr (keep)
		return SurfaceInfo(convertReplace(img), GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	else
		return SurfaceInfo(convertReplace(img), iformRgba8, GL_RGBA, GL_UNSIGNED_BYTE);
}

void RendererGl::setCompression(Settings::Compression cmpr) noexcept {
	switch (cmpr) {
	using enum Settings::Compression;
	case b8:
		if (hasPackedPixels && core) {
			iformRgba8 = iformRgb8 = iformRgba10 = iformRgba5 = iformRgb5 = iformRgba4 = GL_R3_G3_B2;
			compression = b8;
			return;
		}
	case b16:
		if (hasPackedPixels) {
			iformRgba8 = iformRgba10 = iformRgba5 = GL_RGB5_A1;
			iformRgb8 = iformRgb5 = core ? GL_RGB5 : GL_RGB565;
			iformRgba4 = GL_RGBA4;
			compression = b16;
			return;
		}
	case compress:
		if (hasTextureCompression) {
			iformRgba8 = GL_COMPRESSED_RGBA;
			iformRgb8 = GL_COMPRESSED_RGB;
			iformRgba10 = GL_COMPRESSED_RGBA;
			iformRgba5 = GL_RGB5_A1;
			iformRgb5 = core ? GL_RGB5 : GL_RGB565;
			iformRgba4 = GL_RGBA4;
			compression = compress;
			return;
		}
	}
	iformRgba8 = GL_RGBA8;
	iformRgb8 = GL_RGB8;
	iformRgba10 = GL_RGB10_A2;
	iformRgba5 = GL_RGB5_A1;
	iformRgb5 = core ? GL_RGB5 : GL_RGB565;
	iformRgba4 = GL_RGBA4;
	compression = Settings::Compression::none;
}

SDL_Surface* RendererGl::prepareImage(SDL_Surface* img, bool rpic) const noexcept {
	if (img = limitSize(img, rpic ? maxPictureSize : maxTextureSize); img) {
		switch (compression) {
		using enum Settings::Compression;
		case b8:
			return convertReplace(img, SDL_PIXELFORMAT_RGB332);
		case b16:
			return convertReplace(img, SDL_ISPIXELFORMAT_ALPHA(surfaceFormat(img)) ? SDL_PIXELFORMAT_RGBA5551 : SDL_PIXELFORMAT_RGB565);
		}
#if SDL_VERSION_ATLEAST(3, 0, 0)
		if (hasPackedPixels && (
			(img->format == SDL_PIXELFORMAT_ARGB2101010 && !hasBgra)
			|| img->format == SDL_PIXELFORMAT_XBGR2101010 || img->format == SDL_PIXELFORMAT_XBGR2101010
			|| SDL_BYTESPERPIXEL(img->format) > 4
		))
			return convertReplace(img, SDL_PIXELFORMAT_ABGR2101010);
#endif
	}
	return img;
}

Renderer::Info RendererGl::getInfo() const noexcept {
	Info info = {
		.compressions = { Settings::Compression::none },
		.texSize = maxTextureSize,
		.curCompression = compression
	};
	if (hasPackedPixels) {
		if (core)
			info.compressions.push_back(Settings::Compression::b8);
		info.compressions.push_back(Settings::Compression::b16);
	}
	if (hasTextureCompression)
		info.compressions.push_back(Settings::Compression::compress);
	return info;
}

// RENDERER GL 1

RendererGl1::ViewGl1::ViewGl1(SDL_Window* window, const Recti& area) noexcept :
	ViewGl(window, area),
	proj(glm::ortho(float(area.x), float(area.x + area.w), float(area.y + area.h), float(area.y)))
{}

RendererGl1::RendererGl1(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor) :
	RendererGl(windows.size())
{
#ifndef _WIN32
	gl.initFunctions();
	gl1.initFunctions();
#endif
	try {
		bool canTexRect = true;
		uintptr_t availableMemory = 0;
		initContexts<ViewGl1>(windows, vofs, viewRes, [this, sets, bgcolor, &canTexRect, &availableMemory](ViewGl1* vw) { initGl(vw, sets->vsync, bgcolor, canTexRect, availableMemory); });
		for (View* it : views) {
			setContext(it);
			gfget
			gl.enable(texType);
		}
		finalizeConstruction(sets, availableMemory);
	} catch (...) {
		cleanup();
		throw;
	}
}

RendererGl1::~RendererGl1() {
	cleanup();
}

void RendererGl1::initGl(ViewGl1* view, bool vsync, const vec4& bgcolor, bool& canTexRect, uintptr_t& availableMemory) {
#ifdef _WIN32
	gf1get
	gl.initFunctions();
	gl1.initFunctions();
#endif
	canTexRect = canTexRect && (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle") || SDL_GL_ExtensionSupported("GL_NV_texture_rectangle"));
	switch (texType) {
	case GL_TEXTURE_2D:
		if (SDL_GL_ExtensionSupported("GL_ARB_texture_non_power_of_two"))
			break;
		texType = GL_TEXTURE_RECTANGLE;
	default:
		if (!canTexRect)
			throw std::runtime_error("No support for non power of two textures");
	}
	hasBgra = hasBgra && SDL_GL_ExtensionSupported("GL_EXT_bgra");
	hasPackedPixels = hasPackedPixels && (SDL_GL_ExtensionSupported("GL_EXT_packed_pixels") || SDL_GL_ExtensionSupported("GL_APPLE_packed_pixels"));
	hasTextureCompression = hasTextureCompression && SDL_GL_ExtensionSupported("GL_ARB_texture_compression");
	hasSwizzle = hasSwizzle && (SDL_GL_ExtensionSupported("GL_ARB_texture_swizzle") || SDL_GL_ExtensionSupported("GL_EXT_texture_swizzle"));
	initGlCommon(view, vsync, bgcolor, availableMemory);
	gl1.enableClientState(GL_VERTEX_ARRAY);
	gl1.enableClientState(GL_TEXTURE_COORD_ARRAY);
	gl1.vertexPointer(vec2::length(), GL_FLOAT, 0, vertices.data());
	gl1.texCoordPointer(vec2::length(), GL_FLOAT, 0, vertices.data());
}

void RendererGl1::cleanup() noexcept {
	for (View* it : views)
		if (auto vw = static_cast<ViewGl1*>(it)) {
			SDL_GL_DeleteContext(vw->ctx);
			delete vw;
		}
}

void RendererGl1::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		auto vw = static_cast<ViewGl1*>(views[0]);
		gfget
		SDL_GL_GetDrawableSize(vw->win, &viewRes.x, &viewRes.y);
		vw->rect.size() = viewRes;
		vw->proj = glm::ortho(0.f, float(viewRes.x), float(viewRes.y), 0.f);
		gl.viewport(0, 0, viewRes.x, viewRes.y);
	}
}

void RendererGl1::startDraw(View* view) {
	setContext(view);
	gf1get
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl1.matrixMode(GL_PROJECTION);
	gl1.loadMatrixf(glm::value_ptr(static_cast<ViewGl1*>(view)->proj));
}

void RendererGl1::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	if (Recti isct; SDL_IntersectRect(&rect.asRect(), &frame.asRect(), &isct.asRect())) {
		gf1get

		setPosScale(model, isct);
		gl1.matrixMode(GL_MODELVIEW);
		gl1.loadMatrixf(glm::value_ptr(model));

		setPosScale(mtex, texType == GL_TEXTURE_2D
			? Rectf(vec2(isct.pos() - rect.pos()) / vec2(rect.size()), vec2(isct.size()) / vec2(rect.size()))
			: cropTexRect(isct, rect, tex->getRes()));
		gl1.matrixMode(GL_TEXTURE);
		gl1.loadMatrixf(glm::value_ptr(mtex));

		gl.bindTexture(texType, static_cast<const TextureGl*>(tex)->id);
		gl1.color4fv(glm::value_ptr(color));
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
	}
}

template <Number T>
void RendererGl1::setPosScale(mat4& matrix, const Rect<T>& rect) {
	matrix[0][0] = float(rect.w);
	matrix[1][1] = float(rect.h);
	matrix[3][0] = float(rect.x);
	matrix[3][1] = float(rect.y);
}

// RENDERER GL 3

RendererGl3::RendererGl3(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor) :
	RendererGl(windows.size())
{
#ifndef _WIN32
	gl.initFunctions();
	gl3.initFunctions();
#endif
	try {
		uintptr_t availableMemory = 0;
		initContexts<ViewGl3>(windows, vofs, viewRes, [this, sets, bgcolor, &availableMemory](ViewGl3* vw) { initGl(vw, sets->vsync, bgcolor, availableMemory); });
		initShader();
		finalizeConstruction(sets, availableMemory);
	} catch (...) {
		cleanup();
		throw;
	}
}

RendererGl3::~RendererGl3() {
	cleanup();
}

void RendererGl3::initGl(ViewGl3* view, bool vsync, const vec4& bgcolor, uintptr_t& availableMemory) {
#ifdef _WIN32
	gf3get
	gl.initFunctions();
	gl3.initFunctions();
#endif
	if (core)
		hasSwizzle = hasSwizzle && (SDL_GL_ExtensionSupported("GL_ARB_texture_swizzle") || SDL_GL_ExtensionSupported("GL_EXT_texture_swizzle"));
	else
		hasBgra = hasBgra && SDL_GL_ExtensionSupported("GL_MESA_bgra");
	initGlCommon(view, vsync, bgcolor, availableMemory);
	gl.disable(GL_DITHER);
}

void RendererGl3::cleanup() noexcept {
	if (View* vw = views[0]) {
		try {
			setContext(vw);
			gf3get
			if (gl3.functionsInitialized()) {
				gl3.deleteBuffers(1, &vbo);
				gl3.deleteProgram(progGui);
			}
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	for (View* it : views)
		if (auto vw = static_cast<ViewGl3*>(it)) {
			try {
				setContext(vw);
				gf3get
				if (gl3.functionsInitialized())
					gl3.deleteVertexArrays(1, &vw->vao);
			} catch (const std::runtime_error& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			}
			SDL_GL_DeleteContext(vw->ctx);
			delete vw;
		}
}

void RendererGl3::initShader() {
	vector<View*>::iterator vw = views.begin();
	setContext(*vw);
	gf3get
	const char* vertSrc =
#ifdef NDEBUG
#include "shaders/glGui.vert.rel.h"
#else
#include "shaders/glGui.vert.dbg.h"
#endif
	;
	const char* fragSrc =
#ifdef NDEBUG
#include "shaders/glGui.frag.rel.h"
#else
#include "shaders/glGui.frag.dbg.h"
#endif
	;
	progGui = createShader(vertSrc, fragSrc, "gui");
	GLuint attrVposGui = gl3.getAttribLocation(progGui, ATTR_GUI_VPOS);
	uniPviewGui = gl3.getUniformLocation(progGui, UNI_GUI_PVIEW);
	uniRectGui = gl3.getUniformLocation(progGui, UNI_GUI_RECT);
	uniFrameGui = gl3.getUniformLocation(progGui, UNI_GUI_FRAME);
	uniColorGui = gl3.getUniformLocation(progGui, UNI_GUI_COLOR);
	gl3.uniform1i(gl3.getUniformLocation(progGui, UNI_GUI_COLORMAP), 0);

	gl3.genBuffers(1, &vbo);
	gl3.bindBuffer(GL_ARRAY_BUFFER, vbo);
	gl3.bufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(*vertices.data()), vertices.data(), GL_STATIC_DRAW);
	for (;;) {
		auto gvw = static_cast<ViewGl3*>(*vw);
		gl3.genVertexArrays(1, &gvw->vao);
		gl3.bindVertexArray(gvw->vao);
		gl3.enableVertexAttribArray(attrVposGui);
		gl3.vertexAttribPointer(attrVposGui, vec2::length(), GL_FLOAT, GL_FALSE, 0, nullptr);

		gl3.useProgram(progGui);
		if (++vw == views.end())
			break;
		setContext(*vw);
		gf3set
		gl3.bindBuffer(GL_ARRAY_BUFFER, vbo);
	}
}

GLuint RendererGl3::createShader(const char* vertSrc, const char* fragSrc, const char* name) const {
	gf3get
	string vertTmp, fragTmp;
	if (!core) {
		pair<std::regex, const char*> replacers[2] = {
			pair(std::regex(R"r(#version\s+\d+)r"), "#version 300 es\nprecision highp float;precision highp int;precision highp sampler2D;"),
			pair(std::regex(R"r(noperspective\s+)r"), "")
		};
		vertTmp = vertSrc;
		fragTmp = fragSrc;
		for (const auto& [rgx, rpl] : replacers) {
			vertTmp = std::regex_replace(vertTmp, rgx, rpl);
			fragTmp = std::regex_replace(fragTmp, rgx, rpl);
		}
		vertSrc = vertTmp.data();
		fragSrc = fragTmp.data();
	}

	GLuint vert = gl3.createShader(GL_VERTEX_SHADER);
	gl3.shaderSource(vert, 1, &vertSrc, nullptr);
	gl3.compileShader(vert);
	checkStatus(vert, GL_COMPILE_STATUS, gl3.getShaderiv, gl3.getShaderInfoLog, std::format("{}.vert", name));

	GLuint frag = gl3.createShader(GL_FRAGMENT_SHADER);
	gl3.shaderSource(frag, 1, &fragSrc, nullptr);
	gl3.compileShader(frag);
	checkStatus(frag, GL_COMPILE_STATUS, gl3.getShaderiv, gl3.getShaderInfoLog, std::format("{}.frag", name));

	GLuint sprog = gl3.createProgram();
	gl3.attachShader(sprog, vert);
	gl3.attachShader(sprog, frag);
	gl3.linkProgram(sprog);
	gl3.detachShader(sprog, vert);
	gl3.detachShader(sprog, frag);
	gl3.deleteShader(vert);
	gl3.deleteShader(frag);
	checkStatus(sprog, GL_LINK_STATUS, gl3.getProgramiv, gl3.getProgramInfoLog, std::format("{} program", name));
	gl3.useProgram(sprog);
	return sprog;
}

void RendererGl3::checkStatus(GLuint id, GLenum stat, PFNGLGETSHADERIVPROC check, PFNGLGETSHADERINFOLOGPROC info, const string& name) {
	int len, res;
	string msg;
	if (check(id, GL_INFO_LOG_LENGTH, &len); len > 1) {
		msg.resize(--len);
		if (info(id, len, &res, msg.data()); res < len)
			msg.resize(res);
		msg = trim(msg);
	}
	if (check(id, stat, &res); res == GL_FALSE)
		throw std::runtime_error(!msg.empty() ? std::format("{}:" LINEND "{}", name, msg) : std::format("{}: unknown error", name));
	if (!msg.empty())
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:" LINEND "%s", name.data(), msg.data());
}

void RendererGl3::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		gfget
		SDL_GL_GetDrawableSize(views[0]->win, &viewRes.x, &viewRes.y);
		views[0]->rect.size() = viewRes;
		gl.viewport(0, 0, viewRes.x, viewRes.y);
	}
}

void RendererGl3::startDraw(View* view) {
	setContext(view);
	gf3get
	gl3.uniform4f(uniPviewGui, float(view->rect.x), float(view->rect.y), float(view->rect.w) / 2.f, float(view->rect.h) / 2.f);
	gl.clear(GL_COLOR_BUFFER_BIT);
}

void RendererGl3::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	gf3get
	gl.bindTexture(GL_TEXTURE_2D, static_cast<const TextureGl*>(tex)->id);
	gl3.uniform4iv(uniRectGui, 1, reinterpret_cast<const int*>(&rect));
	gl3.uniform4iv(uniFrameGui, 1, reinterpret_cast<const int*>(&frame));
	gl3.uniform4fv(uniColorGui, 1, glm::value_ptr(color));
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
}
#endif
