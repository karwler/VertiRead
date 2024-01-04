#pragma once

#ifdef NDEBUG
#define ATTR_GUI_VPOS "d"
#define ATTR_SEL_VPOS "d"
#define UNI_GUI_COLOR "g"
#define UNI_GUI_COLORMAP "f"
#define UNI_GUI_FRAME "c"
#define UNI_GUI_PVIEW "a"
#define UNI_GUI_RECT "b"
#define UNI_SEL_ADDR "e"
#define UNI_SEL_FRAME "c"
#define UNI_SEL_PVIEW "a"
#define UNI_SEL_RECT "b"
#else
#define ATTR_GUI_VPOS "vpos"
#define ATTR_SEL_VPOS "vpos"
#define UNI_GUI_COLOR "color"
#define UNI_GUI_COLORMAP "colorMap"
#define UNI_GUI_FRAME "frame"
#define UNI_GUI_PVIEW "pview"
#define UNI_GUI_RECT "rect"
#define UNI_SEL_ADDR "addr"
#define UNI_SEL_FRAME "frame"
#define UNI_SEL_PVIEW "pview"
#define UNI_SEL_RECT "rect"
#endif
