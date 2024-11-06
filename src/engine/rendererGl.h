#pragma once

#ifdef WITH_OPENGL
#include "renderer.h"
#include <SDL_opengl.h>
#include <glm/mat4x4.hpp>

struct FunctionsGl {
	decltype(glBindTexture)* bindTexture;
	decltype(glBlendFunc)* blendFunc;
	decltype(glDeleteTextures)* deleteTextures;
	decltype(glDisable)* disable;
	decltype(glDrawArrays)* drawArrays;
	decltype(glEnable)* enable;
	decltype(glFrontFace)* frontFace;
	decltype(glGenTextures)* genTextures;
	decltype(glGetIntegerv)* getIntegerv;
	decltype(glPixelStorei)* pixelStorei;
	decltype(glTexImage2D)* texImage2D;
	decltype(glTexParameteri)* texParameteri;
	decltype(glViewport)* viewport;

	void initFunctions();
};

#if !defined(__arm__) && !defined(__aarch64__)
struct FunctionsGl1 {
	decltype(glClear)* clear;
	decltype(glClearColor)* clearColor;
	decltype(glColor4fv)* color4fv;
	decltype(glEnableClientState)* enableClientState;
	decltype(glLoadMatrixf)* loadMatrixf;
	decltype(glMatrixMode)* matrixMode;
	decltype(glTexCoordPointer)* texCoordPointer;
	decltype(glVertexPointer)* vertexPointer;

	void initFunctions();
};
#endif

struct FunctionsGl3 {
	PFNGLATTACHSHADERPROC attachShader;
	PFNGLBINDBUFFERPROC bindBuffer;
	PFNGLBINDFRAMEBUFFERPROC bindFramebuffer;
	PFNGLBINDVERTEXARRAYPROC bindVertexArray;
	PFNGLBUFFERDATAPROC bufferData;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebufferStatus;
	PFNGLCLEARBUFFERFVPROC clearBufferfv;
	PFNGLCOMPILESHADERPROC compileShader;
	PFNGLCREATEPROGRAMPROC createProgram;
	PFNGLCREATESHADERPROC createShader;
	PFNGLDELETEBUFFERSPROC deleteBuffers;
	PFNGLDELETEFRAMEBUFFERSPROC deleteFramebuffers;
	PFNGLDELETESHADERPROC deleteShader;
	PFNGLDELETEPROGRAMPROC deleteProgram;
	PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays;
	PFNGLDETACHSHADERPROC detachShader;
	PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray;
	PFNGLFRAMEBUFFERTEXTURE2DPROC framebufferTexture2D;
	PFNGLGENBUFFERSPROC genBuffers;
	PFNGLGENFRAMEBUFFERSPROC genFramebuffers;
	PFNGLGENVERTEXARRAYSPROC genVertexArrays;
	PFNGLGETATTRIBLOCATIONPROC getAttribLocation;
	PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog;
	PFNGLGETPROGRAMIVPROC getProgramiv;
	PFNGLGETSHADERINFOLOGPROC getShaderInfoLog;
	PFNGLGETSHADERIVPROC getShaderiv;
	PFNGLGETUNIFORMLOCATIONPROC getUniformLocation;
	PFNGLLINKPROGRAMPROC linkProgram;
	decltype(glReadBuffer)* readBuffer;
	PFNGLSHADERSOURCEPROC shaderSource;
	PFNGLUNIFORM1FPROC uniform1f;
	PFNGLUNIFORM1IPROC uniform1i;
	PFNGLUNIFORM1UIPROC uniform1ui;
	PFNGLUNIFORM4FPROC uniform4f;
	PFNGLUNIFORM4FVPROC uniform4fv;
	PFNGLUNIFORM4IVPROC uniform4iv;
	PFNGLUSEPROGRAMPROC useProgram;
	PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer = nullptr;

	void initFunctions();
	bool functionsInitialized() const noexcept { return vertexAttribPointer; }
};

class RendererGl : public Renderer {
protected:
	class TextureGl : public Texture {
	private:
		GLuint id;

		using Texture::Texture;

		friend class RendererGl;
		friend class RendererGl1;
		friend class RendererGl3;
	};

	struct ViewGl : View {
		SDL_GLContext ctx = nullptr;
#ifdef _WIN32
		FunctionsGl gl;
#endif

		using View::View;
	};

private:
	struct Swizzle {
		uint16 r, g, b, a;
	};

	struct SurfaceInfo {
		uptr<SDL_Surface> img;
		Swizzle swizzle;
		uint16 ifmt;
		uint16 pfmt;
		uint16 type;
		uint8 align;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, uint16 internal, uint16 format, uint16 texel, Swizzle components = {}) noexcept;
	};

protected:
	uint16 iformRgba8, iformRgb8, iformRgba10;
	uint16 texType = GL_TEXTURE_2D;
	bool core;
	bool canBgra = true;
	bool canTextureCompression;
	bool canSwizzle;
	bool usesSrgb = false;	// whether sRGB is currently being used
#ifdef _WIN32
	ViewGl* cvw = nullptr;	// current context's view
#else
	FunctionsGl gl;
#endif
	PixmapColor textBuffer;

	RendererGl(size_t numViews, bool modern);

public:
	Texture* texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept override;
	bool texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;

protected:
#ifdef _WIN32
	void setContext(View* view);
	bool trySetContext(View* view) noexcept;
#else
	static void setContext(View* view);
	static bool trySetContext(View* view) noexcept;
#endif
	template <Class T, class F> void initContexts(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, F initGl);
	void initGlCommon(ViewGl* view, bool vsync, uintptr_t& availableMemory) noexcept;
	template <class F> void finalizeConstruction(Settings* sets, Texture*& tooltip, uintptr_t availableMemory, F finGl);
	static void setSwapInterval(bool vsync) noexcept;
	void setCompression(Settings* sets) noexcept;
	pair<SDL_PixelFormatEnum, uint8> prepareImageFormat(SDL_Surface* img) const noexcept override;
private:
	GLuint initTexture(GLint filter) noexcept;
	void setSwizzle(GLint red, GLint green, GLint blue, GLint alpha) noexcept;
	void uploadTexture(TextureGl* tex, SurfaceInfo& si) noexcept;
	void uploadTexture(TextureGl* tex, const Pixmap& pm) noexcept;
	SurfaceInfo pickPixFormat(SDL_Surface* img, bool rpic) const noexcept;
	uint8 internalBytesPpx() const noexcept;
#ifndef NDEBUG
	static void APIENTRY debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam) noexcept;
#endif
};

inline uint8 RendererGl::internalBytesPpx() const noexcept {
	return compression == Settings::Compression::b16 ? 2 : 4;
}

#if !defined(__arm__) && !defined(__aarch64__)
class RendererGl1 final : public RendererGl {
private:
	struct ViewGl1 : ViewGl {
#ifdef _WIN32
		FunctionsGl1 gl1;
#endif
		mat4 proj;

		ViewGl1(SDL_Window* window, const Recti& area) noexcept;
	};

#ifndef _WIN32
	FunctionsGl1 gl1;
#endif
	mat4 model = mat4(1.f);
	mat4 mtex = mat4(1.f);
	array<vec4, Settings::defaultColors.size() - 1> rectColors;

public:
	RendererGl1(InitParams& initParams, Settings* sets);
	~RendererGl1() override;

	void setColors(array<vec4, Settings::defaultColors.size()>& colors) override;
	bool setSettings(Settings* sets) override;
	void updateView(ivec2& viewRes) override;
	Info getInfo() const noexcept override;

	Action startDraw(View* view) noexcept override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept override;
	Action finishDraw(View* view) noexcept override;

private:
	void initGl(ViewGl1* view, bool vsync, bool& canTexRect, uintptr_t& availableMemory);
	void cleanup() noexcept;
	template <Number T> static void setPosScale(mat4& matrix, const Rect<T>& rect) noexcept;
};
#endif

class RendererGl3 final : public RendererGl {
private:
	struct ViewGl3 : ViewGl {
#ifdef _WIN32
		FunctionsGl3 gl3;
#endif
		GLuint vaoGui = 0, vaoFin = 0;
		GLuint fbo = 0, tex = 0;

		using ViewGl::ViewGl;
	};

	static constexpr array scrVertices = {
		ScreenVertex(vec2(-1.f, 1.f), vec2(0.f, 1.f)),
		ScreenVertex(vec2(1.f, 1.f), vec2(1.f, 1.f)),
		ScreenVertex(vec2(-1.f, -1.f), vec2(0.f, 0.f)),
		ScreenVertex(vec2(1.f, -1.f), vec2(1.f, 0.f))
	};

#ifndef _WIN32
	FunctionsGl3 gl3;
#endif
	GLint uniPviewGui, uniRectGui, uniFrameGui;
	GLint uniColorsGui, uniColorIdGui;
	GLint uniGammaFin;
	GLuint progGui = 0, progFin = 0;
	GLuint vboGui = 0, vboFin = 0;
	vec4 bgColor;
	bool canSrgb = true;	// whether the GL is capable of sRGB
	bool hasSrgb = true;	// whether the current framebuffers support sRGB

public:
	RendererGl3(InitParams& initParams, Settings* sets);
	~RendererGl3() override;

	void setColors(array<vec4, Settings::defaultColors.size()>& colors) override;
	bool setSettings(Settings* sets) override;
	void setGammaValue(int gamma) override;
	void updateView(ivec2& viewRes) override;
	Info getInfo() const noexcept override;

	Action startDraw(View* view) noexcept override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept override;
	Action finishDraw(View* view) noexcept override;

private:
	void initGl(ViewGl3* view, bool vsync, uintptr_t& availableMemory);
	void cleanup() noexcept;
	void initShaders(Settings* sets);
	pair<GLint, GLint> createFinShader(Settings* sets) noexcept;
	bool createFinData(ViewGl3* view, GLint attrVpos, GLint attrVtuv) noexcept;
	void initFinFramebuffer(ViewGl3* view);
	void freeFinShader() noexcept;
	void rollbackFinData(Settings::Gamma& gamma) noexcept;
	void freeFinData(ViewGl3* view) noexcept;
	GLuint createShader(const char* vertSrc, const char* fragSrc) const;
	static void checkStatus(GLuint id, GLenum stat, PFNGLGETSHADERIVPROC check, PFNGLGETSHADERINFOLOGPROC info, const char* name);
	void checkFramebufferStatus();
	void setUsesSrgb(Settings* sets) noexcept;
};
#endif
