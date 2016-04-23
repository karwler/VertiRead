A so far simple cross-platform manga reader and music player.  
Still in progress.  

TO DO
- add double click
- add keyhold text control
- finish playlist editor
- add settings menu lists
- improve mouse wheel scrolling
- test audio system
- maybe add some animations
- load textures while displaying reader

LINUX  
For simple build just use the Makefile.  
The Qt project file is configured for Linux only.  
All dependencies need to be installed manually:  
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev  
The default font is "Arial" so you need the mscorefonts as well:  
sudo apt-get install ttf-mscorefonts-installer  

WINDOWS  
Project files were created with MS Visual Studio 2015.  
All libraries are already included and linked in the project.  

