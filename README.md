Doom64EX-Plus
========

Differences from Kaiser's C++ version on Github.

* Support for the Nightdive IWAD
* Support for the Lost Levels
* Support for loading PWADS
* Better performance (especially when compared to the official Nightdive version, which is slow as hell)
* Secret notifications
* MAP slots up to MAP40
* The MEDKIT You Really Need fix
* Many bugfixes
* Kex - This is pretty much removed.  The only remnants are some comments with [kex] and the rendering stuff that is used in later versions.  I wanted to keep Doom64EX-Plus very close in format to other source ports, for familiarity purposes and ease-of-porting code from Erick's Doom64-RE project.

There are a few bugs still present, which I am slowly fixing.  This GitHub repo is the same as: https://sourceforge.net/projects/doom64ex-plus/

But if any contributors wish to help, then GitHub is a better place for it.

# MOD Support

For modders, wanting to make their mods for EX+ there are a few things that deviate from ancient EX.

1. DM_START and DM_END like the remaster, instead of DS_START and DS_END
2. Graphics tags aren't implemented (like the remaster).  Instead:
	a. The FIRST tag for graphics MUST BE 'SYMBOLS' (without the quotes)
	b. The END tag for graphics MUST BE 'MOUNTC' (without the quotes)
They can be either a tag or a graphic.  This is due to me not flat-out reading all PNG's in a WAD
but instead, to support the official IWAD, I read the content like this to catch all the graphics that
EX+ needs, from the IWAD.

No other changes are needed.

# Info

Doom64EX-Plus is a reverse-engineering project aimed to recreate Doom64 as close as possible with additional modding features.

**NOTE for Linux users:** As of Feb. 24, 2016, the save data is located in `$XDG_DATA_HOME/doom64ex-plus` (typically `~/.local/share/doom64ex-plus`) and not in `~/.doom64ex-plus`. The files can be safely moved to their new home.

## Dependencies

* SDL2
* SDL2_net
* zlib
* libpng
* FluidSynth

## Compiling

### Linux, MSYS and cross compilation.

Clone this repo

    $ git clone https://github.com/atsb/Doom64EX-Plus
    
For Linux, use the build.sh script.
For cross compilation for Windows, use the build_win_cross.sh

For Windows, use the provided Visual Studio project file for 32bit builds.

## Usage

### Linux

Doom64EX-Plus needs the Doom 64 data to be present in any of the following directories:

* The directory in which `doom64ex-plus` resides
* `$XDG_DATA_HOME/doom64ex-plus` (eg. `~/.local/share/doom64ex-plus`)
* `/usr/local/share/games/doom64ex-plus`
* `/usr/local/share/doom64ex-plus`
* `/usr/share/games/doom64ex-plus`
* `/usr/share/doom64ex-plus`

The data files are:

* `doom64ex-plus.wad`
* `doom64.wad`
* `doomsnd.sf2`

After this you can play.

    $ doom64ex-plus
    
