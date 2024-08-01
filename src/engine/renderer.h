#pragma once

#include "utils/settings.h"
#include "utils/stvector.h"

struct Pixmap {
	uptr<uint8[]> pix;
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
		stvector<Settings::Compression, Settings::compressionNames.size()> compressions;
		uint texSize;
		Settings::Compression curCompression;
	};

protected:
	static constexpr array vertices = {
		vec2(0.f, 0.f),
		vec2(1.f, 0.f),
		vec2(0.f, 1.f),
		vec2(1.f, 1.f)
	};

	vector<View*> views;
	uint maxTextureSize;
	uint maxPictureSize;	// should only get accessed from one thread at a time
	Settings::Compression compression;

	Renderer(size_t numViews, uint maxTexRes) : views(numViews), maxTextureSize(maxTexRes) {}
public:
	virtual ~Renderer() = default;

	virtual void setClearColor(const vec4& color) = 0;
	virtual void setVsync(bool vsync) = 0;
	virtual void updateView(ivec2& viewRes) = 0;
	virtual void setCompression(Settings::Compression cmpr) noexcept = 0;
	virtual SDL_Surface* prepareImage(SDL_Surface* img, bool rpic) const noexcept = 0;	// converts the image to a format and size that can be handed to the graphics driver (must be thread safe)
	virtual Info getInfo() const noexcept = 0;
	virtual void startDraw(View* view) = 0;
	virtual void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) = 0;
	virtual void finishDraw(View* view) = 0;
	virtual void finishRender() {}
	virtual Texture* texFromEmpty() = 0;									// creates empty texture handle (currently only used for text)
	virtual Texture* texFromIcon(SDL_Surface* img) noexcept = 0;			// scales down image to largest possible size
	virtual bool texFromIcon(Texture* tex, SDL_Surface* img) noexcept = 0;	// ^ but refills tex and returns true if successful
	virtual Texture* texFromRpic(SDL_Surface* img) noexcept = 0;			// image must have been scaled down in advance
	virtual Texture* texFromText(const Pixmap& pm) noexcept = 0;			// cuts off image if it's too large and uses nearest filter if possible
	virtual bool texFromText(Texture* tex, const Pixmap& pm) noexcept = 0;	// ^ but refills tex and returns true if successful
	virtual void freeTexture(Texture* tex) noexcept = 0;
	virtual void synchTransfer() noexcept {}

	const vector<View*>& getViews() const { return views; }
	View* findView(SDL_Window* win) noexcept;
	View* findView(ivec2 point) noexcept;
	void setMaxPicRes(uint& size) noexcept;
protected:
	static SDL_Surface* convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format = SDL_PIXELFORMAT_ABGR8888) noexcept;
	static SDL_Surface* limitSize(SDL_Surface* img, uint32 limit) noexcept;	// scales down the image so that it's width/height fits within the limit
	static bool isIndexedGrayscale(SDL_Surface* img) noexcept;
	static Rectf cropTexRect(const Recti& isct, const Recti& rect, uvec2 texRes) noexcept;
	static void copyTextPixels(void* dst, const Pixmap& pm, uvec2 res, uint dpitch) noexcept;
	static void copyPalette(uint* dst, const SDL_Palette* palette) noexcept;
	static void recommendPicRamLimit(uintptr_t& mem) noexcept;
};

class RendererSf final : public Renderer {
private:
	class TextureSf : public Texture {
	private:
		SDL_Surface* srf;

		TextureSf(uvec2 size, SDL_Surface* img) : Texture(size), srf(img) {}

		friend class RendererSf;
	};

	SDL_Surface* curViewSrf;
	uint64 lastDraw;
	uint drawDelay;
	ivec2 curViewPos;
	u8vec4 bgColor;

public:
	RendererSf(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor);
	~RendererSf() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression cmpr) noexcept override;
	SDL_Surface* prepareImage(SDL_Surface* img, bool rpic) const noexcept override;
	Info getInfo() const noexcept override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;
	void finishDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) noexcept override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) noexcept override;
	Texture* texFromRpic(SDL_Surface* img) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;

private:
	void cleanup() noexcept;
	static void copyTextPixels(SDL_Surface* img, const Pixmap& pm) noexcept;
	static u8vec4 colorToBytes(const vec4& color);
};

inline u8vec4 RendererSf::colorToBytes(const vec4& color) {
	return glm::round(color * 255.f);
}
