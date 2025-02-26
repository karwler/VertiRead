#pragma once

#ifdef WITH_DIRECT3D
#include "renderer.h"
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class RendererDx11 final : public Renderer {
private:
	enum class OptTexFmt : uint8 {
		R5G6B5,
		A1R5G5B5,
		A4R4G4B4
	};
	static constexpr array optTexFmtsSdl = {
		SDL_PIXELFORMAT_RGB565,
		SDL_PIXELFORMAT_ARGB1555,
		SDL_PIXELFORMAT_ARGB4444
	};

	class TextureDx : public Texture {
	private:
		ComPtr<ID3D11ShaderResourceView> view;

		using Texture::Texture;

		friend class RendererDx11;
	};

	struct ViewDx : View {
		ComPtr<IDXGISwapChain> sc;
		array<ComPtr<ID3D11RenderTargetView>, 2> tgts;
		ComPtr<ID3D11ShaderResourceView> view;	// for when doing post-processing

		using View::View;

		void reset();
	};

	struct ViewPview {
		alignas(16) vec4 pview;
	};

	struct GlobalColors {
		alignas(16) vec4 colors[Settings::defaultColors.size() - 1];
	};

	struct InstanceRect {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
	};

	struct InstanceColor {
		alignas(16) uint colorId;
	};

	struct FinalData {
		alignas(16) float gamma;
	};

	struct Offset {
		alignas(16) uint offset;
	};

	struct Palette {
		alignas(16) uint colors[256];
	};

	enum class FormatConv : uint8 {
		rgb24,
		bgr24,
		red,
		index8
	};

	struct SurfaceInfo {
		uptr<SDL_Surface> img;
		DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
		FormatConv fcid;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, DXGI_FORMAT format) noexcept : img(surface), fmt(format) {}
		SurfaceInfo(SDL_Surface* surface, FormatConv convert) noexcept : img(surface), fcid(convert) {}
	};

#ifdef NDEBUG
	static constexpr uint deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
	static constexpr uint deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#endif
	static constexpr array<D3D_FEATURE_LEVEL, 4> featureLevels = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	static constexpr uint finBufferSlot = 2;
	static constexpr uint convWgrpSize = 32;
	static constexpr uint32 convStep = convWgrpSize * 4;	// 4 texels per invocation

	static constexpr array scrVertices = {
		ScreenVertex(vec2(-1.f, 1.f), vec2(0.f, 0.f)),
		ScreenVertex(vec2(1.f, 1.f), vec2(1.f, 0.f)),
		ScreenVertex(vec2(-1.f, -1.f), vec2(0.f, 1.f)),
		ScreenVertex(vec2(1.f, -1.f), vec2(1.f, 1.f))
	};

	ComPtr<ID3D11Device> dev;
	ComPtr<ID3D11DeviceContext> ctx;
	ComPtr<ID3D11BlendState> blendState;

	ComPtr<ID3D11VertexShader> vertGui;
	ComPtr<ID3D11PixelShader> pixlGui;
	ComPtr<ID3D11InputLayout> vertexLayoutGui;
	ComPtr<ID3D11Buffer> vertexBufGui;
	ComPtr<ID3D11Buffer> pviewBuf;
	ComPtr<ID3D11Buffer> colorBuf;
	ComPtr<ID3D11Buffer> instRectBuf;
	ComPtr<ID3D11Buffer> instColorBuf;

	ComPtr<ID3D11VertexShader> vertFin;
	ComPtr<ID3D11PixelShader> pixlFin;
	ComPtr<ID3D11InputLayout> vertexLayoutFin;
	ComPtr<ID3D11Buffer> vertexBufFin;
	ComPtr<ID3D11Buffer> finBuf;

	array<ComPtr<ID3D11ComputeShader>, eint(FormatConv::index8) + 1> compConv;
	ComPtr<ID3D11Buffer> offsetBuf;
	ComPtr<ID3D11Buffer> paletteBuf;
	ComPtr<ID3D11Buffer> inputBuf;
	ComPtr<ID3D11ShaderResourceView> inputView;
	PixmapColor textBuffer;
	uint inputSize = 0;

	vec4 bgColor;
	uint syncInterval;
	array<bool, optTexFmtsSdl.size()> optionalFormats;
	bool usesSrgb;

public:
	RendererDx11(InitParams& initParams, Settings* sets);
	~RendererDx11() override;

	void setColors(array<vec4, Settings::defaultColors.size()>& colors) override;
	bool setSettings(Settings* sets) override;
	void setGammaValue(int gamma) override;
	bool updateView(ivec2& viewRes) override;
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
	static ComPtr<IDXGIFactory1> createFactory();
	void initGuiShader();
	void initFinShader(Settings* sets) noexcept;
	void cleanupFinShader() noexcept;
	void initConverter() noexcept;
	void cleanupConverter() noexcept;
	void createSwapchain(IDXGIFactory1* factory, ViewDx* view);
	void createRenderTargets(ViewDx* view);
	void setCompression(Settings* sets) noexcept;

	D3D11_MAPPED_SUBRESOURCE mapResource(ID3D11Resource* rsc);
	template <Class T> T* mapBuffer(ID3D11Buffer* buffer);
	ComPtr<ID3D11ShaderResourceView> createTextureDirect(const void* pix, uvec2 res, uint pitch, DXGI_FORMAT format);
	ComPtr<ID3D11ShaderResourceView> createTextureIndirect(const void* pix, uvec2 res, uint pitch, uint8 bpp, const SDL_Palette* palette, FormatConv fcid, DXGI_FORMAT format);
	void replaceInputBuffer(uint isize);
	SurfaceInfo pickPixFormat(SDL_Surface* img, bool srgb) const noexcept;
	pair<SDL_PixelFormatEnum, uint8> pickImageFormat(std::initializer_list<OptTexFmt> fmtv, SDL_PixelFormatEnum orig) const noexcept;
	bool canTexturesB16() const noexcept;
	ComPtr<ID3D11Buffer> createConstantBuffer(uint size) const;
	ComPtr<ID3D11Texture2D> createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags = 0, const D3D11_SUBRESOURCE_DATA* subrscData = nullptr) const;
	ComPtr<ID3D11ShaderResourceView> createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format);
	ComPtr<ID3D11ShaderResourceView> createBufferView(ID3D11Buffer* buffer, uint size);
	static tuple<ComPtr<IDXGIAdapter1>, size_t, D3D_DRIVER_TYPE> pickAdapter(IDXGIFactory1* factory, u32vec2& preferred) noexcept;
	static string hresultToStr(HRESULT rs) noexcept;
};

template <Class T>
T* RendererDx11::mapBuffer(ID3D11Buffer* buffer) {
	return static_cast<T*>(mapResource(buffer).pData);
}

inline bool RendererDx11::canTexturesB16() const noexcept {
	return optionalFormats[eint(OptTexFmt::R5G6B5)] || optionalFormats[eint(OptTexFmt::A1R5G5B5)];
}

inline string RendererDx11::hresultToStr(HRESULT rs) noexcept {
	return winErrorMessage(HRESULT_CODE(rs));
}
#endif
