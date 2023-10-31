#pragma once

#if CAN_FONTCFG
#include <fontconfig/fontconfig.h>

namespace LibFontconfig {

extern decltype(FcInitLoadConfigAndFonts)* fcInitLoadConfigAndFonts;
extern decltype(FcPatternCreate)* fcPatternCreate;
extern decltype(FcPatternDestroy)* fcPatternDestroy;
extern decltype(FcPatternGetString)* fcPatternGetString;
extern decltype(FcDefaultSubstitute)* fcDefaultSubstitute;
extern decltype(FcNameParse)* fcNameParse;
extern decltype(FcFontSetDestroy)* fcFontSetDestroy;
extern decltype(FcObjectSetDestroy)* fcObjectSetDestroy;
extern decltype(FcObjectSetBuild)* fcObjectSetBuild;
extern decltype(FcConfigDestroy)* fcConfigDestroy;
extern decltype(FcConfigSubstitute)* fcConfigSubstitute;
extern decltype(FcFontMatch)* fcFontMatch;
extern decltype(FcFontList)* fcFontList;

bool symFontconfig();
void closeFontconfig();

}
#endif
