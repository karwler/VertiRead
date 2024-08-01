#pragma once

#ifdef WITH_DIRECT3D
#include "renderer.h"
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d11.h>

class RendererDx11 final : public Renderer {
private:
	class TextureDx : public Texture {
	private:
		ID3D11ShaderResourceView* view;

		TextureDx(uvec2 size, ID3D11ShaderResourceView* textureView) : Texture(size), view(textureView) {}

		friend class RendererDx11;
	};

	struct ViewDx : View {
		IDXGISwapChain* sc = nullptr;
		ID3D11RenderTargetView* tgt = nullptr;

		using View::View;
	};

	struct Pview {
		alignas(16) vec4 pview;
	};

	struct Instance {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
	};

	struct InstanceColor {
		alignas(16) vec4 color;
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
		SDL_Surface* img = nullptr;
		DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
		FormatConv fcid;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, DXGI_FORMAT format) : img(surface), fmt(format) {}
		SurfaceInfo(SDL_Surface* surface, FormatConv convert) : img(surface), fcid(convert) {}
	};

#ifdef NDEBUG
	static constexpr uint deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
	static constexpr uint deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#endif
	static constexpr array<D3D_FEATURE_LEVEL, 3> featureLevels = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	static constexpr uint convWgrpSize = 32;
	static constexpr uint32 convStep = convWgrpSize * 4;	// 4 texels per invocation

	ID3D11Device* dev = nullptr;
	ID3D11DeviceContext* ctx = nullptr;
	ID3D11BlendState* blendState = nullptr;
	ID3D11RasterizerState* rasterizerGui = nullptr;

	ID3D11VertexShader* vertGui = nullptr;
	ID3D11PixelShader* pixlGui = nullptr;
	ID3D11Buffer* pviewBuf = nullptr;
	ID3D11Buffer* instBuf = nullptr;
	ID3D11Buffer* instColorBuf = nullptr;

	array<ID3D11ComputeShader*, eint(FormatConv::index8) + 1> compConv{};
	ID3D11Buffer* offsetBuf = nullptr;
	ID3D11Buffer* colorBuf = nullptr;
	ID3D11Buffer* inputBuf = nullptr;
	ID3D11ShaderResourceView* inputView = nullptr;
	uint inputSize = 0;

	vec4 bgColor;
	uint syncInterval;
	bool canBgra5551;
	bool canBgr565;
	bool canBgra4;

public:
	RendererDx11(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor);
	~RendererDx11() override;

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
	void cleanupConverter() noexcept;
	static IDXGIFactory* createFactory();
	pair<IDXGISwapChain*, ID3D11RenderTargetView*> createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res);
	void recreateSwapchain(IDXGIFactory* factory, ViewDx* view);
	void initShader();
	void initConverter();

	D3D11_MAPPED_SUBRESOURCE mapResource(ID3D11Resource* rsc);
	template <Class T> T* mapBuffer(ID3D11Buffer* buffer);
	ID3D11ShaderResourceView* createTextureDirect(const byte_t* pix, uvec2 res, uint pitch, DXGI_FORMAT format);
	ID3D11ShaderResourceView* createTextureIndirect(const byte_t* pix, uvec2 res, uint8 bpp, uint pitch, const SDL_Palette* palette, FormatConv fcid);
	ID3D11ShaderResourceView* createTextureText(const Pixmap& pm, uvec2 res);
	static void replaceTexture(TextureDx* tex, ID3D11ShaderResourceView* tview, uvec2 res) noexcept;
	void replaceInputBuffer(uint isize);
	SurfaceInfo pickPixFormat(SDL_Surface* img) const noexcept;
	ID3D11Buffer* createConstantBuffer(uint size) const;
	ID3D11Texture2D* createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags = 0, const D3D11_SUBRESOURCE_DATA* subrscData = nullptr) const;
	ID3D11ShaderResourceView* createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format);
	ID3D11ShaderResourceView* createBufferView(ID3D11Buffer* buffer, uint size);
	static tuple<IDXGIAdapter*, size_t, D3D_DRIVER_TYPE> pickAdapter(IDXGIFactory* factory, u32vec2& preferred) noexcept;
	template <Derived<IUnknown> T> static void comRelease(T*& obj) noexcept;
	static string hresultToStr(HRESULT rs);
};

template <Class T>
T* RendererDx11::mapBuffer(ID3D11Buffer* buffer) {
	return static_cast<T*>(mapResource(buffer).pData);
}

inline string RendererDx11::hresultToStr(HRESULT rs) {
	return winErrorMessage(HRESULT_CODE(rs));
}
#endif
