#ifdef CAN_FONTCFG
#include "fontconfig.h"
#include <iostream>
#include <dlfcn.h>

#define LIB_NAME "fontconfig"

namespace LibFontconfig {

static void* lib = nullptr;
static bool failed = false;
decltype(FcInitLoadConfigAndFonts)* fcInitLoadConfigAndFonts = nullptr;
decltype(FcPatternCreate)* fcPatternCreate = nullptr;
decltype(FcPatternDestroy)* fcPatternDestroy = nullptr;
decltype(FcPatternGetString)* fcPatternGetString = nullptr;
decltype(FcDefaultSubstitute)* fcDefaultSubstitute = nullptr;
decltype(FcNameParse)* fcNameParse = nullptr;
decltype(FcFontSetDestroy)* fcFontSetDestroy = nullptr;
decltype(FcObjectSetDestroy)* fcObjectSetDestroy = nullptr;
decltype(FcObjectSetBuild)* fcObjectSetBuild = nullptr;
decltype(FcConfigDestroy)* fcConfigDestroy = nullptr;
decltype(FcConfigSubstitute)* fcConfigSubstitute = nullptr;
decltype(FcFontMatch)* fcFontMatch = nullptr;
decltype(FcFontList)* fcFontList = nullptr;

bool symFontconfig() {
	if (lib || failed)
		return lib;
	if (lib = dlopen("lib" LIB_NAME ".so", RTLD_LAZY | RTLD_LOCAL); !lib) {
		const char* err = dlerror();
		std::cerr << (err ? err : "Failed to open " LIB_NAME) << std::endl;
		failed = true;
		return false;
	}

	fcInitLoadConfigAndFonts = reinterpret_cast<decltype(fcInitLoadConfigAndFonts)>(dlsym(lib, "FcInitLoadConfigAndFonts"));
	fcPatternCreate = reinterpret_cast<decltype(fcPatternCreate)>(dlsym(lib, "FcPatternCreate"));
	fcPatternDestroy = reinterpret_cast<decltype(fcPatternDestroy)>(dlsym(lib, "FcPatternDestroy"));
	fcPatternGetString = reinterpret_cast<decltype(fcPatternGetString)>(dlsym(lib, "FcPatternGetString"));
	fcDefaultSubstitute = reinterpret_cast<decltype(fcDefaultSubstitute)>(dlsym(lib, "FcDefaultSubstitute"));
	fcNameParse = reinterpret_cast<decltype(fcNameParse)>(dlsym(lib, "FcNameParse"));
	fcFontSetDestroy = reinterpret_cast<decltype(fcFontSetDestroy)>(dlsym(lib, "FcFontSetDestroy"));
	fcObjectSetDestroy = reinterpret_cast<decltype(fcObjectSetDestroy)>(dlsym(lib, "FcObjectSetDestroy"));
	fcObjectSetBuild = reinterpret_cast<decltype(fcObjectSetBuild)>(dlsym(lib, "FcObjectSetBuild"));
	fcConfigDestroy = reinterpret_cast<decltype(fcConfigDestroy)>(dlsym(lib, "FcConfigDestroy"));
	fcConfigSubstitute = reinterpret_cast<decltype(fcConfigSubstitute)>(dlsym(lib, "FcConfigSubstitute"));
	fcFontMatch = reinterpret_cast<decltype(fcFontMatch)>(dlsym(lib, "FcFontMatch"));
	fcFontList = reinterpret_cast<decltype(fcFontList)>(dlsym(lib, "FcFontList"));
	if (!(fcInitLoadConfigAndFonts && fcPatternCreate && fcPatternDestroy && fcPatternGetString && fcDefaultSubstitute && fcNameParse && fcFontSetDestroy && fcObjectSetDestroy && fcObjectSetBuild && fcConfigDestroy && fcConfigSubstitute && fcFontMatch && fcFontList)) {
		std::cerr << "Failed to find " LIB_NAME " functions" << std::endl;
		dlclose(lib);
		lib = nullptr;
		failed = true;
		return false;
	}
	return true;
}

void closeFontconfig() {
	if (lib) {
		dlclose(lib);
		lib = nullptr;
	}
	failed = false;
}

}
#endif
