#ifdef WITH_DIRECT3D
#include "rendererDx.h"
#include "optional/d3d.h"
#include <SDL_log.h>
#if !SDL_VERSION_ATLEAST(3, 0, 0)
#include <SDL_syswm.h>
#endif
#include <format>
#include <glm/gtc/type_ptr.hpp>

RendererDx11::RendererDx11(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor) :
	Renderer(windows.size(), D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION),
	bgColor(bgcolor),
	syncInterval(sets->vsync)
{
	IDXGIFactory* factory = createFactory();
	try {
		auto [adapter, adapterMem, driverType] = pickAdapter(factory, sets->device);
		if (HRESULT rs = d3d11CreateDevice(adapter, driverType, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, &dev, nullptr, &ctx); FAILED(rs)) {
			comRelease(adapter);
			throw std::runtime_error(std::format("Failed to create device: {}", hresultToStr(rs)));
		}
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
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
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
			if (adapterMem)
				sets->picLim.size = adapterMem / 2;
			else
				recommendPicRamLimit(sets->picLim.size);
		}
	} catch (...) {
		comRelease(factory);
		cleanup();
		throw;
	}
}

RendererDx11::~RendererDx11() {
	cleanup();
}

void RendererDx11::cleanup() noexcept {
	cleanupConverter();
	comRelease(instColorBuf);
	comRelease(instBuf);
	comRelease(pviewBuf);
	comRelease(pixlGui);
	comRelease(vertGui);
	for (View* it : views)
		if (auto vw = static_cast<ViewDx*>(it)) {
			comRelease(vw->tgt);
			comRelease(vw->sc);
			delete vw;
		}
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
#if SDL_VERSION_ATLEAST(3, 0, 0)
	SDL_PropertiesID props = SDL_GetWindowProperties(win);
	if (!props)
		throw std::runtime_error(SDL_GetError());
	auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
#else
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(win, &wmInfo))
		throw std::runtime_error(SDL_GetError());
	HWND hwnd = wmInfo.info.win.window;
#endif
	if (!hwnd)
		throw std::runtime_error("Window has no handle");

	DXGI_SWAP_CHAIN_DESC schainDesc = {
		.BufferDesc = {
			.Width = res.x,
			.Height = res.y,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM
		},
		.SampleDesc = { .Count = 1 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.OutputWindow = hwnd,
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

	pviewBuf = createConstantBuffer(sizeof(Pview));
	instBuf = createConstantBuffer(sizeof(Instance));
	instColorBuf = createConstantBuffer(sizeof(InstanceColor));

	ID3D11Buffer* vsBuffers[2] = { pviewBuf, instBuf };
	ctx->VSSetConstantBuffers(0, std::size(vsBuffers), vsBuffers);
	ctx->PSSetConstantBuffers(0, 1, &instColorBuf);
	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);

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
	static constexpr uint32 srcRed[] = {
#ifdef NDEBUG
#include "shaders/dxRedCs.rel.h"
#else
#include "shaders/dxRedCs.dbg.h"
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
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create rgb compute shader: %s", hresultToStr(rs));
	if (HRESULT rs = dev->CreateComputeShader(srcBgr, sizeof(srcBgr), nullptr, &compConv[eint(FormatConv::bgr24)]); FAILED(rs))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create bgr compute shader: %s", hresultToStr(rs));
	if (HRESULT rs = dev->CreateComputeShader(srcRed, sizeof(srcRed), nullptr, &compConv[eint(FormatConv::red)]); FAILED(rs))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create red compute shader: %s", hresultToStr(rs));
	if (HRESULT rs = dev->CreateComputeShader(srcIdx, sizeof(srcIdx), nullptr, &compConv[eint(FormatConv::index8)]); FAILED(rs))
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create index compute shader: %s", hresultToStr(rs));

	if (rng::any_of(compConv, [](ID3D11ComputeShader* it) -> bool { return it; })) {
		offsetBuf = createConstantBuffer(sizeof(Offset));
		colorBuf = createConstantBuffer(256 * sizeof(uint));

		ID3D11Buffer* buffers[2] = { offsetBuf, colorBuf };
		ctx->CSSetConstantBuffers(0, std::size(buffers), buffers);
	}
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

	mapBuffer<Pview>(pviewBuf)->pview = vec4(vw->rect.pos(), vec2(vw->rect.size()) / 2.f);
	ctx->Unmap(pviewBuf, 0);
}

void RendererDx11::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	Instance* minstance = mapBuffer<Instance>(instBuf);
	minstance->rect = rect.asVec();
	minstance->frame = frame.asVec();
	ctx->Unmap(instBuf, 0);

	mapBuffer<InstanceColor>(instColorBuf)->color = color;
	ctx->Unmap(instColorBuf, 0);

	ctx->PSSetShaderResources(0, 1, &static_cast<const TextureDx*>(tex)->view);
	ctx->Draw(vertices.size(), 0);
}

void RendererDx11::finishDraw(View* view) {
	static_cast<ViewDx*>(view)->sc->Present(syncInterval, 0);
}

D3D11_MAPPED_SUBRESOURCE RendererDx11::mapResource(ID3D11Resource* rsc) {
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(rsc, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(std::format("Failed to map resource: {}", hresultToStr(rs)));
	return mapRsc;
}

Texture* RendererDx11::texFromEmpty() {
	return new TextureDx(uvec2(0), nullptr);
}

Texture* RendererDx11::texFromIcon(SDL_Surface* img) noexcept {
	return texFromRpic(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
}

bool RendererDx11::texFromIcon(Texture* tex, SDL_Surface* img) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)); si.img) {
		try {
			uvec2 res(si.img->w, si.img->h);
			ID3D11ShaderResourceView* view = si.fmt
				? createTextureDirect(static_cast<byte_t*>(si.img->pixels), res, si.img->pitch, si.fmt)
				: createTextureIndirect(static_cast<byte_t*>(si.img->pixels), res, surfaceBytesPpx(si.img), si.img->pitch, surfacePalette(si.img), si.fcid);
			replaceTexture(static_cast<TextureDx*>(tex), view, res);
			SDL_FreeSurface(si.img);
			return true;
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(si.img);
		}
	}
	return false;
}

Texture* RendererDx11::texFromRpic(SDL_Surface* img) noexcept {
	if (SurfaceInfo si = pickPixFormat(img); si.img) {
		try {
			uvec2 res(si.img->w, si.img->h);
			ID3D11ShaderResourceView* view = si.fmt
				? createTextureDirect(static_cast<byte_t*>(si.img->pixels), res, si.img->pitch, si.fmt)
				: createTextureIndirect(static_cast<byte_t*>(si.img->pixels), res, surfaceBytesPpx(si.img), si.img->pitch, surfacePalette(si.img), si.fcid);
			SDL_FreeSurface(si.img);
			return new TextureDx(res, view);
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(si.img);
		}
	}
	return nullptr;
}

Texture* RendererDx11::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			return new TextureDx(res, compConv[eint(FormatConv::red)]
				? createTextureIndirect(reinterpret_cast<byte_t*>(pm.pix.get()), res, 1, pm.res.x, nullptr, FormatConv::red)
				: createTextureText(pm, res));
		} catch (const std::runtime_error&) {}
	}
	return nullptr;
}

bool RendererDx11::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (pm.res.x) {
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			replaceTexture(static_cast<TextureDx*>(tex), compConv[eint(FormatConv::red)]
				? createTextureIndirect(reinterpret_cast<byte_t*>(pm.pix.get()), res, 1, pm.res.x, nullptr, FormatConv::red)
				: createTextureText(pm, res), res);
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
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		comRelease(view);
		comRelease(texture);
		throw;
	}
	return view;
}

ID3D11ShaderResourceView* RendererDx11::createTextureIndirect(const byte_t* pix, uvec2 res, uint8 bpp, uint pitch, const SDL_Palette* palette, FormatConv fcid) {
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	ID3D11UnorderedAccessView* uav = nullptr;
	try {
		texture = createTexture(res, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
		view = createTextureView(texture, DXGI_FORMAT_R8G8B8A8_UNORM);

		if (fcid == FormatConv::index8) {
			copyPalette(mapBuffer<Palette>(colorBuf)->colors, palette);
			ctx->Unmap(colorBuf, 0);
		}

		uint rowSize = res.x * bpp;
		uint texels = res.x * res.y;
		if (uint isize = texels * bpp; isize > inputSize)
			replaceInputBuffer(roundToMultiple(isize, uint(sizeof(uint))));
		copyPixels(mapResource(inputBuf).pData, pix, rowSize, pitch, rowSize, res.y);
		ctx->Unmap(inputBuf, 0);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
		};
		if (HRESULT rs = dev->CreateUnorderedAccessView(texture, &uavDesc, &uav); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create UAV: {}", hresultToStr(rs)));

		ctx->CSSetShader(compConv[eint(fcid)], nullptr, 0);
		ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		uint numGroups = texels / convStep + bool(texels % convStep);
		for (uint gcnt, offs = 0; offs < numGroups; offs += gcnt) {
			gcnt = std::min(numGroups - offs, uint(D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION));
			mapBuffer<Offset>(offsetBuf)->offset = offs;
			ctx->Unmap(offsetBuf, 0);
			ctx->Dispatch(gcnt, 1, 1);
		}

		ID3D11UnorderedAccessView* nullUav = nullptr;
		ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
		comRelease(uav);
		comRelease(texture);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		comRelease(uav);
		comRelease(view);
		comRelease(texture);
		throw;
	}
	return view;
}

ID3D11ShaderResourceView* RendererDx11::createTextureText(const Pixmap& pm, uvec2 res) {
	ID3D11Texture2D* texture = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	try {
		texture = createTexture(res, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE);
		view = createTextureView(texture, DXGI_FORMAT_R8G8B8A8_UNORM);

		D3D11_MAPPED_SUBRESOURCE mapRsc = mapResource(texture);
		copyTextPixels(mapRsc.pData, pm, res, mapRsc.RowPitch);
		ctx->Unmap(texture, 0);

		comRelease(texture);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
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

void RendererDx11::replaceInputBuffer(uint isize) {
	ID3D11Buffer* buffer = nullptr;
	ID3D11ShaderResourceView* view = nullptr;
	try {
		D3D11_BUFFER_DESC bufferDesc = {
			.ByteWidth = isize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
		};
		if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &buffer); FAILED(rs))
			throw std::runtime_error(std::format("Failed to create input buffer: {}", hresultToStr(rs)));

		view = createBufferView(buffer, isize);
		comRelease(inputView);
		comRelease(inputBuf);
		inputBuf = buffer;
		inputView = view;
		inputSize = isize;
		ctx->CSSetShaderResources(0, 1, &inputView);
	} catch (const std::runtime_error&) {
		comRelease(view);
		comRelease(buffer);
		throw;
	}
}

RendererDx11::SurfaceInfo RendererDx11::pickPixFormat(SDL_Surface* img) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (surfaceFormat(img)) {
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
#if SDL_VERSION_ATLEAST(3, 0, 0)
	case SDL_PIXELFORMAT_ABGR2101010:
		return SurfaceInfo(img, DXGI_FORMAT_R10G10B10A2_UNORM);
	case SDL_PIXELFORMAT_RGBA64:
		return SurfaceInfo(img, DXGI_FORMAT_R16G16B16A16_UNORM);
#endif
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
	return SurfaceInfo(convertReplace(img), DXGI_FORMAT_R8G8B8A8_UNORM);
}

void RendererDx11::setCompression(Settings::Compression cmpr) noexcept {
	compression = cmpr == Settings::Compression::b16 && (canBgra5551 || canBgr565) ? cmpr : Settings::Compression::none;
}

SDL_Surface* RendererDx11::prepareImage(SDL_Surface* img, bool rpic) const noexcept {
	if (img = limitSize(img, rpic ? maxPictureSize : maxTextureSize); img) {
		if (compression == Settings::Compression::b16)
			return convertReplace(img, SDL_ISPIXELFORMAT_ALPHA(surfaceFormat(img))
				? canBgra5551 ? SDL_PIXELFORMAT_ARGB1555 : SDL_PIXELFORMAT_RGB565
				: canBgr565 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_ARGB1555);
#if SDL_VERSION_ATLEAST(3, 0, 0)
		if (img->format == SDL_PIXELFORMAT_ARGB2101010 || img->format == SDL_PIXELFORMAT_XBGR2101010 || img->format == SDL_PIXELFORMAT_XBGR2101010)
			return convertReplace(img, SDL_PIXELFORMAT_ABGR2101010);
		if (img->format != SDL_PIXELFORMAT_RGBA64 && SDL_BYTESPERPIXEL(img->format) > 4)
			return convertReplace(img, SDL_PIXELFORMAT_RGBA64);
#endif
	}
	return img;
}

ID3D11Buffer* RendererDx11::createConstantBuffer(uint size) const {
	D3D11_BUFFER_DESC bufferDesc = {
		.ByteWidth = roundToMultiple(size, 16u),
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
		.SampleDesc = {.Count = 1 },
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
		.Texture2D = {.MipLevels = 1 }
	};
	ID3D11ShaderResourceView* view;
	if (HRESULT rs = dev->CreateShaderResourceView(tex, &srvDesc, &view); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create texture view: {}", hresultToStr(rs)));
	return view;
}

ID3D11ShaderResourceView* RendererDx11::createBufferView(ID3D11Buffer* buffer, uint size) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
		.BufferEx = {
			.NumElements = size / sizeof(uint),
			.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW
		}
	};
	ID3D11ShaderResourceView* view;
	if (HRESULT rs = dev->CreateShaderResourceView(buffer, &srvDesc, &view); FAILED(rs))
		throw std::runtime_error(std::format("Failed to create buffer view: {}", hresultToStr(rs)));
	return view;
}

tuple<IDXGIAdapter*, size_t, D3D_DRIVER_TYPE> RendererDx11::pickAdapter(IDXGIFactory* factory, u32vec2& preferred) noexcept {
	IDXGIAdapter* adapter = nullptr;
	DXGI_ADAPTER_DESC desc;
	if (preferred != u32vec2(0))
		for (uint i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); ++i) {
			if (SUCCEEDED(adapter->GetDesc(&desc)) && desc.VendorId == preferred.x && desc.DeviceId == preferred.y) {
				if (HRESULT rs = d3d11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, nullptr, nullptr, nullptr); FAILED(rs)) {
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the specified device %04X:%04X: %s", preferred.x, preferred.y, hresultToStr(rs).data());
					comRelease(adapter);
					break;
				}
				return tuple(adapter, desc.DedicatedVideoMemory, D3D_DRIVER_TYPE_UNKNOWN);
			}
			comRelease(adapter);
		}
	preferred = u32vec2(0);

	size_t memest;
	uint score;
	IDXGIAdapter* nxtAdp;
	D3D_FEATURE_LEVEL nxtLevel;
	for (uint i = 0; SUCCEEDED(factory->EnumAdapters(i, &nxtAdp)); ++i) {
		if (SUCCEEDED(d3d11CreateDevice(nxtAdp, D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, nullptr, &nxtLevel, nullptr))) {
			size_t nxtMemest = SUCCEEDED(nxtAdp->GetDesc(&desc)) ? desc.DedicatedVideoMemory : 0;
			if (uint nxtScore = (featureLevels.size() - (rng::find(featureLevels, nxtLevel) - featureLevels.begin())) * 4 + nxtMemest / 1024 / 1024 / 1024; !adapter || nxtScore > score) {
				comRelease(adapter);
				adapter = nxtAdp;
				memest = nxtMemest;
				score = nxtScore;
				continue;
			}
		}
		comRelease(nxtAdp);
	}
	if (!adapter) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to find an adapter");
		return tuple(nullptr, 0, D3D_DRIVER_TYPE_HARDWARE);
	}
	return tuple(adapter, memest, D3D_DRIVER_TYPE_UNKNOWN);
}

template <Derived<IUnknown> T>
void RendererDx11::comRelease(T*& obj) noexcept {
	if (obj) {
		obj->Release();
		obj = nullptr;
	}
}

Renderer::Info RendererDx11::getInfo() const noexcept {
	Info info = {
		.devices = { Info::Device(u32vec2(0), "auto") },
		.compressions = { Settings::Compression::none },
		.texSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
		.curCompression = compression
	};
	if (canBgra5551 || canBgr565)
		info.compressions.push_back(Settings::Compression::b16);

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
