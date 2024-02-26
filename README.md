# VertiRead
A simple comic/manga reader for Linux and Windows.  
It's basically just an image viewer that shows all pictures of a directory/archive.  
Currently supported file formats are whatever SDL2_image and libarchive support.  

## Build
The CMakeLists.txt is written for at least CMake 3.16.9 with Clang, GCC, MinGW or MSVC which need to support C++ 20.  
By default the Program uses OpenGL with the lowest possible version which can be disabled with "-DOPENGL=OFF".  
Support for Direct3D 11 and Vulkan 1.0 can be disabled by setting the options "-DDIRECT3D=OFF" and "-DVULKAN=OFF".  

### Linux
All dependencies except for GLM need to be installed manually. Installing the development packages for SDL 2, SDL_image 2, FreeType 2 and libarchive should be enough.  
To build an AppImage run CMake with the "-DAPPIMAGE=ON" option.  
Settings files are being saved in "~/.local/share/vertiread".  

There's a launcher file which is copied to the build directory after compilation.  
If you want a menu entry for the program, set the executable's and icon's path in the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

Example:  
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Windows
When building with MSVC, all necessary libraries are downloaded and compiled while running CMake. For this to work, MSVC must be in the console's PATH.  
Otherwise the needed MinGW libraries except for GLM have to be installed.  
Settings files are being saved in "%AppData%\VertiRead".  

Example:  
```batch
mkdir build
cd build
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
nmake
```

### Directly used Libraries
|Library|Optional|Description|
|-|-|-|
|d3d11 & dxgi|runtime|Rendering with Direct3D. (loaded dynamically at runtime)|
|fontconfig|runtime|Speeds up font file lookup. (loaded dynamically at runtime)|
|freetype2|no|Needed for rendering text.|
|GL/GLES|runtime|Rendering with OpenGL/OpenGL ES. (linked dynamically at runtime)|
|glm|no|Needed mostly for widget positioning.|
|ICU|compile time|Improves sorting of file lists. (linked if present at compile time)|
|libarchive|no|Needed for reading archive files.|
|libsecret|runtime|For storing credentials to remote locations. (loaded dynamically at runtime)|
|libssh2|runtime|For browsing SFTP locations. (loaded dynamically at runtime)|
|Poppler|runtime|For displaying PDF files. (loaded dynamically at runtime)|
|SDL2|no|Needed for everything.|
|SDL2_image|no|Needed to load images.|
|smbclient|runtime|For browsing SMB shares. (loaded dynamically at runtime)|
|vulkan|runtime|Rendering with Vulkan. (loaded dynamically at runtime)|

### Recompiling shaders
The shader code for Direct3D, OpenGL and Vulkan is stored in the header files in "src/engine/shaders".  
These files can be recreated from the files in "rsc/shaders" with these python scripts as long as fxc or glslc are installed.  

```bash
python rsc/dx_shaders.py
python rsc/gl_shaders.py
python rsc/vk_shaders.py
```

## How to use it
The idea is to have a main directory in which all books are stored in the form of pictures or archives of pictures. The location of this directory can be changed in the settings.  
Left clicking on a book in the book list will take you to the file explorer, while right clicking on a book will show you further options, like going to the last viewed page.  
The last button in the book list allows you to navigate through files outside of the main directory. While in the book list or explorer, you can drag and drop a folder or file into the window to browse/open it. It's also possible to start the program with a file/directory path as a command line argument to start in the explorer or reader.  
The reader has a hidden side panel on the left with buttons for navigation, zoom, etc..  
You can set an image count or size limit in the settings for how many pictures can be loaded at once. If set, the directory switching buttons/keys will instead load the next or previous batch of pictures or go to the next directory/archive if there are no more pictures to load in the current one.  
The direction in which pictures in the reader are stacked can be set in the settings.  

The program supports keyboard and controller bindings. DirectInput and XInput are handled separately. The bindings can be changed in the settings.  
To reset certain settings, edit or delete the corresponding ini files in the settings directory or use the reset button in the settings menu to reset all settings.  
Among the program's resource files is a "themes.ini" file which can be used to edit the available color schemes. If there's a not empty "themes.ini" in the settings directory, it'll override the default themes file.  

### Command line arguments
|Argument|Description|
|-|-|
|&lt;path&gt;|Path to a file or directory to open.|
|-c|Don't bypass the X11 compositor.|
|-d11|Force to launch with Direct3D 11.|
|-e3|Force to launch with OpenGL ES 3.0.|
|-g1|Force to launch with OpenGL 1.1.|
|-g3|Force to launch with OpenGL 3.0.|
|-l|Don't write to a log file.|
|-sf|Force to launch with a software renderer.|
|-vk|Force to launch with Vulkan 1.0.|

## Supported files
- images: bmp, gif, jpg, lbm, pcx, png, pnm, svg, tga, tiff, webp, xcf, xpm, xv
- archives: 7z, ar, cab, cpio, ISO9660, lha, lzh, mtree, pax, rar, tar, ustar, xar, zip, zipx
