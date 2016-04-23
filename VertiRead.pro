TEMPLATE = app
TARGET = vertiread
CONFIG += c++11
CONFIG -= app_bundle qt

copydata.commands = $(COPY_DIR) $$PWD/data $$OUT_PWD
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

INCLUDEPATH += src

LIBS += -lSDL2 \
        -lSDL2_image \
        -lSDL2_ttf \
        -lSDL2_mixer \
        -lboost_system \
        -lboost_filesystem

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
    src/utils/utils.h \
    src/prog/browser.h \
    src/prog/playlistEditor.h \
    src/utils/scrollAreas.h \
    src/utils/popups.h \
    src/prog/library.h \
    src/utils/capturers.h
