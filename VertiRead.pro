# config
TEMPLATE = app
CONFIG += c++11
CONFIG -= app_bundle qt

win32:CONFIG(debug, debug|release) {
    CONFIG += console
} else {
    CONFIG -= console
}

# output
win32: TARGET = VertiRead
else: TARGET = vertiread

OBJECTS_DIR = $$OUT_PWD/bin
macx: DESTDIR = $$OUT_PWD/VertiRead.app/Contents/MacOS
else: DESTDIR = $$OUT_PWD/build

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

    postbuild.commands = $$quote(cmd /c copy $$LIB_WIN\\*.dll $${DEST_WIN}) && \
                         $$quote(cmd /c xcopy /e/i/y $$PWD_WIN\\data\\* $$DEST_WIN)
}
macx {
    postbuild.commands = mkdir -p $$DESTDIR/../Resources && \
                         cp $$PWD/rsc/Info.plist $$DESTDIR/.. && \
                         cp $$PWD/rsc/icon.icns $$DESTDIR/../Resources && \
                         cp -r $$PWD/data/* $$DESTDIR/../Resources
}
linux {
    postbuild.commands = cp $$PWD/rsc/vertiread.desktop $$DESTDIR && \
                         cp -r $$PWD/data/* $$DESTDIR
}
QMAKE_EXTRA_TARGETS += postbuild
POST_TARGETDEPS += postbuild

# includepaths
INCLUDEPATH += $$PWD/src
win32: INCLUDEPATH += $$PWD/include

# dependencies' directories
win32 {
    LIBS += -L$$LIB_WIN
}
macx {
    FRMWK = -F/Library/Frameworks

    QMAKE_CFLAGS += $$FRMWK
    QMAKE_CXXFLAGS += $$FRMWK

    LIBS += $$FRMWK
}

# linker flags
macx {
    LIBS += -framework SDL2 \
            -framework SDL2_image \
            -framework SDL2_ttf \
            -framework SDL2_mixer
}
else {
    LIBS += -lSDL2 \
            -lSDL2_image \
            -lSDL2_ttf \
            -lSDL2_mixer
}

# set sources
SOURCES += src/engine/audioSys.cpp \
    src/engine/engine.cpp \
    src/engine/filer.cpp \
    src/engine/inputSys.cpp \
    src/engine/main.cpp \
    src/engine/scene.cpp \
    src/engine/windowSys.cpp \
    src/engine/world.cpp \
    src/prog/program.cpp \
    src/utils/items.cpp \
    src/utils/objects.cpp \
    src/utils/types.cpp \
    src/utils/utils.cpp \
    src/prog/browser.cpp \
    src/prog/playlistEditor.cpp \
    src/utils/scrollAreas.cpp \
    src/utils/popups.cpp \
    src/prog/library.cpp \
    src/utils/capturers.cpp

HEADERS += src/engine/audioSys.h \
    src/engine/engine.h \
    src/engine/filer.h \
    src/engine/inputSys.h \
    src/engine/scene.h \
    src/engine/windowSys.h \
    src/engine/world.h \
    src/prog/program.h \
    src/utils/items.h \
    src/utils/objects.h \
    src/utils/types.h \
	src/prog/defaults.h \
    src/utils/utils.h \
    src/prog/browser.h \
    src/prog/playlistEditor.h \
    src/utils/scrollAreas.h \
    src/utils/popups.h \
    src/prog/library.h \
    src/utils/capturers.h \
	src/kklib/aliases.h \
	src/kklib/sptr.h \
	src/kklib/vec2.h \
	src/kklib/vec3.h \
	src/kklib/vec4.h \

win32: RC_FILE = rsc/resource.rc
