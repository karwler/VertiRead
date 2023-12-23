#ifdef WITH_DIRECTX
#include "rendererDx.h"
#include <comdef.h>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_syswm.h>

RendererDx::ViewDx::ViewDx(SDL_Window* window, const Recti& area, IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer) :
	View(window, area),
	sc(swapchain),
	tgt(backbuffer)
{}

RendererDx::RendererDx(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	Renderer({ SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_BGRA32, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_BGR565 }),
	bgColor(bgcolor),
	syncInterval(sets->vsync),
	squashPicTexels(sets->compression == Settings::Compression::b16)
{
	IDXGIFactory* factory = createFactory();
	IDXGIAdapter* adapter = nullptr;
	try {
		if (sets->device != u32vec2(0)) {
			for (uint i = 0; factory->EnumAdapters(i, &adapter) == S_OK; ++i) {
				if (DXGI_ADAPTER_DESC desc; SUCCEEDED(adapter->GetDesc(&desc)) && desc.VendorId == sets->device.x && desc.DeviceId == sets->device.y)
					break;
				comRelease(adapter);
			}
			if (!adapter)
				sets->device = u32vec2(0);
		}
#ifdef NDEBUG
		uint flags = 0;
#else
		uint flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
		if (HRESULT rs = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create device: {}", hresultToStr(rs)));
		comRelease(adapter);

		if (isSingleWindow(windows)) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(windows.begin()->second, &viewRes.x, &viewRes.y);
#else
			SDL_GetWindowSize(windows.begin()->second, &viewRes.x, &viewRes.y);
#endif
			auto [swapchain, backbuffer] = createSwapchain(factory, windows.begin()->second, viewRes);
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
				auto [swapchain, backbuffer] = createSwapchain(factory, win, wrect.size());
				views.emplace(id, new ViewDx(win, wrect, swapchain, backbuffer));
				viewRes = glm::max(viewRes, wrect.end());
			}
		}
		comRelease(factory);
	} catch (const std::runtime_error&) {
		comRelease(factory);
		comRelease(adapter);
		throw;
	}

	D3D11_BLEND_DESC blendDesc = {
		.RenderTarget = { {
			.BlendEnable = TRUE,
			.SrcBlend = D3D11_BLEND_SRC_ALPHA,
			.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
			.BlendOp = D3D11_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D11_BLEND_ONE,
			.DestBlendAlpha = D3D11_BLEND_ZERO,
			.BlendOpAlpha = D3D11_BLEND_OP_ADD,
			.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
		} }
	};
	if (HRESULT rs = dev->CreateBlendState(&blendDesc, &blendState); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create blend state: {}", hresultToStr(rs)));
	ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);

	D3D11_RASTERIZER_DESC rasterizerDesc = {
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_BACK,
		.FrontCounterClockwise = FALSE,
		.DepthClipEnable = TRUE,
		.ScissorEnable = FALSE
	};
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerGui); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create graphics rasterizer: {}", hresultToStr(rs)));
	ctx->RSSetState(rasterizerGui);

	rasterizerDesc.ScissorEnable = TRUE;
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerSel); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create address rasterizer: {}", hresultToStr(rs)));
	D3D11_RECT scissor = { .left = 0, .top = 0, .right = 1, .bottom = 1 };
	ctx->RSSetScissorRects(1, &scissor);

	D3D11_SAMPLER_DESC samplerDesc = {
		.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
		.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
		.MaxAnisotropy = 1,
		.ComparisonFunc = D3D11_COMPARISON_ALWAYS,
		.MaxLOD = D3D11_FLOAT32_MAX
	};
	if (HRESULT rs = dev->CreateSamplerState(&samplerDesc, &sampleState); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create sampler state: {}", hresultToStr(rs)));
	ctx->PSSetSamplers(0, 1, &sampleState);

	initShader();
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	setMaxPicRes(sets->maxPicRes);
}

RendererDx::~RendererDx() {
	comRelease(outAddr);
	comRelease(tgtAddr);
	comRelease(texAddr);
	comRelease(instAddrBuf);
	comRelease(instColorBuf);
	comRelease(instBuf);
	comRelease(pviewBuf);
	comRelease(pixlSel);
	comRelease(vertSel);
	comRelease(pixlGui);
	comRelease(vertGui);
	for (auto [id, view] : views) {
		auto vw = static_cast<ViewDx*>(view);
		comRelease(vw->tgt);
		comRelease(vw->sc);
		delete vw;
	}
	comRelease(sampleState);
	comRelease(rasterizerSel);
	comRelease(rasterizerGui);
	comRelease(blendState);
	comRelease(ctx);
	comRelease(dev);
}

IDXGIFactory* RendererDx::createFactory() {
	IDXGIFactory* factory;
	if (HRESULT rs = CreateDXGIFactory(IID_PPV_ARGS(&factory)); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create factory: {}", hresultToStr(rs)));
	return factory;
}

pair<IDXGISwapChain*, ID3D11RenderTargetView*> RendererDx::createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(win, &wmInfo))
		throw std::runtime_error(SDL_GetError());

	DXGI_SWAP_CHAIN_DESC schainDesc = {
		.BufferDesc = {
			.Width = res.x,
			.Height = res.y,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM
		},
		.SampleDesc = { .Count = 1 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.OutputWindow = wmInfo.info.win.window,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	};
	IDXGISwapChain* swapchain = nullptr;
	ID3D11Texture2D* scBackBuffer = nullptr;
	ID3D11RenderTargetView* tgtBackbuffer = nullptr;
	try {
		if (HRESULT rs = factory->CreateSwapChain(dev, &schainDesc, &swapchain); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create swapchain: {}", hresultToStr(rs)));
		if (HRESULT rs = swapchain->GetBuffer(0, IID_PPV_ARGS(&scBackBuffer)); FAILED(rs))
			throw std::runtime_error(std::format("Failed get swapchain buffer: {}", hresultToStr(rs)));
		if (HRESULT rs = dev->CreateRenderTargetView(scBackBuffer, nullptr, &tgtBackbuffer); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create render target: {}", hresultToStr(rs)));
		comRelease(scBackBuffer);
	} catch (const std::runtime_error&) {
		comRelease(tgtBackbuffer);
		comRelease(scBackBuffer);
		comRelease(swapchain);
		throw;
	}
	return pair(swapchain, tgtBackbuffer);
}

void RendererDx::recreateSwapchain(IDXGIFactory* factory, ViewDx* view) {
	comRelease(view->tgt);
	comRelease(view->sc);
	std::tie(view->sc, view->tgt) = createSwapchain(factory, view->win, view->rect.size());
}

void RendererDx::initShader() {
	static constexpr uint8 vertSrcGui[] = {
#ifdef NDEBUG
#include "shaders/dxGuiVs.rel.h"
#else
#include "shaders/dxGuiVs.dbg.h"
#endif
	};
	static constexpr uint8 pixlSrcGui[] = {
#ifdef NDEBUG
#include "shaders/dxGuiPs.rel.h"
#else
#include "shaders/dxGuiPs.dbg.h"
#endif
	};
	if (HRESULT rs = dev->CreateVertexShader(vertSrcGui, sizeof(vertSrcGui), nullptr, &vertGui); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create graphics vertex shader: {}", hresultToStr(rs)));
	if (HRESULT rs = dev->CreatePixelShader(pixlSrcGui, sizeof(pixlSrcGui), nullptr, &pixlGui); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create graphics pixel shader: {}", hresultToStr(rs)));

	static constexpr uint8 vertSrcSel[] = {
#ifdef NDEBUG
#include "shaders/dxSelVs.rel.h"
#else
#include "shaders/dxSelVs.dbg.h"
#endif
	};
	static constexpr uint8 pixlSrcSel[] = {
#ifdef NDEBUG
#include "shaders/dxSelPs.rel.h"
#else
#include "shaders/dxSelPs.dbg.h"
#endif
	};
	if (HRESULT rs = dev->CreateVertexShader(vertSrcSel, sizeof(vertSrcSel), nullptr, &vertSel); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create address vertex shader: {}", hresultToStr(rs)));
	if (HRESULT rs = dev->CreatePixelShader(pixlSrcSel, sizeof(pixlSrcSel), nullptr, &pixlSel); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create address pixel shader: {}", hresultToStr(rs)));

	pviewBuf = createConstantBuffer(sizeof(Pview));
	instBuf = createConstantBuffer(sizeof(Instance));
	instColorBuf = createConstantBuffer(sizeof(InstanceColor));
	instAddrBuf = createConstantBuffer(sizeof(InstanceAddr));

	array<ID3D11Buffer*, 2> vsBuffers = { pviewBuf, instBuf };
	array<ID3D11Buffer*, 2> psBuffers = { instColorBuf, instAddrBuf };
	ctx->VSSetConstantBuffers(0, vsBuffers.size(), vsBuffers.data());
	ctx->PSSetConstantBuffers(0, psBuffers.size(), psBuffers.data());
	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);

	texAddr = createTexture(uvec2(1), DXGI_FORMAT_R32G32_UINT, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET);
	outAddr = createTexture(uvec2(1), DXGI_FORMAT_R32G32_UINT, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);

	D3D11_RENDER_TARGET_VIEW_DESC tgtDesc = {
		.Format = DXGI_FORMAT_R32G32_UINT,
		.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D
	};
	if (HRESULT rs = dev->CreateRenderTargetView(texAddr, &tgtDesc, &tgtAddr); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create address render target: {}", hresultToStr(rs)));
}

ID3D11Buffer* RendererDx::createConstantBuffer(uint size) const {
	D3D11_BUFFER_DESC bufferDesc = {
		.ByteWidth = size,
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
	};
	ID3D11Buffer* buffer;
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &buffer); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create constant buffer: {}", hresultToStr(rs)));
	return buffer;
}

ID3D11Texture2D* RendererDx::createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags, const D3D11_SUBRESOURCE_DATA* subrscData) const {
	D3D11_TEXTURE2D_DESC texDesc = {
		.Width = res.x,
		.Height = res.y,
		.MipLevels = 1,
		.ArraySize = 1,
		.Format = format,
		.SampleDesc = { .Count = 1 },
		.Usage = usage,
		.BindFlags = bindFlags,
		.CPUAccessFlags = accessFlags
	};
	ID3D11Texture2D* tex;
	if (HRESULT rs = dev->CreateTexture2D(&texDesc, subrscData, &tex); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create texture: {}", hresultToStr(rs)));
	return tex;
}

ID3D11ShaderResourceView* RendererDx::createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = { .MipLevels = 1 }
	};
	ID3D11ShaderResourceView* view;
	if (HRESULT rs = dev->CreateShaderResourceView(tex, &srvDesc, &view); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create texture view: {}", hresultToStr(rs)));
	return view;
}

void RendererDx::setClearColor(const vec4& color) {
	bgColor = color;
}

void RendererDx::setVsync(bool vsync) {
	syncInterval = vsync;
	IDXGIFactory* factory = createFactory();
	try {
		for (auto [id, view] : views)
			recreateSwapchain(factory, static_cast<ViewDx*>(view));
		comRelease(factory);
	} catch (const std::runtime_error&) {
		comRelease(factory);
		throw;
	}
}

void RendererDx::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_GetWindowSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;

		IDXGIFactory* factory = createFactory();
		try {
			recreateSwapchain(factory, static_cast<ViewDx*>(views.begin()->second));
			comRelease(factory);
		} catch (const std::runtime_error&) {
			comRelease(factory);
			throw;
		}
	}
}

void RendererDx::startDraw(View* view) {
	auto vw = static_cast<ViewDx*>(view);
	D3D11_VIEWPORT viewport = {
		.Width = float(vw->rect.w),
		.Height = float(vw->rect.h)
	};
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, &vw->tgt, nullptr);
	ctx->ClearRenderTargetView(vw->tgt, glm::value_ptr(bgColor));
	uploadBuffer(pviewBuf, Pview{ vec4(vw->rect.pos(), vec2(vw->rect.size()) / 2.f) });
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
	D3D11_VIEWPORT viewport = {
		.TopLeftX = float(-pos.x),
		.TopLeftY = float(-pos.y),
		.Width = float(view->rect.w),
		.Height = float(view->rect.h)
	};
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
	uploadBuffer(instAddrBuf, InstanceAddr{ uvec2(uintptr_t(wgt), uintptr_t(wgt) >> 32) });
	ctx->Draw(4, 0);
}

Widget* RendererDx::finishSelDraw(View*) {
	ctx->CopyResource(outAddr, texAddr);
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(outAddr, 0, D3D11_MAP_READ, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(std::format("Failed to map address memory: {}", hresultToStr(rs)));
	uvec2 val;
	memcpy(&val, mapRsc.pData, sizeof(uvec2));
	ctx->Unmap(outAddr, 0);

	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);
	ctx->RSSetState(rasterizerGui);
	ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
	return std::bit_cast<Widget*>(uintptr_t(val.x) | (uintptr_t(val.y) << 32));
}

template <class T>
void RendererDx::uploadBuffer(ID3D11Buffer* buffer, const T& data) {
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(std::format("Failed to map buffer: {}", hresultToStr(rs)));
	memcpy(mapRsc.pData, &data, sizeof(T));
	ctx->Unmap(buffer, 0);
}

Texture* RendererDx::texFromEmpty() {
	return new TextureDx(uvec2(0), nullptr);
}

Texture* RendererDx::texFromIcon(SDL_Surface* img) {
	return texFromRpic(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
}

bool RendererDx::texFromIcon(Texture* tex, SDL_Surface* img) {
	if (auto [pic, fmt] = pickPixFormat(img); pic) {
		try {
			uvec2 res(pic->w, pic->h);
			ID3D11ShaderResourceView* view = createTexture(static_cast<byte_t*>(pic->pixels), res, pic->pitch, fmt);
			SDL_FreeSurface(pic);

			auto dtx = static_cast<TextureDx*>(tex);
			comRelease(dtx->view);
			dtx->res = res;
			dtx->view = view;
			return true;
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(pic);
		}
	}
	return false;
}

Texture* RendererDx::texFromRpic(SDL_Surface* img) {
	if (auto [pic, fmt] = pickPixFormat(img); pic) {
		try {
			uvec2 res(pic->w, pic->h);
			ID3D11ShaderResourceView* view = createTexture(static_cast<byte_t*>(pic->pixels), res, pic->pitch, fmt);
			SDL_FreeSurface(pic);
			return new TextureDx(res, view);
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(pic);
		}
	}
	return nullptr;
}

Texture* RendererDx::texFromText(const PixmapRgba& pm) {
	if (pm.pix) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			return new TextureDx(res, createTexture(reinterpret_cast<const byte_t*>(pm.pix), res, pm.res.x * 4, DXGI_FORMAT_B8G8R8A8_UNORM));
		} catch (const std::runtime_error&) {}
	}
	return nullptr;
}

bool RendererDx::texFromText(Texture* tex, const PixmapRgba& pm) {
	if (pm.pix) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			ID3D11ShaderResourceView* view = createTexture(reinterpret_cast<const byte_t*>(pm.pix), res, pm.res.x * 4, DXGI_FORMAT_B8G8R8A8_UNORM);

			auto dtx = static_cast<TextureDx*>(tex);
			comRelease(dtx->view);
			dtx->res = res;
			dtx->view = view;
			return true;
		} catch (const std::runtime_error&) {}
	}
	return false;
}

void RendererDx::freeTexture(Texture* tex) {
	auto dtx = static_cast<TextureDx*>(tex);
	comRelease(dtx->view);
	delete dtx;
}

ID3D11ShaderResourceView* RendererDx::createTexture(const byte_t* pix, uvec2 res, uint pitch, DXGI_FORMAT format) {
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	try {
		D3D11_SUBRESOURCE_DATA subrscData{};
		subrscData.pSysMem = pix;
		subrscData.SysMemPitch = pitch;

		texture = createTexture(res, format, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, &subrscData);
		view = createTextureView(texture, format);
		comRelease(texture);
	} catch (const std::runtime_error& err) {
		logError(err.what());
		comRelease(view);
		comRelease(texture);
		throw;
	}
	return view;
}

pair<SDL_Surface*, DXGI_FORMAT> RendererDx::pickPixFormat(SDL_Surface* img) {
	if (img)
		switch (img->format->format) {
		case SDL_PIXELFORMAT_RGBA32:
			return pair(img, DXGI_FORMAT_R8G8B8A8_UNORM);
		case SDL_PIXELFORMAT_BGRA32:
			return pair(img, DXGI_FORMAT_B8G8R8A8_UNORM);
		case SDL_PIXELFORMAT_BGRA5551:
			return pair(img, DXGI_FORMAT_B5G5R5A1_UNORM);
		case SDL_PIXELFORMAT_BGR565:
			return pair(img, DXGI_FORMAT_B5G6R5_UNORM);
		default:
			img = convertReplace(img);
		}
	return pair(img, DXGI_FORMAT_R8G8B8A8_UNORM);
}

uint RendererDx::maxTexSize() const {
	return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
}

const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* RendererDx::getSquashableFormats() const {
	return !squashPicTexels ? nullptr : &squashableFormats;
}

void RendererDx::setCompression(Settings::Compression compression) {
	squashPicTexels = compression == Settings::Compression::b16;
}

pair<uint, Settings::Compression> RendererDx::getSettings(vector<pair<u32vec2, string>>& devices) const {
	devices = { pair(u32vec2(0), "auto")};
	IDXGIFactory* factory = createFactory();
	IDXGIAdapter* adapter = nullptr;
	for (uint i = 0; factory->EnumAdapters(i, &adapter) == S_OK; ++i) {
		if (DXGI_ADAPTER_DESC desc; SUCCEEDED(adapter->GetDesc(&desc)))
			devices.emplace_back(u32vec2(desc.VendorId, desc.DeviceId), swtos(desc.Description));
		comRelease(adapter);
	}
	comRelease(factory);
	return pair(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION, Settings::Compression::b16);
}

template <class T>
void RendererDx::comRelease(T*& obj) {
	if (obj) {
		obj->Release();
		obj = nullptr;
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
