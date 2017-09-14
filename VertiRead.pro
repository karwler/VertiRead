# config
TEMPLATE = app
CONFIG += c++11

win32 {
    CONFIG(debug, debug|release) {
        CONFIG += console
    }
}

# output
win32: TARGET = VertiRead
else: TARGET = vertiread

OBJECTS_DIR = $$OUT_PWD/bin
DESTDIR = $$OUT_PWD/build

# copy data dir and dependencies
win32 {
    PWD_WIN = $$PWD
    PWD_WIN ~= s,/,\\,g
    DEST_WIN = $$DESTDIR
    DEST_WIN ~= s,/,\\,g

    contains(QT_ARCH, i386) {
        LIB_WIN = $$PWD/lib/win32
    } else {
        LIB_WIN = $$PWD/lib/win64
    }
    LIB_WIN ~= s,/,\\,g

    postbuild.commands = $$quote(cmd /c copy $$LIB_WIN\\*.dll $$DEST_WIN) && \
                         $$quote(cmd /c copy $$PWD_WIN\\rsc\\icon.ico $$DEST_WIN) && \
                         $$quote(cmd /c xcopy /e/i/y $$PWD_WIN\\data\\* $$DEST_WIN)
}
linux {
    postbuild.commands = cp $$PWD/rsc/vertiread.desktop $$DESTDIR && \
                         cp $$PWD/rsc/icon.ico $$DESTDIR && \
                         cp -r $$PWD/data/* $$DESTDIR
}

QMAKE_EXTRA_TARGETS += postbuild
POST_TARGETDEPS += postbuild

# includepaths
INCLUDEPATH += $$PWD/src
win32: INCLUDEPATH += $$PWD/include

# dependencies' directories
win32: LIBS += -L$$LIB_WIN

# linker flags
LIBS += -lSDL2 \
        -lSDL2_image \
        -lSDL2_ttf \
        -lSDL2_mixer

# definitions
win32: DEFINES += _UNICODE \
                  _CRT_SECURE_NO_WARNINGS

# set sources
SOURCES += src/engine/audioSys.cpp \
    src/engine/base.cpp \
    src/engine/drawSys.cpp \
    src/engine/filer.cpp \
    src/engine/inputSys.cpp \
    src/engine/library.cpp \
    src/engine/main.cpp \
    src/engine/scene.cpp \
    src/engine/windowSys.cpp \
    src/engine/world.cpp \
    src/prog/browser.cpp \
    src/prog/playlistEditor.cpp \
    src/prog/program.cpp \
    src/utils/capturers.cpp \
    src/utils/items.cpp \
    src/utils/scrollAreas.cpp \
    src/utils/settings.cpp \
    src/utils/types.cpp \
    src/utils/utils.cpp \
    src/utils/widgets.cpp

HEADERS += src/engine/audioSys.h \
    src/engine/base.h \
    src/engine/drawSys.h \
    src/engine/filer.h \
    src/engine/inputSys.h \
    src/engine/library.h \
    src/engine/scene.h \
    src/engine/windowSys.h \
    src/engine/world.h \
    src/kklib/aliases.h \
    src/kklib/grid2.h \
    src/kklib/sptr.h \
    src/kklib/vec2.h \
    src/kklib/vec3.h \
    src/kklib/vec4.h \
    src/prog/browser.h \
    src/prog/defaults.h \
    src/prog/playlistEditor.h \
    src/prog/program.h \
    src/utils/capturers.h \
    src/utils/items.h \
    src/utils/scrollAreas.h \
    src/utils/settings.h \
    src/utils/types.h \
    src/utils/utils.h \
    src/utils/widgets.h

win32: RC_FILE = rsc/resource.rc
