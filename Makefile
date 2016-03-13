TARGET = vertiread
TDIR = build
CC = clang++
CFLAGS = -std=c++11 -stdlib=libc++
INCPATH = -Isrc
LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lboost_system -lboost_filesystem 
SOURCES = $(wildcard src/*/*.cpp)

$(TARGET):
	mkdir -p $(TDIR)
	$(CC) $(CFLAGS) $(LIBS) $(INCPATH) $(SOURCES) -o $(TDIR)/$(TARGET)
	cp -r data $(TDIR)

clean:
	rm -rf build

