#pragma once

#ifdef WITH_OPENGL
#include "renderer.h"
#include <glm/mat4x4.hpp>
#include <SDL2/SDL_opengl.h>

struct FunctionsGl {
	decltype(glBindTexture)* bindTexture;
	decltype(glBlendFunc)* blendFunc;
	decltype(glClear)* clear;
	decltype(glClearColor)* clearColor;
	decltype(glCullFace)* cullFace;
	decltype(glDeleteTextures)* deleteTextures;
	decltype(glDisable)* disable;
	decltype(glDrawArrays)* drawArrays;
	decltype(glEnable)* enable;
	decltype(glFrontFace)* frontFace;
	decltype(glGenTextures)* genTextures;
	decltype(glGetIntegerv)* getIntegerv;
	decltype(glPixelStorei)* pixelStorei;
	decltype(glReadPixels)* readPixels;
	decltype(glScissor)* scissor;
	decltype(glTexImage2D)* texImage2D;
	decltype(glTexParameteri)* texParameteri;
	decltype(glViewport)* viewport;

	void initFunctions();
};

struct FunctionsGl1 {
	decltype(glColor4fv)* color4fv;
	decltype(glEnableClientState)* enableClientState;
	decltype(glLoadMatrixf)* loadMatrixf;
	decltype(glMatrixMode)* matrixMode;
	decltype(glTexCoordPointer)* texCoordPointer;
	decltype(glVertexPointer)* vertexPointer;

	void initFunctions();
};

struct FunctionsGl3 {
	PFNGLATTACHSHADERPROC attachShader;
	PFNGLBINDBUFFERPROC bindBuffer;
	PFNGLBINDFRAMEBUFFERPROC bindFramebuffer;
	PFNGLBINDVERTEXARRAYPROC bindVertexArray;
	PFNGLBUFFERDATAPROC bufferData;
	PFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebufferStatus;
	PFNGLCLEARBUFFERUIVPROC clearBufferuiv;
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
	PFNGLSHADERSOURCEPROC shaderSource;
	PFNGLUNIFORM1IPROC uniform1i;
	PFNGLUNIFORM2FPROC uniform2f;
	PFNGLUNIFORM2UIPROC uniform2ui;
	PFNGLUNIFORM4FPROC uniform4f;
	PFNGLUNIFORM4FVPROC uniform4fv;
	PFNGLUNIFORM4IVPROC uniform4iv;
	PFNGLUSEPROGRAMPROC useProgram;
	PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer = nullptr;

	void initFunctions();
	bool functionsInitialized() const { return vertexAttribPointer; }
};

class RendererGl : public Renderer {
protected:
	class TextureGl : public Texture {
	private:
		GLuint id = 0;

		TextureGl(uvec2 size, GLuint tex) : Texture(size), id(tex) {}

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
	struct SurfaceInfo {
		SDL_Surface* img = nullptr;
		uint16 ifmt;
		uint16 pfmt;
		uint16 type;
		uint8 align;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, uint16 internal, uint16 format, uint16 texel);
	};

	uint16 iformRgb;
	uint16 iformRgba;
	uint16 iformRgba10;
protected:
	uint16 texType = GL_TEXTURE_2D;
	bool core;
	bool hasBgra = true;
	bool hasPackedPixels = true;
	bool hasTextureCompression;
#ifdef _WIN32
	ViewGl* cvw = nullptr;	// current context's view
#else
	FunctionsGl gl;
#endif

	RendererGl();

public:
	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void setCompression(Settings::Compression compression) override;

	void finishDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) override;
	Texture* texFromRpic(SDL_Surface* img) override;
	Texture* texFromText(const PixmapRgba& pm) override;
	bool texFromText(Texture* tex, const PixmapRgba& pm) override;
	void freeTexture(Texture* tex) override;

protected:
#ifdef _WIN32
	template <Class T = ViewGl> T* setContext(View* view);
#else
	template <Class T = ViewGl> static T* setContext(View* view);
#endif
	template <Class T, class F> void initContexts(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, F initGl);
	void initGlCommon(ViewGl* view, bool vsync, const vec4& bgcolor);
	void finalizeConstruction(Settings* sets, uintptr_t availableMemory);
	static void setSwapInterval(bool vsync);
private:
	GLuint initTexture(GLint filter);
	void uploadTexture(TextureGl* tex, SurfaceInfo& si);
	void uploadTexture(TextureGl* tex, const PixmapRgba& pm);
	template <bool keep> SurfaceInfo pickPixFormat(SDL_Surface* img) const;
#ifndef NDEBUG
	static void APIENTRY debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
#endif
};

class RendererGl1 final : public RendererGl {
private:
	struct ViewGl1 : ViewGl {
#ifdef _WIN32
		FunctionsGl1 gl1;
#endif
		mat4 proj;

		ViewGl1(SDL_Window* window, const Recti& area);
	};

#ifndef _WIN32
	FunctionsGl1 gl1;
#endif
	mat4 model = mat4(1.f);
	mat4 mtex = mat4(1.f);

public:
	RendererGl1(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererGl1() override;

	void updateView(ivec2& viewRes) override;
	Info getInfo() const override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;

private:
	void initGl(ViewGl1* view, bool vsync, const vec4& bgcolor, bool& canTexRect, uintptr_t& availableMemory);
	template <Number T> static void setPosScale(mat4& matrix, const Rect<T>& rect);
};

class RendererGl3 final : public RendererGl {
private:
	struct ViewGl3 : ViewGl {
#ifdef _WIN32
		FunctionsGl3 gl3;
#endif
		GLuint vao = 0;

		using ViewGl::ViewGl;
	};

#ifndef _WIN32
	FunctionsGl3 gl3;
#endif
	GLint uniPviewGui, uniRectGui, uniFrameGui, uniColorGui;
	GLint uniPviewSel, uniRectSel, uniFrameSel, uniAddrSel;
	GLuint progGui = 0, progSel = 0;
	GLuint fboSel = 0, texSel = 0;	// framebuffer is bound to the first context
	GLuint vbo = 0;

public:
	RendererGl3(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererGl3() override;

	void updateView(ivec2& viewRes) override;
	Info getInfo() const override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;

	void startSelDraw(View* view, ivec2 pos) override;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) override;
	Widget* finishSelDraw(View* view) override;

private:
	void initGl(ViewGl3* view, bool vsync, const vec4& bgcolor, uintptr_t& availableMemory);
	void initShader();
	GLuint createShader(const char* vertSrc, const char* fragSrc, const char* name) const;
	void checkFramebufferStatus(const char* name);
	static void checkStatus(GLuint id, GLenum stat, PFNGLGETSHADERIVPROC check, PFNGLGETSHADERINFOLOGPROC info, const string& name);
};
#endif
