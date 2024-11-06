#pragma once

#ifdef NDEBUG
#define ATTR_FIN_VPOS "a"
#define ATTR_FIN_VTUV "b"
#define ATTR_GUI_VPOS "d"
#define UNI_FIN_GAMMA "e"
#define UNI_FIN_SCENEMAP "d"
#define UNI_GUI_COLORID "h"
#define UNI_GUI_COLORMAP "f"
#define UNI_GUI_COLORS "g"
#define UNI_GUI_FRAME "c"
#define UNI_GUI_PVIEW "a"
#define UNI_GUI_RECT "b"
#else
#define ATTR_FIN_VPOS "vpos"
#define ATTR_FIN_VTUV "vtuv"
#define ATTR_GUI_VPOS "vpos"
#define UNI_FIN_GAMMA "gamma"
#define UNI_FIN_SCENEMAP "sceneMap"
#define UNI_GUI_COLORID "colorId"
#define UNI_GUI_COLORMAP "colorMap"
#define UNI_GUI_COLORS "colors"
#define UNI_GUI_FRAME "frame"
#define UNI_GUI_PVIEW "pview"
#define UNI_GUI_RECT "rect"
#endif
