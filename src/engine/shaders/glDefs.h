#pragma once

#ifdef NDEBUG
#define ATTR_GUI_VPOS "d"
#define UNI_GUI_COLOR "g"
#define UNI_GUI_COLORMAP "f"
#define UNI_GUI_FRAME "c"
#define UNI_GUI_PVIEW "a"
#define UNI_GUI_RECT "b"
#else
#define ATTR_GUI_VPOS "vpos"
#define UNI_GUI_COLOR "color"
#define UNI_GUI_COLORMAP "colorMap"
#define UNI_GUI_FRAME "frame"
#define UNI_GUI_PVIEW "pview"
#define UNI_GUI_RECT "rect"
#endif
