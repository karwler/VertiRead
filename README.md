# VertiRead
A simple comic/manga reader for Linux and Windows.  
It's basically just an image viewer that shows all pictures of a directory/archive.  
Currently supported file formats are whatever SDL2_image and libarchive support.  

## Build
Used libraries are SDL2, SDL2_image, SDL2_ttf, libarchive, glm and by extension FreeType, HarfBuzz, libtiff, libwebp, stb_image and zlib. The included default font is BrisaSans.  
The CMakeLists.txt is written for at least CMake 3.12.4 with Clang, GCC or MSVC which need to support C++17.  
You can generate project files for a debug build by running CMake with the "-DCMAKE_BUILD_TYPE=Debug" option. Otherwise it'll default to a release build.  
By default the Program uses OpenGL 3.0, which can be switched to OpenGL ES 3.0 with "-DOPENGLES=1" or entirely disabled with "-DOPENGL=0".  
Support for DirectX 11 and Vulkan 1.0 can be enabled by setting the options "-DDIRECTX=1" and "-DVULKAN=1".  

### Linux
Most dependencies need to be installed manually. Installing the development packages for libsdl2 libsdl2-image libsdl2-ttf and libarchive should do the trick.  
To build an AppImage run CMake with the "-DAPPIMAGE=1" option.  
Settings files are being saved in "~/.local/share/vertiread".  

There’s a launcher file, which is copied to the build directory after compilation.  
If you want a menu entry for the program, set the executable’s and icon’s path in the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

```bash
mkdir build
cd build
cmake .. -DVULKAN=1
make
```

### Windows
Only MS Visual Studio and MinGW are supported. All necessary libraries are downloaded while running CMake.  
Settings files are being saved in "%AppData%\VertiRead".  

```batch
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DDIRECTX=1
nmake
```

## How to use it
The idea is that you have a library directory in which you have your comics saved in form of pictures or archives of pictures. The location of this directory can be changed in the settings.  
Left clicking on a book in the book list will take you to the file explorer, while right clicking on a book will show you further options, like going to the last viewed page.  
The last button in the book list allows you to navigate through files outside of the library directory. While in the book list or browser view, you can drag and drop a folder or file into the window to browse/open it. It's also possible to start the program with a file/directory path as a command line argument to start in the page browser or reader.  
The reader has a hidden side panel on the left.  
You can set an image count or size limit in the settings for how many pictures can be loaded at once. If set, the directory switching buttons/keys will instead load the next or previous batch of pictures, or go to the next directory/archive if there are no more pictures to load in the current one.  
The direction in which pictures in the reader are stacked can be set in the settings.  

The program supports keyboard and controller bindings. DirectInput and XInput are handled separately. The bindings can be changed in the settings.  
To reset certain settings, edit or delete the corresponding ini files in the settings directory or use the reset button in the settings menu to reset all settings.  
Among the program's resource files is a "themes.ini" file which can be used to edit the available color schemes. If there's a not empty "themes.ini" in the settings directory, it'll override the default themes file.  

## Supported files
- images: bmp, gif, jpg, lbm, pcx, png, pnm, svg, tga, tiff, webp, xcf, xpm, xv
- archives: 7z, ar, cab, cpio, ISO9660, lha, lzh, mtree, pax, rar, tar, ustar, xar, zip, zipx
