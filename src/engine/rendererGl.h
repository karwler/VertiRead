#pragma once

#ifdef WITH_OPENGL
#include "renderer.h"
#include <SDL_opengl.h>
#include <glm/mat4x4.hpp>

struct FunctionsGl {
	decltype(glBindTexture)* bindTexture;
	decltype(glBlendFunc)* blendFunc;
	decltype(glClear)* clear;
	decltype(glClearColor)* clearColor;
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
	PFNGLBINDVERTEXARRAYPROC bindVertexArray;
	PFNGLBUFFERDATAPROC bufferData;
	PFNGLCOMPILESHADERPROC compileShader;
	PFNGLCREATEPROGRAMPROC createProgram;
	PFNGLCREATESHADERPROC createShader;
	PFNGLDELETEBUFFERSPROC deleteBuffers;
	PFNGLDELETESHADERPROC deleteShader;
	PFNGLDELETEPROGRAMPROC deleteProgram;
	PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays;
	PFNGLDETACHSHADERPROC detachShader;
	PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray;
	PFNGLGENBUFFERSPROC genBuffers;
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
		SurfaceInfo(SDL_Surface* surface, uint16 internal, uint16 format, uint16 texel) noexcept;
	};

	GLint iformRgba8, iformRgb8;
	GLint iformRgba10;
	GLint iformRgba5, iformRgb5, iformRgba4;
protected:
	uint16 texType = GL_TEXTURE_2D;
	bool core;
	bool hasBgra = true;
	bool hasPackedPixels = true;
	bool hasTextureCompression;
	bool hasSwizzle = true;
#ifdef _WIN32
	ViewGl* cvw = nullptr;	// current context's view
#else
	FunctionsGl gl;
#endif

	RendererGl(size_t numViews);

public:
	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void setCompression(Settings::Compression cmpr) noexcept override;
	SDL_Surface* prepareImage(SDL_Surface* img, bool rpic) const noexcept override;
	Info getInfo() const noexcept override;

	void finishDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) noexcept override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) noexcept override;
	Texture* texFromRpic(SDL_Surface* img) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;

protected:
#ifdef _WIN32
	void setContext(View* view);
#else
	static void setContext(View* view);
#endif
	template <Class T, class F> void initContexts(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, F initGl);
	void initGlCommon(ViewGl* view, bool vsync, const vec4& bgcolor, uintptr_t& availableMemory) noexcept;
	void finalizeConstruction(Settings* sets, uintptr_t availableMemory) noexcept;
	static void setSwapInterval(bool vsync) noexcept;
private:
	GLuint initTexture(GLint filter) noexcept;
	void uploadTexture(TextureGl* tex, SurfaceInfo& si) noexcept;
	void uploadTexture(TextureGl* tex, const Pixmap& pm) noexcept;
	template <bool keep> SurfaceInfo pickPixFormat(SDL_Surface* img) const noexcept;
#ifndef NDEBUG
	static void APIENTRY debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam) noexcept;
#endif
};

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

public:
	RendererGl1(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor);
	~RendererGl1() override;

	void updateView(ivec2& viewRes) override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;

private:
	void initGl(ViewGl1* view, bool vsync, const vec4& bgcolor, bool& canTexRect, uintptr_t& availableMemory);
	void cleanup() noexcept;
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
	GLuint progGui = 0;
	GLuint vbo = 0;

public:
	RendererGl3(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor);
	~RendererGl3() override;

	void updateView(ivec2& viewRes) override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;

private:
	void initGl(ViewGl3* view, bool vsync, const vec4& bgcolor, uintptr_t& availableMemory);
	void initShader();
	void cleanup() noexcept;
	GLuint createShader(const char* vertSrc, const char* fragSrc, const char* name) const;
	static void checkStatus(GLuint id, GLenum stat, PFNGLGETSHADERIVPROC check, PFNGLGETSHADERINFOLOGPROC info, const string& name);
};
#endif
