#ifdef WITH_DIRECT3D
#include "d3d.h"
#include <stdexcept>
#include <tchar.h>

#define DLIB_NAME "d3d11"
#define ILIB_NAME "dxgi"
#define NAME_CREATE_DEVICE "D3D11CreateDevice"
#define NAME_CREATE_FACTORY "CreateDXGIFactory"

namespace LibD3d11 {

static HMODULE libi = nullptr;
static HMODULE libd = nullptr;
static bool failed = false;
decltype(CreateDXGIFactory)* createDXGIFactory = nullptr;
decltype(D3D11CreateDevice)* d3d11CreateDevice = nullptr;

void load() {
	if (libi || failed)
		return;
	try {
		if (libd = LoadLibrary(_T(DLIB_NAME ".dll")); !libd)
			throw std::runtime_error("Failed to load " DLIB_NAME);
		if (libi = LoadLibrary(_T(ILIB_NAME ".dll")); !libi)
			throw std::runtime_error("Failed to load " ILIB_NAME);
	} catch (const std::runtime_error&) {
		if (libi)
			FreeLibrary(libi);
		if (libd)
			FreeLibrary(libd);
		libi = libd = nullptr;
		failed = true;
		throw;
	}

	try {
		if (d3d11CreateDevice = reinterpret_cast<decltype(d3d11CreateDevice)>(GetProcAddress(libd, NAME_CREATE_DEVICE)); !d3d11CreateDevice)
			throw std::runtime_error("Failed to find " NAME_CREATE_DEVICE);
		if (createDXGIFactory = reinterpret_cast<decltype(createDXGIFactory)>(GetProcAddress(libi, NAME_CREATE_FACTORY)); !createDXGIFactory)
			throw std::runtime_error("Failed to find " NAME_CREATE_FACTORY);
	} catch (const std::runtime_error&) {
		FreeLibrary(libi);
		FreeLibrary(libd);
		libi = libd = nullptr;
		failed = true;
		throw;
	}
}

void free() {
	if (libi) {
		FreeLibrary(libi);
		FreeLibrary(libd);
		libi = libd = nullptr;
	}
	failed = false;
}

}
#endif
