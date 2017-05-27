#VertiRead  
A simple manga reader and music player for Linux and Windows.  
So far it's usable for reading, playing music and basic playlist editing.  
It still has some bugs and untested parts.  
  
Used libraries are SDL2, SDL2_image, SDL2_ttf and SDL2_mixer.  
The minimum required Cmake version is 3.1.0 and your compiler needs to support at least C++11.  
  
Note: When setting the output directory in Qt, don’t name it ‘build’, cause it might mess up the executable’s location.  
  
##Linux  
All dependencies need to be installed manually.  
Installing the packages "libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev" should do the trick. (Assuming that all necessary dependencies for those are installed automatically in the process.)  
  
There’s a pre-made launcher, which is copied to the build directory after compilation.  
If you want a menu entry for the program, just set the executable’s and icon’s path in the .desktop file and move it to either "/usr/share/applications" or "~/.local/share/applications".  
  
Settings files are being saved in "~/.vertiread".  
Note: It's possible that the mouse wheel won't work properly under Linux and I have no idea why.  
  
##Windows  
All necessary libraries are already included in the project, however they're built only for the MSVC 14 (2015).  
Settings files are being saved in "%AppData%\VertiRead".  
  
##How to use it  
The idea is that you have a library directory in which you have your manga. The location of this directory can be viewed and/or changed in the general settings menu.  
It is advised (but not necessary) to sort the files and directories by things like chapters, volumes, etc.  
Opening an image file opens the reader, which displays all images in the directory of the chosen file.  
The reader has a hidden side pane on the left. The top two buttons are for switching between directories. (This doesn’t work recursively.)  
  
You can create playlists and assign them to specific manga, using the editor. In the playlist editor you can add either single files or entire directories of songs to a playlist. The file browser can be a bit of a pain, so you can just use drag and drop instead.  
All playlists are saved and looked up in the playlist directory, which can be viewed and/or changed in the general settings menu.  
The reader has a hidden player at the bottom.  
  
The program supports keyboard and controller bindings. DirectInput and XInput are handled separately. The key bindings can be viewed and/or edited in the controls settings menu.  
  
To reset certain groups of settings, edit or delete the corresponding ini files in the settings directory.  
Note: If the dead zone is set too low, lists might scroll on their own or the mouse cursor might move on its own.    
