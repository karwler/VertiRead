#ifdef WITH_OPENGL
#include "rendererGl.h"
#include "shaders/glDefs.h"
#include <SDL_log.h>
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

#if !defined(__arm__) && !defined(__aarch64__)
void FunctionsGl1::initFunctions() {
	if (!((clear = reinterpret_cast<decltype(clear)>(SDL_GL_GetProcAddress("glClear")))
		&& (clearColor = reinterpret_cast<decltype(clearColor)>(SDL_GL_GetProcAddress("glClearColor")))
		&& (color4fv = reinterpret_cast<decltype(color4fv)>(SDL_GL_GetProcAddress("glColor4fv")))
		&& (enableClientState = reinterpret_cast<decltype(enableClientState)>(SDL_GL_GetProcAddress("glEnableClientState")))
		&& (loadMatrixf = reinterpret_cast<decltype(loadMatrixf)>(SDL_GL_GetProcAddress("glLoadMatrixf")))
		&& (matrixMode = reinterpret_cast<decltype(matrixMode)>(SDL_GL_GetProcAddress("glMatrixMode")))
		&& (texCoordPointer = reinterpret_cast<decltype(texCoordPointer)>(SDL_GL_GetProcAddress("glTexCoordPointer")))
		&& (vertexPointer = reinterpret_cast<decltype(vertexPointer)>(SDL_GL_GetProcAddress("glVertexPointer")))
	))
		throw std::runtime_error("Failed to find legacy OpenGL functions");
}
#endif

void FunctionsGl3::initFunctions() {
	if (!((attachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(SDL_GL_GetProcAddress("glAttachShader")))
		&& (bindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(SDL_GL_GetProcAddress("glBindBuffer")))
		&& (bindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(SDL_GL_GetProcAddress("glBindFramebuffer")))
		&& (bindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(SDL_GL_GetProcAddress("glBindVertexArray")))
		&& (bufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(SDL_GL_GetProcAddress("glBufferData")))
		&& (checkFramebufferStatus = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(SDL_GL_GetProcAddress("glCheckFramebufferStatus")))
		&& (clearBufferfv = reinterpret_cast<PFNGLCLEARBUFFERFVPROC>(SDL_GL_GetProcAddress("glClearBufferfv")))
		&& (compileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(SDL_GL_GetProcAddress("glCompileShader")))
		&& (createProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(SDL_GL_GetProcAddress("glCreateProgram")))
		&& (createShader = reinterpret_cast<PFNGLCREATESHADERPROC>(SDL_GL_GetProcAddress("glCreateShader")))
		&& (deleteBuffers = reinterpret_cast<PFNGLDELETEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteBuffers")))
		&& (deleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glDeleteFramebuffers")))
		&& (deleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(SDL_GL_GetProcAddress("glDeleteShader")))
		&& (deleteProgram = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(SDL_GL_GetProcAddress("glDeleteProgram")))
		&& (deleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glDeleteVertexArrays")))
		&& (detachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(SDL_GL_GetProcAddress("glDetachShader")))
		&& (enableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(SDL_GL_GetProcAddress("glEnableVertexAttribArray")))
		&& (framebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(SDL_GL_GetProcAddress("glFramebufferTexture2D")))
		&& (genBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(SDL_GL_GetProcAddress("glGenBuffers")))
		&& (genFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(SDL_GL_GetProcAddress("glGenFramebuffers")))
		&& (genVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(SDL_GL_GetProcAddress("glGenVertexArrays")))
		&& (getAttribLocation = reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(SDL_GL_GetProcAddress("glGetAttribLocation")))
		&& (getProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(SDL_GL_GetProcAddress("glGetProgramInfoLog")))
		&& (getProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(SDL_GL_GetProcAddress("glGetProgramiv")))
		&& (getShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(SDL_GL_GetProcAddress("glGetShaderInfoLog")))
		&& (getShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(SDL_GL_GetProcAddress("glGetShaderiv")))
		&& (getUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(SDL_GL_GetProcAddress("glGetUniformLocation")))
		&& (linkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(SDL_GL_GetProcAddress("glLinkProgram")))
		&& (readBuffer = reinterpret_cast<decltype(readBuffer)>(SDL_GL_GetProcAddress("glReadBuffer")))
		&& (shaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(SDL_GL_GetProcAddress("glShaderSource")))
		&& (uniform1f = reinterpret_cast<PFNGLUNIFORM1FPROC>(SDL_GL_GetProcAddress("glUniform1f")))
		&& (uniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(SDL_GL_GetProcAddress("glUniform1i")))
		&& (uniform1ui = reinterpret_cast<PFNGLUNIFORM1UIPROC>(SDL_GL_GetProcAddress("glUniform1ui")))
		&& (uniform4f = reinterpret_cast<PFNGLUNIFORM4FPROC>(SDL_GL_GetProcAddress("glUniform4f")))
		&& (uniform4fv = reinterpret_cast<PFNGLUNIFORM4FVPROC>(SDL_GL_GetProcAddress("glUniform4fv")))
		&& (uniform4iv = reinterpret_cast<PFNGLUNIFORM4IVPROC>(SDL_GL_GetProcAddress("glUniform4iv")))
		&& (useProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(SDL_GL_GetProcAddress("glUseProgram")))
		&& (vertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(SDL_GL_GetProcAddress("glVertexAttribPointer")))
	))
		throw std::runtime_error("Failed to find OpenGL extension functions");
}

// RENDERER GL

RendererGl::SurfaceInfo::SurfaceInfo(SDL_Surface* surface, uint16 internal, uint16 format, uint16 texel, Swizzle components) noexcept :
	img(surface),
	swizzle(components),
	ifmt(internal),
	pfmt(format),
	type(texel)
{
	for (align = 8; align > 1 && (uintptr_t(img->pixels) % align || uint(img->pitch) % align); align /= 2);
}

RendererGl::RendererGl(size_t numViews, bool modern) :
	Renderer(numViews, UINT_MAX),
	canSwizzle(modern)
{
	int profile;
	if (sdlFailed(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile)))
		throw std::runtime_error(SDL_GetError());
	core = profile != SDL_GL_CONTEXT_PROFILE_ES;
	canTextureCompression = core;
}

void RendererGl::setContext(View* view) {
	if (!trySetContext(view))
		throw std::runtime_error(SDL_GetError());
}

bool RendererGl::trySetContext(View* view) noexcept {
	auto vw = static_cast<ViewGl*>(view);
#ifdef _WIN32
	cvw = vw;
#endif
	return sdlSucceeded(SDL_GL_MakeCurrent(vw->win, vw->ctx));
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

void RendererGl::initGlCommon(ViewGl* view, bool vsync, uintptr_t& availableMemory) noexcept {
	gfget
	setSwapInterval(vsync);

	GLint gval[4];
	if (gl.getIntegerv(GL_MAX_TEXTURE_SIZE, gval); uint(gval[0]) < maxTextureSize)
		maxTextureSize = gval[0];
	if (SDL_GL_ExtensionSupported("GL_NVX_gpu_memory_info")) {	// no version
		if (gl.getIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, gval); uint(gval[0]) > availableMemory)
			availableMemory = gval[0];
	} else if (SDL_GL_ExtensionSupported("GL_ATI_meminfo"))		// no version
		if (gl.getIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, gval); uint(gval[0]) > availableMemory)
			availableMemory = gval[0];

#ifndef NDEBUG
	if (bool khr = SDL_GL_ExtensionSupported("GL_KHR_debug"); khr || SDL_GL_ExtensionSupported("GL_ARB_debug_output")) {	// since 4.3 or 3.2 ES
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
	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

template <class F>
void RendererGl::finalizeConstruction(Settings* sets, Texture*& tooltip, uintptr_t availableMemory, F finGl) {
	for (View* it : views) {
		setContext(it);
		finGl();
	}

	static_cast<TextureGl*>(tooltip = new TextureGl(uvec2(0)))->id = initTexture(GL_NEAREST);
	if (canSwizzle)
		setSwizzle(GL_ONE, GL_ONE, GL_ONE, GL_RED);

	setCompression(sets);
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

void RendererGl::setSwapInterval(bool vsync) noexcept {
	if (!vsync || (SDL_GL_SetSwapInterval(-1) && SDL_GL_SetSwapInterval(1)))
		SDL_GL_SetSwapInterval(0);
}

Texture* RendererGl::texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, maxTextureSize), rpic); si.img) {
		gfget
		try {
			auto tex = new TextureGl(uvec2(si.img->w, si.img->h));
			tex->id = initTexture(linear ? GL_LINEAR : GL_NEAREST);
			if (si.swizzle.r)
				setSwizzle(si.swizzle.r, si.swizzle.g, si.swizzle.b, si.swizzle.a);
			uploadTexture(tex, si);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	return nullptr;
}

bool RendererGl::texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, maxTextureSize), rpic); si.img) {
		gfget
		auto gtx = static_cast<TextureGl*>(tex);
		gtx->res = uvec2(si.img->w, si.img->h);
		gl.bindTexture(texType, gtx->id);
		uploadTexture(gtx, si);
		return true;
	}
	return false;
}

Texture* RendererGl::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x) {
		gfget
		try {
			auto tex = new TextureGl(glm::min(pm.res, uvec2(maxTextureSize)));
			tex->id = initTexture(GL_NEAREST);
			if (canSwizzle)
				setSwizzle(GL_ONE, GL_ONE, GL_ONE, GL_RED);
			uploadTexture(tex, pm);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
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

void RendererGl::setSwizzle(GLint red, GLint green, GLint blue, GLint alpha) noexcept {
	gfget
	gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_R, red);
	gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_G, green);
	gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_B, blue);
	gl.texParameteri(texType, GL_TEXTURE_SWIZZLE_A, alpha);
}

void RendererGl::uploadTexture(TextureGl* tex, SurfaceInfo& si) noexcept {
	gfget
	gl.pixelStorei(GL_UNPACK_ROW_LENGTH, uint(si.img->pitch) / surfaceBytesPpx(si.img));
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, si.align);
	gl.texImage2D(texType, 0, si.ifmt, tex->res.x, tex->res.y, 0, si.pfmt, si.type, si.img->pixels);
}

void RendererGl::uploadTexture(TextureGl* tex, const Pixmap& pm) noexcept {
	gfget
	if (canSwizzle) {
		gl.pixelStorei(GL_UNPACK_ROW_LENGTH, pm.res.x);
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(uint8));
		gl.texImage2D(texType, 0, GL_R8, tex->res.x, tex->res.y, 0, GL_RED, GL_UNSIGNED_BYTE, pm.pix.get());
	} else {
		gl.pixelStorei(GL_UNPACK_ROW_LENGTH,  tex->res.x);
		gl.pixelStorei(GL_UNPACK_ALIGNMENT, sizeof(uint32));
		gl.texImage2D(texType, 0, GL_RGBA8, tex->res.x, tex->res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, textBuffer.fromText(pm, tex->res));
	}
}

RendererGl::SurfaceInfo RendererGl::pickPixFormat(SDL_Surface* img, bool rpic) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (surfaceFormat(img)) {
	case SDL_PIXELFORMAT_ABGR8888:
		return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	case SDL_PIXELFORMAT_ARGB8888:
		return canBgra	// if canBgra is false then we're in ES which means we can swizzle
			? SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE)
			: SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_BLUE, GL_GREEN, GL_RED, GL_ONE });
	case SDL_PIXELFORMAT_BGRA8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_GREEN, GL_BLUE, GL_ALPHA, GL_RED });
		break;
	case SDL_PIXELFORMAT_RGBA8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_ALPHA, GL_BLUE, GL_GREEN, GL_RED });
		break;
	case SDL_PIXELFORMAT_XBGR8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_RED, GL_GREEN, GL_BLUE, GL_ONE });
		break;
	case SDL_PIXELFORMAT_XRGB8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_BLUE, GL_GREEN, GL_RED, GL_ONE });
		break;
	case SDL_PIXELFORMAT_BGRX8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_GREEN, GL_BLUE, GL_ALPHA, GL_ONE });
		break;
	case SDL_PIXELFORMAT_RGBX8888:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, { GL_ALPHA, GL_BLUE, GL_GREEN, GL_ONE });
		break;
	case SDL_PIXELFORMAT_RGB24:
		return SurfaceInfo(img, rpic ? iformRgb8 : GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
	case SDL_PIXELFORMAT_BGR24:
		return canBgra
			? SurfaceInfo(img, rpic ? iformRgb8 : GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE)
			: SurfaceInfo(img, rpic ? iformRgb8 : GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, { GL_BLUE, GL_GREEN, GL_RED, GL_ONE });
#if SDL_VERSION_ATLEAST(3, 2, 0)
	case SDL_PIXELFORMAT_ABGR2101010:
		return SurfaceInfo(img, rpic ? iformRgba10 : GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
#endif
	case SDL_PIXELFORMAT_ARGB2101010:
		return core	// if we're not in core GL then swizzle willl always be available (GL_BGRA doesn't work with GL_UNSIGNED_INT_2_10_10_10_REV in ES)
			? SurfaceInfo(img, rpic ? iformRgba10 : GL_RGB10_A2, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV)
			: SurfaceInfo(img, rpic ? iformRgba10 : GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA });
#if SDL_VERSION_ATLEAST(3, 2, 0)
	case SDL_PIXELFORMAT_XBGR2101010:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba10 : GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, { GL_RED, GL_GREEN, GL_BLUE, GL_ONE });
		break;
	case SDL_PIXELFORMAT_XRGB2101010:
		if (canSwizzle)
			return SurfaceInfo(img, rpic ? iformRgba10 : GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, { GL_BLUE, GL_GREEN, GL_RED, GL_ONE });
		break;
#endif
	case SDL_PIXELFORMAT_BGR565:
		return core
			? SurfaceInfo(img, GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV)
			: SurfaceInfo(img, GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, { GL_BLUE, GL_GREEN, GL_RED, GL_ONE });
	case SDL_PIXELFORMAT_RGB565:
		return SurfaceInfo(img, core ? GL_RGB5 : GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
	case SDL_PIXELFORMAT_ABGR1555:
		if (core)
			return SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		break;
	case SDL_PIXELFORMAT_ARGB1555:
		if (core)
			return SurfaceInfo(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		break;
	case SDL_PIXELFORMAT_BGRA5551:
		return core
			? SurfaceInfo(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1)
			: SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA });
	case SDL_PIXELFORMAT_RGBA5551:
		return SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
	case SDL_PIXELFORMAT_XBGR1555:
		if (core && canSwizzle)
			return SurfaceInfo(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, { GL_RED, GL_GREEN, GL_BLUE, GL_ONE });
		break;
	case SDL_PIXELFORMAT_XRGB1555:
		if (core && canSwizzle)
			return SurfaceInfo(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, { GL_RED, GL_GREEN, GL_BLUE, GL_ONE });
		break;
	case SDL_PIXELFORMAT_ABGR4444:
		return core
			? SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV)
			: SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, { GL_ALPHA, GL_BLUE, GL_GREEN, GL_RED });
	case SDL_PIXELFORMAT_ARGB4444:
		return core
			? SurfaceInfo(img, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV)
			: SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, { GL_GREEN, GL_BLUE, GL_ALPHA, GL_RED });
	case SDL_PIXELFORMAT_BGRA4444:
		return core
			? SurfaceInfo(img, GL_RGBA4, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4)
			: SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA });
	case SDL_PIXELFORMAT_RGBA4444:
		return SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
	case SDL_PIXELFORMAT_XBGR4444:
		if (canSwizzle)
			return SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, { GL_ALPHA, GL_BLUE, GL_GREEN, GL_ONE });
		break;
	case SDL_PIXELFORMAT_XRGB4444:
		if (canSwizzle)
			return SurfaceInfo(img, GL_RGBA4, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, { GL_GREEN, GL_BLUE, GL_ALPHA, GL_ONE });
		break;
	case SDL_PIXELFORMAT_RGB332:
		if (core)
			return SurfaceInfo(img, GL_R3_G3_B2, GL_RGB, GL_UNSIGNED_BYTE_3_3_2);	// the driver will turn this into some other format
		break;
	case SDL_PIXELFORMAT_INDEX8:
		if (canSwizzle && !usesSrgb && isIndexedGrayscale(img))
			return SurfaceInfo(img, GL_R8, GL_RED, GL_UNSIGNED_BYTE, { GL_RED, GL_RED, GL_RED, GL_ONE });
	}
	return SurfaceInfo(convertReplace(img), rpic ? iformRgba8 : GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
}

pair<SDL_PixelFormatEnum, uint8> RendererGl::prepareImageFormat(SDL_Surface* img) const noexcept {
	SDL_PixelFormatEnum fmt = surfaceFormat(img);
	switch (fmt) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	case SDL_PIXELFORMAT_XBGR2101010: case SDL_PIXELFORMAT_XRGB2101010:
		return pair(canSwizzle ? fmt : SDL_PIXELFORMAT_ABGR2101010, internalBytesPpx());
#endif
	case SDL_PIXELFORMAT_BGR565: case SDL_PIXELFORMAT_RGB565: case SDL_PIXELFORMAT_BGRA5551: case SDL_PIXELFORMAT_RGBA5551: case SDL_PIXELFORMAT_ABGR4444: case SDL_PIXELFORMAT_ARGB4444: case SDL_PIXELFORMAT_BGRA4444: case SDL_PIXELFORMAT_RGBA4444:
		return pair(fmt, 2);
	case SDL_PIXELFORMAT_ABGR1555: case SDL_PIXELFORMAT_ARGB1555:
		return pair(core ? fmt : SDL_PIXELFORMAT_RGBA5551, 2);
	case SDL_PIXELFORMAT_XBGR1555: case SDL_PIXELFORMAT_XRGB1555:
		return pair(core && canSwizzle ? fmt : SDL_PIXELFORMAT_RGBA5551, 2);
	case SDL_PIXELFORMAT_XBGR4444: case SDL_PIXELFORMAT_XRGB4444:
		return pair(canSwizzle ? fmt : SDL_PIXELFORMAT_RGBA4444, 2);
	case SDL_PIXELFORMAT_RGB332:
		return pair(core ? SDL_PIXELFORMAT_RGB332 : SDL_PIXELFORMAT_RGB565, 2);
	case SDL_PIXELFORMAT_INDEX8:
		return pair(SDL_PIXELFORMAT_INDEX8, canSwizzle && !usesSrgb && isIndexedGrayscale(img) ? 1 : internalBytesPpx());
#if SDL_VERSION_ATLEAST(3, 2, 0)
	default:
		if (SDL_BYTESPERPIXEL(fmt) > 4)
			return pair(SDL_PIXELFORMAT_ABGR2101010, internalBytesPpx());
#endif
	}
	return pair(fmt, internalBytesPpx());
}

void RendererGl::setCompression(Settings* sets) noexcept {
	switch (sets->compression) {
	using enum Settings::Compression;
	case b16:
		iformRgba8 = iformRgba10 = GL_RGB5_A1;
		iformRgb8 = core ? GL_RGB5 : GL_RGB565;
		break;
	case compress:
		if (canTextureCompression) {
			iformRgba8 = usesSrgb ? GL_COMPRESSED_SRGB_ALPHA : GL_COMPRESSED_RGBA;
			iformRgb8 = usesSrgb ? GL_COMPRESSED_SRGB : GL_COMPRESSED_RGB;
			iformRgba10 = iformRgba8;
			break;
		}
		sets->compression = none;
	default:
		iformRgba8 = usesSrgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		iformRgb8 = usesSrgb ? GL_SRGB8 : GL_RGB8;
		iformRgba10 = GL_RGB10_A2;
	}
	compression = sets->compression;
}

// RENDERER GL 1

#if !defined(__arm__) && !defined(__aarch64__)
RendererGl1::ViewGl1::ViewGl1(SDL_Window* window, const Recti& area) noexcept :
	ViewGl(window, area),
	proj(glm::ortho(float(area.x), float(area.x + area.w), float(area.y + area.h), float(area.y)))
{}

RendererGl1::RendererGl1(InitParams& initParams, Settings* sets) :
	RendererGl(initParams.windows.size(), false)
{
#ifndef _WIN32
	gl.initFunctions();
	gl1.initFunctions();
#endif
	try {
		bool canTexRect = true;
		uintptr_t availableMemory = 0;
		initContexts<ViewGl1>(initParams.windows, initParams.vofs, initParams.viewRes, [this, sets, &canTexRect, &availableMemory](ViewGl1* vw) { initGl(vw, sets->vsync, canTexRect, availableMemory); });
		std::copy(initParams.colors.begin(), initParams.colors.end() - 1, rectColors.begin());
		finalizeConstruction(sets, initParams.tooltipTexture, availableMemory, [this, &initParams]() {
			const vec4& bgclr = initParams.colors[eint(Color::background)];
			gf1get
			gl.enable(texType);
			gl1.clearColor(bgclr.r, bgclr.g, bgclr.b, bgclr.a);
		});
	} catch (const std::exception&) {
		freeTexture(initParams.tooltipTexture);
		cleanup();
		throw;
	}
}

RendererGl1::~RendererGl1() {
	cleanup();
}

void RendererGl1::initGl(ViewGl1* view, bool vsync, bool& canTexRect, uintptr_t& availableMemory) {
#ifdef _WIN32
	gf1get
	gl.initFunctions();
	gl1.initFunctions();
#endif
	canTexRect = canTexRect && (SDL_GL_ExtensionSupported("GL_ARB_texture_rectangle") || SDL_GL_ExtensionSupported("GL_NV_texture_rectangle"));	// since 3.1
	switch (texType) {
	case GL_TEXTURE_2D:
		if (SDL_GL_ExtensionSupported("GL_ARB_texture_non_power_of_two"))	// since 2.0
			break;
		texType = GL_TEXTURE_RECTANGLE;
	default:
		if (!canTexRect)
			throw std::runtime_error("No support for non power of two textures");
	}
	canTextureCompression = canTextureCompression && SDL_GL_ExtensionSupported("GL_ARB_texture_compression");	// since 1.3
	initGlCommon(view, vsync, availableMemory);
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

void RendererGl1::setColors(array<vec4, Settings::defaultColors.size()>& colors) {
	const vec4& bgclr = colors[eint(Color::background)];
	for (View* it : views) {
		setContext(it);
		gf1get
		gl1.clearColor(bgclr.r, bgclr.g, bgclr.b, bgclr.a);
	}
	std::copy(colors.begin(), colors.end() - 1, rectColors.begin());
}

bool RendererGl1::setSettings(Settings* sets) {
	for (View* it : views) {
		setContext(it);
		setSwapInterval(sets->vsync);
	}
	setCompression(sets);
	return false;
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

Renderer::Action RendererGl1::startDraw(View* view) noexcept {
	if (!trySetContext(view)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
		return Action::skip;
	}
	gf1get
	gl1.clear(GL_COLOR_BUFFER_BIT);
	gl1.matrixMode(GL_PROJECTION);
	gl1.loadMatrixf(glm::value_ptr(static_cast<ViewGl1*>(view)->proj));
	return Action::yes;
}

void RendererGl1::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
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
		gl1.color4fv(glm::value_ptr(rectColors[eint(color)]));
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
	}
}

template <Number T>
void RendererGl1::setPosScale(mat4& matrix, const Rect<T>& rect) noexcept {
	matrix[0][0] = rect.w;
	matrix[1][1] = rect.h;
	matrix[3][0] = rect.x;
	matrix[3][1] = rect.y;
}

Renderer::Action RendererGl1::finishDraw(View* view) noexcept {
	SDL_GL_SwapWindow(static_cast<ViewGl*>(view)->win);
	return Action::yes;
}

Renderer::Info RendererGl1::getInfo() const noexcept {
	Info info = {
		.compressions = { Settings::Compression::none, Settings::Compression::b16 },
		.texSize = maxTextureSize,
		.curCompression = compression
	};
	if (canTextureCompression)
		info.compressions.push_back(Settings::Compression::compress);
	return info;
}
#endif

// RENDERER GL 3

RendererGl3::RendererGl3(InitParams& initParams, Settings* sets) :
	RendererGl(initParams.windows.size(), true)
{
#ifndef _WIN32
	gl.initFunctions();
	gl3.initFunctions();
#endif
	try {
		uintptr_t availableMemory = 0;
		initContexts<ViewGl3>(initParams.windows, initParams.vofs, initParams.viewRes, [this, sets, &availableMemory](ViewGl3* vw) { initGl(vw, sets->vsync, availableMemory); });
		setUsesSrgb(sets);
		initShaders(sets);
		setColors(initParams.colors);
		finalizeConstruction(sets, initParams.tooltipTexture, availableMemory, [this]() {
			gfget
			if (usesSrgb)
				gl.enable(GL_FRAMEBUFFER_SRGB);
		});
	} catch (const std::exception&) {
		freeTexture(initParams.tooltipTexture);
		cleanup();
		throw;
	}
}

RendererGl3::~RendererGl3() {
	cleanup();
}

void RendererGl3::initGl(ViewGl3* view, bool vsync, uintptr_t& availableMemory) {
#ifdef _WIN32
	gf3get
	gl.initFunctions();
	gl3.initFunctions();
#endif
	if (core)
		canSwizzle = canSwizzle && (SDL_GL_ExtensionSupported("GL_ARB_texture_swizzle") || SDL_GL_ExtensionSupported("GL_EXT_texture_swizzle"));	// since 3.3 or 3.0 ES
	else {
		canBgra = canBgra && SDL_GL_ExtensionSupported("GL_MESA_bgra");
		canSrgb = canSrgb && SDL_GL_ExtensionSupported("GL_EXT_sRGB_write_control");
	}
	int gval;
	hasSrgb = hasSrgb && canSrgb && SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &gval) && gval;
	initGlCommon(view, vsync, availableMemory);
}

void RendererGl3::cleanup() noexcept {
	gf3get
	if (gl3.functionsInitialized()) {
		GLuint vbos[2] = { vboGui, vboFin };
		gl3.deleteBuffers(std::size(vbos), vbos);
		gl3.deleteProgram(progGui);
		gl3.deleteProgram(progFin);
	}
	for (View* it : views)
		if (auto vw = static_cast<ViewGl3*>(it)) {
			if (trySetContext(vw)) {
				gf3set
				if (gl3.functionsInitialized()) {
					GLuint vaos[2] = { vw->vaoGui, vw->vaoFin };
					gl3.deleteVertexArrays(std::size(vaos), vaos);
					gl.deleteTextures(1, &vw->tex);
					gl3.deleteFramebuffers(1, &vw->fbo);
				}
			}
			SDL_GL_DeleteContext(vw->ctx);
			delete vw;
		}
}

void RendererGl3::initShaders(Settings* sets) {
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
	progGui = createShader(vertSrc, fragSrc);
	GLuint attrVposGui = gl3.getAttribLocation(progGui, ATTR_GUI_VPOS);
	uniPviewGui = gl3.getUniformLocation(progGui, UNI_GUI_PVIEW);
	uniRectGui = gl3.getUniformLocation(progGui, UNI_GUI_RECT);
	uniFrameGui = gl3.getUniformLocation(progGui, UNI_GUI_FRAME);
	uniColorsGui = gl3.getUniformLocation(progGui, UNI_GUI_COLORS);
	uniColorIdGui = gl3.getUniformLocation(progGui, UNI_GUI_COLORID);
	gl3.uniform1i(gl3.getUniformLocation(progGui, UNI_GUI_COLORMAP), 0);

	gl3.genBuffers(1, &vboGui);
	gl3.bindBuffer(GL_ARRAY_BUFFER, vboGui);
	gl3.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

	GLint attrVposFin, attrVtuvFin;
	if (sets->gammaType == Settings::Gamma::value)
		std::tie(attrVposFin, attrVtuvFin) = createFinShader(sets);

	bool rollbackPp = false;
	for (View* it : views) {
		auto vw = static_cast<ViewGl3*>(it);
		setContext(vw);
		gf3set

		gl3.genVertexArrays(1, &vw->vaoGui);
		gl3.bindVertexArray(vw->vaoGui);
		gl3.bindBuffer(GL_ARRAY_BUFFER, vboGui);
		gl3.enableVertexAttribArray(attrVposGui);
		gl3.vertexAttribPointer(attrVposGui, vec2::length(), GL_FLOAT, GL_FALSE, 0, nullptr);

		if (progFin)
			rollbackPp = !createFinData(vw, attrVposFin, attrVtuvFin);
	}
	if (rollbackPp)
		rollbackFinData(sets->gammaType);
}

pair<GLint, GLint> RendererGl3::createFinShader(Settings* sets) noexcept {
	gf3get
	const char* vertSrc =
#ifdef NDEBUG
#include "shaders/glFin.vert.rel.h"
#else
#include "shaders/glFin.vert.dbg.h"
#endif
	;
	const char* fragSrc =
#ifdef NDEBUG
#include "shaders/glFin.frag.rel.h"
#else
#include "shaders/glFin.frag.dbg.h"
#endif
	;
	try {
		progFin = createShader(vertSrc, fragSrc);
		uniGammaFin = gl3.getUniformLocation(progFin, UNI_FIN_GAMMA);
		gl3.uniform1i(gl3.getUniformLocation(progFin, UNI_FIN_SCENEMAP), 0);
		setGammaValue(sets->gammaValue);

		gl3.genBuffers(1, &vboFin);
		gl3.bindBuffer(GL_ARRAY_BUFFER, vboFin);
		gl3.bufferData(GL_ARRAY_BUFFER, sizeof(scrVertices), scrVertices.data(), GL_STATIC_DRAW);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		freeFinShader();
		sets->gammaType = Settings::Gamma::none;
		return pair(-1, -1);
	}
	return pair(gl3.getAttribLocation(progFin, ATTR_FIN_VPOS), gl3.getAttribLocation(progFin, ATTR_FIN_VTUV));
}

bool RendererGl3::createFinData(ViewGl3* view, GLint attrVpos, GLint attrVtuv) noexcept {
	gf3get
	try {
		gl3.genVertexArrays(1, &view->vaoFin);
		gl3.bindVertexArray(view->vaoFin);
		gl3.bindBuffer(GL_ARRAY_BUFFER, vboFin);
		gl3.enableVertexAttribArray(attrVpos);
		gl3.vertexAttribPointer(attrVpos, vec2::length(), GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), std::bit_cast<void*>(offsetof(ScreenVertex, pos)));
		gl3.enableVertexAttribArray(attrVtuv);
		gl3.vertexAttribPointer(attrVtuv, vec2::length(), GL_FLOAT, GL_FALSE, sizeof(ScreenVertex), std::bit_cast<void*>(offsetof(ScreenVertex, tuv)));

		gl3.genFramebuffers(1, &view->fbo);
		gl.genTextures(1, &view->tex);
		initFinFramebuffer(view);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		freeFinShader();
		return false;
	}
	return true;
}

void RendererGl3::initFinFramebuffer(ViewGl3* view) {
	gf3get
	gl3.bindFramebuffer(GL_FRAMEBUFFER, view->fbo);
	gl.bindTexture(GL_TEXTURE_2D, view->tex);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, view->rect.w, view->rect.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	gl3.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, view->tex, 0);
	gl3.readBuffer(GL_NONE);
	checkFramebufferStatus();
}

void RendererGl3::freeFinShader() noexcept {
	gf3get
	gl3.deleteBuffers(1, &vboFin);
	gl3.deleteProgram(progFin);
	progFin = vboFin = 0;
}

void RendererGl3::rollbackFinData(Settings::Gamma& gamma) noexcept {
	for (View* it : views) {
		auto vw = static_cast<ViewGl3*>(it);
		if (trySetContext(vw))
			freeFinData(vw);
	}
	gamma = Settings::Gamma::none;
}

void RendererGl3::freeFinData(ViewGl3* view) noexcept {
	gf3get
	gl3.deleteVertexArrays(1, &view->vaoFin);
	gl.deleteTextures(1, &view->tex);
	gl3.deleteFramebuffers(1, &view->fbo);
	view->vaoFin = view->fbo = view->tex = 0;
}

GLuint RendererGl3::createShader(const char* vertSrc, const char* fragSrc) const {
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
	checkStatus(vert, GL_COMPILE_STATUS, gl3.getShaderiv, gl3.getShaderInfoLog, "vertex shader");

	GLuint frag = gl3.createShader(GL_FRAGMENT_SHADER);
	gl3.shaderSource(frag, 1, &fragSrc, nullptr);
	gl3.compileShader(frag);
	checkStatus(frag, GL_COMPILE_STATUS, gl3.getShaderiv, gl3.getShaderInfoLog, "fragment shader");

	GLuint sprog = gl3.createProgram();
	gl3.attachShader(sprog, vert);
	gl3.attachShader(sprog, frag);
	gl3.linkProgram(sprog);
	gl3.detachShader(sprog, vert);
	gl3.detachShader(sprog, frag);
	gl3.deleteShader(vert);
	gl3.deleteShader(frag);
	checkStatus(sprog, GL_LINK_STATUS, gl3.getProgramiv, gl3.getProgramInfoLog, "shader program");
	gl3.useProgram(sprog);
	return sprog;
}

void RendererGl3::checkStatus(GLuint id, GLenum stat, PFNGLGETSHADERIVPROC check, PFNGLGETSHADERINFOLOGPROC info, const char* name) {
	int len, res;
	string msg;
	if (check(id, GL_INFO_LOG_LENGTH, &len); len > 1) {
		msg.resize(--len);
		if (info(id, len, &res, msg.data()); res < len)
			msg.resize(res);
		msg = trim(msg);
	}
	if (check(id, stat, &res); res == GL_FALSE)
		throw std::runtime_error(!msg.empty() ? fmt::format("{}:" LINEND "{}", name, msg) : fmt::format("{}: unknown error", name));
	if (!msg.empty())
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:" LINEND "%s", name, msg.data());
}

void RendererGl3::checkFramebufferStatus() {
	gf3get
	switch (GLenum rc = gl3.checkFramebufferStatus(GL_FRAMEBUFFER)) {
	case GL_FRAMEBUFFER_UNDEFINED:
		throw std::runtime_error("GL_FRAMEBUFFER_UNDEFINED");
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
	case GL_FRAMEBUFFER_UNSUPPORTED:
		throw std::runtime_error("GL_FRAMEBUFFER_UNSUPPORTED");
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		throw std::runtime_error("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
	default:
		if (rc != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error("unknown framebuffer error");
	}
}

void RendererGl3::setColors(array<vec4, Settings::defaultColors.size()>& colors) {
	gf3get
	convertColors(colors.data(), colors.size(), usesSrgb, progFin);
	bgColor = colors[eint(Color::background)];
	gl3.useProgram(progGui);
	gl3.uniform4fv(uniColorsGui, colors.size() - 1, glm::value_ptr(colors[0]));
}

bool RendererGl3::setSettings(Settings* sets) {
	GLint attrVposFin, attrVtuvFin;
	bool prevSrgb = usesSrgb;
	setUsesSrgb(sets);
	bool reloadSrgb = usesSrgb != prevSrgb;
	bool reloadGamma = bool(progFin) != (sets->gammaType == Settings::Gamma::value);
	if (reloadGamma) {
		if (!progFin)
			std::tie(attrVposFin, attrVtuvFin) = createFinShader(sets);
		else
			freeFinShader();
	}

	bool rollbackPp = false;
	for (View* it : views) {
		auto vw = static_cast<ViewGl3*>(it);
		setContext(it);
		setSwapInterval(sets->vsync);

		gf3get
		if (reloadSrgb) {
			if (usesSrgb)
				gl.enable(GL_FRAMEBUFFER_SRGB);
			else
				gl.disable(GL_FRAMEBUFFER_SRGB);
		}
		if (reloadGamma) {
			if (progFin)
				rollbackPp = !createFinData(vw, attrVposFin, attrVtuvFin);
			else
				freeFinData(vw);
		}
	}
	if (rollbackPp)
		rollbackFinData(sets->gammaType);
	setCompression(sets);
	return reloadSrgb || reloadGamma;
}

void RendererGl3::setUsesSrgb(Settings* sets) noexcept {
	if (usesSrgb = sets->gammaType == Settings::Gamma::srgb; usesSrgb)
		if (usesSrgb = hasSrgb; !usesSrgb)
			sets->gammaType = Settings::Gamma::none;
}

void RendererGl3::setGammaValue(int gamma) {
	gf3get
	gl3.useProgram(progFin);
	gl3.uniform1f(uniGammaFin, 10.f / float(gamma));
}

void RendererGl3::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		gfget
		SDL_GL_GetDrawableSize(views[0]->win, &viewRes.x, &viewRes.y);
		views[0]->rect.size() = viewRes;
		gl.viewport(0, 0, viewRes.x, viewRes.y);
		if (progFin)
			initFinFramebuffer(static_cast<ViewGl3*>(views[0]));
	}
}

Renderer::Action RendererGl3::startDraw(View* view) noexcept {
	auto vw = static_cast<ViewGl3*>(view);
	if (!trySetContext(vw)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
		return Action::skip;
	}
	gf3get
	gl3.bindVertexArray(vw->vaoGui);
	gl3.bindFramebuffer(GL_FRAMEBUFFER, vw->fbo);
	gl3.useProgram(progGui);
	gl3.uniform4f(uniPviewGui, float(vw->rect.x), float(vw->rect.y), float(vw->rect.w) / 2.f, float(vw->rect.h) / 2.f);
	gl3.clearBufferfv(GL_COLOR, 0, glm::value_ptr(bgColor));
	return Action::yes;
}

void RendererGl3::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
	gf3get
	gl.bindTexture(GL_TEXTURE_2D, static_cast<const TextureGl*>(tex)->id);
	gl3.uniform4iv(uniRectGui, 1, reinterpret_cast<const int*>(&rect));
	gl3.uniform4iv(uniFrameGui, 1, reinterpret_cast<const int*>(&frame));
	gl3.uniform1ui(uniColorIdGui, eint(color));
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());
}

Renderer::Action RendererGl3::finishDraw(View* view) noexcept {
	auto vw = static_cast<ViewGl3*>(view);
	if (progFin) {
		vec4 clrClr(0.f, 0.f, 0.f, 1.f);
		gf3get
		gl3.bindVertexArray(vw->vaoFin);
		gl3.bindFramebuffer(GL_FRAMEBUFFER, 0);
		gl3.useProgram(progFin);
		gl.disable(GL_BLEND);
		gl3.clearBufferfv(GL_COLOR, 0, glm::value_ptr(clrClr));
		gl.bindTexture(GL_TEXTURE_2D, vw->tex);
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, scrVertices.size());
		gl.enable(GL_BLEND);
	}
	SDL_GL_SwapWindow(vw->win);
	return Action::yes;
}

Renderer::Info RendererGl3::getInfo() const noexcept {
	Info info = {
		.gamma = { Settings::Gamma::none, Settings::Gamma::value },
		.compressions = { Settings::Compression::none, Settings::Compression::b16 },
		.texSize = maxTextureSize,
		.srgbNeedsWindowRecreate = !hasSrgb,
		.curGamma = usesSrgb ? Settings::Gamma::srgb : progFin ? Settings::Gamma::value : Settings::Gamma::none,
		.curCompression = compression
	};
	if (canSrgb)
		info.gamma.insert(info.gamma.begin() + 1, Settings::Gamma::srgb);
	if (canTextureCompression)
		info.compressions.push_back(Settings::Compression::compress);
	return info;
}
#endif
