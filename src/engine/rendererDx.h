#pragma once

#ifdef WITH_DIRECTX
#include "renderer.h"
#include <d3d11.h>

class RendererDx : public Renderer {
private:
	class TextureDx : public Texture {
	private:
		ID3D11ShaderResourceView* view;

		TextureDx(ivec2 size, ID3D11ShaderResourceView* textureView);

		friend class RendererDx;
	};

	struct ViewDx : View {
		IDXGISwapChain* sc;
		ID3D11RenderTargetView* tgt;

		ViewDx(SDL_Window* window, const Recti& area, IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer);
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

	struct InstanceAddr {
		alignas(16) uvec2 addr = uvec2(0);
	};

	static inline const std::set<SDL_PixelFormatEnum> supportedFormats = { SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_BGRA32 };

	ID3D11Device* dev = nullptr;
	ID3D11DeviceContext* ctx = nullptr;
	ID3D11BlendState* blendState = nullptr;
	ID3D11SamplerState* sampleState = nullptr;
	ID3D11RasterizerState* rasterizerGui = nullptr;
	ID3D11RasterizerState* rasterizerSel = nullptr;

	ID3D11VertexShader* vertGui = nullptr;
	ID3D11PixelShader* pixlGui = nullptr;
	ID3D11Buffer* pviewBuf = nullptr;
	ID3D11Buffer* instBuf = nullptr;
	ID3D11Buffer* instColorBuf = nullptr;

	ID3D11VertexShader* vertSel = nullptr;
	ID3D11PixelShader* pixlSel = nullptr;
	ID3D11Buffer* instAddrBuf = nullptr;
	ID3D11Texture2D* texAddr = nullptr;
	ID3D11RenderTargetView* tgtAddr = nullptr;
	ID3D11Texture2D* outAddr = nullptr;

	vec4 bgColor;
	uint syncInterval;

public:
	RendererDx(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererDx() final;

	void setClearColor(const vec4& color) final;
	void setVsync(bool vsync) final;
	void updateView(ivec2& viewRes) final;
	void getSettings(uint& maxRes, bool& compression, vector<pair<u32vec2, string>>& devices) const final;
	void setMaxPicRes(uint& size) final;

	void startDraw(View* view) final;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) final;
	void finishDraw(View* view) final;

	void startSelDraw(View* view, ivec2 pos) final;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) final;
	Widget* finishSelDraw(View* view) final;

	Texture* texFromIcon(SDL_Surface* img) final;
	Texture* texFromRpic(SDL_Surface* img) final;
	Texture* texFromText(SDL_Surface* img) final;
	void freeTexture(Texture* tex) final;

protected:
	pair<uint, const std::set<SDL_PixelFormatEnum>*> getLimits() const final;

private:
	static IDXGIFactory* createFactory();
	pair<IDXGISwapChain*, ID3D11RenderTargetView*> createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res);
	void recreateSwapchain(IDXGIFactory* factory, ViewDx* view);
	void initShader();
	ID3D11Buffer* createConstantBuffer(uint size) const;
	ID3D11Texture2D* createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags = 0, const D3D11_SUBRESOURCE_DATA* subrscData = nullptr) const;
	ID3D11ShaderResourceView* createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format);

	template <class T> void uploadBuffer(ID3D11Buffer* buffer, const T& data);
	TextureDx* createTexture(SDL_Surface* img, uvec2 res, DXGI_FORMAT format);
	static pair<SDL_Surface*, DXGI_FORMAT> pickPixFormat(SDL_Surface* img);
	template <class T> static void comRelease(T*& obj);
	static string hresultToStr(HRESULT rs);
};
#endif
