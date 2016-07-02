#VertiRead
A simple manga reader and music player for Linux, Windows and OS X.  
So far it's usable for reading, basic playlist editing and should be theoretically able to play music.  
It still has some bugs and unfinished/untested parts.  

Used libraries are boost_system, boost_filesystem, SDL2, SDL2_image, SDL2_ttf and SDL2_mixer.  
The minimum required cmake version is 3.1.0 and your compiler needs to support at least c++11.  

The Qt project and CMakeLists files are configured for Linux, Windows and OS X.  
Note: When setting the output directory in Qt, don’t name it ‘build’, cause it might mess up the executable’s location.  

If you want to change the default font, go to "src/utils/types.h", look for the "VideoSettings" class and change the default value for "FNT" in the constructor.  
It doesn't matter whether you enter a fontname, filename or filepath as long as the search algorithm can find it.  

##Linux
All dependencies need to be installed manually.  
Installing the packages "libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev" should do the trick. (Assuming that all necessary dependencies for those are installed automatically in the process.)  

The default font is set to "Arial" so you need the mscorefonts as well:  
Installing the package "ttf-mscorefonts-installer" should take care of that.  

There’s a pre-made launcher, which is copied to the build directory after compilation.  
If you want a menu entry for the program, just set the executable’s and icon’s path to the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

Settings files are being saved in "~/.vertiread".  
Note: It's possible that the mouse wheel won't work properly under Linux and I've got no idea why.  

##Windows
All necessary libraries are already included in the project, however they're built only for the MSVC 14 (2015).  
Settings files are being saved in "%AppData%\VertiRead".  

##OS X
The project is configured to use the SDL2 frameworks and boost library.  
Both need to be installed manually.  
The frameworks’ location is set to /Library/Frameworks.  
Boost is assumed to be located in /usr/local/include (headers) and /usr/local/lib (objects).  

Settings files are being saved in "~/Library/Application Support/VertiRead".  

##To do list
- add browser to playlist editor
- fix mosue scroll under linux
- maybe add some animations
- maybe load textures while displaying reader
