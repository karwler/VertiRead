#ifdef WITH_DIRECT3D
#include "rendererDx.h"
#include "optional/d3d.h"
#ifdef EXT_DIRECT3D_SHADERS
#include "fileSys.h"
#include "world.h"
#endif
#ifdef WITH_SDL3
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_version.h>
#else
#include <SDL_log.h>
#include <SDL_syswm.h>
#include <SDL_version.h>
#endif
#include <glm/gtc/type_ptr.hpp>

void RendererDx11::ViewDx::reset() {
	for (ComPtr<ID3D11RenderTargetView>& it : tgts)
		it.Reset();
	view.Reset();
}

RendererDx11::RendererDx11(InitParams& initParams, Settings* sets) :
	Renderer(initParams.numWindows, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION),
	views(std::make_unique<ViewDx[]>(initParams.numWindows)),
	syncInterval(sets->vsync),
	usesSrgb(sets->gammaType == Settings::Gamma::srgb)
{
	try {
		ComPtr<IDXGIFactory1> factory = createFactory();
		auto [adapter, adapterMem, driverType] = pickAdapter(factory.Get(), sets->device);
		if (HRESULT rs = d3d11CreateDevice(adapter.Get(), driverType, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, dev.GetAddressOf(), nullptr, ctx.GetAddressOf()); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create device: {}", hresultToStr(rs)));

		initGuiShader();
		if (sets->gammaType == Settings::Gamma::value)
			initFinShader(sets);
		if (D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS opts; SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &opts, sizeof(opts))) && opts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
			initConverter();
		ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		for (uint8 i = 0; i < numViews; ++i) {
			viewRefs[i] = &views[i];
			views[i].win = initParams.windows[i];
			views[i].rect.pos() = initParams.vofs[i] - initParams.vofs[numViews];
#if SDL_VERSION_ATLEAST(2, 26, 0)
			SDL_GetWindowSizeInPixels(initParams.windows[i], &views[i].rect.w, &views[i].rect.h);
#else
			SDL_GetWindowSize(initParams.windows[i], &views[i].rect.w, &views[i].rect.h);
#endif
			initParams.viewRes = glm::max(initParams.viewRes, views[i].rect.end());
			createSwapchain(factory.Get(), views[i]);
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
		if (HRESULT rs = dev->CreateBlendState(&blendDesc, blendState.GetAddressOf()); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create blend state: {}", hresultToStr(rs)));
		ctx->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);

		D3D11_RASTERIZER_DESC rasterizerDesc = {
			.FillMode = D3D11_FILL_SOLID,
			.CullMode = D3D11_CULL_NONE,
			.DepthClipEnable = TRUE,
			.ScissorEnable = FALSE
		};
		ComPtr<ID3D11RasterizerState> rasterizerGui;
		if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, rasterizerGui.GetAddressOf()); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create graphics rasterizer: {}", hresultToStr(rs)));
		ctx->RSSetState(rasterizerGui.Get());

		D3D11_SAMPLER_DESC samplerDesc = {
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.MaxAnisotropy = 1,
			.ComparisonFunc = D3D11_COMPARISON_ALWAYS,
			.MaxLOD = D3D11_FLOAT32_MAX
		};
		ComPtr<ID3D11SamplerState> sampleStates[2];
		if (HRESULT rs = dev->CreateSamplerState(&samplerDesc, sampleStates[0].GetAddressOf()); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create sampler state: {}", hresultToStr(rs)));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		if (HRESULT rs = dev->CreateSamplerState(&samplerDesc, sampleStates[1].GetAddressOf()); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create sampler state: {}", hresultToStr(rs)));
		ctx->PSSetSamplers(0, std::size(sampleStates), reinterpret_cast<ID3D11SamplerState**>(sampleStates));

		uint format;
		optionalFormats = {
			SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B5G6R5_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D),
			SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B5G5R5A1_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D),
			SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_B4G4R4A4_UNORM, &format)) && (format & D3D11_FORMAT_SUPPORT_TEXTURE2D)
		};
		initParams.tooltipTexture = new TextureDx(uvec2(0));
		setColors(initParams.colors);
		setCompression(sets);
		setMaxPicRes(sets->maxPicRes);
		if (!sets->picLim.size) {
			if (adapterMem)
				sets->picLim.size = adapterMem / 2;
			else
				recommendPicRamLimit(sets->picLim.size);
		}
	} catch (const std::exception&) {
		freeTexture(initParams.tooltipTexture);
		throw;
	}
}

ComPtr<IDXGIFactory1> RendererDx11::createFactory() {
	ComPtr<IDXGIFactory1> factory;
	if (HRESULT rs = createDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create factory: {}", hresultToStr(rs)));
	return factory;
}

void RendererDx11::initGuiShader() {
#ifdef EXT_DIRECT3D_SHADERS
	auto [vertData, vertSize] = World::fileSys()->readShaderFile("dxGuiVs.fxc");
	auto [pixlData, pixlSize] = World::fileSys()->readShaderFile("dxGuiPs.fxc");
	const uint32* vertSrc = vertData.get();
	const uint32* pixlSrc = pixlData.get();
#else
	static constexpr uint32 vertSrc[] = {
#ifdef NDEBUG
#include "shaders/dxGuiVs.rel.h"
#else
#include "shaders/dxGuiVs.dbg.h"
#endif
	};
	static constexpr uint32 pixlSrc[] = {
#ifdef NDEBUG
#include "shaders/dxGuiPs.rel.h"
#else
#include "shaders/dxGuiPs.dbg.h"
#endif
	};
	constexpr size_t vertSize = sizeof(vertSrc);
	constexpr size_t pixlSize = sizeof(pixlSrc);
#endif
	if (HRESULT rs = dev->CreateVertexShader(vertSrc, vertSize, nullptr, &vertGui); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create vertex shader: {}", hresultToStr(rs)));
	if (HRESULT rs = dev->CreatePixelShader(pixlSrc, pixlSize, nullptr, &pixlGui); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create pixel shader: {}", hresultToStr(rs)));

	pviewBuf = createConstantBuffer(sizeof(ViewPview));
	colorBuf = createConstantBuffer(sizeof(GlobalColors));
	instRectBuf = createConstantBuffer(sizeof(InstanceRect));
	instColorBuf = createConstantBuffer(sizeof(InstanceColor));

	ID3D11Buffer* vsBuffers[2] = { pviewBuf.Get(), instRectBuf.Get() };
	ID3D11Buffer* psBuffers[2] = { colorBuf.Get(), instColorBuf.Get() };
	ctx->VSSetConstantBuffers(0, std::size(vsBuffers), vsBuffers);
	ctx->PSSetConstantBuffers(0, std::size(psBuffers), psBuffers);

	D3D11_INPUT_ELEMENT_DESC vertexElementDesc = {
		.SemanticName = "POSITION0",
		.Format = DXGI_FORMAT_R32G32_FLOAT,
		.AlignedByteOffset = 0,
		.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA
	};
	if (HRESULT rs = dev->CreateInputLayout(&vertexElementDesc, 1, vertSrc, vertSize, &vertexLayoutGui); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create input layout: {}", hresultToStr(rs)));

	D3D11_BUFFER_DESC vertexBufferDesc = {
		.ByteWidth = sizeof(vertices),
		.Usage = D3D11_USAGE_IMMUTABLE,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER
	};
	D3D11_SUBRESOURCE_DATA vertexSubrsc = { .pSysMem = vertices.data() };
	if (HRESULT rs = dev->CreateBuffer(&vertexBufferDesc, &vertexSubrsc, &vertexBufGui); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create vertex buffer: {}", hresultToStr(rs)));
}

void RendererDx11::initFinShader(Settings* sets) noexcept {
	try {
#ifdef EXT_DIRECT3D_SHADERS
		auto [vertData, vertSize] = World::fileSys()->readShaderFile("dxFinVs.fxc");
		auto [pixlData, pixlSize] = World::fileSys()->readShaderFile("dxFinPs.fxc");
		const uint32* vertSrc = vertData.get();
		const uint32* pixlSrc = pixlData.get();
#else
		static constexpr uint32 vertSrc[] = {
#ifdef NDEBUG
#include "shaders/dxFinVs.rel.h"
#else
#include "shaders/dxFinVs.dbg.h"
#endif
		};
		static constexpr uint32 pixlSrc[] = {
#ifdef NDEBUG
#include "shaders/dxFinPs.rel.h"
#else
#include "shaders/dxFinPs.dbg.h"
#endif
		};
		constexpr size_t vertSize = sizeof(vertSrc);
		constexpr size_t pixlSize = sizeof(pixlSrc);
#endif
		if (HRESULT rs = dev->CreateVertexShader(vertSrc, vertSize, nullptr, &vertFin); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create vertex shader: {}", hresultToStr(rs)));
		if (HRESULT rs = dev->CreatePixelShader(pixlSrc, pixlSize, nullptr, &pixlFin); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create pixel shader: {}", hresultToStr(rs)));

		finBuf = createConstantBuffer(sizeof(FinalData));
		setGammaValue(sets->gammaValue);
		ctx->PSSetConstantBuffers(finBufferSlot, 1, finBuf.GetAddressOf());

		D3D11_INPUT_ELEMENT_DESC vertexElementDescs[2] = { {
			.SemanticName = "POSITION0",
			.Format = DXGI_FORMAT_R32G32_FLOAT,
			.AlignedByteOffset = 0,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA
		}, {
			.SemanticName = "TEXCOORD",
			.SemanticIndex = 0,
			.Format = DXGI_FORMAT_R32G32_FLOAT,
			.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
			.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA
		} };
		if (HRESULT rs = dev->CreateInputLayout(vertexElementDescs, std::size(vertexElementDescs), vertSrc, vertSize, &vertexLayoutFin); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create input layout: {}", hresultToStr(rs)));

		D3D11_BUFFER_DESC vertexBufferDesc = {
			.ByteWidth = sizeof(scrVertices),
			.Usage = D3D11_USAGE_IMMUTABLE,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER
		};
		D3D11_SUBRESOURCE_DATA vertexSubrsc = { .pSysMem = scrVertices.data() };
		if (HRESULT rs = dev->CreateBuffer(&vertexBufferDesc, &vertexSubrsc, &vertexBufFin); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create vertex buffer: {}", hresultToStr(rs)));
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		cleanupFinShader();
		sets->gammaType = Settings::Gamma::srgb;
		usesSrgb = true;
	}
}

void RendererDx11::cleanupFinShader() noexcept {
	ID3D11Buffer* nullBuf = nullptr;
	ctx->PSSetConstantBuffers(finBufferSlot, 1, &nullBuf);
	finBuf.Reset();
	vertexBufFin.Reset();
	vertexLayoutFin.Reset();
	pixlFin.Reset();
	vertFin.Reset();
}

void RendererDx11::initConverter() noexcept {
	try {
#ifdef EXT_DIRECT3D_SHADERS
		auto [dataRgb, sizeRgb] = World::fileSys()->readShaderFile("dxRgbCs.fxc");
		auto [dataBgr, sizeBgr] = World::fileSys()->readShaderFile("dxBgrCs.fxc");
		auto [dataRed, sizeRed] = World::fileSys()->readShaderFile("dxRedCs.fxc");
		auto [dataIdx, sizeIdx] = World::fileSys()->readShaderFile("dxIdxCs.fxc");
		const uint32* srcRgb = dataRgb.get();
		const uint32* srcBgr = dataBgr.get();
		const uint32* srcRed = dataRed.get();
		const uint32* srcIdx = dataIdx.get();
#else
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
		constexpr size_t sizeRgb = sizeof(srcRgb);
		constexpr size_t sizeBgr = sizeof(srcBgr);
		constexpr size_t sizeRed = sizeof(srcRed);
		constexpr size_t sizeIdx = sizeof(srcIdx);
#endif
		if (HRESULT rs = dev->CreateComputeShader(srcRgb, sizeRgb, nullptr, &compConv[eint(FormatConv::rgb24)]); FAILED(rs))
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute shader: %s", hresultToStr(rs).data());
		if (HRESULT rs = dev->CreateComputeShader(srcBgr, sizeBgr, nullptr, &compConv[eint(FormatConv::bgr24)]); FAILED(rs))
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute shader: %s", hresultToStr(rs).data());
		if (HRESULT rs = dev->CreateComputeShader(srcRed, sizeRed, nullptr, &compConv[eint(FormatConv::red)]); FAILED(rs))
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute shader: %s", hresultToStr(rs).data());
		if (HRESULT rs = dev->CreateComputeShader(srcIdx, sizeIdx, nullptr, &compConv[eint(FormatConv::index8)]); FAILED(rs))
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create compute shader: %s", hresultToStr(rs).data());

		if (rng::any_of(compConv, [](const ComPtr<ID3D11ComputeShader>& it) -> bool { return it; })) {
			offsetBuf = createConstantBuffer(sizeof(Offset));
			paletteBuf = createConstantBuffer(sizeof(Palette));

			ID3D11Buffer* buffers[2] = { offsetBuf.Get(), paletteBuf.Get() };
			ctx->CSSetConstantBuffers(0, std::size(buffers), buffers);
		}
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		cleanupConverter();
	}
}

void RendererDx11::cleanupConverter() noexcept {
	inputView.Reset();
	inputBuf.Reset();
	paletteBuf.Reset();
	offsetBuf.Reset();
	for (ComPtr<ID3D11ComputeShader>& it : compConv)
		it.Reset();
}

void RendererDx11::createSwapchain(IDXGIFactory1* factory, ViewDx& view) {
#ifdef WITH_SDL3
	SDL_PropertiesID props = SDL_GetWindowProperties(view.win);
	if (!props)
		throw std::runtime_error(SDL_GetError());
	auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
#else
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(view.win, &wmInfo))
		throw std::runtime_error(SDL_GetError());
	HWND hwnd = wmInfo.info.win.window;
#endif
	if (!hwnd)
		throw std::runtime_error("Window has no handle");

	DXGI_SWAP_CHAIN_DESC schainDesc = {
		.BufferDesc = {
			.Width = uint(view.rect.w),
			.Height = uint(view.rect.h),
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM
		},
		.SampleDesc = { .Count = 1 },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2,
		.OutputWindow = hwnd,
		.Windowed = TRUE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	};
	if (HRESULT rs = factory->CreateSwapChain(dev.Get(), &schainDesc, &view.sc); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create swapchain: {}", hresultToStr(rs)));
	createRenderTargets(view);
}

void RendererDx11::createRenderTargets(ViewDx& view) {
	ComPtr<ID3D11Texture2D> backBuffer;
	if (HRESULT rs = view.sc->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed get swapchain buffer: {}", hresultToStr(rs)));

	if (usesSrgb) {
		D3D11_RENDER_TARGET_VIEW_DESC tgtViewDesc = {
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D
		};
		if (HRESULT rs = dev->CreateRenderTargetView(backBuffer.Get(), &tgtViewDesc, &view.tgts[0]); FAILED(rs)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create sRGB render target: %s", hresultToStr(rs).data());
			usesSrgb = false;
		}
	}
	if (!usesSrgb) {
		if (HRESULT rs = dev->CreateRenderTargetView(backBuffer.Get(), nullptr, &view.tgts[bool(vertFin)]); FAILED(rs))
			throw std::runtime_error(fmt::format("Failed to create render target: {}", hresultToStr(rs)));
		if (vertFin) {
			ComPtr<ID3D11Texture2D> finTex = createTexture(view.rect.size(), DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
			view.view = createTextureView(finTex.Get(), DXGI_FORMAT_R8G8B8A8_UNORM);
			if (HRESULT rs = dev->CreateRenderTargetView(finTex.Get(), nullptr, &view.tgts[0]); FAILED(rs))
				throw std::runtime_error(fmt::format("Failed to create render target: {}", hresultToStr(rs)));
		}
	}
}

void RendererDx11::setColors(array<vec4, Settings::defaultColors.size()>& colors) {
	convertColors(colors.data(), colors.size(), usesSrgb, vertFin);
	bgColor = colors[eint(Color::background)];
	std::copy(colors.begin(), colors.end() - 1, mapBuffer<GlobalColors>(colorBuf.Get())->colors);
	ctx->Unmap(colorBuf.Get(), 0);
}

bool RendererDx11::setSettings(Settings* sets) {
	bool prevSrgb = usesSrgb;
	usesSrgb = sets->gammaType == Settings::Gamma::srgb;
	bool reloadGamma = bool(vertFin) != (sets->gammaType == Settings::Gamma::value);
	bool reload = usesSrgb != prevSrgb || reloadGamma;
	if (reloadGamma) {
		if (!vertFin)
			initFinShader(sets);
		else
			cleanupFinShader();
	}
	if (reload)
		for (uint8 i = 0; i < numViews; ++i) {
			views[i].reset();
			createRenderTargets(views[i]);
		}
	syncInterval = sets->vsync;
	setCompression(sets);
	return reload;
}

void RendererDx11::setGammaValue(int gamma) {
	mapBuffer<FinalData>(finBuf.Get())->gamma = 10.f / float(gamma);
	ctx->Unmap(finBuf.Get(), 0);
}

bool RendererDx11::updateView(ivec2& viewRes) {
	if (numViews == 1) {
#if SDL_VERSION_ATLEAST(2, 26, 0)
		SDL_GetWindowSizeInPixels(views[0].win, &views[0].rect.w, &views[0].rect.h);
#else
		SDL_GetWindowSize(views[0].win, &views[0].rect.w, &views[0].rect.h);
#endif
		if (views[0].rect.size() != viewRes) {
			viewRes = views[0].rect.size();
			views[0].reset();

			DXGI_SWAP_CHAIN_DESC desc;
			if (HRESULT rs = views[0].sc->GetDesc(&desc); FAILED(rs))
				throw std::runtime_error(fmt::format("Failed to get swapchain desc: {}", hresultToStr(rs)));
			if (HRESULT rs = views[0].sc->ResizeBuffers(desc.BufferCount, views[0].rect.w, views[0].rect.h, desc.BufferDesc.Format, desc.Flags); FAILED(rs))
				throw std::runtime_error(fmt::format("Failed to resize buffers: {}", hresultToStr(rs)));
			createRenderTargets(views[0]);
			return true;
		}
	}
	return false;
}

Renderer::Action RendererDx11::startDraw(uint vid) noexcept {
	D3D11_VIEWPORT viewport = {
		.Width = float(views[vid].rect.w),
		.Height = float(views[vid].rect.h)
	};
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, views[vid].tgts[0].GetAddressOf(), nullptr);
	ctx->ClearRenderTargetView(views[vid].tgts[0].Get(), glm::value_ptr(bgColor));

	uint vertexStride = sizeof(vec2);
	uint vertexOffset = 0;
	ctx->VSSetShader(vertGui.Get(), nullptr, 0);
	ctx->PSSetShader(pixlGui.Get(), nullptr, 0);
	ctx->IASetInputLayout(vertexLayoutGui.Get());
	ctx->IASetVertexBuffers(0, 1, vertexBufGui.GetAddressOf(), &vertexStride, &vertexOffset);

	try {
		mapBuffer<ViewPview>(pviewBuf.Get())->pview = vec4(views[vid].rect.pos(), vec2(views[vid].rect.size()) / 2.f);
		ctx->Unmap(pviewBuf.Get(), 0);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return Action::skip;
	}
	return Action::yes;
}

void RendererDx11::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
	try {
		InstanceRect* minstance = mapBuffer<InstanceRect>(instRectBuf.Get());
		minstance->rect = rect.asVec();
		minstance->frame = frame.asVec();
		ctx->Unmap(instRectBuf.Get(), 0);

		mapBuffer<InstanceColor>(instColorBuf.Get())->colorId = eint(color);
		ctx->Unmap(instColorBuf.Get(), 0);

		ctx->PSSetShaderResources(0, 1, static_cast<const TextureDx*>(tex)->view.GetAddressOf());
		ctx->Draw(vertices.size(), 0);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
}

Renderer::Action RendererDx11::finishDraw(uint vid) noexcept {
	if (vertFin) {
		vec4 clearColor(0.f, 0.f, 0.f, 1.f);
		ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
		ctx->OMSetRenderTargets(1, views[vid].tgts[1].GetAddressOf(), nullptr);
		ctx->ClearRenderTargetView(views[vid].tgts[1].Get(), glm::value_ptr(clearColor));

		uint vertexStride = sizeof(ScreenVertex);
		uint vertexOffset = 0;
		ctx->VSSetShader(vertFin.Get(), nullptr, 0);
		ctx->PSSetShader(pixlFin.Get(), nullptr, 0);
		ctx->IASetInputLayout(vertexLayoutFin.Get());
		ctx->IASetVertexBuffers(0, 1, vertexBufFin.GetAddressOf(), &vertexStride, &vertexOffset);

		ID3D11ShaderResourceView* nullView = nullptr;
		ctx->PSSetShaderResources(0, 1, views[vid].view.GetAddressOf());
		ctx->Draw(scrVertices.size(), 0);
		ctx->PSSetShaderResources(0, 1, &nullView);
		ctx->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);
	}
	views[vid].sc->Present(syncInterval, 0);
	return Action::yes;
}

D3D11_MAPPED_SUBRESOURCE RendererDx11::mapResource(ID3D11Resource* rsc) {
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(rsc, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to map resource: {}", hresultToStr(rs)));
	return mapRsc;
}

Texture* RendererDx11::texFromSurface(SDL_Surface* img, bool rpic, bool) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION), rpic && usesSrgb); si.img) {
		TextureDx* tex = nullptr;
		try {
			tex = new TextureDx(uvec2(si.img->w, si.img->h));
			tex->view = si.fmt
				? createTextureDirect(si.img->pixels, tex->res, si.img->pitch, si.fmt)
				: createTextureIndirect(si.img->pixels, tex->res, si.img->pitch, surfaceBytesPpx(si.img.get()), surfacePalette(si.img.get()), si.fcid, rpic && usesSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			delete tex;
		}
	}
	return nullptr;
}

bool RendererDx11::texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION), rpic && usesSrgb); si.img) {
		auto dtx = static_cast<TextureDx*>(tex);
		try {
			uvec2 res(si.img->w, si.img->h);
			dtx->view = si.fmt
				? createTextureDirect(si.img->pixels, res, si.img->pitch, si.fmt)
				: createTextureIndirect(si.img->pixels, res, si.img->pitch, surfaceBytesPpx(si.img.get()), surfacePalette(si.img.get()), si.fcid, rpic && usesSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
			dtx->res = res;
			return true;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	return false;
}

Texture* RendererDx11::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x) {
		TextureDx* tex = nullptr;
		try {
			tex = new TextureDx(glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)));
			tex->view = compConv[eint(FormatConv::red)]
				? createTextureIndirect(pm.pix.get(), tex->res, pm.res.x, 1, nullptr, FormatConv::red, DXGI_FORMAT_R8G8B8A8_UNORM)
				: createTextureDirect(textBuffer.fromText(pm, tex->res), tex->res, tex->res.x * 4, DXGI_FORMAT_R8G8B8A8_UNORM);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			delete tex;
		}
	}
	return nullptr;
}

bool RendererDx11::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (pm.res.x) {
		auto dtx = static_cast<TextureDx*>(tex);
		try {
			uvec2 res = glm::min(pm.res, uvec2(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION));
			dtx->view = compConv[eint(FormatConv::red)]
				? createTextureIndirect(pm.pix.get(), res, pm.res.x, 1, nullptr, FormatConv::red, DXGI_FORMAT_R8G8B8A8_UNORM)
				: createTextureDirect(textBuffer.fromText(pm, res), res, res.x * 4, DXGI_FORMAT_R8G8B8A8_UNORM);
			dtx->res = res;
			return true;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
	return false;
}

void RendererDx11::freeTexture(Texture* tex) noexcept {
	delete static_cast<TextureDx*>(tex);
}

ComPtr<ID3D11ShaderResourceView> RendererDx11::createTextureDirect(const void* pix, uvec2 res, uint pitch, DXGI_FORMAT format) {
	D3D11_SUBRESOURCE_DATA subrscData = {
		.pSysMem = pix,
		.SysMemPitch = pitch
	};
	return createTextureView(createTexture(res, format, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, &subrscData).Get(), format);
}

ComPtr<ID3D11ShaderResourceView> RendererDx11::createTextureIndirect(const void* pix, uvec2 res, uint pitch, uint8 bpp, const SDL_Palette* palette, FormatConv fcid, DXGI_FORMAT format) {
	ComPtr<ID3D11Texture2D> texture = createTexture(res, format, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
	ComPtr<ID3D11ShaderResourceView> view = createTextureView(texture.Get(), format);

	if (fcid == FormatConv::index8) {
		copyPalette(mapBuffer<Palette>(paletteBuf.Get())->colors, palette);
		ctx->Unmap(paletteBuf.Get(), 0);
	}

	uint rowSize = res.x * bpp;
	uint texels = res.x * res.y;
	if (uint isize = rowSize * res.y; isize > inputSize)
		replaceInputBuffer(ceilAlignment(isize, sizeof(uint)));
	copyPixels(mapResource(inputBuf.Get()).pData, pix, rowSize, pitch, res.y);
	ctx->Unmap(inputBuf.Get(), 0);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
	};
	ComPtr<ID3D11UnorderedAccessView> uav;
	if (HRESULT rs = dev->CreateUnorderedAccessView(texture.Get(), &uavDesc, uav.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create UAV: {}", hresultToStr(rs)));

	ctx->CSSetShader(compConv[eint(fcid)].Get(), nullptr, 0);
	ctx->CSSetUnorderedAccessViews(0, 1, uav.GetAddressOf(), nullptr);

	uint numGroups = texels / convStep + bool(texels % convStep);
	for (uint gcnt, offs = 0; offs < numGroups; offs += gcnt) {
		gcnt = std::min(numGroups - offs, uint(D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION));
		mapBuffer<Offset>(offsetBuf.Get())->offset = offs;
		ctx->Unmap(offsetBuf.Get(), 0);
		ctx->Dispatch(gcnt, 1, 1);
	}

	ID3D11UnorderedAccessView* nullUav = nullptr;
	ctx->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);
	return view;
}

void RendererDx11::replaceInputBuffer(uint isize) {
	D3D11_BUFFER_DESC bufferDesc = {
		.ByteWidth = isize,
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_SHADER_RESOURCE,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
	};
	ComPtr<ID3D11Buffer> buffer;
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create input buffer: {}", hresultToStr(rs)));

	ComPtr<ID3D11ShaderResourceView> view = createBufferView(buffer.Get(), isize);
	inputBuf.Attach(buffer.Detach());
	inputView.Attach(view.Detach());
	inputSize = isize;
	ctx->CSSetShaderResources(0, 1, inputView.GetAddressOf());
}

RendererDx11::SurfaceInfo RendererDx11::pickPixFormat(SDL_Surface* img, bool srgb) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (surfaceFormat(img)) {
	case SDL_PIXELFORMAT_ABGR8888:
		return SurfaceInfo(img, srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
	case SDL_PIXELFORMAT_ARGB8888:
		return SurfaceInfo(img, srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM);
	case SDL_PIXELFORMAT_XRGB8888:
		return SurfaceInfo(img, srgb ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM);
	case SDL_PIXELFORMAT_RGB24:
		if (compConv[eint(FormatConv::rgb24)])
			return SurfaceInfo(img, FormatConv::rgb24);
		break;
	case SDL_PIXELFORMAT_BGR24:
		if (compConv[eint(FormatConv::bgr24)])
			return SurfaceInfo(img, FormatConv::bgr24);
		break;
#ifdef WITH_SDL3
	case SDL_PIXELFORMAT_ABGR2101010:
		return SurfaceInfo(img, DXGI_FORMAT_R10G10B10A2_UNORM);
#endif
	case SDL_PIXELFORMAT_RGB565:
		if (optionalFormats[eint(OptTexFmt::R5G6B5)])
			return SurfaceInfo(img, DXGI_FORMAT_B5G6R5_UNORM);
		break;
	case SDL_PIXELFORMAT_ARGB1555:
		if (optionalFormats[eint(OptTexFmt::A1R5G5B5)])
			return SurfaceInfo(img, DXGI_FORMAT_B5G5R5A1_UNORM);
		break;
	case SDL_PIXELFORMAT_ARGB4444:
		if (optionalFormats[eint(OptTexFmt::A4R4G4B4)])
			return SurfaceInfo(img, DXGI_FORMAT_B4G4R4A4_UNORM);
		break;
	case SDL_PIXELFORMAT_INDEX8:
		if (compConv[eint(FormatConv::index8)])
			return SurfaceInfo(img, FormatConv::index8);
	}
	return SurfaceInfo(convertReplace(img), srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
}

pair<SDL_PixelFormatEnum, uint8> RendererDx11::prepareImageFormat(SDL_Surface* img) const noexcept {
	SDL_PixelFormatEnum fmt = surfaceFormat(img);
	if (compression == Settings::Compression::b16 && (SDL_BYTESPERPIXEL(fmt) > 2 || SDL_ISPIXELFORMAT_INDEXED(fmt)))
		fmt = SDL_ISPIXELFORMAT_ALPHA(fmt) ? SDL_PIXELFORMAT_ARGB1555 : SDL_PIXELFORMAT_RGB565;

	switch (fmt) {
	case SDL_PIXELFORMAT_BGR565: case SDL_PIXELFORMAT_RGB565: case SDL_PIXELFORMAT_RGB332:
		return pickImageFormat({ OptTexFmt::R5G6B5, OptTexFmt::A1R5G5B5 }, fmt);
	case SDL_PIXELFORMAT_ABGR1555: case SDL_PIXELFORMAT_ARGB1555: case SDL_PIXELFORMAT_BGRA5551: case SDL_PIXELFORMAT_RGBA5551: case SDL_PIXELFORMAT_XBGR1555: case SDL_PIXELFORMAT_XRGB1555:
		return pickImageFormat({ OptTexFmt::A1R5G5B5, OptTexFmt::R5G6B5 }, fmt);
	case SDL_PIXELFORMAT_ABGR4444: case SDL_PIXELFORMAT_ARGB4444: case SDL_PIXELFORMAT_BGRA4444: case SDL_PIXELFORMAT_RGBA4444: case SDL_PIXELFORMAT_XBGR4444: case SDL_PIXELFORMAT_XRGB4444:
		return pickImageFormat({ OptTexFmt::A4R4G4B4, SDL_ISPIXELFORMAT_ALPHA(fmt) ? OptTexFmt::A1R5G5B5 : OptTexFmt::R5G6B5, SDL_ISPIXELFORMAT_ALPHA(fmt) ? OptTexFmt::R5G6B5 : OptTexFmt::A1R5G5B5 }, fmt);
#ifdef WITH_SDL3
	default:
		if (SDL_ISPIXELFORMAT_10BIT(fmt) || SDL_BYTESPERPIXEL(fmt) > 4)
			return pair(SDL_PIXELFORMAT_ABGR2101010, 4);
#endif
	}
	return pair(fmt, 4);
}

pair<SDL_PixelFormatEnum, uint8> RendererDx11::pickImageFormat(std::initializer_list<OptTexFmt> fmtv, SDL_PixelFormatEnum orig) const noexcept {
	for (OptTexFmt it : fmtv)
		if (optionalFormats[eint(it)])
			return pair(optTexFmtsSdl[eint(it)], SDL_BYTESPERPIXEL(optTexFmtsSdl[eint(it)]));
	return pair(orig, 4);
}

void RendererDx11::setCompression(Settings* sets) noexcept {
	if (sets->compression != Settings::Compression::none && !(sets->compression == Settings::Compression::b16 && canTexturesB16()))
		sets->compression = Settings::Compression::none;
	compression = sets->compression;
}

ComPtr<ID3D11Buffer> RendererDx11::createConstantBuffer(uint size) const {
	D3D11_BUFFER_DESC bufferDesc = {
		.ByteWidth = uint(ceilAlignment(size, 16)),
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
	};
	ComPtr<ID3D11Buffer> buffer;
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create constant buffer: {}", hresultToStr(rs)));
	return buffer;
}

ComPtr<ID3D11Texture2D> RendererDx11::createTexture(uvec2 res, DXGI_FORMAT format, D3D11_USAGE usage, uint bindFlags, uint accessFlags, const D3D11_SUBRESOURCE_DATA* subrscData) const {
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
	ComPtr<ID3D11Texture2D> tex;
	if (HRESULT rs = dev->CreateTexture2D(&texDesc, subrscData, tex.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create texture: {}", hresultToStr(rs)));
	return tex;
}

ComPtr<ID3D11ShaderResourceView> RendererDx11::createTextureView(ID3D11Texture2D* tex, DXGI_FORMAT format) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = format,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = { .MipLevels = 1 }
	};
	ComPtr<ID3D11ShaderResourceView> view;
	if (HRESULT rs = dev->CreateShaderResourceView(tex, &srvDesc, view.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create texture view: {}", hresultToStr(rs)));
	return view;
}

ComPtr<ID3D11ShaderResourceView> RendererDx11::createBufferView(ID3D11Buffer* buffer, uint size) {
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		.Format = DXGI_FORMAT_R32_TYPELESS,
		.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX,
		.BufferEx = {
			.NumElements = size / uint(sizeof(uint)),
			.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW
		}
	};
	ComPtr<ID3D11ShaderResourceView> view;
	if (HRESULT rs = dev->CreateShaderResourceView(buffer, &srvDesc, view.GetAddressOf()); FAILED(rs))
		throw std::runtime_error(fmt::format("Failed to create buffer view: {}", hresultToStr(rs)));
	return view;
}

tuple<ComPtr<IDXGIAdapter1>, size_t, D3D_DRIVER_TYPE> RendererDx11::pickAdapter(IDXGIFactory1* factory, u32vec2& preferred) noexcept {
	ComPtr<IDXGIAdapter1> adapter;
	DXGI_ADAPTER_DESC1 desc;
	if (preferred != u32vec2(0))
		for (uint i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); ++i)
			if (SUCCEEDED(adapter->GetDesc1(&desc)) && desc.VendorId == preferred.x && desc.DeviceId == preferred.y) {
				if (HRESULT rs = d3d11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, nullptr, nullptr, nullptr); FAILED(rs)) {
					SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create the specified device %04X:%04X: %s", preferred.x, preferred.y, hresultToStr(rs).data());
					break;
				}
				return tuple(std::move(adapter), desc.DedicatedVideoMemory, D3D_DRIVER_TYPE_UNKNOWN);
			}
	preferred = u32vec2(0);

	size_t memest;
	uint score;
	ComPtr<IDXGIAdapter1> nxtAdp;
	D3D_FEATURE_LEVEL nxtLevel;
	for (uint i = 0; SUCCEEDED(factory->EnumAdapters1(i, &nxtAdp)); ++i)
		if (SUCCEEDED(d3d11CreateDevice(nxtAdp.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, deviceFlags, featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, nullptr, &nxtLevel, nullptr))) {
			size_t nxtMemest = SUCCEEDED(nxtAdp->GetDesc1(&desc)) ? desc.DedicatedVideoMemory : 0;
			uint nxtScore = (featureLevels.size() - (rng::find(featureLevels, nxtLevel) - featureLevels.begin())) * 4;
			if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
				nxtScore += nxtMemest / 1024 / 1024 / 1024;
			if (!adapter || nxtScore > score) {
				adapter.Attach(nxtAdp.Detach());
				memest = nxtMemest;
				score = nxtScore;
			}
		}
	if (!adapter) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to find an adapter");
		return tuple(ComPtr<IDXGIAdapter1>(), 0, D3D_DRIVER_TYPE_HARDWARE);
	}
	return tuple(std::move(adapter), memest, D3D_DRIVER_TYPE_UNKNOWN);
}

Renderer::Info RendererDx11::getInfo() const {
	Info info = {
		.devices = { Info::Device(u32vec2(0), "auto") },
		.gamma = { Settings::Gamma::none, Settings::Gamma::srgb, Settings::Gamma::value },
		.compressions = { Settings::Compression::none },
		.texSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
		.curGamma = vertFin ? Settings::Gamma::value : usesSrgb ? Settings::Gamma::srgb : Settings::Gamma::none,
		.curCompression = compression
	};
	if (canTexturesB16())
		info.compressions.push_back(Settings::Compression::b16);

	try {
		ComPtr<IDXGIFactory1> factory = createFactory();
		ComPtr<IDXGIAdapter1> adapter;
		for (uint i = 0; SUCCEEDED(factory->EnumAdapters1(i, &adapter)); ++i)
			if (DXGI_ADAPTER_DESC1 desc; SUCCEEDED(adapter->GetDesc1(&desc)))
				info.devices.emplace_back(u32vec2(desc.VendorId, desc.DeviceId), swtos(desc.Description), desc.DedicatedVideoMemory);
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
	}
	return info;
}
#endif
