#pragma once

#include "utils/settings.h"
#include "utils/stvector.h"
#include <SDL_render.h>
#include <set>

struct Pixmap {
	uptr<uint8[]> pix;
	uvec2 res;
};

class Texture {
protected:
	uvec2 res;

	Texture(uvec2 size) noexcept : res(size) {}

public:
	uvec2 getRes() const noexcept { return res; }
};

class Renderer {
public:
	static constexpr int singleDspId = -1;

	struct View {
		SDL_Window* win;
		Recti rect;

		View(SDL_Window* window, const Recti& area) noexcept : win(window), rect(area) {}
	};

	struct InitParams {
		const vector<SDL_Window*>& windows;
		const ivec2* vofs;
		ivec2& viewRes;
		Texture*& tooltipTexture;
		array<vec4, Settings::defaultColors.size()> colors;
	};

	struct Info {
		struct Device {
			u32vec2 id;
			Cstring name;
			uintptr_t dmem;

			Device(u32vec2 vendev, Cstring&& devname, uintptr_t memory = 0) noexcept;
		};

		vector<Device> devices;
		stvector<Settings::Gamma, Settings::gammaNames.size()> gamma;
		stvector<Settings::Compression, Settings::compressionNames.size()> compressions;
		uint texSize;
		bool srgbNeedsWindowRecreate;
		Settings::Gamma curGamma;
		Settings::Compression curCompression;
	};

	enum class Action : int8 {
		skip = -1,
		no,
		yes
	};

protected:
	struct ScreenVertex {
		vec2 pos, tuv;

		constexpr ScreenVertex(vec2 dp, vec2 uv) noexcept : pos(dp), tuv(uv) {}
	};

	struct PixmapColor {
		uptr<uint32[]> pix;
		size_t len = 0;

		uint32* fromText(const Pixmap& pm, uvec2 res);
	};

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

	Renderer(size_t numViews, uint maxTexRes) noexcept : views(numViews), maxTextureSize(maxTexRes) {}
public:
	virtual ~Renderer() = default;

	virtual void setColors(array<vec4, Settings::defaultColors.size()>& colors) = 0;
	virtual bool setSettings(Settings* sets) = 0;	// returns whether the color palette needs to be reloaded
	virtual void setGammaValue(int) {}
	virtual void updateView(ivec2& viewRes) = 0;
	virtual Info getInfo() const noexcept = 0;
	virtual Action startDraw(View* view) noexcept = 0;
	virtual void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept = 0;
	virtual Action finishDraw(View* view) noexcept = 0;
	virtual Action finishRender() noexcept;
	virtual Texture* texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept = 0;	// scales down image to largest possible size
	virtual bool texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept = 0;	// ^ but refills tex and returns true if successful
	virtual Texture* texFromText(const Pixmap& pm) noexcept = 0;			// cuts off image if it's too large and uses nearest filter if possible
	virtual bool texFromText(Texture* tex, const Pixmap& pm) noexcept = 0;	// ^ but refills tex and returns true if successful
	virtual void freeTexture(Texture* tex) noexcept = 0;
	virtual void waitIdle() noexcept {}

	const vector<View*>& getViews() const noexcept { return views; }
	View* findView(SDL_Window* win) noexcept;
	View* findView(ivec2 point) noexcept;
	void setMaxPicRes(uint& size) noexcept;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	static void copyPalette(SDL_Surface* dst, SDL_Surface* src) noexcept;	// SDL3 can't blit indexed surfaces anymore without manually setting the palette first (dst and src should both be indexed)
#endif

	SDL_Surface* prepareImage(SDL_Surface* img, uint8* rpbpp = nullptr) const noexcept;	// converts the image to a format and size that can be handed to the graphics driver (must be thread safe)
protected:
	virtual pair<SDL_PixelFormatEnum, uint8> prepareImageFormat(SDL_Surface* img) const noexcept = 0;

	static SDL_Surface* convertReplace(SDL_Surface* img, SDL_PixelFormatEnum format = SDL_PIXELFORMAT_ABGR8888) noexcept;
	static SDL_Surface* limitSize(SDL_Surface* img, uint limit) noexcept;	// scales down the image so that it's width/height fits within the limit
	static bool isIndexedGrayscale(SDL_Surface* img) noexcept;
	static Rectf cropTexRect(const Recti& isct, const Recti& rect, uvec2 texRes) noexcept;
	static void copyPalette(uint* dst, const SDL_Palette* palette) noexcept;
	static void recommendPicRamLimit(uintptr_t& mem) noexcept;
	static void convertColors(vec4* vecv, size_t num, bool srgb, bool gamma22) noexcept;
private:
	static double srgb2linear(double x) noexcept;
};

class RendererSf final : public Renderer {
private:
	class TextureSf : public Texture {
	private:
		SDL_Texture* tex;

		TextureSf(uvec2 size, SDL_Texture* texture) : Texture(size), tex(texture) {}

		friend class RendererSf;
	};

	struct ViewSf : View {
		SDL_Renderer* renderer = nullptr;

		using View::View;
	};

	ViewSf* curView;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	array<vec4, Settings::defaultColors.size() - 1> rectColors;
#else
	array<u8vec4, Settings::defaultColors.size() - 1> rectColors;
#endif
	PixmapColor textBuffer;
	std::set<SDL_PixelFormatEnum> textureFormats;
	SDL_PixelFormatEnum defaultFormat = SDL_PIXELFORMAT_ABGR8888;

public:
	RendererSf(InitParams& initParams, Settings* sets);
	~RendererSf() override;

	void setColors(array<vec4, Settings::defaultColors.size()>& colors) override;
	bool setSettings(Settings* sets) override;
	void updateView(ivec2& viewRes) override;
	Info getInfo() const noexcept override;

	Action startDraw(View* view) noexcept override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept override;
	Action finishDraw(View* view) noexcept override;

	Texture* texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept override;
	bool texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;

protected:
	pair<SDL_PixelFormatEnum, uint8> prepareImageFormat(SDL_Surface* img) const noexcept override;

private:
	void cleanup() noexcept;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	void createRenderer(ViewSf* view, SDL_PropertiesID props);
#else
	void createRenderer(ViewSf* view, SDL_RendererFlags flags);
#endif
	void setCompression(Settings* sets) noexcept;
	static void replaceTexture(TextureSf* tex, SDL_Texture* ntex, uvec2 res) noexcept;
	pair<SDL_Texture*, uvec2> createTexture(SDL_Surface* img, bool linear) noexcept;
	pair<SDL_Texture*, uvec2> createTextureText(const Pixmap& pm) noexcept;
	pair<SDL_PixelFormatEnum, uint8> pickImageFormat(std::initializer_list<SDL_PixelFormatEnum> fmtv, SDL_PixelFormatEnum orig) const noexcept;
	bool canTexturesB16() const noexcept;
#if !SDL_VERSION_ATLEAST(3, 2, 0)
	static u8vec4 colorToBytes(const vec4& color) noexcept;
#endif
};

inline bool RendererSf::canTexturesB16() const noexcept {
	return rng::any_of(textureFormats, [](SDL_PixelFormatEnum it) -> bool { return it == SDL_PIXELFORMAT_BGR565 || it == SDL_PIXELFORMAT_RGB565 || it == SDL_PIXELFORMAT_ABGR1555 || it == SDL_PIXELFORMAT_ARGB1555 || it == SDL_PIXELFORMAT_BGRA5551 || it == SDL_PIXELFORMAT_RGBA5551 || it == SDL_PIXELFORMAT_XBGR1555 || it == SDL_PIXELFORMAT_XRGB1555; });
}

#if !SDL_VERSION_ATLEAST(3, 2, 0)
inline u8vec4 RendererSf::colorToBytes(const vec4& color) noexcept {
	return glm::round(color * 255.f);
}
#endif
