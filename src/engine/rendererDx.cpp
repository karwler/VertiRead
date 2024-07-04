#ifdef WITH_DIRECT3D
#include "rendererDx.h"
#include "optional/d3d.h"
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_syswm.h>

RendererDx11::RendererDx11(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor) :
	Renderer(windows.size(), D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION),
	bgColor(bgcolor),
	syncInterval(sets->vsync)
{
	IDXGIFactory* factory = createFactory();
	IDXGIAdapter* adapter = nullptr;
	try {
		DXGI_ADAPTER_DESC adapterDesc;
#ifdef NDEBUG
		uint flags = 0;
#else
		uint flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
		if (sets->device != u32vec2(0)) {
			for (uint i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); ++i) {
				if (SUCCEEDED(adapter->GetDesc(&adapterDesc)) && adapterDesc.VendorId == sets->device.x && adapterDesc.DeviceId == sets->device.y) {
					HRESULT rs = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr);
					if (SUCCEEDED(rs))
						break;
					logError("Unable to create the specified device ", toStr<16>(sets->device, ":"), ": ", hresultToStr(rs));
				}
				comRelease(adapter);
			}
		}
		if (!adapter) {
			for (uint i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); ++i) {
				if (SUCCEEDED(d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr)))
					break;
				comRelease(adapter);
			}
			if (!adapter)
				throw std::runtime_error("Failed to find an adapter");
			if (HRESULT rs = adapter->GetDesc(&adapterDesc); FAILED(rs)) {
				logError("Failed to get adapter info: ", hresultToStr(rs));
				adapterDesc = {};
			}
			sets->device = u32vec2(0);
		}
		if (HRESULT rs = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create device: {}", hresultToStr(rs)));
		comRelease(adapter);

		if (!vofs) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(windows[0], &viewRes.x, &viewRes.y);
#else
			SDL_GetWindowSize(windows[0], &viewRes.x, &viewRes.y);
#endif
			auto vw = static_cast<ViewDx*>(views[0] = new ViewDx(windows[0], Recti(ivec2(0), viewRes)));
			std::tie(vw->sc, vw->tgt) = createSwapchain(factory, windows[0], viewRes);
		} else
			for (size_t i = 0; i < views.size(); ++i) {
				Recti wrect;
				wrect.pos() = vofs[i] - vofs[views.size()];
#if SDL_VERSION_ATLEAST(2, 26, 0)
				SDL_GetWindowSizeInPixels(windows[i], &wrect.w, &wrect.h);
#else
				SDL_GetWindowSize(windows[i], &wrect.w, &wrect.h);
#endif
				viewRes = glm::max(viewRes, wrect.end());
				auto vw = static_cast<ViewDx*>(views[i] = new ViewDx(windows[i], wrect));
				std::tie(vw->sc, vw->tgt) = createSwapchain(factory, windows[i], wrect.size());
			}
		comRelease(factory);

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
			.CullMode = D3D11_CULL_NONE,
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
		ID3D11SamplerState* sampleState;
		if (HRESULT rs = dev->CreateSamplerState(&samplerDesc, &sampleState); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create sampler state: {}", hresultToStr(rs)));
		ctx->PSSetSamplers(0, 1, &sampleState);
		comRelease(sampleState);

		initShader();
		if (D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS opts; SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &opts, sizeof(opts))) && opts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x) {
			try {
				initConverter();
			} catch (const std::runtime_error& err) {
				logError(err.what());
				cleanupConverter();
			}
		}

		uint format;
		canBgra5551 = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B5G5R5A1_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D);
		canBgr565 = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B5G6R5_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D);
		canBgra4 = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B4G4R4A4_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D);
		setCompression(sets->compression);
		setMaxPicRes(sets->maxPicRes);
		if (!sets->picLim.size) {
			if (adapterDesc.DedicatedVideoMemory)
				sets->picLim.size = adapterDesc.DedicatedVideoMemory / 2;
			else
				recommendPicRamLimit(sets->picLim.size);
		}
	} catch (...) {
		comRelease(factory);
		comRelease(adapter);
		cleanup();
		throw;
	}
}

RendererDx11::~RendererDx11() {
	cleanup();
}

void RendererDx11::cleanup() noexcept {
	cleanupConverter();
	comRelease(tgtAddr);
	comRelease(outAddr);
	comRelease(texAddr);
	comRelease(instAddrBuf);
	comRelease(instColorBuf);
	comRelease(instBuf);
	comRelease(pviewBuf);
	comRelease(pixlSel);
	comRelease(vertSel);
	comRelease(pixlGui);
	comRelease(vertGui);
	for (View* it : views) {
		auto vw = static_cast<ViewDx*>(it);
		comRelease(vw->tgt);
		comRelease(vw->sc);
		delete vw;
	}
	comRelease(rasterizerSel);
	comRelease(rasterizerGui);
	comRelease(blendState);
	comRelease(ctx);
	comRelease(dev);
}

void RendererDx11::cleanupConverter() noexcept {
	comRelease(inputView);
	comRelease(inputBuf);
	comRelease(colorBuf);
	comRelease(offsetBuf);
	for (ID3D11ComputeShader* it : compConv)
		comRelease(it);
}

IDXGIFactory* RendererDx11::createFactory() {
	IDXGIFactory* factory;
	if (HRESULT rs = createDXGIFactory(IID_PPV_ARGS(&factory)); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create factory: {}", hresultToStr(rs)));
	return factory;
}

pair<IDXGISwapChain*, ID3D11RenderTargetView*> RendererDx11::createSwapchain(IDXGIFactory* factory, SDL_Window* win, uvec2 res) {
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

void RendererDx11::recreateSwapchain(IDXGIFactory* factory, ViewDx* view) {
	comRelease(view->tgt);
	comRelease(view->sc);
	std::tie(view->sc, view->tgt) = createSwapchain(factory, view->win, view->rect.size());
}

void RendererDx11::initShader() {
	static constexpr uint32 vertSrcGui[] = {
#ifdef NDEBUG
#include "shaders/dxGuiVs.rel.h"
#else
#include "shaders/dxGuiVs.dbg.h"
#endif
	};
	static constexpr uint32 pixlSrcGui[] = {
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

	static constexpr uint32 vertSrcSel[] = {
#ifdef NDEBUG
#include "shaders/dxSelVs.rel.h"
#else
#include "shaders/dxSelVs.dbg.h"
#endif
	};
	static constexpr uint32 pixlSrcSel[] = {
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

	texAddr = createTexture(uvec2(1), DXGI_FORMAT_R32G32_UINT, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET);
	outAddr = createTexture(uvec2(1), DXGI_FORMAT_R32G32_UINT, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);

	D3D11_RENDER_TARGET_VIEW_DESC tgtDesc = {
		.Format = DXGI_FORMAT_R32G32_UINT,
		.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D
	};
	if (HRESULT rs = dev->CreateRenderTargetView(texAddr, &tgtDesc, &tgtAddr); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create address render target: {}", hresultToStr(rs)));

	D3D11_INPUT_ELEMENT_DESC vertexElementDesc = {
		.SemanticName = "SV_POSITION",
		.Format = DXGI_FORMAT_R32G32_FLOAT,
		.AlignedByteOffset = 0,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA
	};
	ID3D11InputLayout* vertexLayout;
	if (HRESULT rs = dev->CreateInputLayout(&vertexElementDesc, 1, vertSrcGui, sizeof(vertSrcGui), &vertexLayout); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create graphics input layout: {}", hresultToStr(rs)));
	ctx->IASetInputLayout(vertexLayout);
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	vertexLayout->Release();

	D3D11_BUFFER_DESC vertexBufferDesc = {
		.ByteWidth = sizeof(vertices),
		.Usage = D3D11_USAGE_IMMUTABLE,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER
	};
	D3D11_SUBRESOURCE_DATA vertexSubrsc = { .pSysMem = vertices.data() };
	ID3D11Buffer* vertexBuf;
	if (HRESULT rs = dev->CreateBuffer(&vertexBufferDesc, &vertexSubrsc, &vertexBuf); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create vertex buffer: {}", hresultToStr(rs)));

	uint vertexStride = sizeof(vec2);
	uint vertexOffset = 0;
	ctx->IASetVertexBuffers(0, 1, &vertexBuf, &vertexStride, &vertexOffset);
	vertexBuf->Release();

	ID3D11Buffer* vsBuffers[2] = { pviewBuf, instBuf };
	ID3D11Buffer* psBuffers[2] = { instColorBuf, instAddrBuf };
	ctx->VSSetConstantBuffers(0, std::size(vsBuffers), vsBuffers);
	ctx->PSSetConstantBuffers(0, std::size(psBuffers), psBuffers);
	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);
}

void RendererDx11::initConverter() {
	static constexpr uint32 srcRgb[] = {
#ifdef NDEBUG
#include "shaders/dxRgbCs.rel.h"
#else
#include "shaders/dxRgbCs.dbg.h"
#endif
	};
	static constexpr uint32 srcBgr[] = {
#ifdef NDEBUG
#include "shaders/dxBgrCs.rel.h"
#else
#include "shaders/dxBgrCs.dbg.h"
#endif
	};
	static constexpr uint32 srcIdx[] = {
#ifdef NDEBUG
#include "shaders/dxIdxCs.rel.h"
#else
#include "shaders/dxIdxCs.dbg.h"
#endif
	};
	if (HRESULT rs = dev->CreateComputeShader(srcRgb, sizeof(srcRgb), nullptr, &compConv[eint(FormatConv::rgb24)]); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create rgb24 compute shader: {}", hresultToStr(rs)));
	if (HRESULT rs = dev->CreateComputeShader(srcBgr, sizeof(srcBgr), nullptr, &compConv[eint(FormatConv::bgr24)]); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create bgr24 compute shader: {}", hresultToStr(rs)));
	if (HRESULT rs = dev->CreateComputeShader(srcIdx, sizeof(srcIdx), nullptr, &compConv[eint(FormatConv::index8)]); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create index compute shader: {}", hresultToStr(rs)));

	offsetBuf = createConstantBuffer(sizeof(Offset));
	colorBuf = createConstantBuffer(256 * sizeof(uint));

	ID3D11Buffer* buffers[2] = { offsetBuf, colorBuf };
	ctx->CSSetConstantBuffers(0, std::size(buffers), buffers);
}

ID3D11Buffer* RendererDx11::createConstantBuffer(uint size) const {
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

ID3D11Texture2D* RendererDx11::createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags, const D3D11_SUBRESOURCE_DATA* subrscData) const {
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

ID3D11ShaderResourceView* RendererDx11::createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format) {
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

ID3D11ShaderResourceView* RendererDx11::createBufferView(ID3D11Buffer* buffer, uint size) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = DXGI_FORMAT_UNKNOWN,
		.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
		.Buffer { .NumElements = size }
	};
	ID3D11ShaderResourceView* view;
	if (HRESULT rs = dev->CreateShaderResourceView(buffer, &srvDesc, &view); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create buffer view: {}", hresultToStr(rs)));
	return view;
}

void RendererDx11::setClearColor(const vec4& color) {
	bgColor = color;
}

void RendererDx11::setVsync(bool vsync) {
	syncInterval = vsync;
	IDXGIFactory* factory = createFactory();
	try {
		for (View* it : views)
			recreateSwapchain(factory, static_cast<ViewDx*>(it));
		comRelease(factory);
	} catch (const std::runtime_error&) {
		comRelease(factory);
		throw;
	}
}

void RendererDx11::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
		SDL_GetWindowSizeInPixels(views[0]->win, &viewRes.x, &viewRes.y);
#else
		SDL_GetWindowSize(views[0]->win, &viewRes.x, &viewRes.y);
#endif
		views[0]->rect.size() = viewRes;

		IDXGIFactory* factory = createFactory();
		try {
			recreateSwapchain(factory, static_cast<ViewDx*>(views[0]));
			comRelease(factory);
		} catch (const std::runtime_error&) {
			comRelease(factory);
			throw;
		}
	}
}

void RendererDx11::startDraw(View* view) {
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

void RendererDx11::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	uploadBuffer(instBuf, Instance{ rect.asVec(), frame.asVec() });
	uploadBuffer(instColorBuf, InstanceColor{ color });
	ctx->PSSetShaderResources(0, 1, &static_cast<const TextureDx*>(tex)->view);
	ctx->Draw(vertices.size(), 0);
}

void RendererDx11::finishDraw(View* view) {
	static_cast<ViewDx*>(view)->sc->Present(syncInterval, 0);
}

void RendererDx11::startSelDraw(View* view, ivec2 pos) {
	vec4 zero(0.f);
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
	ctx->ClearRenderTargetView(tgtAddr, glm::value_ptr(zero));
	uploadBuffer(pviewBuf, Pview{ vec4(view->rect.pos(), vec2(view->rect.size()) / 2.f) });
}

void RendererDx11::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	uploadBuffer(instBuf, Instance{ rect.asVec(), frame.asVec() });
	uploadBuffer(instAddrBuf, InstanceAddr{ uvec2(uintptr_t(wgt), uintptr_t(wgt) >> 32) });
	ctx->Draw(vertices.size(), 0);
}

Widget* RendererDx11::finishSelDraw(View*) {
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

template <Class T>
void RendererDx11::uploadBuffer(ID3D11Buffer* buffer, const T& data) {
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(std::format("Failed to map buffer: {}", hresultToStr(rs)));
	memcpy(mapRsc.pData, &data, sizeof(T));
	ctx->Unmap(buffer, 0);
}

Texture* RendererDx11::texFromEmpty() {
	return new TextureDx(uvec2(0), nullptr);
}

Texture* RendererDx11::texFromIcon(SDL_Surface* img) {
	return texFromRpic(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
}

bool RendererDx11::texFromIcon(Texture* tex, SDL_Surface* img) {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)); si.img) {
		try {
			uvec2 res(si.img->w, si.img->h);
			ID3D11ShaderResourceView* view = si.fmt
				? createTextureDirect(static_cast<byte_t*>(si.img->pixels), res, si.img->pitch, si.fmt)
				: createTextureIndirect(si.img, si.fcid);
			replaceTexture(static_cast<TextureDx*>(tex), view, res);
			SDL_FreeSurface(si.img);
			return true;
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(si.img);
		}
	}
	return false;
}

Texture* RendererDx11::texFromRpic(SDL_Surface* img) {
	if (SurfaceInfo si = pickPixFormat(img); si.img) {
		try {
			uvec2 res(si.img->w, si.img->h);
			ID3D11ShaderResourceView* view = si.fmt
				? createTextureDirect(static_cast<byte_t*>(si.img->pixels), res, si.img->pitch, si.fmt)
				: createTextureIndirect(si.img, si.fcid);
			SDL_FreeSurface(si.img);
			return new TextureDx(res, view);
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(si.img);
		}
	}
	return nullptr;
}

Texture* RendererDx11::texFromText(const PixmapRgba& pm) {
	if (pm.res.x) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			return new TextureDx(res, createTextureDirect(reinterpret_cast<const byte_t*>(pm.pix.get()), res, pm.res.x * 4, DXGI_FORMAT_B8G8R8A8_UNORM));
		} catch (const std::runtime_error&) {}
	}
	return nullptr;
}

bool RendererDx11::texFromText(Texture* tex, const PixmapRgba& pm) {
	if (pm.res.x) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			replaceTexture(static_cast<TextureDx*>(tex), createTextureDirect(reinterpret_cast<const byte_t*>(pm.pix.get()), res, pm.res.x * 4, DXGI_FORMAT_B8G8R8A8_UNORM), res);
			return true;
		} catch (const std::runtime_error&) {}
	}
	return false;
}

void RendererDx11::freeTexture(Texture* tex) noexcept {
	if (auto dtx = static_cast<TextureDx*>(tex)) {
		comRelease(dtx->view);
		delete dtx;
	}
}

ID3D11ShaderResourceView* RendererDx11::createTextureDirect(const byte_t* pix, uvec2 res, uint pitch, DXGI_FORMAT format) {
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	try {
		D3D11_SUBRESOURCE_DATA subrscData = {
			.pSysMem = pix,
			.SysMemPitch = pitch
		};
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

ID3D11ShaderResourceView* RendererDx11::createTextureIndirect(const SDL_Surface* img, FormatConv fcid) {
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;
	try {
		uvec2 res(img->w, img->h);
		texture = createTexture(res, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0);
		view = createTextureView(texture, DXGI_FORMAT_R8G8B8A8_UNORM);

		D3D11_MAPPED_SUBRESOURCE mapRsc;
		if (fcid == FormatConv::index8) {
			if (HRESULT rs = ctx->Map(colorBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
				throw std::runtime_error(std::format("Failed to map input buffer: {}", hresultToStr(rs)));
			memcpy(mapRsc.pData, img->format->palette->colors, img->format->palette->ncolors * sizeof(SDL_Color));
			ctx->Unmap(colorBuf, 0);
		}

		uint rowSize = res.x * img->format->BytesPerPixel;
		uint texels = res.x * res.y;
		if (uint isize = texels * img->format->BytesPerPixel; isize > inputSize)
			replaceInputBuffer(isize);
		if (HRESULT rs = ctx->Map(inputBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
			throw std::runtime_error(std::format("Failed to map input buffer: {}", hresultToStr(rs)));
		copyPixels(mapRsc.pData, img->pixels, rowSize, img->pitch, rowSize, res.y);
		ctx->Unmap(inputBuf, 0);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
		};
		if (HRESULT rs = dev->CreateUnorderedAccessView(texture, &uavDesc, &uav); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create UAV: {}", hresultToStr(rs)));

		ctx->CSSetShader(compConv[eint(fcid)], nullptr, 0);
		ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		uint numGroups = texels / 32;
		for (uint gcnt, offs = 0; offs < numGroups; offs += gcnt) {
			gcnt = std::min(numGroups - offs, uint(D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION));
			uploadBuffer(offsetBuf, Offset{ offs });
			ctx->Dispatch(gcnt, 1, 1);
		}
		comRelease(uav);
		comRelease(texture);
	} catch (const std::runtime_error& err) {
		logError(err.what());
		comRelease(uav);
		comRelease(view);
		comRelease(texture);
		throw;
	}
	return view;
}

void RendererDx11::replaceTexture(TextureDx* tex, ID3D11ShaderResourceView* tview, uvec2 res) noexcept {
	comRelease(tex->view);
	tex->res = res;
	tex->view = tview;
}

void RendererDx11::replaceInputBuffer(uint inputSize) {
	ID3D11Buffer* buffer = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	try {
		D3D11_BUFFER_DESC bufferDesc = {
			.ByteWidth = inputSize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &buffer); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create input buffer: {}", hresultToStr(rs)));

		view = createBufferView(buffer, inputSize);
		comRelease(inputView);
		comRelease(inputBuf);
		inputBuf = buffer;
		inputView = view;
		inputSize = inputSize;
		ctx->CSGetShaderResources(0, 1, &inputView);
	} catch (const std::runtime_error&) {
		comRelease(view);
		comRelease(buffer);
		throw;
	}
}

RendererDx11::SurfaceInfo RendererDx11::pickPixFormat(SDL_Surface* img) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (img->format->format) {
	case SDL_PIXELFORMAT_ABGR8888:
		return SurfaceInfo(img, DXGI_FORMAT_R8G8B8A8_UNORM);
	case SDL_PIXELFORMAT_ARGB8888:
		return SurfaceInfo(img, DXGI_FORMAT_B8G8R8A8_UNORM);
	case SDL_PIXELFORMAT_XRGB8888:
		return SurfaceInfo(img, DXGI_FORMAT_B8G8R8X8_UNORM);
	case SDL_PIXELFORMAT_RGB24:
		if (compConv[eint(FormatConv::rgb24)])
			return SurfaceInfo(img, FormatConv::rgb24);
		break;
	case SDL_PIXELFORMAT_BGR24:
		if (compConv[eint(FormatConv::bgr24)])
			return SurfaceInfo(img, FormatConv::bgr24);
		break;
	case SDL_PIXELFORMAT_RGB565:
		if (canBgr565)
			return SurfaceInfo(img, DXGI_FORMAT_B5G6R5_UNORM);
		break;
	case SDL_PIXELFORMAT_ARGB1555:
		if (canBgra5551)
			return SurfaceInfo(img, DXGI_FORMAT_B5G5R5A1_UNORM);
		break;
	case SDL_PIXELFORMAT_ARGB4444:
		if (canBgra4)
			return SurfaceInfo(img, DXGI_FORMAT_B4G4R4A4_UNORM);
		break;
	case SDL_PIXELFORMAT_INDEX8:
		if (compConv[eint(FormatConv::index8)])
			return SurfaceInfo(img, FormatConv::index8);
	}

	if (img->format->BytesPerPixel < 3) {
		if (img->format->Amask && canBgra5551)
			return SurfaceInfo(convertReplace(img, SDL_PIXELFORMAT_ARGB1555), DXGI_FORMAT_B5G5R5A1_UNORM);
		if (!img->format->Amask && canBgr565)
			return SurfaceInfo(convertReplace(img, SDL_PIXELFORMAT_RGB565), DXGI_FORMAT_B5G6R5_UNORM);
	}
	return SurfaceInfo(convertReplace(img), DXGI_FORMAT_R8G8B8A8_UNORM);
}

template <Derived<IUnknown> T>
void RendererDx11::comRelease(T*& obj) noexcept {
	if (obj) {
		obj->Release();
		obj = nullptr;
	}
}

void RendererDx11::setCompression(Settings::Compression compression) {
	if (compression == Settings::Compression::b16 && canBgra5551 && canBgr565) {
		preconvertFormats = {
			{ SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_BGRA8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_RGBA8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_XBGR8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_BGRX8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_RGBX8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_BGR24, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_INDEX8, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_INDEX4LSB, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_INDEX4MSB, SDL_PIXELFORMAT_RGB565 }
		};
	} else
		preconvertFormats.clear();
}

Renderer::Info RendererDx11::getInfo() const {
	Info info = {
		.devices = { Info::Device(u32vec2(0), "auto") },
		.compressions = { Settings::Compression::none },
		.texSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
		.selecting = true
	};
	if (canBgra5551 && canBgr565)
		info.compressions.insert(Settings::Compression::b16);

	IDXGIFactory* factory = createFactory();
	IDXGIAdapter* adapter = nullptr;
	for (uint i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); ++i) {
		if (DXGI_ADAPTER_DESC desc; SUCCEEDED(adapter->GetDesc(&desc)))
			info.devices.emplace_back(u32vec2(desc.VendorId, desc.DeviceId), swtos(desc.Description), desc.DedicatedVideoMemory);
		comRelease(adapter);
	}
	comRelease(factory);
	return info;
}
#endif
