#ifdef WITH_OPENGL
#include "rendererGl.h"
#include "utils/settings.h"
#include <glm/gtc/type_ptr.hpp>
#include <regex>

RendererGl::TextureGl::TextureGl(ivec2 size, GLuint tex) :
	Texture(size),
	id(tex)
{}

RendererGl::ViewGl::ViewGl(SDL_Window* window, const Recti& area, SDL_GLContext context) :
	View(window, area),
	ctx(context)
{}

RendererGl::RendererGl(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	Renderer({
#ifdef OPENGLES
		SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_RGB24,
		SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_RGB565
#else
		SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_BGRA32, SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_BGR24,
		SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565
#endif
	})
{
	if (windows.size() == 1 && windows.begin()->first == singleDspId) {
		SDL_GL_GetDrawableSize(windows.begin()->second, &viewRes.x, &viewRes.y);
		if (!static_cast<ViewGl*>(views.emplace(singleDspId, new ViewGl(windows.begin()->second, Recti(ivec2(0), viewRes), SDL_GL_CreateContext(windows.begin()->second))).first->second)->ctx)
			throw std::runtime_error("Failed to create context:"s + linend + SDL_GetError());
		initGl(viewRes, sets->vsync, bgcolor);
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			Recti wrect = sets->displays.at(id).translate(-origin);
			SDL_GL_GetDrawableSize(windows.begin()->second, &wrect.w, &wrect.h);
			if (!static_cast<ViewGl*>(views.emplace(id, new ViewGl(win, wrect, SDL_GL_CreateContext(win))).first->second)->ctx)
				throw std::runtime_error("Failed to create context:"s + linend + SDL_GetError());
			viewRes = glm::max(viewRes, wrect.end());
			initGl(wrect.size(), sets->vsync, bgcolor);
		}
	}
#ifndef OPENGLES
	initFunctions();
#endif
	initShader();
	setCompression(sets->compression);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	setMaxPicRes(sets->maxPicRes);
}

RendererGl::~RendererGl() {
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &texSel);
	glDeleteFramebuffers(1, &fboSel);
	glDeleteProgram(progSel);
	glDeleteProgram(progGui);

	for (auto [id, view] : views) {
		SDL_GL_DeleteContext(static_cast<ViewGl*>(view)->ctx);
		delete view;
	}
}

void RendererGl::setClearColor(const vec4& color) {
	for (auto [id, view] : views) {
		SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx);
		glClearColor(color.r, color.g, color.b, color.a);
	}
}

void RendererGl::initGl(ivec2 res, bool vsync, const vec4& bgcolor) {
	setSwapInterval(vsync);
	glViewport(0, 0, res.x, res.y);
	glClearColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	glDisable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glScissor(0, 0, 1, 1);	// for address pass
}

void RendererGl::setVsync(bool vsync) {
	for (auto [id, view] : views) {
		SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx);
		setSwapInterval(vsync);
	}
}

void RendererGl::setSwapInterval(bool vsync) {
	if (!vsync || (SDL_GL_SetSwapInterval(-1) && SDL_GL_SetSwapInterval(1)))
		SDL_GL_SetSwapInterval(0);
}

void RendererGl::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_GL_GetDrawableSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;
		glViewport(0, 0, viewRes.x, viewRes.y);
	}
}

#ifndef OPENGLES
void RendererGl::initFunctions() {
	glActiveTexture = reinterpret_cast<decltype(glActiveTexture)>(SDL_GL_GetProcAddress("glActiveTexture"));
	glAttachShader = reinterpret_cast<decltype(glAttachShader)>(SDL_GL_GetProcAddress("glAttachShader"));
	glBindFramebuffer = reinterpret_cast<decltype(glBindFramebuffer)>(SDL_GL_GetProcAddress("glBindFramebuffer"));
	glBindVertexArray = reinterpret_cast<decltype(glBindVertexArray)>(SDL_GL_GetProcAddress("glBindVertexArray"));
	glCheckFramebufferStatus = reinterpret_cast<decltype(glCheckFramebufferStatus)>(SDL_GL_GetProcAddress("glCheckFramebufferStatus"));
	glClearBufferuiv = reinterpret_cast<decltype(glClearBufferuiv)>(SDL_GL_GetProcAddress("glClearBufferuiv"));
	glCompileShader = reinterpret_cast<decltype(glCompileShader)>(SDL_GL_GetProcAddress("glCompileShader"));
	glCreateProgram = reinterpret_cast<decltype(glCreateProgram)>(SDL_GL_GetProcAddress("glCreateProgram"));
	glCreateShader = reinterpret_cast<decltype(glCreateShader)>(SDL_GL_GetProcAddress("glCreateShader"));
	glDeleteFramebuffers = reinterpret_cast<decltype(glDeleteFramebuffers)>(SDL_GL_GetProcAddress("glDeleteFramebuffers"));
	glDeleteShader = reinterpret_cast<decltype(glDeleteShader)>(SDL_GL_GetProcAddress("glDeleteShader"));
	glDeleteProgram = reinterpret_cast<decltype(glDeleteProgram)>(SDL_GL_GetProcAddress("glDeleteProgram"));
	glDeleteVertexArrays = reinterpret_cast<decltype(glDeleteVertexArrays)>(SDL_GL_GetProcAddress("glDeleteVertexArrays"));
	glDetachShader = reinterpret_cast<decltype(glDetachShader)>(SDL_GL_GetProcAddress("glDetachShader"));
	glFramebufferTexture1D = reinterpret_cast<decltype(glFramebufferTexture1D)>(SDL_GL_GetProcAddress("glFramebufferTexture1D"));
	glGenFramebuffers = reinterpret_cast<decltype(glGenFramebuffers)>(SDL_GL_GetProcAddress("glGenFramebuffers"));
	glGenVertexArrays = reinterpret_cast<decltype(glGenVertexArrays)>(SDL_GL_GetProcAddress("glGenVertexArrays"));
	glGetProgramInfoLog = reinterpret_cast<decltype(glGetProgramInfoLog)>(SDL_GL_GetProcAddress("glGetProgramInfoLog"));
	glGetProgramiv = reinterpret_cast<decltype(glGetProgramiv)>(SDL_GL_GetProcAddress("glGetProgramiv"));
	glGetShaderInfoLog = reinterpret_cast<decltype(glGetShaderInfoLog)>(SDL_GL_GetProcAddress("glGetShaderInfoLog"));
	glGetShaderiv = reinterpret_cast<decltype(glGetShaderiv)>(SDL_GL_GetProcAddress("glGetShaderiv"));
	glGetUniformLocation = reinterpret_cast<decltype(glGetUniformLocation)>(SDL_GL_GetProcAddress("glGetUniformLocation"));
	glLinkProgram = reinterpret_cast<decltype(glLinkProgram)>(SDL_GL_GetProcAddress("glLinkProgram"));
	glShaderSource = reinterpret_cast<decltype(glShaderSource)>(SDL_GL_GetProcAddress("glShaderSource"));
	glUniform1i = reinterpret_cast<decltype(glUniform1i)>(SDL_GL_GetProcAddress("glUniform1i"));
	glUniform2f = reinterpret_cast<decltype(glUniform2f)>(SDL_GL_GetProcAddress("glUniform2f"));
	glUniform2fv = reinterpret_cast<decltype(glUniform2fv)>(SDL_GL_GetProcAddress("glUniform2fv"));
	glUniform2ui = reinterpret_cast<decltype(glUniform2ui)>(SDL_GL_GetProcAddress("glUniform2ui"));
	glUniform2uiv = reinterpret_cast<decltype(glUniform2uiv)>(SDL_GL_GetProcAddress("glUniform2uiv"));
	glUniform4f = reinterpret_cast<decltype(glUniform4f)>(SDL_GL_GetProcAddress("glUniform4f"));
	glUniform4fv = reinterpret_cast<decltype(glUniform4fv)>(SDL_GL_GetProcAddress("glUniform4fv"));
	glUniform4iv = reinterpret_cast<decltype(glUniform4iv)>(SDL_GL_GetProcAddress("glUniform4iv"));
	glUseProgram = reinterpret_cast<decltype(glUseProgram)>(SDL_GL_GetProcAddress("glUseProgram"));
#ifndef NDEBUG
	int gval;
	if (glGetIntegerv(GL_CONTEXT_FLAGS, &gval); gval & GL_CONTEXT_FLAG_DEBUG_BIT && SDL_GL_ExtensionSupported("GL_KHR_debug")) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		reinterpret_cast<void (APIENTRY*)(void (APIENTRY*)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*), const void*)>(SDL_GL_GetProcAddress("glDebugMessageCallback"))(debugMessage, nullptr);
		reinterpret_cast<void (APIENTRY*)(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean)>(SDL_GL_GetProcAddress("glDebugMessageControl"))(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif
}
#endif

void RendererGl::initShader() {
	const char* vertSrc = R"r(#version 130

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec4 pview;
uniform ivec4 rect;
uniform ivec4 frame;

noperspective out vec2 fragUV;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec4 uvrc = vec4(dst.xy - vec2(rect.xy), dst.zw) / vec4(rect.zwzw);
		fragUV = vposs[gl_VertexID] * uvrc.zw + uvrc.xy;
		vec2 loc = vposs[gl_VertexID] * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.0, 1.0);
   } else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
})r";
	const char* fragSrc = R"r(#version 130

uniform sampler2D colorMap;
uniform vec4 color;

noperspective in vec2 fragUV;

out vec4 outColor;

void main() {
	outColor = texture(colorMap, fragUV) * color;
})r";
	progGui = createShader(vertSrc, fragSrc, "gui");
	uniPviewGui = glGetUniformLocation(progGui, "pview");
	uniRectGui = glGetUniformLocation(progGui, "rect");
	uniFrameGui = glGetUniformLocation(progGui, "frame");
	uniColorGui = glGetUniformLocation(progGui, "color");
	glUniform1i(glGetUniformLocation(progGui, "colorMap"), 0);

	vertSrc = R"r(#version 130

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec4 pview;
uniform ivec4 rect;
uniform ivec4 frame;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec2 loc = vposs[gl_VertexID] * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview[2] - 1.0, -(loc.y - pview.y) / pview[3] + 1.0, 0.0, 1.0);
   } else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
})r";
	fragSrc = R"r(#version 130

uniform uvec2 addr;

out uvec2 outAddr;

void main() {
	if (addr.x != 0u || addr.y != 0u)
		outAddr = addr;
	else
		discard;
})r";
	progSel = createShader(vertSrc, fragSrc, "sel");
	uniPviewSel = glGetUniformLocation(progSel, "pview");
	uniRectSel = glGetUniformLocation(progSel, "rect");
	uniFrameSel = glGetUniformLocation(progSel, "frame");
	uniAddrSel = glGetUniformLocation(progSel, "addr");

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &texSel);
	glBindTexture(addrTargetType, texSel);
	glTexParameteri(addrTargetType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(addrTargetType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(addrTargetType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(addrTargetType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(addrTargetType, GL_TEXTURE_MAX_LEVEL, 0);
#ifdef OPENGLES
	glTexImage2D(addrTargetType, 0, GL_RG32UI, 1, 1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
#else
	glTexImage1D(addrTargetType, 0, GL_RG32UI, 1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);
#endif

	glGenFramebuffers(1, &fboSel);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSel);
#ifdef OPENGLES
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, addrTargetType, texSel, 0);
#else
	glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, addrTargetType, texSel, 0);
#endif
	checkFramebufferStatus("sel");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glUseProgram(progGui);
}

GLuint RendererGl::createShader(const char* vertSrc, const char* fragSrc, const char* name) const {
#ifdef OPENGLES
	array<pair<std::regex, const char*>, 2> replacers = {
		pair(std::regex(R"r(#version\s+\d+)r"), "#version 300 es\nprecision highp float;precision highp int;precision highp sampler2D;"),
		pair(std::regex(R"r(noperspective\s+)r"), "")
	};
	string vertTmp = vertSrc, fragTmp = fragSrc;
	for (const auto& [rgx, rpl] : replacers) {
		vertTmp = std::regex_replace(vertTmp, rgx, rpl);
		fragTmp = std::regex_replace(fragTmp, rgx, rpl);
	}
	vertSrc = vertTmp.c_str();
	fragSrc = fragTmp.c_str();
#endif
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	checkStatus(vert, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + ".vert"s);

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	checkStatus(frag, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + ".frag"s);

	GLuint sprog = glCreateProgram();
	glAttachShader(sprog, vert);
	glAttachShader(sprog, frag);
	glLinkProgram(sprog);
	glDetachShader(sprog, vert);
	glDetachShader(sprog, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	checkStatus(sprog, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog, name + " program"s);
	glUseProgram(sprog);
	return sprog;
}

template <class C, class I>
void RendererGl::checkStatus(GLuint id, GLenum stat, C check, I info, const string& name) {
	int res;
	if (check(id, stat, &res); res == GL_FALSE) {
		string err;
		if (check(id, GL_INFO_LOG_LENGTH, &res); res) {
			err.resize(res);
			info(id, res, nullptr, err.data());
			err = trim(err);
		}
		err = !err.empty() ? name + ":" + linend + err : name + ": unknown error";
		logError(err);
		throw std::runtime_error(err);
	}
}

void RendererGl::checkFramebufferStatus(const char* name) {
	switch (GLenum rc = glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
	case GL_FRAMEBUFFER_UNDEFINED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNDEFINED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"s);
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"s);
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNSUPPORTED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"s);
#endif
	default:
		if (rc != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error(name + ": unknown framebuffer error"s);
	}
}

void RendererGl::startDraw(View* view) {
	SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx);
	glUniform4f(uniPviewGui, float(view->rect.x), float(view->rect.y), float(view->rect.w) / 2.f, float(view->rect.h) / 2.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RendererGl::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	glBindTexture(GL_TEXTURE_2D, static_cast<const TextureGl*>(tex)->id);
	glUniform4iv(uniRectGui, 1, reinterpret_cast<const int*>(&rect));
	glUniform4iv(uniFrameGui, 1, reinterpret_cast<const int*>(&frame));
	glUniform4fv(uniColorGui, 1, glm::value_ptr(color));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RendererGl::finishDraw(View* view) {
	SDL_GL_SwapWindow(static_cast<ViewGl*>(view)->win);
}

void RendererGl::startSelDraw(View* view, ivec2 pos) {
	uint zero[4] = { 0, 0, 0, 0 };
	SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx);
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);
	glEnable(GL_SCISSOR_TEST);
	glViewport(-pos.x, pos.y - view->rect.h + 1, view->rect.w, view->rect.h);
	glUseProgram(progSel);
	glUniform4f(uniPviewSel, float(view->rect.x), float(view->rect.y), float(view->rect.w) / 2.f, float(view->rect.h) / 2.f);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSel);
	glClearBufferuiv(GL_COLOR, 0, zero);
}

void RendererGl::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	glUniform4iv(uniRectSel, 1, reinterpret_cast<const int*>(&rect));
	glUniform4iv(uniFrameSel, 1, reinterpret_cast<const int*>(&frame));
	glUniform2ui(uniAddrSel, uintptr_t(wgt), uintptr_t(wgt) >> 32);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

Widget* RendererGl::finishSelDraw(View* view) {
	uvec2 val;
	glReadPixels(0, 0, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, glm::value_ptr(val));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, view->rect.w, view->rect.h);
	glEnable(GL_DITHER);
	glEnable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glUseProgram(progGui);
	return reinterpret_cast<Widget*>(uintptr_t(val.x) | (uintptr_t(val.y) << 32));
}

Texture* RendererGl::texFromIcon(SDL_Surface* img) {
	if (auto [pic, ifmt, pfmt, type] = pickPixFormat<true>(limitSize(img, maxTextureSize)); pic) {
		TextureGl* tex = createTexture(pic->pixels, uvec2(pic->w, pic->h), pic->pitch / pic->format->BytesPerPixel, ifmt, pfmt, type, GL_LINEAR);
		SDL_FreeSurface(pic);
		return tex;
	}
	return nullptr;
}

Texture* RendererGl::texFromRpic(SDL_Surface* img) {
	if (auto [pic, ifmt, pfmt, type] = pickPixFormat<false>(img); pic) {
		TextureGl* tex = createTexture(pic->pixels, uvec2(pic->w, pic->h), pic->pitch / pic->format->BytesPerPixel, ifmt, pfmt, type, GL_LINEAR);
		SDL_FreeSurface(pic);
		return tex;
	}
	return nullptr;
}

Texture* RendererGl::texFromText(const Pixmap& pm) {
	return pm.pix ? createTexture(pm.pix.get(), glm::min(pm.res, uvec2(maxTextureSize)), pm.res.x, GL_RGBA8, textPixFormat, GL_UNSIGNED_BYTE, GL_NEAREST) : nullptr;
}

void RendererGl::freeTexture(Texture* tex) {
	glDeleteTextures(1, &static_cast<TextureGl*>(tex)->id);
	delete tex;
}

RendererGl::TextureGl* RendererGl::createTexture(const void* pix, uvec2 res, uint tpitch, GLint iform, GLenum pform, GLenum type, GLint filter) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, tpitch);
	glTexImage2D(GL_TEXTURE_2D, 0, iform, res.x, res.y, 0, pform, type, pix);
	return new TextureGl(res, id);
}

template <bool keep>
tuple<SDL_Surface*, GLint, GLenum, GLenum> RendererGl::pickPixFormat(SDL_Surface* img) const {
	if (img)
		switch (img->format->format) {
		case SDL_PIXELFORMAT_RGBA32:
			if constexpr (keep)
				return tuple(img, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
			else
				return tuple(img, iformRgba, GL_RGBA, GL_UNSIGNED_BYTE);
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGRA32:
			if constexpr (keep)
				return tuple(img, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE);
			else
				return tuple(img, iformRgba, GL_BGRA, GL_UNSIGNED_BYTE);
#endif
		case SDL_PIXELFORMAT_RGB24:
			if constexpr (keep)
				return tuple(img, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
			else
				return tuple(img, iformRgb, GL_RGB, GL_UNSIGNED_BYTE);
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGR24:
			if constexpr (keep)
				return tuple(img, GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE);
			else
				return tuple(img, iformRgb, GL_BGR, GL_UNSIGNED_BYTE);
#endif
		case SDL_PIXELFORMAT_RGBA5551:
			return tuple(img, GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_5_6_5);
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGRA5551:
			return tuple(img, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_5_6_5);
#endif
		case SDL_PIXELFORMAT_RGB565:
#ifdef OPENGLES
			return tuple(img, GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
#else
			return tuple(img, GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
#endif
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGR565:
			return tuple(img, GL_RGB5, GL_BGR, GL_UNSIGNED_SHORT_5_6_5);
#endif
		default:
			img = convertReplace(img);
		}
	if constexpr (keep)
		return tuple(img, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	else
		return tuple(img, iformRgba, GL_RGBA, GL_UNSIGNED_BYTE);
}

uint RendererGl::maxTexSize() const {
	return maxTextureSize;
}

const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* RendererGl::getSquashableFormats() const {
	return nullptr;
}

void RendererGl::setCompression(Settings::Compression compression) {
	switch (compression) {
	case Settings::Compression::none:
		iformRgb = GL_RGB8;
		iformRgba = GL_RGBA8;
		break;
	case Settings::Compression::b16:
#ifdef OPENGLES
		iformRgb = GL_RGB565;
#else
		iformRgb = GL_RGB5;
#endif
		iformRgba = GL_RGB5_A1;
		break;
	case Settings::Compression::compress:
		iformRgb = GL_COMPRESSED_RGB;
		iformRgba = GL_COMPRESSED_RGBA;
	}
}

pair<uint, Settings::Compression> RendererGl::getSettings(vector<pair<u32vec2, string>>& devices) const {
	devices.clear();
	return pair(maxTextureSize, Settings::Compression::compress);
}

#ifndef OPENGLES
#ifndef NDEBUG
void APIENTRY RendererGl::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;
	string msg = "Debug message " + toStr(id) + ": " + message;
	if (msg.back() != '\n' && msg.back() != '\r')
		msg += linend;

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		msg += "Source: API, ";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		msg += "Source: window system, ";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		msg += "Source: shader compiler, ";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		msg += "Source: third party, ";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		msg += "Source: application, ";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		msg += "Source: other, ";
		break;
	default:
		msg += "Source: unknown, ";
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		msg += "Type: error, ";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		msg += "Type: deprecated behavior, ";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		msg += "Type: undefined behavior, ";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		msg += "Type: portability, ";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		msg += "Type: performance, ";
		break;
	case GL_DEBUG_TYPE_MARKER:
		msg += "Type: marker, ";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		msg += "Type: push group, ";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		msg += "Type: pop group, ";
		break;
	case GL_DEBUG_TYPE_OTHER:
		msg += "Type: other, ";
		break;
	default:
		msg += "Type: unknown, ";
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		logError(std::move(msg), "Severity: high");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		logError(std::move(msg), "Severity: medium");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		logError(std::move(msg), "Severity: low");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		logInfo(std::move(msg), "Severity: notification");
		break;
	default:
		logError(std::move(msg), "Severity: unknown");
	}
}
#endif
#endif
#endif
