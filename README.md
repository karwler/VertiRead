# VertiRead
A simple comic reader for Linux and Windows.  
It's basically just an image viewer that shows all pictures in a directory/archive.  
Currently supported file formats are whatever SDL2_image and libarchive support.  

Used libraries are SDL2, SDL2_image, SDL2_ttf, libarchive and by extension libjpeg, libpng, libtiff, libwebp, FreeType and zlib.   
The CMakeLists.txt is written for at least CMake 3.10.2 with Clang, GCC or MSVC which need to support C++17.  
You can create a Makefile for a debug build by running CMake with the "-DCMAKE_BUILD_TYPE=Debug" option. Otherwise it'll default to a release build.  

## Linux
All dependencies need to be installed manually.  
Installing the development packages for libsdl2 libsdl2-image libsdl2-ttf and libarchive should do the trick, assuming that all necessary dependencies are installed automatically.  
The default font is set to "Arial", so you probably need to install some kind of ttf-ms-fonts package.  
If you don't want to install any new fonts, you can change the default font in the "src/utils/settings.h" file under "Settings::defaultFont".  
Settings files are being saved in "~/.vertiread".  

There’s a pre-made launcher, which is copied to the build directory after compilation.  
If you want a menu entry for the program, just set the executable’s and icon’s path in the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

## Windows
All necessary libraries are already included in the project. However they're built for MSVC. If you're using a different compiler, you'll need to replace the existing library files or link the new ones.  
Settings files are being saved in "%AppData%\VertiRead".  

## How to use it
The idea is that you have a library directory in which you have your comics saved in form of pictures or archives of pictures. The location of this directory can be changed in the settings.  
Left clicking on a book in the book list wil take you to the file explorer, while right clicking on a book will take you to the last viewed page.  
The last button in the book list allows you to navigate through files outside of the library directory. When in the book list or browser view, you can drag and drop a folder or file into the window to browse/open it. It's also possible to start the program with a file/directory path as a command line argument to start in the page browser or reader.  
The reader has a hidden side panel on the left.  

The direction in which pictures in the reader are stacked can be set in the settings menu.  
The program supports keyboard and controller bindings. DirectInput and XInput are handled separately. The bindings can be changed in the settings.  
To reset certain settings, edit or delete the corresponding ini files in the settings directory or use the reset button in the settings menu to reset all settings.  

## Supported files
- images: bmp, gif, jpg, lbm, pcx, png, pnm, svg, tga, tiff, webp, xcf, xpm, xv
- archives: 7z, ar, cab, cpio, empty, ISO9660, lha, lzh, mtree, pax, rar, raw, tar, xar, zip