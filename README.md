#VertiRead  
A simple manga reader and music player for Linux and Windows.  
So far it's usable for reading and basic playlist editing, though it still has some unfinished parts.  
There's no settings menu yet, so in order to change the settings, you need to edit the .ini files directly.  
Used libraries are boost and SDL.  
  
##Linux  
For simple build just use the Makefile.  
The Qt project file is configured for Linux only. 
   
All dependencies need to be installed manually:  
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev  
sudo apt-get install -f  
  
The default font is set to "Arial" so you need the mscorefonts as well:  
sudo apt-get install ttf-mscorefonts-installer  
  
##Windows  
Project files were created with MS Visual Studio 2015.  
All libraries are already included and linked in the project.  
  
##To do list
- add double click
- finish playlist editor
- add settings menu lists
- improve mouse wheel scrolling
- test audio system
- maybe add some animations
- load textures while displaying reader