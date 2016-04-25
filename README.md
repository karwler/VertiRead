#VertiRead
A simple manga reader and music player for Linux and Windows.  
So far it's usable for reading, basic playlist editing and should be theoretically able to play music.  
It still has some bugs and unfinished parts.  

There's no settings menu yet, so in order to change the settings, you need to edit the .ini files directly.  
Used libraries are boost and SDL.  

The Qt project file is configured for Linux as well as Windows.  
(I might add OS X support if I feel like it.)  

##Linux
For simple build just use the makefile.  
It's currently configured for the g++ compiler.  
If you want to use clang++ or set a specific architecture, just look in the top section of the makefile.

All dependencies need to be installed manually:  
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev  

The default font is set to "Arial" so you need the mscorefonts as well:  
sudo apt-get install ttf-mscorefonts-installer 

##Windows
Project files were created with MS Visual Studio 2015.  
All needed libraries are already included in the project, however they're only build for the MSVC 2015.  

##To do list
- add double click
- add browser to playlist editor
- add settings menu lists
- improve mouse wheel scrolling
- test audio system
- maybe add some animations
- load textures while displaying reader
