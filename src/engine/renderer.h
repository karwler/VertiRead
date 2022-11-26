#pragma once

#include "utils/settings.h"
#if WITH_DIRECTX
#include <d3d11.h>
#endif
#ifdef _WIN32
#include <windows.h>	// needs to be here because SDL_opengl.h breaks some declarations
#include <SDL_opengl.h>
#else
#ifdef OPENGLES
#include <GLES3/gl3.h>
#endif
#include <SDL2/SDL_opengl.h>
#endif

class Texture {
private:
	ivec2 res = ivec2(0);

protected:
	Texture(ivec2 size);

public:
	virtual void free() = 0;
	ivec2 getRes() const;
};

inline Texture::Texture(ivec2 size) :
	res(size)
{}

inline ivec2 Texture::getRes() const {
	return res;
}

class Renderer {
public:
	static constexpr int singleDspId = -1;

	struct View {
		SDL_Window* win;
		Rect rect;

		View(SDL_Window* window, const Rect& area = Rect());
	};

protected:
	umap<int, View*> views;

public:
	virtual ~Renderer() = default;

	virtual void setClearColor(const vec4& color) = 0;
	virtual void setSwapInterval(Settings::VSync& vsync) = 0;
	virtual void updateView(ivec2& viewRes) = 0;
	virtual void startDraw(const View* view) = 0;
	virtual void drawRect(const Texture* tex, const Rect& rect, const Rect& frame, const vec4& color) = 0;
	virtual void finishDraw(const View* view) = 0;
	virtual void startSelDraw(const View* view, ivec2 pos) = 0;
	virtual void drawSelRect(const Widget* wgt, const Rect& rect, const Rect& frame) = 0;
	virtual Widget* finishSelDraw(const View* view) = 0;
	virtual Texture* texFromColor(u8vec4 color) = 0;
	virtual Texture* texFromText(SDL_Surface* img) = 0;
	virtual Texture* texFromIcon(SDL_Surface* pic) = 0;
	const umap<int, View*>& getViews() const;
};

inline const umap<int, Renderer::View*>& Renderer::getViews() const {
	return views;
}

#ifdef WITH_DIRECTX
class RendererDx : public Renderer {
private:
	class TextureDx : public Texture {
	private:
		ID3D11Texture2D* tex = nullptr;
		ID3D11ShaderResourceView* view = nullptr;

		TextureDx(ivec2 size, ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureView);
	public:
		void free() final;

		friend class RendererDx;
	};

	struct ViewDx : View {
		IDXGISwapChain* sc;
		ID3D11RenderTargetView* tgt;

		ViewDx(SDL_Window* window, const Rect& area, IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer);
	};

	struct VsData {
		alignas(16) ivec4 rect = ivec4(0);
		alignas(16) ivec4 frame = ivec4(0);
		alignas(16) vec2 pview = vec2(0.f);
	};

	struct PsData {
		alignas(16) vec4 color = vec4(0.f);
	};

	struct AddrData {
		alignas(16) uvec2 addr = uvec2(0);
	};

	ID3D11Device* dev = nullptr;
	ID3D11DeviceContext* ctx = nullptr;

	ID3D11VertexShader* vertGui = nullptr;
	ID3D11PixelShader* pixlGui = nullptr;
	ID3D11VertexShader* vertSel = nullptr;
	ID3D11PixelShader* pixlSel = nullptr;

	ID3D11Buffer* vsDatBuf = nullptr;
	ID3D11Buffer* psDatBuf = nullptr;
	ID3D11Buffer* addrDatBuf = nullptr;
	ID3D11SamplerState* sampleState = nullptr;

	ID3D11RasterizerState* rasterizerGui = nullptr;
	ID3D11RasterizerState* rasterizerSel = nullptr;
	ID3D11Texture2D* texAddr = nullptr;
	ID3D11RenderTargetView* tgtAddr = nullptr;
	ID3D11Texture2D* outAddr = nullptr;

	DXGI_SWAP_EFFECT syncInterval;
	vec4 bgColor;
	VsData vsData;

public:
	RendererDx(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, const vec4& bgcolor);
	~RendererDx() final;

	void setClearColor(const vec4& color) final;
	void setSwapInterval(Settings::VSync& vsync) final;
	void updateView(ivec2& viewRes) final;

	void startDraw(const View* view) final;
	void drawRect(const Texture* tex, const Rect& rect, const Rect& frame, const vec4& color) final;
	void finishDraw(const View* view) final;

	void startSelDraw(const View* view, ivec2 pos) final;
	void drawSelRect(const Widget* wgt, const Rect& rect, const Rect& frame) final;
	Widget* finishSelDraw(const View* view) final;

	Texture* texFromColor(u8vec4 color) final;
	Texture* texFromText(SDL_Surface* img) final;
	Texture* texFromIcon(SDL_Surface* pic) final;

private:
	static IDXGIFactory* createFactory();
	pair<IDXGISwapChain*, ID3D11RenderTargetView*> createSwapchain(IDXGIFactory* factory, SDL_Window* win, ivec2 res);
	void recreateSwapchain(IDXGIFactory* factory, ViewDx* view);
	void initShader();
	pair<ID3D11VertexShader*, ID3D11PixelShader*> createShader(const char* vertSrc, const char* pixlSrc, const char* name) const;

	template <class T> void uploadBuffer(ID3D11Buffer* buffer, const T& data);
	TextureDx* createTexture(const void* pixels, ivec2 res, uint rowLen, DXGI_FORMAT format);
	static pair<SDL_Surface*, DXGI_FORMAT> pickPixFormat(SDL_Surface* img);
	static DXGI_SWAP_EFFECT toSwapEffect(Settings::VSync vsync);
	static string hresultToStr(HRESULT rs);
};
#endif

class RendererGl : public Renderer {
private:
#ifdef OPENGLES
	static constexpr GLenum textPixFormat = GL_RGBA;
#else
	static constexpr GLenum textPixFormat = GL_BGRA;
#endif

	class TextureGl : public Texture {
	private:
		GLuint id = 0;

		TextureGl(ivec2 size, GLuint tex);
	public:
		void free() final;

		friend class RendererGl;
	};

	struct ViewGl : View {
		SDL_GLContext ctx;

		ViewGl(SDL_Window* window, const Rect& area, SDL_GLContext context);
	};

	GLint uniPviewGui, uniRectGui, uniFrameGui, uniColorGui;
	GLint uniPviewSel, uniRectSel, uniFrameSel, uniAddrSel;
	GLuint progGui = 0, progSel = 0;
	GLuint fboSel = 0, texSel = 0;
	GLuint vao = 0;

#ifndef OPENGLES
	void (APIENTRY* glActiveTexture)(GLenum texture);
	void (APIENTRY* glAttachShader)(GLuint program, GLuint shader);
	void (APIENTRY* glBindFramebuffer)(GLenum target, GLuint framebuffer);
	void (APIENTRY* glBindVertexArray)(GLuint array);
	GLenum (APIENTRY* glCheckFramebufferStatus)(GLenum target);
	void (APIENTRY* glClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint* value);
	void (APIENTRY* glCompileShader)(GLuint shader);
	GLuint (APIENTRY* glCreateProgram)();
	GLuint (APIENTRY* glCreateShader)(GLenum shaderType);
	void (APIENTRY* glDeleteFramebuffers)(GLsizei n, GLuint* framebuffers);
	void (APIENTRY* glDeleteShader)(GLuint shader);
	void (APIENTRY* glDeleteProgram)(GLuint program);
	void (APIENTRY* glDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
	void (APIENTRY* glDetachShader)(GLuint program, GLuint shader);
	void (APIENTRY* glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	void (APIENTRY* glGenerateMipmap)(GLenum target);
	void (APIENTRY* glGenFramebuffers)(GLsizei n, GLuint* ids);
	void (APIENTRY* glGenVertexArrays)(GLsizei n, GLuint* arrays);
	void (APIENTRY* glGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
	void (APIENTRY* glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
	void (APIENTRY* glGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
	void (APIENTRY* glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
	GLint (APIENTRY* glGetUniformLocation)(GLuint program, const GLchar* name);
	void (APIENTRY* glLinkProgram)(GLuint program);
	void (APIENTRY* glShaderSource)(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
	void (APIENTRY* glUniform1i)(GLint location, GLint v0);
	void (APIENTRY* glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
	void (APIENTRY* glUniform2fv)(GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY* glUniform2ui)(GLint location, GLuint v0, GLuint v1);
	void (APIENTRY* glUniform2uiv)(GLint location, GLsizei count, const GLuint* value);
	void (APIENTRY* glUniform4fv)(GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY* glUniform4iv)(GLint location, GLsizei count, const GLint* value);
	void (APIENTRY* glUseProgram)(GLuint program);
#endif
public:
	RendererGl(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, const vec4& bgcolor);
	~RendererGl() final;

	void setClearColor(const vec4& color) final;
	void setSwapInterval(Settings::VSync& vsync) final;
	void updateView(ivec2& viewRes) final;

	void startDraw(const View* view) final;
	void drawRect(const Texture* tex, const Rect& rect, const Rect& frame, const vec4& color) final;
	void finishDraw(const View* view) final;

	void startSelDraw(const View* view, ivec2 pos) final;
	void drawSelRect(const Widget* wgt, const Rect& rect, const Rect& frame) final;
	Widget* finishSelDraw(const View* view) final;

	Texture* texFromColor(u8vec4 color) final;
	Texture* texFromText(SDL_Surface* img) final;
	Texture* texFromIcon(SDL_Surface* pic) final;

private:
	void initGl(ivec2 res, Settings::VSync& vsync, const vec4& bgcolor);
	bool trySetSwapInterval(Settings::VSync vsync);
	bool trySetSwapIntervalSingle(Settings::VSync vsync);
#ifndef OPENGLES
	void initFunctions();
#endif
	void initShader();
	GLuint createShader(const char* vertSrc, const char* fragSrc, const char* name) const;
	void checkFramebufferStatus(const char* name);

	template <class C, class I> static void checkStatus(GLuint id, GLenum stat, C check, I info, const string& name);
	template <GLint min, GLint mag, GLint mip> static GLuint createTexture(const void* pixels, int width, int height, int rowLen, GLenum format);
	static pair<SDL_Surface*, GLenum> pickPixFormat(SDL_Surface* img);
#ifndef OPENGLES
#ifndef NDEBUG
	static void APIENTRY debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
#endif
#endif
};
