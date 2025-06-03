# Doom 64 EX+

Doom 64 EX+ is a continuation project of Samuel "Kaiser" Villarreal's Doom 64 EX aimed to recreate DOOM 64 as closely as possible with additional modding features.

Note for Linux users: Get it on Flathub!

https://flathub.org/apps/io.github.atsb.Doom64EX-Plus

## Differences from Kaiser's C++ version of EX on GitHub:

* Support for the IWAD from Nightdive Studios' official remaster.  Just so it is clear for everyone (NO IT DOES NOT SUPPORT THE OLD EX ROM DUMP IWAD).
* Support for the Lost Levels campaign
* Support for loading PWADs
* FMOD Studio audio (no more fluidsynth!!!)
* Better performance (especially when compared to Nightdive Studios' official version)
* Messages for discovering secret areas
* Support of MAP slots up to MAP40
* The "medkit you REALLY need!" message fix
* Many bugfixes
* KEX - This is pretty much removed.  The only remnants are some comments with [kex] and the rendering stuff that is used in later versions. I wanted to keep Doom 64 EX+ very close in format to other source ports for familiarity purposes and for ease of porting code from Erick's DOOM64-RE project.
* SDL3 - I believe DOOM64 EX+ is the first Doom source port to officially move to SDL3 as standard.

* NOTE for NVIDIA users, disable vsync from the control panel to fix stuttering.  No idea why it only happens on NVidia cards, though I can't say I'm surprised.
* NOTE - for the launcher, you MUST have .NET 6 or higher installed as the launcher was programmed using .NET 6.0 and anything below won't work.

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

* SDL3
* ZLib
* LibPNG
* FMOD Studio
* OpenGL

## Installation

Windows

- Windows does not yet support installing the software, however you are able to manually put the software in any directory
	of your choosing and it will work fine.

GNU/Linux / BSD

When compiling `doom64ex-plus`, you can use compile-time macros to control where the game engine looks for its data files (like IWADs and supporting resources).

### `-DDOOM_UNIX_INSTALL`

- When enabled, the game will store and search for its data in `~/.local/share/doom64ex-plus`.
  - `~` represents the user’s home directory.
  - `~/.local/share` is a standard location for user-specific data on Linux and BSD systems.
- For single-user installations without needing special permissions to write to system directories.

### `-DDOOM_UNIX_SYSTEM_DATADIR="/some/system/path"`

- Sets a system-wide directory where the game should look for its data files.
- When set, the game will search the specified directory (e.g., `/usr/share/games/doom64ex-plus`) for mandatory game files like `doom64ex-plus.wad` and `doomsnd.sf2`.
- Suitable for system administrators or packagers who want the game installed for all users on the system.

## How to Use These Macros

### 1. Command Line Compilation

To compile the software manually using `gcc` or `make`, specify the macros directly:

- For user-specific installation:
  ```bash
  gcc -DDOOM_UNIX_INSTALL -o doom64ex-plus <c files> <library links>
  ```

- To set a specific system-wide directory:
  ```bash
  gcc -DDOOM_UNIX_SYSTEM_DATADIR=\"/usr/share/games/doom64ex-plus\" -o doom64ex-plus <c files> <library links>
  ```

### 2. Using a Makefile

If the software uses a `Makefile`, you can add these macros as options:

- For user-specific installation:
  ```makefile
  CFLAGS += DOOM_UNIX_INSTALL
  ```

- For specifying a system-wide directory:
  ```makefile
  CFLAGS += DOOM_UNIX_SYSTEM_DATADIR=/usr/share/games/doom64ex-plus
  ```

Finally, if a data file cannot be found in one of the two folders above, it will look inside the current directory.

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

You can place the asset data described above into any of the following directories,
searched in that order:

* `~/.local/share/doom64ex-plus` if compiled with the *DOOM_UNIX_INSTALL* macro
* the folder specified by the *DOOM_UNIX_SYSTEM_DATADIR* compile macro, or `/usr/local/share/doom64ex-plus` if not specified

If data files are not found in these directories, it will search in the current directory.

## macOS

You can place the asset data described above to:

* `/Users/*user*/Library/Application Support/doom64ex-plus`

Then, you can start playing:

 $ DOOM64EX+

**NOTE for Linux and FreeBSD/OpenBSD users:** As of Nov. 5, 2022, the save data is located in the same directory as the Linux executable and not in 
`~/.local/share/doom64ex-plus`. The files can be securely moved into their new place.  Note: This assumes you have not compiled the software with the *-DDOOM_UNIX_INSTALL*

### Windows

The asset data files need to be located in the same directory as `DOOM64EX+.exe`.
Then, you can start playing by launching `DOOM64EX+.exe` (or optionally by using `DOOM64EX+ Launcher.exe` instead).
