#ifdef WITH_DIRECT3D
#include "d3d.h"
#include "internal.h"

static LibType libi = nullptr;
static LibType libd = nullptr;
static bool failed = false;
decltype(CreateDXGIFactory)* createDXGIFactory1 = nullptr;
decltype(D3D11CreateDevice)* d3d11CreateDevice = nullptr;

bool symD3d11() noexcept {
	if (!(libi || failed || ((libd = libOpen("d3d11" LIB_EXT)) && (libi = libOpen("dxgi" LIB_EXT))
		&& (d3d11CreateDevice = libSym<decltype(d3d11CreateDevice)>(libd, "D3D11CreateDevice"))
		&& (createDXGIFactory1 = libSym<decltype(createDXGIFactory1)>(libi, "CreateDXGIFactory1"))
	))) {
		libClose(libi);
		libClose(libd);
		failed = true;
	}
	return libi;
}

void closeD3d11() noexcept {
	libClose(libi);
	libClose(libd);
	failed = false;
}
#endif
