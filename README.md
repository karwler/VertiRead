#VertiRead
A simple manga reader and music player for Linux, Windows and OS X.  
So far it's usable for reading, basic playlist editing and should be theoretically able to play music.  
It still has some bugs and unfinished/untested parts.  

There's no settings menu yet, so in order to change the settings, you need to edit the .ini files directly.  
Used libraries are boost_filesystem, SDL2, SDL2_image, SDL2_ttf and SDL2_mixer.  

The Qt project and CMakeLists files are configured for Linux, Windows and OS X.  
Note: When setting the output directory in Qt, don’t name it ‘build’, cause it might mess up the executable’s location.   

##Linux
There’s also a pre-made launcher, which is copied to the build directory after compilation.  
You just need to add the executable’s and icon’s path to it.  

All dependencies need to be installed manually:  
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev  

The default font is set to "Arial" so you need the mscorefonts as well:  
sudo apt-get install ttf-mscorefonts-installer 

##Windows
All needed libraries are already included in the project, however they're built only for the MSVC 14 (2015).  

##OS X
The project is configured to use the SDL2 frameworks and boost library.  
Both need to be installed manually.  
The frameworks’ location is set to /Library/Frameworks.  
Boost is assumed to be located in /usr/local/include (headers) and /usr/local/lib (objects).  

##To do list
- add browser to playlist editor
- fix in-list line edit
- add text scroll to line edit
- finish settings menu lists
- improve mouse wheel scrolling
- test audio system under linux and os x
- maybe add some animations
- figure out a way to make zooming faster
- load textures while displaying reader
