#pragma once

#include "utils/settings.h"
#include <set>
#include <SDL_video.h>

struct PixmapRgba {
	uptr<uint32[]> pix;
	uvec2 res;
};

class Texture {
protected:
	uvec2 res;

	Texture(uvec2 size) : res(size) {}

public:
	uvec2 getRes() const { return res; }
};

class Renderer {
public:
	static constexpr int singleDspId = -1;

	struct View {
		SDL_Window* win;
		Recti rect;

		View(SDL_Window* window, const Recti& area) : win(window), rect(area) {}
	};

	struct ErrorSkip {};

	struct Info {
		struct Device {
			u32vec2 id;
			Cstring name;
			uintptr_t dmem;

			Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory = 0) noexcept;
		};

		vector<Device> devices;
		std::set<Settings::Compression> compressions;
		uint texSize;
		bool selecting;
	};

protected:
	static constexpr array vertices = {
		vec2(0.f, 0.f),
		vec2(1.f, 0.f),
		vec2(0.f, 1.f),
		vec2(1.f, 1.f)
	};

	umap<int, View*> views;
	umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum> preconvertFormats;	// formats of pictures to be converted
	uint maxTextureSize;
	uint maxPictureSize;	// should only get accessed from one thread at a time

	Renderer(uint maxTexRes) : maxTextureSize(maxTexRes) {}
public:
	virtual ~Renderer() = default;

	virtual void setClearColor(const vec4& color) = 0;
	virtual void setVsync(bool vsync) = 0;
	virtual void updateView(ivec2& viewRes) = 0;
	virtual void setCompression(Settings::Compression compression) = 0;
	virtual Info getInfo() const = 0;
	virtual void startDraw(View* view) = 0;
	virtual void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) = 0;
	virtual void finishDraw(View* view) = 0;
	virtual void finishRender() {}
	virtual void startSelDraw(View*, ivec2) {}
	virtual void drawSelRect(const Widget*, const Recti&, const Recti&) {}
	virtual Widget* finishSelDraw(View* view);
	virtual Texture* texFromEmpty() = 0;					// creates empty texture handle (currently only used for text)
	virtual Texture* texFromIcon(SDL_Surface* img) = 0;		// scales down image to largest possible size
	virtual bool texFromIcon(Texture* tex, SDL_Surface* img) = 0;	// ^ but refills tex and returns true if successful
	virtual Texture* texFromRpic(SDL_Surface* img) = 0;		// image must have been scaled down in advance
	virtual Texture* texFromText(const PixmapRgba& pm) = 0;	// cuts off image if it's too large and uses nearest filter if possible
	virtual bool texFromText(Texture* tex, const PixmapRgba& pm) = 0;	// ^ but refills tex and returns true if successful
	virtual void freeTexture(Texture* tex) noexcept = 0;
	virtual void synchTransfer() {}

	const umap<int, View*>& getViews() const { return views; }
	void setMaxPicRes(uint& size) noexcept;
	SDL_Surface* makeCompatible(SDL_Surface* img, bool rpic) const noexcept;	// converts the image to a format and size that can be directly handed to the graphics driver (must be thread safe)
	static bool isSingleWindow(const umap<int, SDL_Window*>& windows);
protected:
	static SDL_Surface* convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format = SDL_PIXELFORMAT_ABGR8888) noexcept;
	static SDL_Surface* limitSize(SDL_Surface* img, uint32 limit) noexcept;	// scales down the image so that it's width/height fits within the limit
	static Rectf cropTexRect(const Recti& isct, const Recti& rect, uvec2 texRes) noexcept;
	static void recommendPicRamLimit(uintptr_t& mem) noexcept;
};

inline bool Renderer::isSingleWindow(const umap<int, SDL_Window*>& windows) {
	return windows.size() == 1 && windows.begin()->first == singleDspId;
}

class RendererSf final : public Renderer {
private:
	class TextureSf : public Texture {
	private:
		SDL_Surface* srf;

		TextureSf(uvec2 size, SDL_Surface* img) : Texture(size), srf(img) {}

		friend class RendererSf;
	};

	SDL_Surface* curViewSrf;
	ivec2 curViewPos;
	uint32 lastDraw = 0;
	uint32 drawDelay;
	u8vec4 bgColor;

public:
	RendererSf(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererSf() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression compression) override;
	Info getInfo() const override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;
	void finishDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) override;
	Texture* texFromRpic(SDL_Surface* img) override;
	Texture* texFromText(const PixmapRgba& pm) override;
	bool texFromText(Texture* tex, const PixmapRgba& pm) override;
	void freeTexture(Texture* tex) noexcept override;

private:
	void cleanup() noexcept;
	static u8vec4 colorToBytes(const vec4& color);
};

inline u8vec4 RendererSf::colorToBytes(const vec4& color) {
	return glm::round(color * 255.f);
}
