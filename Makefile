#CXX     = clang++
#CFLAGS  = -pipe -std=c++11 -Wall -W -fPIC
#LFLAGS  = -ccc-gcc-name g++ -Wl

# add the -m32 or -m64 option to the CFLAGS and LFLAGS in order to compile for either 32bit or 64bit systems
CXX     = g++
CFLAGS  = -pipe -std=c++11 -Wall -W
INCPATH = -Isrc
LFLAGS  = 
LIBS    = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lboost_system -lboost_filesystem
TARGET  = vertiread
TDIR	= build

SOURCES = src/engine/audioSys.cpp \
		src/engine/engine.cpp \
		src/engine/filer.cpp \
		src/engine/inputSys.cpp \
		src/engine/main.cpp \
		src/engine/scene.cpp \
		src/engine/windowSys.cpp \
		src/engine/world.cpp \
		src/prog/program.cpp \
		src/prog/library.cpp \
		src/utils/items.cpp \
		src/utils/objects.cpp \
		src/utils/popups.cpp \
		src/utils/scrollAreas.cpp \
		src/utils/types.cpp \
		src/utils/utils.cpp \
		src/utils/capturers.cpp \
		src/prog/browser.cpp \
		src/prog/playlistEditor.cpp

OBJECTS = bin/audioSys.o \
		bin/engine.o \
		bin/filer.o \
		bin/inputSys.o \
		bin/main.o \
		bin/scene.o \
		bin/windowSys.o \
		bin/world.o \
		bin/program.o \
		bin/library.o \
		bin/items.o \
		bin/objects.o \
		bin/popups.o \
		bin/scrollAreas.o \
		bin/types.o \
		bin/utils.o \
		bin/capturers.o \
		bin/browser.o \
		bin/playlistEditor.o

$(TARGET): bindir $(OBJECTS)
	mkdir -p $(TDIR)
	$(CXX) $(LFLAGS) -o $(TDIR)/$(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)
	cp -r data $(TDIR)
	cp rsc/vertiread.desktop $(TDIR)
	@echo "executable '"$(TARGET)"' has been written to '"$(TDIR)"'"

bin/audioSys.o: src/engine/audioSys.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/audioSys.o src/engine/audioSys.cpp

bin/engine.o: src/engine/engine.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/engine.o src/engine/engine.cpp

bin/filer.o: src/engine/filer.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/filer.o src/engine/filer.cpp

bin/inputSys.o: src/engine/inputSys.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/inputSys.o src/engine/inputSys.cpp

bin/main.o: src/engine/main.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/main.o src/engine/main.cpp

bin/scene.o: src/engine/scene.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/scene.o src/engine/scene.cpp

bin/windowSys.o: src/engine/windowSys.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/windowSys.o src/engine/windowSys.cpp

bin/world.o: src/engine/world.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/world.o src/engine/world.cpp

bin/program.o: src/prog/program.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/program.o src/prog/program.cpp
	
bin/library.o: src/prog/library.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/library.o src/prog/library.cpp

bin/items.o: src/utils/items.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/items.o src/utils/items.cpp

bin/objects.o: src/utils/objects.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/objects.o src/utils/objects.cpp
	
bin/popups.o: src/utils/popups.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/popups.o src/utils/popups.cpp

bin/scrollAreas.o: src/utils/scrollAreas.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/scrollAreas.o src/utils/scrollAreas.cpp

bin/types.o: src/utils/types.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/types.o src/utils/types.cpp

bin/utils.o: src/utils/utils.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/utils.o src/utils/utils.cpp

bin/capturers.o: src/utils/capturers.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/capturers.o src/utils/capturers.cpp

bin/browser.o: src/prog/browser.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/browser.o src/prog/browser.cpp

bin/playlistEditor.o: src/prog/playlistEditor.cpp
	$(CXX) -c $(CFLAGS) $(INCPATH) -o bin/playlistEditor.o src/prog/playlistEditor.cpp

bindir:
	mkdir -p bin

clean:
	rm -rf bin

distclean: clean
	rm -rf build

