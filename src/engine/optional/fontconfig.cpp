#ifdef CAN_FONTCFG
#include "fontconfig.h"
#include "internal.h"

static LibType lib = nullptr;
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
	if (!(lib || failed || ((lib = libOpen("libfontconfig" LIB_EXT))
		&& (fcInitLoadConfigAndFonts = libSym<decltype(fcInitLoadConfigAndFonts)>(lib, "FcInitLoadConfigAndFonts"))
		&& (fcPatternCreate = libSym<decltype(fcPatternCreate)>(lib, "FcPatternCreate"))
		&& (fcPatternDestroy = libSym<decltype(fcPatternDestroy)>(lib, "FcPatternDestroy"))
		&& (fcPatternGetString = libSym<decltype(fcPatternGetString)>(lib, "FcPatternGetString"))
		&& (fcDefaultSubstitute = libSym<decltype(fcDefaultSubstitute)>(lib, "FcDefaultSubstitute"))
		&& (fcNameParse = libSym<decltype(fcNameParse)>(lib, "FcNameParse"))
		&& (fcFontSetDestroy = libSym<decltype(fcFontSetDestroy)>(lib, "FcFontSetDestroy"))
		&& (fcObjectSetDestroy = libSym<decltype(fcObjectSetDestroy)>(lib, "FcObjectSetDestroy"))
		&& (fcObjectSetBuild = libSym<decltype(fcObjectSetBuild)>(lib, "FcObjectSetBuild"))
		&& (fcConfigDestroy = libSym<decltype(fcConfigDestroy)>(lib, "FcConfigDestroy"))
		&& (fcConfigSubstitute = libSym<decltype(fcConfigSubstitute)>(lib, "FcConfigSubstitute"))
		&& (fcFontMatch = libSym<decltype(fcFontMatch)>(lib, "FcFontMatch"))
		&& (fcFontList = libSym<decltype(fcFontList)>(lib, "FcFontList"))
	))) {
		libClose(lib);
		failed = true;
	}
	return lib;
}

void closeFontconfig() {
	libClose(lib);
	failed = false;
}
#endif
