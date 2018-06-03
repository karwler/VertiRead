# VertiRead
A simple comic reader for Linux and Windows.  

Used libraries are SDL2, SDL2_image, SDL2_ttf and all that come with those three.   
The minimum required Cmake version is 3.1.3 and your compiler needs to support at least C++11.  

Note: When setting the output directory in Qt, don’t name it ‘build’, cause it might mess up the executable’s location.  

## Linux
All dependencies need to be installed manually.  
Installing the packages "libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev" should do the trick. (Assuming that all necessary dependencies for those are installed automatically in the process.)  
The default font is set to "Arial", so you probably need to install some kind of "ttf-ms-fonts" package.  
If you don't want to install any new fonts, you can just change the default font in the "src/prog/defaults.h" file.  
Settings files are being saved in "~/.vertiread".  

There’s a pre-made launcher, which is copied to the build directory after compilation.  
If you want a menu entry for the program, just set the executable’s and icon’s path in the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  

## Windows
All necessary libraries are already included in the project. However they're built for MSVC. If you're using a different compiler, you'll need to replace the existing library files or link the new ones.  
Settings files are being saved in "%AppData%\VertiRead".  

## How to use it
The idea is that you have a library directory in which you have your comics saved in form of pictures. The location of this directory can be changed in the settings.  
The last button in the book list allows you to navigate through files outside of the library directory.  
Left clicking on a book in the book list wil take you to the file explorer, while right clicking on a book will take you to the last viewed page.  
The reader has a hidden side panel on the left.  

The program supports keyboard and controller bindings. DirectInput and XInput are handled separately. The bindings can be changed in the settings.  
To reset certain groups of settings, edit or delete the corresponding ini files in the settings directory.  
