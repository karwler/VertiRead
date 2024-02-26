#pragma once

#include "utils/utils.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

using LibType = HMODULE; 

#define LIB_EXT ".dll"
#else
#include <dlfcn.h>

using LibType = void*; 

#define LIB_EXT ".so"
#endif

template <bool global = false>
LibType libOpen(const char* name) {
	LibType lib;
#ifdef _WIN32
	if (lib = LoadLibraryA(name); !lib)
		logError("Failed to open ", name, ": ", winErrorMessage(GetLastError()));
#else
	if constexpr (global)
		lib = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
	else
		lib = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
	if (!lib)
		logError("Failed to open ", name, ": ", coalesce(const_cast<const char*>(dlerror()), ""));
#endif
	return lib;
}

inline void libClose(LibType& lib) {
	if (lib) {
#ifdef _WIN32
		FreeLibrary(lib);
#else
		dlclose(lib);
#endif
		lib = nullptr;
	}
}

template <Pointer T>
T libSym(LibType lib, const char* name) {
	T func;
#ifdef _WIN32
	if (func = reinterpret_cast<T>(GetProcAddress(lib, name)); !func)
		logError("Failed to find ", name, ": ", winErrorMessage(GetLastError()));
#else
	if (func = reinterpret_cast<T>(dlsym(lib, name)); !func)
		logError("Failed to find ", name, ": ", coalesce(const_cast<const char*>(dlerror()), ""));
#endif
	return func;
}
