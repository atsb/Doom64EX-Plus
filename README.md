# Doom 64 EX+

Doom 64 EX+ is a continuation project of Samuel "Kaiser" Villarreal's Doom 64 EX aimed to recreate DOOM 64 as closely as possible with additional modding features.

## Differences from Kaiser's C++ version of EX on GitHub:

* Support for the IWAD from Nightdive Studios' official remaster
* Support for the Lost Levels campaign
* Support for loading PWADs
* Better performance (especially when compared to Nightdive Studios' official version which is slow as hell)
* Messages for discovering secret areas
* Support of MAP slots up to MAP40
* The "medkit you REALLY need!" message fix
* Many bugfixes
* KEX - This is pretty much removed.  The only remnants are some comments with [kex] and the rendering stuff that is used in later versions. I wanted to keep Doom 64 EX+ very close in format to other source ports for familiarity purposes and for ease of porting code from Erick's DOOM64-RE project.

There are a few bugs still present, which I am slowly fixing.

This GitHub repo is the same as: https://sourceforge.net/projects/doom64ex-plus/ but if any contributors wish to help, then GitHub is a better place for it.

## Mod Support

For modders wanting to make their mods work with EX+, there are a few things that deviate from ancient EX:

1. DM_START and DM_END tags are used (like the remaster) instead of DS_START and DS_END.
2. G_START and G_END graphics tags aren't implemented (like the remaster).

Instead:

* The **first** tag for graphics **MUST BE** called SYMBOLS.
* The **last** tag for graphics **MUST BE** called MOUNTC.

They can either be a tag or a graphic.  This is due to me not flat-out reading all PNGs in a WAD but instead, to support the official IWAD, I read the content like this to catch all the graphics that EX+ needs from the IWAD.

No other changes are needed.

## Dehacked Support

For modders interested in dehacked for DOOM64EX+, please refer to the 'modding' directory for reference.

## [Discord server](https://discord.gg/Ktxz8nz)

## Dependencies

* SDL2
* SDL2_mixer(WIP)
* zlib
* libpng

## System Requirements - 32 or 64bit

Linux Single Board Computer

- Raspberry Pi 3B

*others may work but are untested*

Linux Desktop / Laptop

- 1.8GHz Dual Core CPU
- 2GB RAM
- 80MB Disk Space
- OpenGL 1.4+ Compliant Video Chip / Card

Windows

- 2.0GHz Dual Core CPU
- 4GB RAM
- 80MB Disk Space
- OpenGL 1.4+ Compliant Video Chip / Card

## Installation

Windows

- Windows does not yet support installing the software, however you are able to manually put the software in any directory
	of your choosing and it will work fine.

GNU/Linux

- GNU/Linux supports system installations using the compile-time macro *-DDOOM_UNIX_INSTALL*
	this will force the software to look for all IWAD and supporting files inside ~/.local/share/doom64ex-plus

Without the macro, it will look inside the current directory that the binary is in.

Packaging will not be done by myself, but any contributor is welcome to package the software for GNU/Linux.

## Compiling

Clone this repo

    $ git clone https://github.com/atsb/Doom64EX-Plus

### Linux or Cross Platform Compilation

Use the `build.sh` script for a native build or the `build_win_cross.sh` script for cross compilation for Windows.

### Windows

Use the Visual Studio solution and project files provided in the `Windows` directory of the repository for both 32-bit or 64-bit builds.

## Usage

Doom 64 EX+ needs the DOOM 64 asset data files to be present for you to be able to play the game. The data files are:

* `DOOM64.wad`
* `doom64ex-plus.wad`
* `doomsnd.sf2`

### Linux 

You can place the asset data described above to any of the following directories:

* The directory in which `doom64ex-plus` resides
* `/usr/local/share/doom64ex-plus` or `/usr/share/games/doom64ex-plus`

Then, you can start playing:

    $ doom64ex-plus

**NOTE for Linux users:** As of Nov. 5, 2022, the save data is located in the same directory as the Linux executable and not in `~/.local/share/doom64ex-plus`. The files can be securely moved into their new place.

### Windows

The asset data files need to be located in the same directory as `Doom64EX-Plus.exe`.

Then, you can start playing by launching `Doom64EX-Plus.exe` (or optionally by using `Doom64EX-Plus Launcher.exe` instead).
