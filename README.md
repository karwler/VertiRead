A so far simple cross-platform manga reader and music player.
Still in progress.

TO DO
- fix rect cropping in lists
- text crop in buttons
- test getting exec dir under linux
- scene resize instead of recreating all objects
- make the actual reader
- make a playlist editor
- improve vector classes
- add settings menu lists
- add exception handling
- give it a proper look
- maybe replace boost with something faster
- maybe add some animations
- write comments
- optimize it somehow

LINUX
Qt project file is configured for Linux only.
All dependencies need to be installed manually. The project file contains all needed linker flags.

WINDOWS
Project files were created with MS Visual Studio 2015.
All SDL libraries are already included and linked in the project.
The boost_filesystem library isn't included so it needs to be linked manually. (no linker flags needed)
