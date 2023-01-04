#ifdef WITH_DIRECTX
#include "rendererDx.h"
#include "utils/settings.h"
#include <glm/gtc/type_ptr.hpp>
#include <comdef.h>
#include <wrl/client.h>
#include <SDL_syswm.h>

using Microsoft::WRL::ComPtr;

RendererDx::TextureDx::TextureDx(ivec2 size, ID3D11ShaderResourceView* textureView) :
	Texture(size),
	view(textureView)
{}

RendererDx::ViewDx::ViewDx(SDL_Window* window, const Recti& area, IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer) :
	View(window, area),
	sc(swapchain),
	tgt(backbuffer)
{}

RendererDx::RendererDx(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	bgColor(bgcolor),
	syncInterval(sets->vsync)
{
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIFactory> factory = createFactory();
	if (sets->device != u32vec2(0)) {
		for (uint i = 0; factory->EnumAdapters(i, &adapter) == S_OK; ++i) {
			if (DXGI_ADAPTER_DESC desc; SUCCEEDED(adapter->GetDesc(&desc)) && desc.VendorId == sets->device.x && desc.DeviceId == sets->device.y)
				break;
			adapter.Reset();
		}
		if (!adapter)
			sets->device = u32vec2(0);
	}
#ifdef NDEBUG
	uint flags = 0;
#else
	uint flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	if (HRESULT rs = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	if (windows.size() == 1 && windows.begin()->first == singleDspId) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
		SDL_GetWindowSizeInPixels(windows.begin()->second, &viewRes.x, &viewRes.y);
#else
		SDL_GetWindowSize(windows.begin()->second, &viewRes.x, &viewRes.y);
#endif
		auto [swapchain, backbuffer] = createSwapchain(factory.Get(), windows.begin()->second, viewRes);
		views.emplace(singleDspId, new ViewDx(windows.begin()->second, Recti(ivec2(0), viewRes), swapchain, backbuffer));
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			Recti wrect = sets->displays.at(id).translate(-origin);
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(win, &wrect.w, &wrect.h);
#else
			SDL_GetWindowSize(win, &wrect.w, &wrect.h);
#endif
			auto [swapchain, backbuffer] = createSwapchain(factory.Get(), win, wrect.size());
			views.emplace(id, new ViewDx(win, wrect, swapchain, backbuffer));
			viewRes = glm::max(viewRes, wrect.end());
		}
	}

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (HRESULT rs = dev->CreateBlendState(&blendDesc, &blendState); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerGui); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->RSSetState(rasterizerGui);

	rasterizerDesc.ScissorEnable = TRUE;
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerSel); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	D3D11_RECT scissor = { 0, 0, 1, 1 };
	ctx->RSSetScissorRects(1, &scissor);

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (HRESULT rs = dev->CreateSamplerState(&samplerDesc, &sampleState); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->PSSetSamplers(0, 1, &sampleState);

	initShader();
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

RendererDx::~RendererDx() {
	if (outAddr)
		outAddr->Release();
	if (tgtAddr)
		tgtAddr->Release();
	if (texAddr)
		texAddr->Release();
	if (instAddrBuf)
		instAddrBuf->Release();
	if (instColorBuf)
		instColorBuf->Release();
	if (instBuf)
		instBuf->Release();
	if (pviewBuf)
		pviewBuf->Release();
	if (pixlSel)
		pixlSel->Release();
	if (vertSel)
		vertSel->Release();
	if (pixlGui)
		pixlGui->Release();
	if (vertGui)
		vertGui->Release();
	for (auto [id, view] : views) {
		ViewDx* vw = static_cast<ViewDx*>(view);
		if (vw->tgt)
			vw->tgt->Release();
		if (vw->sc)
			vw->sc->Release();
		delete vw;
	}
	if (sampleState)
		sampleState->Release();
	if (rasterizerSel)
		rasterizerSel->Release();
	if (rasterizerGui)
		rasterizerGui->Release();
	if (blendState)
		blendState->Release();
	if (ctx)
		ctx->Release();
	if (dev)
		dev->Release();
}

IDXGIFactory* RendererDx::createFactory() {
	IDXGIFactory* factory;
	if (HRESULT rs = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	return factory;
}

pair<IDXGISwapChain*, ID3D11RenderTargetView*> RendererDx::createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(win, &wmInfo))
		throw std::runtime_error(SDL_GetError());

	DXGI_SWAP_CHAIN_DESC schainDesc{};
	schainDesc.BufferCount = 2;
	schainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	schainDesc.BufferDesc.Width = res.x;
	schainDesc.BufferDesc.Height = res.y;
	schainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	schainDesc.OutputWindow = wmInfo.info.win.window;
	schainDesc.SampleDesc.Count = 1;
	schainDesc.Windowed = TRUE;
	schainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	ComPtr<IDXGISwapChain> swapchain;
	if (HRESULT rs = factory->CreateSwapChain(dev, &schainDesc, &swapchain); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ComPtr<ID3D11Texture2D> scBackBuffer;
	if (HRESULT rs = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), &scBackBuffer); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ComPtr<ID3D11RenderTargetView> tgtBackbuffer;
	if (HRESULT rs = dev->CreateRenderTargetView(scBackBuffer.Get(), nullptr, &tgtBackbuffer); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	return pair(swapchain.Detach(), tgtBackbuffer.Detach());
}

void RendererDx::recreateSwapchain(IDXGIFactory* factory, ViewDx* view) {
	view->tgt->Release();
	view->tgt = nullptr;
	view->sc->Release();
	view->sc = nullptr;
	std::tie(view->sc, view->tgt) = createSwapchain(factory, view->win, view->rect.size());
}

void RendererDx::initShader() {
	const uint8 vertSrcGui[] = {
#ifdef NDEBUG
#include "shaders/dx.gui.vert.rel.h"
#else
#include "shaders/dx.gui.vert.dbg.h"
#endif
	};
	const uint8 pixlSrcGui[] = {
#ifdef NDEBUG
#include "shaders/dx.gui.pixl.rel.h"
#else
#include "shaders/dx.gui.pixl.dbg.h"
#endif
	};
	if (HRESULT rs = dev->CreateVertexShader(vertSrcGui, sizeof(vertSrcGui), nullptr, &vertGui); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	if (HRESULT rs = dev->CreatePixelShader(pixlSrcGui, sizeof(pixlSrcGui), nullptr, &pixlGui); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);

	const uint8 vertSrcSel[] = {
#ifdef NDEBUG
#include "shaders/dx.sel.vert.rel.h"
#else
#include "shaders/dx.sel.vert.dbg.h"
#endif
	};
	const uint8 pixlSrcSel[] = {
#ifdef NDEBUG
#include "shaders/dx.sel.pixl.rel.h"
#else
#include "shaders/dx.sel.pixl.dbg.h"
#endif
	};
	if (HRESULT rs = dev->CreateVertexShader(vertSrcSel, sizeof(vertSrcSel), nullptr, &vertSel); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	if (HRESULT rs = dev->CreatePixelShader(pixlSrcSel, sizeof(pixlSrcSel), nullptr, &pixlSel); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(Pview);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &pviewBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	bufferDesc.ByteWidth = sizeof(Instance);
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &instBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	array<ID3D11Buffer*, 2> vsBuffers = { pviewBuf, instBuf };
	ctx->VSSetConstantBuffers(0, vsBuffers.size(), vsBuffers.data());

	bufferDesc.ByteWidth = sizeof(InstanceColor);
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &instColorBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	bufferDesc.ByteWidth = sizeof(InstanceAddr);
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &instAddrBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	array<ID3D11Buffer*, 2> psBuffers = { instColorBuf, instAddrBuf };
	ctx->PSSetConstantBuffers(0, psBuffers.size(), psBuffers.data());

	D3D11_TEXTURE2D_DESC texDesc{};
	texDesc.Width = 1;
	texDesc.Height = 1;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32_UINT;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	if (HRESULT rs = dev->CreateTexture2D(&texDesc, nullptr, &texAddr); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	D3D11_RENDER_TARGET_VIEW_DESC tgtDesc{};
	tgtDesc.Format = texDesc.Format;
	tgtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	if (HRESULT rs = dev->CreateRenderTargetView(texAddr, &tgtDesc, &tgtAddr); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if (HRESULT rs = dev->CreateTexture2D(&texDesc, nullptr, &outAddr); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
}

void RendererDx::setClearColor(const vec4& color) {
	bgColor = color;
}

void RendererDx::setVsync(bool vsync) {
	syncInterval = vsync;
	ComPtr<IDXGIFactory> factory = createFactory();
	for (auto [id, view] : views)
		recreateSwapchain(factory.Get(), static_cast<ViewDx*>(view));
}

void RendererDx::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		ComPtr<IDXGIFactory> factory = createFactory();
		SDL_GetWindowSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;
		recreateSwapchain(factory.Get(), static_cast<ViewDx*>(views.begin()->second));
	}
}

void RendererDx::startDraw(View* view) {
	D3D11_VIEWPORT viewport{};
	viewport.Width = float(view->rect.w);
	viewport.Height = float(view->rect.h);

	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, &static_cast<ViewDx*>(view)->tgt, nullptr);
	ctx->ClearRenderTargetView(static_cast<ViewDx*>(view)->tgt, glm::value_ptr(bgColor));
	uploadBuffer(pviewBuf, Pview{ vec4(view->rect.pos(), vec2(view->rect.size()) / 2.f) });
}

void RendererDx::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	uploadBuffer(instBuf, Instance{ rect.toVec(), frame.toVec() });
	uploadBuffer(instColorBuf, InstanceColor{ color });
	ctx->PSSetShaderResources(0, 1, &static_cast<const TextureDx*>(tex)->view);
	ctx->Draw(4, 0);
}

void RendererDx::finishDraw(View* view) {
	static_cast<ViewDx*>(view)->sc->Present(syncInterval, 0);
}

void RendererDx::startSelDraw(View* view, ivec2 pos) {
	float zero[4] = { 0.f, 0.f, 0.f, 0.f };
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = float(-pos.x);
	viewport.TopLeftY = float(-pos.y);
	viewport.Width = float(view->rect.w);
	viewport.Height = float(view->rect.h);

	ctx->VSSetShader(vertSel, nullptr, 0);
	ctx->PSSetShader(pixlSel, nullptr, 0);
	ctx->RSSetState(rasterizerSel);
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	ctx->OMSetRenderTargets(1, &tgtAddr, nullptr);
	ctx->ClearRenderTargetView(tgtAddr, zero);
	uploadBuffer(pviewBuf, Pview{ vec4(view->rect.pos(), vec2(view->rect.size()) / 2.f) });
}

void RendererDx::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	uploadBuffer(instBuf, Instance{ rect.toVec(), frame.toVec() });
	uploadBuffer(instAddrBuf, InstanceAddr{ uvec2(uptrt(wgt), uptrt(wgt) >> 32) });
	ctx->Draw(4, 0);
}

Widget* RendererDx::finishSelDraw(View*) {
	ctx->CopyResource(outAddr, texAddr);
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(outAddr, 0, D3D11_MAP_READ, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	uvec2 val;
	memcpy(&val, mapRsc.pData, sizeof(uvec2));
	ctx->Unmap(outAddr, 0);

	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);
	ctx->RSSetState(rasterizerGui);
	ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
	return reinterpret_cast<Widget*>(uptrt(val.x) | (uptrt(val.y) << 32));
}

template <class T>
void RendererDx::uploadBuffer(ID3D11Buffer* buffer, const T& data) {
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	memcpy(mapRsc.pData, &data, sizeof(T));
	ctx->Unmap(buffer, 0);
}

vector<pair<sizet, Texture*>> RendererDx::initIconTextures(vector<pair<sizet, SDL_Surface*>>&& iconImp) {
	vector<pair<sizet, Texture*>> ictx;
	ictx.reserve(iconImp.size() + 1);
	if (u8vec4 blank(UINT8_MAX); TextureDx* tex = createTexture(array<u8vec4, 4>{ blank, blank, blank, blank }.data(), uvec2(2), sizeof(uint8) * 8, DXGI_FORMAT_B8G8R8A8_UNORM))
		ictx.emplace_back(iconImp.size(), tex);
	massLoadTextures(std::move(iconImp), ictx);
	return ictx;
}

vector<pair<sizet, Texture*>> RendererDx::initRpicTextures(vector<pair<sizet, SDL_Surface*>>&& rpicImp) {
	vector<pair<sizet, Texture*>> ictx;
	ictx.reserve(rpicImp.size());
	massLoadTextures(std::move(rpicImp), ictx);
	return ictx;
}

void RendererDx::massLoadTextures(vector<pair<sizet, SDL_Surface*>>&& pics, vector<pair<sizet, Texture*>>& ictx) {
	for (auto [id, pic] : pics)
		if (auto [img, fmt] = pickPixFormat(pic); img) {
			ictx.emplace_back(id, createTexture(img->pixels, uvec2(img->w, img->h), img->pitch, fmt));
			SDL_FreeSurface(img);
		}
}

Texture* RendererDx::texFromText(SDL_Surface* img) {
	if (img) {
		TextureDx* tex = createTexture(img->pixels, uvec2(img->w, img->h), img->pitch, DXGI_FORMAT_B8G8R8A8_UNORM);
		SDL_FreeSurface(img);
		return tex;
	}
	return nullptr;
}

void RendererDx::freeIconTextures(umap<string, Texture*>& texes) {
	for (auto& [name, tex] : texes) {
		static_cast<TextureDx*>(tex)->view->Release();
		delete tex;
	}
}

void RendererDx::freeRpicTextures(vector<pair<string, Texture*>>&& texes) {
	for (auto& [name, tex] : texes) {
		static_cast<TextureDx*>(tex)->view->Release();
		delete tex;
	}
}

void RendererDx::freeTextTexture(Texture* tex) {
	static_cast<TextureDx*>(tex)->view->Release();
	delete tex;
}

RendererDx::TextureDx* RendererDx::createTexture(const void* pixels, uvec2 res, uint pitch, DXGI_FORMAT format) {
	try {
		D3D11_TEXTURE2D_DESC texDesc{};
		texDesc.Width = res.x;
		texDesc.Height = res.y;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_IMMUTABLE;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA subrscData{};
		subrscData.pSysMem = pixels;
		subrscData.SysMemPitch = pitch;

		ComPtr<ID3D11Texture2D> texture;
		if (HRESULT rs = dev->CreateTexture2D(&texDesc, &subrscData, &texture); FAILED(rs))
			throw std::runtime_error(hresultToStr(rs));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

		ComPtr<ID3D11ShaderResourceView> textureView;
		if (HRESULT rs = dev->CreateShaderResourceView(texture.Get(), &srvDesc, &textureView); FAILED(rs))
			throw std::runtime_error(hresultToStr(rs));
		return new TextureDx(res, textureView.Detach());
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
	return nullptr;
}

pair<SDL_Surface*, DXGI_FORMAT> RendererDx::pickPixFormat(SDL_Surface* img) {
	if (img) {
		switch (img->format->format) {
		case SDL_PIXELFORMAT_BGRA32:
			return pair(img, DXGI_FORMAT_B8G8R8A8_UNORM);
		case SDL_PIXELFORMAT_RGBA32:
			return pair(img, DXGI_FORMAT_R8G8B8A8_UNORM);
		}
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_BGRA32, 0);
		SDL_FreeSurface(img);
		img = dst;
	}
	return pair(img, DXGI_FORMAT_B8G8R8A8_UNORM);
}

void RendererDx::getAdditionalSettings(bool& compression, vector<pair<u32vec2, string>>& devices) {
	compression = false;
	devices = { pair(u32vec2(0), "auto")};

	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIFactory> factory = createFactory();
	for (uint i = 0; factory->EnumAdapters(i, &adapter) == S_OK; ++i) {
		if (DXGI_ADAPTER_DESC desc; SUCCEEDED(adapter->GetDesc(&desc)))
			devices.emplace_back(u32vec2(desc.VendorId, desc.DeviceId), swtos(desc.Description));
		adapter.Reset();
	}
}

string RendererDx::hresultToStr(HRESULT rs) {
	_com_error err(rs);
#if TCHAR == WCHAR
	return swtos(err.ErrorMessage());
#else
	return err.ErrorMessage();
#endif
}
#endif
