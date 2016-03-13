A so far simple cross-platform manga reader and music player.
Still in progress.

TO DO
- make reader zoom
- make reader interface
- make a playlist editor
- add selectable items
- improve vector classes
- add settings menu lists
- improve exception handling
- give it a proper look
- maybe add some animations
- optimize it somehow
- fix input lag

LINUX
The Qt project file is configured for Linux only.
There's also a half-assed makefile which should do the trick.
All dependencies need to be installed manually:
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libboost-filesystem1.55-dev

WINDOWS
Project files were created with MS Visual Studio 2015.
All libraries are already included and linked in the project.

