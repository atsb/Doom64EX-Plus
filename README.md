## DOOM64EX+

[![Doom64EX-Plus! Icon](https://github.com/atsb/Doom64EX-Plus/blob/develop-wolfy/engine/DOOM64EX+.png)](https://github.com/atsb/Doom64EX-Plus)

DOOM64EX+ is a continuation project of Samuel "Kaiser" Villarreal's DOOM64EX aimed to recreate DOOM 64 as closely as possible with additional modding features.

## Differences from Kaiser's C++ version of EX on GitHub:

* Support for the IWAD from Nightdive Studios' official remaster.  Just so it is clear for everyone (NO IT DOES NOT SUPPORT THE OLD EX ROM DUMP IWAD).
* Support for the Lost Levels campaign
* Support for the Alpha Version of the game
* Support for loading PWADs
* Port to the C Programming Language like the original game
* Support for C89(for linux users: only -std=gnu89 flag will compile) and C99
* Support for higher resolutions including 21:9
* Replaced xinput to SDL2_GameController API Which supports all the modern controlls/gamepads/joysticks we have.
* More platforms support including Raspberry PI 3, FreeBSD, OpenBSD, Nintendo Switch(WIP), Playstation Vita(WIP) and Microsoft Xbox(At the moment it´s suspended due to the fakegl linking needed study more).
* MSVC Compatible you can run on Visual c++ 6.0(Not recommended but usefull depending of the situation)
* Better performance (especially when compared to Nightdive Studios' official version (which is slow as hell)
* Messages for discovering secret areas
* Support of MAP slots up to MAP40
* The "medkit you REALLY need!" message fix
* Many bugfixes and cleanups
* KEX - This is pretty much removed.  The only remnants are some comments with [kex] and the rendering stuff that is used in later versions. I wanted to keep Doom 64 EX+ very close in format to other source ports for familiarity purposes and for ease of porting code from Erick's DOOM64-RE project.

There are a few bugs still present, which I am slowly fixing.

This [GitHub repo](https://github.com/atsb/Doom64EX-Plus) is the same as [sourceforge](https://sourceforge.net/projects/doom64ex-plus/) but if any contributors wish to help, then github is a better place for it.

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
You can find them on [moddb](https://www.moddb.com/games/doom-64/downloads/)

## Dehacked Support

For modders interested in dehacked for DOOM64EX+, please refer to the 'modding' directory for reference.

## [DOOM64 Discord server](https://discord.gg/Ktxz8nz)

## Dependencies

* SDL2
* SDL2_OPENGL(Optional)
* ZLib
* LibPNG
* FluidSynth
* OpenGL
* GLAD
* GLFW(Optional)

## System Requirements - 32 or 64bit

Linux Single Board Computer

- Raspberry Pi 3B

*others may work but are untested*

Linux / *BSD Desktop / Laptop

- 1.8GHz Dual Core CPU
- 2GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Or Later Compliant Video Chip / Card

Windows

- 1.8GHz Dual Core CPU
- 4GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Or Later Compliant Video Chip / Card

macOS

- mid-2013 macbook air or better (also arm64 systems)
- 4GB RAM
- 80MB Disk Space
- OpenGL 2.1+ Or Later Compliant Video Chip / Card

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

Install it from [MacSourcePorts](https://macsourceports.com/game/doom64)

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

## Vita

Install the VITASDK and compile with `make all -f Makefile.vita` to build the vpk files.

## Nintendo Switch

Install the devkitpro and compile with `make all -f Makefile.ns` to build the nso and nro files.

## Microsoft Xbox

Use the vcproj provided on the xbox folder to compile with vs2003 and you will need to have the latest Xbox XDK Installed + a Windows XP Machine
to compile it.

## Usage

Doom 64 EX+ needs the DOOM 64 asset data files to be present for you to be able to play the game. The data files are:

* `DOOM64.wad`
* `doom64ex-plus.wad`
* `doomsnd.sf2`

## Linux, FreeBSD/OpenBSD and Raspberry pi3

You can place the asset data described above to any of the following directories:

* The directory in which `DOOM64EX+` resides
  `/usr/local/share/DOOM64EX+`

## macOS

You can place the asset data described above to:

* `/Users/*user*/Library/Application Support/DOOM64EX+`

Then, you can start playing:

* `$ DOOM64EX+`

**NOTE for Linux and FreeBSD/OpenBSD users:** As of Nov. 5, 2022, the save data is located in the same directory as the Linux executable and not in
`~usr/.local/share/DOOM64EX+`. The files can be securely moved into their new place.  Note: This assumes you have not compiled the software with the `-DDOOM_UNIX_INSTALL`
**NOTE for Linux and FreeBSD/OpenBSD users** As January 2 of 2023 The DOOM64EX+ by standard will be compiled with `-DDOOM_UNIX_INSTALL` due to SDL Path filesystem

## Windows

The asset data files need to be located in the same directory as `DOOM64EX+.exe`, you can execute the game directly through
of the executable or you can start playing by launching `DOOM64EX+ Launcher.exe`.

## Playstation Vita

The data files will be atm on: `ux0:/data/DOOM64EX+`

## Microsoft Xbox(Suspended)
At the moment the assets will be on the d:\\.
