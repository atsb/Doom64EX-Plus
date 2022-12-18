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

## Where are the PWADs?

Since most EX PWADS would be incompatible with EX+ for many reasons, I have been adapting them one-by-one for use on EX+
You can find them on moddb, just look for the EX+ / EX Plus wads:
https://www.moddb.com/games/doom-64/downloads/

## Dehacked Support

For modders interested in dehacked for DOOM64EX+, please refer to the 'modding' directory for reference.

## [Discord server](https://discord.gg/Ktxz8nz)

## Dependencies

* SDL2 and SDL2_net
* ZLib
* LibPNG
* FluidSynth
* OpenGL
* GLFW

## System Requirements - 32 or 64bit

Linux Single Board Computer

- Raspberry Pi 3B

*others may work but are untested*

Linux / *BSD Desktop / Laptop

- 1.8GHz Dual Core CPU
- 2GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Compliant Video Chip / Card

Windows

- 1.8GHz Dual Core CPU
- 4GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Compliant Video Chip / Card

macOS

- mid-2013 macbook air or better (also arm64 systems)
- 4GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Compliant Video Chip / Card

## Installation

Windows

- Windows does not yet support installing the software, however you are able to manually put the software in any directory
	of your choosing and it will work fine.

GNU/Linux / BSD

- GNU/Linux and BSD supports system installations using the compile-time macro *-DDOOM_UNIX_INSTALL*
	this will force the software to look for all IWAD and supporting files inside ~/.local/share/doom64ex-plus

Without the macro, it will look inside the current directory that the binary is in.

Packaging will not be done by myself, but any contributor is welcome to package the software for GNU/Linux or macOS.

## Compiling

Clone this repo

    $ git clone https://github.com/atsb/Doom64EX-Plus

## Linux

Use the `build.sh` script for a native build.

## macOS
Install it from MacSourcePorts:
https://macsourceports.com/game/doom64

Or if you feel adventurous, read below:
Install MacPorts and get the dependencies.

Then use the XCode project file, which is the only supported way to compile on macOS.  Everything is already defined.
The IWAD needs to be placed in: /Users/*user*/Library/Application Support/doom64ex-plus directory along with the files found within the Resources directory inside the bundle (.wad and .sf2).

## Raspberry Pi 3

Use `build_rpi3_raspbian.sh` for a native build of Raspberry Pi 3B

## FreeBSD

Use the `build_freebsd.sh` script for a native build of FreeBSD

## OpenBSD

Use the `build_openbsd.sh` script for a native build of OpenBSD

## Windows

Use the Visual Studio solution and project files provided in the `Windows` directory of the repository for both 32-bit or 64-bit builds.

## Usage

Doom 64 EX+ needs the DOOM 64 asset data files to be present for you to be able to play the game. The data files are:

* `DOOM64.wad`
* `doom64ex-plus.wad`
* `doomsnd.sf2`

## Linux, FreeBSD/OpenBSD and Raspberry pi3

You can place the asset data described above to any of the following directories:

* The directory in which `doom64ex-plus` resides
* `/usr/local/share/doom64ex-plus` or `/usr/share/games/doom64ex-plus`

## macOS

You can place the asset data described above to:

* `/Users/*user*/Library/Application Support/doom64ex-plus`

Then, you can start playing:

    $ doom64ex-plus

**NOTE for Linux and FreeBSD/OpenBSD users:** As of Nov. 5, 2022, the save data is located in the same directory as the Linux executable and not in 
`~/.local/share/doom64ex-plus`. The files can be securely moved into their new place.  Note: This assumes you have not compiled the software with the *-DDOOM_UNIX_INSTALL*

### Windows

The asset data files need to be located in the same directory as `Doom64EX-Plus.exe`.

Then, you can start playing by launching `Doom64EX-Plus.exe` (or optionally by using `Doom64EX-Plus Launcher.exe` instead).
