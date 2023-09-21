#pragma once

#ifdef WITH_OPENGL
#include "renderer.h"
#ifdef _WIN32
#include <SDL_opengl.h>
#else
#ifdef OPENGLES
#include <GLES3/gl3.h>
#endif
#include <SDL2/SDL_opengl.h>
#endif

class RendererGl final : public Renderer {
private:
#ifdef OPENGLES
	static constexpr GLenum textPixFormat = GL_RGBA;
	static constexpr GLenum addrTargetType = GL_TEXTURE_2D;
#else
	static constexpr GLenum textPixFormat = GL_BGRA;
	static constexpr GLenum addrTargetType = GL_TEXTURE_1D;
#endif

	class TextureGl : public Texture {
	private:
		GLuint id = 0;

		TextureGl(ivec2 size, GLuint tex);

		friend class RendererGl;
	};

	struct ViewGl : View {
		SDL_GLContext ctx;

		ViewGl(SDL_Window* window, const Recti& area, SDL_GLContext context);
	};

	GLint uniPviewGui, uniRectGui, uniFrameGui, uniColorGui;
	GLint uniPviewSel, uniRectSel, uniFrameSel, uniAddrSel;
	GLuint progGui = 0, progSel = 0;
	GLuint fboSel = 0, texSel = 0;
	GLuint vao = 0;
	GLint iformRgb;
	GLint iformRgba;
	int maxTextureSize;

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
	void (APIENTRY* glFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
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
	void (APIENTRY* glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void (APIENTRY* glUniform4fv)(GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY* glUniform4iv)(GLint location, GLsizei count, const GLint* value);
	void (APIENTRY* glUseProgram)(GLuint program);
#endif
public:
	RendererGl(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererGl() final;

	void setClearColor(const vec4& color) final;
	void setVsync(bool vsync) final;
	void updateView(ivec2& viewRes) final;
	void setCompression(Settings::Compression compression) final;
	pair<uint, Settings::Compression> getSettings(vector<pair<u32vec2, string>>& devices) const final;

	void startDraw(View* view) final;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) final;
	void finishDraw(View* view) final;

	void startSelDraw(View* view, ivec2 pos) final;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) final;
	Widget* finishSelDraw(View* view) final;

	Texture* texFromIcon(SDL_Surface* img) final;
	Texture* texFromRpic(SDL_Surface* img) final;
	Texture* texFromText(const Pixmap& pm) final;
	void freeTexture(Texture* tex) final;

protected:
	uint maxTexSize() const final;
	const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* getSquashableFormats() const final;

private:
	void initGl(ivec2 res, bool vsync, const vec4& bgcolor);
	void setSwapInterval(bool vsync);
#ifndef OPENGLES
	void initFunctions();
#endif
	void initShader();
	GLuint createShader(const char* vertSrc, const char* fragSrc, const char* name) const;
	void checkFramebufferStatus(const char* name);

	template <Invocable<GLuint, GLenum, GLint*> C, Invocable<GLuint, GLsizei, GLsizei*, GLchar*> I> static void checkStatus(GLuint id, GLenum stat, C check, I info, const string& name);
	static TextureGl* createTexture(const cbyte* pix, uvec2 res, uint tpitch, GLint iform, GLenum pform, GLenum type, GLint filter);
	template <bool keep> tuple<SDL_Surface*, GLint, GLenum, GLenum> pickPixFormat(SDL_Surface* img) const;
#ifndef OPENGLES
#ifndef NDEBUG
	static void APIENTRY debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
#endif
#endif
};
#endif
