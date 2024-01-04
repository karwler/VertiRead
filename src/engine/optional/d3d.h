#pragma once

#ifdef WITH_DIRECT3D
#define WIN32_LEAN_AND_MEAN
#define D3D11_NO_HELPERS
#include <d3d11.h>

namespace LibD3d11 {

extern decltype(CreateDXGIFactory)* createDXGIFactory;
extern decltype(D3D11CreateDevice)* d3d11CreateDevice;

void load();
void free();

}
#endif
