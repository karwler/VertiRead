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

	struct InstanceAddr {
		alignas(16) uvec2 addr = uvec2(0);
	};

	ID3D11Device* dev = nullptr;
	ID3D11DeviceContext* ctx = nullptr;
	ID3D11BlendState* blendState = nullptr;
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
	ID3D11Texture2D* outAddr = nullptr;
	ID3D11RenderTargetView* tgtAddr = nullptr;

	vec4 bgColor;
	uint syncInterval;
	bool canBgra5551;
	bool canBgr565;
	bool canBgra4;

public:
	RendererDx11(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererDx11() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression compression) override;
	Info getInfo() const override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;
	void finishDraw(View* view) override;

	void startSelDraw(View* view, ivec2 pos) override;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) override;
	Widget* finishSelDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) override;
	Texture* texFromRpic(SDL_Surface* img) override;
	Texture* texFromText(const PixmapRgba& pm) override;
	bool texFromText(Texture* tex, const PixmapRgba& pm) override;
	void freeTexture(Texture* tex) override;

private:
	static IDXGIFactory* createFactory();
	pair<IDXGISwapChain*, ID3D11RenderTargetView*> createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res);
	void recreateSwapchain(IDXGIFactory* factory, ViewDx* view);
	void initShader();
	ID3D11Buffer* createConstantBuffer(uint size) const;
	ID3D11Texture2D* createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags = 0, const D3D11_SUBRESOURCE_DATA* subrscData = nullptr) const;
	ID3D11ShaderResourceView* createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format);

	template <Class T> void uploadBuffer(ID3D11Buffer* buffer, const T& data);
	ID3D11ShaderResourceView* createTexture(const byte_t* pix, uvec2 res, uint pitch, DXGI_FORMAT format);
	static void replaceTexture(TextureDx* tex, ID3D11ShaderResourceView* tview, uvec2 res);
	pair<SDL_Surface*, DXGI_FORMAT> pickPixFormat(SDL_Surface* img) const;
	template <Derived<IUnknown> T> static void comRelease(T*& obj);
	static string hresultToStr(HRESULT rs);
};

inline string RendererDx11::hresultToStr(HRESULT rs) {
	return winErrorMessage(HRESULT_CODE(rs));
}
#endif
