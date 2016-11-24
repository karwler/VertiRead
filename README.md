#VertiRead
A simple manga reader and music player for Linux, Windows and OS X.  
So far it's usable for reading, playing music and basic playlist editing.  
It still has some bugs and unfinished/untested parts.  

Used libraries are SDL2, SDL2_image, SDL2_ttf and SDL2_mixer.  
The minimum required cmake version is 3.1.0 and your compiler needs to support at least c++11.  

Note: When setting the output directory in Qt, don’t name it ‘build’, cause it might mess up the executable’s location.  

##Linux
All dependencies need to be installed manually.  
Installing the packages "libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev" should do the trick. (Assuming that all necessary dependencies for those are installed automatically in the process.)  

There’s a pre-made launcher, which is copied to the build directory after compilation.  
If you want a menu entry for the program, just set the executable’s and icon’s path to the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

Settings files are being saved in "~/.vertiread".  
Note: It's possible that the mouse wheel won't work properly under Linux and I've got no idea why.  

##Windows
All necessary libraries are already included in the project, however they're built only for the MSVC 14 (2015).  
Settings files are being saved in "%AppData%\VertiRead".  

##OS X
The project is configured to use the SDL2 frameworks, which need to be installed manually.  
The frameworks’ location is set to /Library/Frameworks.  

Settings files are being saved in "~/Library/Application Support/VertiRead".  

##Changing the default settings
Most defaults and constants are defined in the "src/prog/defaults.h" file.  
Just look around and see what you can find.
