#include "renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <regex>
#ifdef WITH_DIRECTX
#include <comdef.h>
#include <d3dcompiler.h>
#include <SDL_syswm.h>
#endif

Renderer::View::View(SDL_Window* window, const Rect& area) :
	win(window),
	rect(area)
{}

// RENDERER DX

#ifdef WITH_DIRECTX
RendererDx::TextureDx::TextureDx(ivec2 size, ID3D11Texture2D* texture, ID3D11ShaderResourceView* textureView) :
	Texture(size),
	tex(texture),
	view(textureView)
{}

void RendererDx::TextureDx::free() {
	view->Release();
	tex->Release();
}

RendererDx::ViewDx::ViewDx(SDL_Window* window, const Rect& area, IDXGISwapChain* swapchain, ID3D11RenderTargetView* backbuffer) :
	View(window, area),
	sc(swapchain),
	tgt(backbuffer)
{}

RendererDx::RendererDx(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, const vec4& bgcolor) :
	syncInterval(toSwapEffect(sets->vsync)),
	bgColor(bgcolor)
{
#ifdef NDEBUG
	uint flags = 0;
#else
	uint flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	if (HRESULT rs = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	IDXGIFactory* factory = createFactory();
	if (windows.size() == 1 && windows.begin()->first == singleDspId) {
		SDL_GetWindowSize(windows.begin()->second, &viewRes.x, &viewRes.y);
		auto [swapchain, backbuffer] = createSwapchain(factory, windows.begin()->second, viewRes);
		views.emplace(singleDspId, new ViewDx(windows.begin()->second, Rect(ivec2(0), viewRes), swapchain, backbuffer));
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			const Rect& area = sets->displays[id];
			auto [swapchain, backbuffer] = createSwapchain(factory, win, area.size());
			views.emplace(id, new ViewDx(win, area, swapchain, backbuffer));
			viewRes = glm::max(viewRes, area.end());
		}
	}
	factory->Release();

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* blendState;
	if (HRESULT rs = dev->CreateBlendState(&blendDesc, &blendState); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
	blendState->Release();
	initShader();
}

RendererDx::~RendererDx() {
	for (auto [id, view] : views)
		static_cast<ViewDx*>(view)->sc->SetFullscreenState(FALSE, nullptr);	// TODO: only when gone fullscreen

	if (outAddr)
		outAddr->Release();
	if (tgtAddr)
		tgtAddr->Release();
	if (texAddr)
		texAddr->Release();
	if (rasterizerSel)
		rasterizerSel->Release();
	if (rasterizerGui)
		rasterizerGui->Release();
	if (sampleState)
		sampleState->Release();
	if (addrDatBuf)
		addrDatBuf->Release();
	if (psDatBuf)
		psDatBuf->Release();
	if (vsDatBuf)
		vsDatBuf->Release();
	if (pixlSel)
		pixlSel->Release();
	if (vertSel)
		vertSel->Release();
	if (pixlGui)
		pixlGui->Release();
	if (vertGui)
		vertGui->Release();
	for (auto [id, view] : views) {
		static_cast<ViewDx*>(view)->tgt->Release();
		static_cast<ViewDx*>(view)->sc->Release();
		delete view;
	}
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

pair<IDXGISwapChain*, ID3D11RenderTargetView*> RendererDx::createSwapchain(IDXGIFactory* factory, SDL_Window* win, ivec2 res) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWindowWMInfo(win, &wmInfo))
		throw std::runtime_error(SDL_GetError());

	DXGI_SWAP_CHAIN_DESC schainDesc{};
	schainDesc.BufferCount = 1;
	schainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	schainDesc.BufferDesc.Width = res.x;
	schainDesc.BufferDesc.Height = res.y;
	schainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	schainDesc.OutputWindow = wmInfo.info.win.window;
	schainDesc.SampleDesc.Count = 1;
	schainDesc.Windowed = !(SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN);
	schainDesc.SwapEffect = syncInterval;

	IDXGISwapChain* swapchain;
	if (HRESULT rs = factory->CreateSwapChain(dev, &schainDesc, &swapchain); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	ID3D11Texture2D* scBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&scBackBuffer));
	ID3D11RenderTargetView* backbuffer;
	if (HRESULT rs = dev->CreateRenderTargetView(scBackBuffer, nullptr, &backbuffer); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	scBackBuffer->Release();
	return pair(swapchain, backbuffer);
}

void RendererDx::recreateSwapchain(IDXGIFactory* factory, ViewDx* view) {
	view->tgt->Release();
	view->sc->Release();
	std::tie(view->sc, view->tgt) = createSwapchain(factory, view->win, view->rect.size());
}

void RendererDx::initShader() {
	const char* vertSrc = R"r(
struct VertOut {
	float4 pos : SV_POSITION;
	float2 tuv : TEXCOORD0;
};

cbuffer VsData : register(b0) {
	int4 rect;
	int4 frame;
	float2 pview;
};

static const float2 vposs[] = {
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

VertOut main(uint vid : SV_VertexId) {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	VertOut vout;
	if (dst[2] > 0.f && dst[3] > 0.f) {
		float4 uvrc = float4(dst.xy - rect.xy, dst.zw) / float4(rect.zwzw);
		vout.tuv = vposs[vid] * uvrc.zw + uvrc.xy;
		float2 loc = vposs[vid] * dst.zw + dst.xy;
		vout.pos = float4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.f, 1.f);
	} else {
		vout.tuv = float2(0.f, 0.f);
		vout.pos = float4(-2.f, -2.f, 0.f, 1.f);
	}
	return vout;
})r";
	const char* pixlSrc = R"r(
Texture2D textureView : register(t0);
SamplerState sampleState : register(s0);

cbuffer PsData : register(b0) {
	float4 color;
};

float4 main(float4 pos : SV_POSITION, float2 tuv : TEXCOORD0) : SV_TARGET {
	return textureView.Sample(sampleState, tuv) * color;
})r";
	std::tie(vertGui, pixlGui) = createShader(vertSrc, pixlSrc, "gui");

	vertSrc = R"r(
cbuffer VsData : register(b0) {
	int4 rect;
	int4 frame;
	float2 pview;
};

static const float2 vposs[] = {
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 1.f)
};

float4 main(uint vid : SV_VertexId) : SV_POSITION {
	float4 dst = float4(0.f, 0.f, 0.f, 0.f);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = max(rect.xy, frame.xy);
		dst.zw = min(rect.xy + rect.zw, frame.xy + frame.zw) - dst.xy;
	}

	if (dst[2] > 0.f && dst[3] > 0.f) {
		float2 loc = vposs[vid] * dst.zw + dst.xy;
		return float4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.f, 1.f);
	}
	return float4(-2.f, -2.f, 0.f, 1.f);
})r";
	pixlSrc = R"r(
cbuffer AddrData : register(b1) {
	uint2 addr;
};

uint2 main(float4 pos : SV_POSITION) : SV_TARGET {
	if (addr.x != 0 || addr.y != 0)
		return addr;
	discard;
	return uint2(0, 0);
})r";
	std::tie(vertSel, pixlSel) = createShader(vertSrc, pixlSrc, "sel");

	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(VsData);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &vsDatBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->VSSetConstantBuffers(0, 1, &vsDatBuf);

	bufferDesc.ByteWidth = sizeof(PsData);
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &psDatBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->PSSetConstantBuffers(0, 1, &psDatBuf);

	bufferDesc.ByteWidth = sizeof(AddrData);
	if (HRESULT rs = dev->CreateBuffer(&bufferDesc, nullptr, &addrDatBuf); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	ctx->PSSetConstantBuffers(1, 1, &addrDatBuf);

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

	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerGui); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));

	rasterizerDesc.ScissorEnable = TRUE;
	if (HRESULT rs = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerSel); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	D3D11_RECT scissor = { 0, 0, 1, 1 };
	ctx->RSSetScissorRects(1, &scissor);
	ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

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

pair<ID3D11VertexShader*, ID3D11PixelShader*> RendererDx::createShader(const char* vertSrc, const char* pixlSrc, const char* name) const {
#ifdef NDEBUG
	uint flags = D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#else
	uint flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ID3DBlob* error;
	ID3DBlob* blob;
	if (HRESULT rs = D3DCompile(vertSrc, strlen(vertSrc), (name + ".vs"s).c_str(), nullptr, nullptr, "main", "vs_5_0", flags, 0, &blob, &error); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs) + ": " + static_cast<const char*>(error->GetBufferPointer()));
	ID3D11VertexShader* vertShader;
	if (HRESULT rs = dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vertShader); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	blob->Release();

	if (HRESULT rs = D3DCompile(pixlSrc, strlen(pixlSrc), (name + ".ps"s).c_str(), nullptr, nullptr, "main", "ps_5_0", flags, 0, &blob, &error); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs) + ": " + static_cast<const char*>(error->GetBufferPointer()));
	ID3D11PixelShader* pixlShader;
	if (HRESULT rs = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pixlShader); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	blob->Release();
	return pair(vertShader, pixlShader);
}

void RendererDx::setClearColor(const vec4& color) {
	bgColor = color;
}

void RendererDx::setSwapInterval(Settings::VSync& vsync) {
	syncInterval = toSwapEffect(vsync);
	IDXGIFactory* factory = createFactory();
	for (auto [id, view] : views)
		recreateSwapchain(factory, static_cast<ViewDx*>(view));
	factory->Release();
}

void RendererDx::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		IDXGIFactory* factory = createFactory();
		SDL_GetWindowSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;
		recreateSwapchain(factory, static_cast<ViewDx*>(views.begin()->second));
		factory->Release();
	}
}

void RendererDx::startDraw(const View* view) {
	D3D11_VIEWPORT viewport{};
	viewport.Width = float(view->rect.w);
	viewport.Height = float(view->rect.h);

	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);
	ctx->RSSetState(rasterizerGui);
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, &static_cast<const ViewDx*>(view)->tgt, nullptr);
	ctx->ClearRenderTargetView(static_cast<const ViewDx*>(view)->tgt, glm::value_ptr(bgColor));
	vsData.pview = vec2(view->rect.size()) / 2.f;
}

void RendererDx::drawRect(const Texture* tex, const Rect& rect, const Rect& frame, const vec4& color) {
	ctx->PSSetShaderResources(0, 1, &static_cast<const TextureDx*>(tex)->view);
	vsData.rect = rect.toVec();
	vsData.frame = frame.toVec();
	uploadBuffer(vsDatBuf, vsData);
	uploadBuffer(psDatBuf, PsData{ color });
	ctx->Draw(4, 0);
}

void RendererDx::finishDraw(const View* view) {
	static_cast<const ViewDx*>(view)->sc->Present(syncInterval, 0);
}

void RendererDx::startSelDraw(const View* view, ivec2 pos) {
	float zero[4] = { 0.f, 0.f, 0.f, 0.f };
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = float(-pos.x);
	viewport.TopLeftY = float(-pos.y);
	viewport.Width = float(view->rect.w);
	viewport.Height = float(view->rect.h);

	ctx->VSSetShader(vertGui, nullptr, 0);
	ctx->PSSetShader(pixlGui, nullptr, 0);
	ctx->RSSetState(rasterizerSel);
	ctx->RSSetViewports(1, &viewport);
	ctx->OMSetRenderTargets(1, &tgtAddr, nullptr);
	ctx->ClearRenderTargetView(tgtAddr, zero);
	vsData.pview = vec2(view->rect.size()) / 2.f;
}

void RendererDx::drawSelRect(const Widget* wgt, const Rect& rect, const Rect& frame) {
	vsData.rect = rect.toVec();
	vsData.frame = frame.toVec();
	uploadBuffer(vsDatBuf, vsData);
	uploadBuffer(addrDatBuf, AddrData{ uvec2(uptrt(wgt), uptrt(wgt) >> 32) });
	ctx->Draw(4, 0);
}

Widget* RendererDx::finishSelDraw(const View* view) {
	ctx->CopyResource(outAddr, texAddr);
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(outAddr, 0, D3D11_MAP_READ, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	uvec2 val;
	memcpy(&val, mapRsc.pData, sizeof(uvec2));
	ctx->Unmap(outAddr, 0);
	return reinterpret_cast<Widget*>(uptrt(val.x) | (uptrt(val.y) << 32));
}

template <class T>
void RendererDx::uploadBuffer(ID3D11Buffer* buffer, const T& data) {	// TODO: mayve use UpdateSubresource instead
	D3D11_MAPPED_SUBRESOURCE mapRsc;
	if (HRESULT rs = ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsc); FAILED(rs))
		throw std::runtime_error(hresultToStr(rs));
	memcpy(mapRsc.pData, &data, sizeof(T));
	ctx->Unmap(buffer, 0);
}

Texture* RendererDx::texFromColor(u8vec4 color) {
	return createTexture(array<u8vec4, 4>{ color, color, color, color }.data(), ivec2(2), sizeof(uint8) * 4, DXGI_FORMAT_B8G8R8A8_UNORM);
}

Texture* RendererDx::texFromText(SDL_Surface* img) {
	if (img) {
		if (SDL_Surface* dst = SDL_CreateRGBSurfaceWithFormat(0, img->w, img->h, img->format->BitsPerPixel, img->format->format)) {	// TODO: this is necessary cause something's fucked
			SDL_BlitSurface(img, nullptr, dst, nullptr);
			SDL_FreeSurface(img);
			img = dst;
		}
		TextureDx* tex = createTexture(img->pixels, ivec2(img->w, img->h), img->pitch, DXGI_FORMAT_B8G8R8A8_UNORM);
		SDL_FreeSurface(img);
		return tex;
	}
	return nullptr;
}

Texture* RendererDx::texFromIcon(SDL_Surface* pic) {
	if (auto [img, fmt] = pickPixFormat(pic); pic) {
		TextureDx* tex = createTexture(img->pixels, ivec2(img->w, img->h), img->pitch, fmt);
		SDL_FreeSurface(img);
		return tex;
	}
	return nullptr;
}

RendererDx::TextureDx* RendererDx::createTexture(const void* pixels, ivec2 res, uint rowLen, DXGI_FORMAT format) {
	try {
		D3D11_TEXTURE2D_DESC texDesc{};
		texDesc.Width = res.x;
		texDesc.Height = res.y;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA subrscData{};
		subrscData.pSysMem = pixels;
		subrscData.SysMemPitch = rowLen;

		ID3D11Texture2D* texture;
		if (HRESULT rs = dev->CreateTexture2D(&texDesc, &subrscData, &texture); FAILED(rs))
			throw std::runtime_error(hresultToStr(rs));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = texDesc.MipLevels;

		ID3D11ShaderResourceView* textureView;
		if (HRESULT rs = dev->CreateShaderResourceView(texture, &srvDesc, &textureView); FAILED(rs)) {
			texture->Release();
			throw std::runtime_error(hresultToStr(rs));
		}
		return new TextureDx(res, texture, textureView);
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
	return nullptr;
}

pair<SDL_Surface*, DXGI_FORMAT> RendererDx::pickPixFormat(SDL_Surface* img) {
	if (img) {
		switch (img->format->format) {
		case SDL_PIXELFORMAT_BGRA32: case SDL_PIXELFORMAT_XRGB8888:
			return pair(img, DXGI_FORMAT_B8G8R8A8_UNORM);
		case SDL_PIXELFORMAT_RGBA32: case SDL_PIXELFORMAT_XBGR8888:
			return pair(img, DXGI_FORMAT_R8G8B8A8_UNORM);
		}
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_BGRA32, 0);
		SDL_FreeSurface(img);
		img = dst;
	}
	return pair(img, DXGI_FORMAT_B8G8R8A8_UNORM);
}

DXGI_SWAP_EFFECT RendererDx::toSwapEffect(Settings::VSync vsync) {
	switch (vsync) {
	case Settings::VSync::adaptive:
		return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	case Settings::VSync::immediate:
		return DXGI_SWAP_EFFECT_DISCARD;
	}
	return DXGI_SWAP_EFFECT_SEQUENTIAL;
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

// RENDERER GL

RendererGl::TextureGl::TextureGl(ivec2 size, GLuint tex) :
	Texture(size),
	id(tex)
{}

void RendererGl::TextureGl::free() {
	glDeleteTextures(1, &id);
}

RendererGl::ViewGl::ViewGl(SDL_Window* window, const Rect& area, SDL_GLContext context) :
	View(window, area),
	ctx(context)
{}

RendererGl::RendererGl(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, const vec4& bgcolor) {
	if (windows.size() == 1 && windows.begin()->first == singleDspId) {
		SDL_GetWindowSize(windows.begin()->second, &viewRes.x, &viewRes.y);
		if (!static_cast<ViewGl*>(views.emplace(singleDspId, new ViewGl(windows.begin()->second, Rect(ivec2(0), viewRes), SDL_GL_CreateContext(windows.begin()->second))).first->second)->ctx)
			throw std::runtime_error("Failed to create context:"s + linend + SDL_GetError());
		initGl(viewRes, sets->vsync, bgcolor);
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			ViewGl* view = static_cast<ViewGl*>(views.emplace(id, new ViewGl(win, sets->displays[id], SDL_GL_CreateContext(win))).first->second);
			if (!view->ctx)
				throw std::runtime_error("Failed to create context:"s + linend + SDL_GetError());
			viewRes = glm::max(viewRes, view->rect.end());
			initGl(view->rect.size(), sets->vsync, bgcolor);
		}
	}
#ifndef OPENGLES
	initFunctions();
#endif
	initShader();
}

RendererGl::~RendererGl() {
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &texSel);
	glDeleteFramebuffers(1, &fboSel);
	glDeleteProgram(progSel);
	glDeleteProgram(progGui);

	for (auto [id, view] : views) {
		SDL_GL_DeleteContext(static_cast<ViewGl*>(view)->ctx);
		delete view;
	}
}

void RendererGl::setClearColor(const vec4& color) {
	for (auto [id, view] : views) {
		SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx);
		glClearColor(color.r, color.g, color.b, color.a);
	}
}

void RendererGl::initGl(ivec2 res, Settings::VSync& vsync, const vec4& bgcolor) {
	switch (vsync) {
	case Settings::VSync::adaptive:
		if (trySetSwapIntervalSingle(vsync))
			break;
		vsync = Settings::VSync::synchronized;
	case Settings::VSync::synchronized:
		if (trySetSwapIntervalSingle(vsync))
			break;
		vsync = Settings::VSync::immediate;
	case Settings::VSync::immediate:
		trySetSwapIntervalSingle(vsync);
	}

	glViewport(0, 0, res.x, res.y);
	glClearColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glScissor(0, 0, 1, 1);	// for address pass
}

void RendererGl::setSwapInterval(Settings::VSync& vsync) {
	switch (vsync) {
	case Settings::VSync::adaptive:
		if (trySetSwapInterval(vsync))
			break;
		vsync = Settings::VSync::synchronized;
	case Settings::VSync::synchronized:
		if (trySetSwapInterval(vsync))
			break;
		vsync = Settings::VSync::immediate;
	case Settings::VSync::immediate:
		trySetSwapInterval(vsync);
	}
}

bool RendererGl::trySetSwapInterval(Settings::VSync vsync) {
	for (auto [id, view] : views)
		if (SDL_GL_MakeCurrent(view->win, static_cast<ViewGl*>(view)->ctx); !trySetSwapIntervalSingle(vsync))
			return false;
	return true;
}

bool RendererGl::trySetSwapIntervalSingle(Settings::VSync vsync) {
	if (SDL_GL_SetSwapInterval(int8(vsync))) {
		logError("swap interval ", int(vsync), " not supported");
		return false;
	}
	return true;
}

void RendererGl::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_GL_GetDrawableSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;
		glViewport(0, 0, viewRes.x, viewRes.y);
	}
}

#ifndef OPENGLES
void RendererGl::initFunctions() {
	glActiveTexture = reinterpret_cast<decltype(glActiveTexture)>(SDL_GL_GetProcAddress("glActiveTexture"));
	glAttachShader = reinterpret_cast<decltype(glAttachShader)>(SDL_GL_GetProcAddress("glAttachShader"));
	glBindFramebuffer = reinterpret_cast<decltype(glBindFramebuffer)>(SDL_GL_GetProcAddress("glBindFramebuffer"));
	glBindVertexArray = reinterpret_cast<decltype(glBindVertexArray)>(SDL_GL_GetProcAddress("glBindVertexArray"));
	glCheckFramebufferStatus = reinterpret_cast<decltype(glCheckFramebufferStatus)>(SDL_GL_GetProcAddress("glCheckFramebufferStatus"));
	glClearBufferuiv = reinterpret_cast<decltype(glClearBufferuiv)>(SDL_GL_GetProcAddress("glClearBufferuiv"));
	glCompileShader = reinterpret_cast<decltype(glCompileShader)>(SDL_GL_GetProcAddress("glCompileShader"));
	glCreateProgram = reinterpret_cast<decltype(glCreateProgram)>(SDL_GL_GetProcAddress("glCreateProgram"));
	glCreateShader = reinterpret_cast<decltype(glCreateShader)>(SDL_GL_GetProcAddress("glCreateShader"));
	glDeleteFramebuffers = reinterpret_cast<decltype(glDeleteFramebuffers)>(SDL_GL_GetProcAddress("glDeleteFramebuffers"));
	glDeleteShader = reinterpret_cast<decltype(glDeleteShader)>(SDL_GL_GetProcAddress("glDeleteShader"));
	glDeleteProgram = reinterpret_cast<decltype(glDeleteProgram)>(SDL_GL_GetProcAddress("glDeleteProgram"));
	glDeleteVertexArrays = reinterpret_cast<decltype(glDeleteVertexArrays)>(SDL_GL_GetProcAddress("glDeleteVertexArrays"));
	glDetachShader = reinterpret_cast<decltype(glDetachShader)>(SDL_GL_GetProcAddress("glDetachShader"));
	glFramebufferTexture2D = reinterpret_cast<decltype(glFramebufferTexture2D)>(SDL_GL_GetProcAddress("glFramebufferTexture2D"));
	glGenerateMipmap = reinterpret_cast<decltype(glGenerateMipmap)>(SDL_GL_GetProcAddress("glGenerateMipmap"));
	glGenFramebuffers = reinterpret_cast<decltype(glGenFramebuffers)>(SDL_GL_GetProcAddress("glGenFramebuffers"));
	glGenVertexArrays = reinterpret_cast<decltype(glGenVertexArrays)>(SDL_GL_GetProcAddress("glGenVertexArrays"));
	glGetProgramInfoLog = reinterpret_cast<decltype(glGetProgramInfoLog)>(SDL_GL_GetProcAddress("glGetProgramInfoLog"));
	glGetProgramiv = reinterpret_cast<decltype(glGetProgramiv)>(SDL_GL_GetProcAddress("glGetProgramiv"));
	glGetShaderInfoLog = reinterpret_cast<decltype(glGetShaderInfoLog)>(SDL_GL_GetProcAddress("glGetShaderInfoLog"));
	glGetShaderiv = reinterpret_cast<decltype(glGetShaderiv)>(SDL_GL_GetProcAddress("glGetShaderiv"));
	glGetUniformLocation = reinterpret_cast<decltype(glGetUniformLocation)>(SDL_GL_GetProcAddress("glGetUniformLocation"));
	glLinkProgram = reinterpret_cast<decltype(glLinkProgram)>(SDL_GL_GetProcAddress("glLinkProgram"));
	glShaderSource = reinterpret_cast<decltype(glShaderSource)>(SDL_GL_GetProcAddress("glShaderSource"));
	glUniform1i = reinterpret_cast<decltype(glUniform1i)>(SDL_GL_GetProcAddress("glUniform1i"));
	glUniform2f = reinterpret_cast<decltype(glUniform2f)>(SDL_GL_GetProcAddress("glUniform2f"));
	glUniform2fv = reinterpret_cast<decltype(glUniform2fv)>(SDL_GL_GetProcAddress("glUniform2fv"));
	glUniform2ui = reinterpret_cast<decltype(glUniform2ui)>(SDL_GL_GetProcAddress("glUniform2ui"));
	glUniform2uiv = reinterpret_cast<decltype(glUniform2uiv)>(SDL_GL_GetProcAddress("glUniform2uiv"));
	glUniform4fv = reinterpret_cast<decltype(glUniform4fv)>(SDL_GL_GetProcAddress("glUniform4fv"));
	glUniform4iv = reinterpret_cast<decltype(glUniform4iv)>(SDL_GL_GetProcAddress("glUniform4iv"));
	glUseProgram = reinterpret_cast<decltype(glUseProgram)>(SDL_GL_GetProcAddress("glUseProgram"));	
#ifndef NDEBUG
	void (APIENTRY* glDebugMessageCallback)(void (APIENTRY* callback)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const void*), const void* userParam) = reinterpret_cast<decltype(glDebugMessageCallback)>(SDL_GL_GetProcAddress("glDebugMessageCallback"));
	void (APIENTRY* glDebugMessageControl)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint * ids, GLboolean enabled) = reinterpret_cast<decltype(glDebugMessageControl)>(SDL_GL_GetProcAddress("glDebugMessageControl"));
	int gval;
	if (glGetIntegerv(GL_CONTEXT_FLAGS, &gval); (gval & GL_CONTEXT_FLAG_DEBUG_BIT) && glDebugMessageCallback && glDebugMessageControl && SDL_GL_ExtensionSupported("GL_KHR_debug")) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(debugMessage, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif
}
#endif

void RendererGl::initShader() {
	const char* vertSrc = R"r(#version 130

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec2 pview;
uniform ivec4 rect;
uniform ivec4 frame;

noperspective out vec2 fragUV;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec4 uvrc = vec4(dst.xy - vec2(rect.xy), dst.zw) / vec4(rect.zwzw);
		fragUV = vposs[gl_VertexID] * uvrc.zw + uvrc.xy;
		vec2 loc = vposs[gl_VertexID] * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.0, 1.0);
   } else {
		fragUV = vec2(0.0);
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
	}
})r";
	const char* fragSrc = R"r(#version 130

uniform sampler2D colorMap;
uniform vec4 color;

noperspective in vec2 fragUV;

out vec4 outColor;

void main() {
	outColor = texture(colorMap, fragUV) * color;
})r";
	progGui = createShader(vertSrc, fragSrc, "gui");
	uniPviewGui = glGetUniformLocation(progGui, "pview");
	uniRectGui = glGetUniformLocation(progGui, "rect");
	uniFrameGui = glGetUniformLocation(progGui, "frame");
	uniColorGui = glGetUniformLocation(progGui, "color");
	glUniform1i(glGetUniformLocation(progGui, "colorMap"), 0);

	vertSrc = R"r(#version 130

const vec2 vposs[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

uniform vec2 pview;
uniform ivec4 rect;
uniform ivec4 frame;

void main() {
	vec4 dst = vec4(0.0);
	if (rect[2] > 0 && rect[3] > 0 && frame[2] > 0 && frame[3] > 0) {
		dst.xy = vec2(max(rect.xy, frame.xy));
		dst.zw = vec2(min(rect.xy + rect.zw, frame.xy + frame.zw)) - dst.xy;
	}

	if (dst[2] > 0.0 && dst[3] > 0.0) {
		vec2 loc = vposs[gl_VertexID] * dst.zw + dst.xy;
		gl_Position = vec4((loc.x - pview.x) / pview.x, -(loc.y - pview.y) / pview.y, 0.0, 1.0);
   } else
		gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
})r";
	fragSrc = R"r(#version 130

uniform uvec2 addr;

out uvec2 outAddr;

void main() {
	if (addr.x != 0u || addr.y != 0u)
		outAddr = addr;
	else
		discard;
})r";
	progSel = createShader(vertSrc, fragSrc, "sel");
	uniPviewSel = glGetUniformLocation(progSel, "pview");
	uniRectSel = glGetUniformLocation(progSel, "rect");
	uniFrameSel = glGetUniformLocation(progSel, "frame");
	uniAddrSel = glGetUniformLocation(progSel, "addr");

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &texSel);
	glBindTexture(GL_TEXTURE_2D, texSel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, 1, 1, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, nullptr);

	glGenFramebuffers(1, &fboSel);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSel);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texSel, 0);
	checkFramebufferStatus("sel");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
}

GLuint RendererGl::createShader(const char* vertSrc, const char* fragSrc, const char* name) const {
#ifdef OPENGLES
	array<pair<std::regex, const char*>, 2> replacers = {
		pair(std::regex(R"r(#version\s+\d+)r"), "#version 300 es\nprecision highp float;precision highp int;precision highp sampler2D;"),
		pair(std::regex(R"r(noperspective\s+)r"), "")
	};
	string vertTmp = vertSrc, fragTmp = fragSrc;
	for (const auto& [rgx, rpl] : replacers) {
		vertTmp = std::regex_replace(vertTmp, rgx, rpl);
		fragTmp = std::regex_replace(fragTmp, rgx, rpl);
	}
	vertSrc = vertTmp.c_str();
	fragSrc = fragTmp.c_str();
#endif
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glCompileShader(vert);
	checkStatus(vert, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + ".vert"s);

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(frag);
	checkStatus(frag, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog, name + ".frag"s);

	GLuint sprog = glCreateProgram();
	glAttachShader(sprog, vert);
	glAttachShader(sprog, frag);
	glLinkProgram(sprog);
	glDetachShader(sprog, vert);
	glDetachShader(sprog, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	checkStatus(sprog, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog, name + " program"s);
	glUseProgram(sprog);
	return sprog;
}

template <class C, class I>
void RendererGl::checkStatus(GLuint id, GLenum stat, C check, I info, const string& name) {
	int res;
	if (check(id, stat, &res); res == GL_FALSE) {
		string err;
		if (check(id, GL_INFO_LOG_LENGTH, &res); res) {
			err.resize(res);
			info(id, res, nullptr, err.data());
			err = trim(err);
		}
		err = !err.empty() ? name + ":" + linend + err : name + ": unknown error";
		logError(err);
		throw std::runtime_error(err);
	}
}

void RendererGl::checkFramebufferStatus(const char* name) {
	switch (GLenum rc = glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
	case GL_FRAMEBUFFER_UNDEFINED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNDEFINED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"s);
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"s);
#endif
	case GL_FRAMEBUFFER_UNSUPPORTED:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_UNSUPPORTED"s);
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"s);
#ifndef OPENGLES
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		throw std::runtime_error(name + ": GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"s);
#endif
	default:
		if (rc != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error(name + ": unknown framebuffer error"s);
	}
}

void RendererGl::startDraw(const View* view) {
	SDL_GL_MakeCurrent(view->win, static_cast<const ViewGl*>(view)->ctx);
	glUseProgram(progGui);
	glUniform2f(uniPviewGui, float(view->rect.w) / 2.f, float(view->rect.h) / 2.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RendererGl::drawRect(const Texture* tex, const Rect& rect, const Rect& frame, const vec4& color) {
	glBindTexture(GL_TEXTURE_2D, static_cast<const TextureGl*>(tex)->id);
	glUniform4iv(uniRectGui, 1, reinterpret_cast<const int*>(&rect));
	glUniform4iv(uniFrameGui, 1, reinterpret_cast<const int*>(&frame));
	glUniform4fv(uniColorGui, 1, glm::value_ptr(color));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RendererGl::finishDraw(const View* view) {
	SDL_GL_SwapWindow(static_cast<const ViewGl*>(view)->win);
}

void RendererGl::startSelDraw(const View* view, ivec2 pos) {
	uint zero[4] = { 0, 0, 0, 0 };
	SDL_GL_MakeCurrent(view->win, static_cast<const ViewGl*>(view)->ctx);
	glEnable(GL_SCISSOR_TEST);
	glViewport(-pos.x, -view->rect.h + pos.y + 1, view->rect.w, view->rect.h);
	glUseProgram(progSel);
	glUniform2f(uniPviewGui, float(view->rect.w) / 2.f, float(view->rect.h) / 2.f);
	glBindFramebuffer(GL_FRAMEBUFFER, fboSel);
	glClearBufferuiv(GL_COLOR, 0, zero);
}

void RendererGl::drawSelRect(const Widget* wgt, const Rect& rect, const Rect& frame) {
	glUniform4iv(uniRectSel, 1, reinterpret_cast<const int*>(&rect));
	glUniform4iv(uniFrameSel, 1, reinterpret_cast<const int*>(&frame));
	glUniform2ui(uniAddrSel, uptrt(wgt), uptrt(wgt) >> 32);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

Widget* RendererGl::finishSelDraw(const View* view) {
	uvec2 val;
	glReadPixels(0, 0, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, glm::value_ptr(val));
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, view->rect.w, view->rect.h);
	glDisable(GL_SCISSOR_TEST);
	return reinterpret_cast<Widget*>(uptrt(val.x) | (uptrt(val.y) << 32));
}

Texture* RendererGl::texFromColor(u8vec4 color) {
	return new TextureGl(ivec2(2), createTexture<GL_NEAREST, GL_NEAREST, 0>(array<u8vec4, 4>{ color, color, color, color }.data(), 2, 2, 0, GL_RGBA));
}

Texture* RendererGl::texFromText(SDL_Surface* img) {
	if (img) {
		GLuint id = createTexture<GL_NEAREST, GL_NEAREST, 0>(img->pixels, img->w, img->h, img->pitch / img->format->BytesPerPixel, textPixFormat);
		SDL_FreeSurface(img);
		return new TextureGl(ivec2(img->w, img->h), id);
	}
	return nullptr;
}

Texture* RendererGl::texFromIcon(SDL_Surface* pic) {
	if (auto [img, fmt] = pickPixFormat(pic); pic) {
		GLuint id = createTexture<GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 1000>(img->pixels, img->w, img->h, img->pitch / img->format->BytesPerPixel, fmt);
		glGenerateMipmap(GL_TEXTURE_2D);
		SDL_FreeSurface(img);
		return new TextureGl(ivec2(img->w, img->h), id);
	}
	return nullptr;
}

template <GLint min, GLint mag, GLint mip>
GLuint RendererGl::createTexture(const void* pixels, int width, int height, int rowLen, GLenum format) {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if constexpr (mip < 1000)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLen);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	return id;
}

pair<SDL_Surface*, GLenum> RendererGl::pickPixFormat(SDL_Surface* img) {
	if (img) {
		switch (img->format->format) {
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGRA32: case SDL_PIXELFORMAT_XRGB8888:
			return pair(img, GL_BGRA);
#endif
		case SDL_PIXELFORMAT_RGBA32: case SDL_PIXELFORMAT_XBGR8888:
			return pair(img, GL_RGBA);
#ifndef OPENGLES
		case SDL_PIXELFORMAT_BGR24:
			return pair(img, GL_BGR);
		case SDL_PIXELFORMAT_RGB24:
			return pair(img, GL_RGB);
#endif
		}
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(img);
		img = dst;
	}
	return pair(img, GL_RGBA);
}

#ifndef OPENGLES
#ifndef NDEBUG
void APIENTRY RendererGl::debugMessage(GLenum source, GLenum type, uint id, GLenum severity, GLsizei, const char* message, const void*) {
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;
	string msg = "Debug message " + toStr(id) + ": " + message;
	if (msg.back() != '\n' && msg.back() != '\r')
		msg += linend;

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		msg += "Source: API, ";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		msg += "Source: window system, ";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		msg += "Source: shader compiler, ";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		msg += "Source: third party, ";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		msg += "Source: application, ";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		msg += "Source: other, ";
		break;
	default:
		msg += "Source: unknown, ";
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		msg += "Type: error, ";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		msg += "Type: deprecated behavior, ";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		msg += "Type: undefined behavior, ";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		msg += "Type: portability, ";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		msg += "Type: performance, ";
		break;
	case GL_DEBUG_TYPE_MARKER:
		msg += "Type: marker, ";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		msg += "Type: push group, ";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		msg += "Type: pop group, ";
		break;
	case GL_DEBUG_TYPE_OTHER:
		msg += "Type: other, ";
		break;
	default:
		msg += "Type: unknown, ";
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		logError(std::move(msg), "Severity: high");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		logError(std::move(msg), "Severity: medium");
		break;
	case GL_DEBUG_SEVERITY_LOW:
		logError(std::move(msg), "Severity: low");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		logInfo(std::move(msg), "Severity: notification");
		break;
	default:
		logError(std::move(msg), "Severity: unknown");
	}
}
#endif
#endif
