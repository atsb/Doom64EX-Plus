### Next version

#### New

* rendering overhaul with shaders (???)
* better gamepad support (???)
* better support for loading mods targeting the DOOM 64 Nightdive Remaster
* support for loading data from the Remaster `Doom64.kpf` file. Currently loads title and legal images, cursor graphic, localizations. Up to 7 alternate kpf arguments can be specified with the new `-kpf <arg>...` command-line option, where `<arg>` can be: a filename (will be searched in the data dirs), a file path (either absolute or releative) to a .kpf file, or a directory path containing kpf data with the same folder structure than a file .kpf. The last choice allows to start a mod that would otherwise normally require patching `Doom64.kpf`, without having to patch it. For example, to start [DOOM 64 Reloaded Remastered](https://www.moddb.com/games/doom-64/downloads/doom64-reloaded-01-09-2023) installed in `<modfolder>`, use command (or create a Desktop shortcut on Windows) `DOOM64Ex-Plus -file <modfolder>/DOOM64RR.wad -kpf <modfolder>/kpf/Doom64`. When using the `-kpf` option, it is not necessary to add the stock `Doom64.kpf` as it is always implicitely added last
* **Windows user data files:** The game was always storing its user files (config, screenshots, saves) in the installation folder where `DOOM64Ex-Plus.exe` is found, which was not always guaranteed to be writable and not best practice. Now these files are written in the user data directory, usually `C:\Users\<username>\AppData\Roaming\doom64ex-plus`. When updating from a previous version, all existing data files will be moved to that new directory
* add 3-point texture filtering (`N64` choice for `Filter` setting) 
* add `Sky filter` setting to change sky texture filtering (mapped to `r_skyFilter` variable) 
* add `Weapon Filter` setting to change weapon texture filtering (mapped to `r_weaponFilter` variable)
* add `map <n>` console command to enter map `<n>`
* add `listmaps` console command to list all maps with their number and title
* add `i_overbright` console variable to enable overbright rendering
* add support for binding `MOUSE2` (middle click), `MOUSE4` and `MOUSE5` buttons
* add new default `MOUSE4` binding for automap free panning
* add ability to pan the automap using the `Use` action binding, same as on the Nintendo 64 and Remaster
* add keypad directional keys as default keys for automap panning 
* add binding options for fist, shotgun and super shotgun
* add option to disable auto weapon pickup switching 
* add support for graphics with palette animation (example: blue/red/yellow key door frames), matching the Remaster
* display a message when a screenshot is taken and its full path in the console
* user data directory and config file path are displayed in the console


#### Fixes

* fix using the mouse on the bindings screen
* fix some sounds possibly not playing
* fix `F12` key binding not working
* fix automap not rendering correctly when panning
* fix very high automap mouse pan sensitivity and increased dezoom
* fix crash on game exit if game started with `-nosound` and/or `-nomusic`
* fix screenshots PNG files always saved in the current directory (now always saved in the user data directory)
* fix nightmare difficulty (`-skill 5`) or using `-respawn` not respawning monsters properly
* fix possible rare crash during gameplay (notably in the starting room with lost souls in `MAP19`)
* fix a vanilla bug for the switches that are in the floor it activates even if it is in the floor when the player shoots them
* fix view pitch not reset when mouse look is disabled
* fix brightness slider
* fix ceilings interpolation frame that was the same as the floor
* fix secret percentage possibly higher than 100% on intermission screen

#### Changes

* improved support for discovering Steam (Windows, Linux) and GOG (Windows) DOOM 64 Remaster data file folder. Notably, Steam game install in non-standard folder is properly found
* better scale default for crosshair on HiRes monitors
* always create a fullscreen window (borderless on Windows). On Windows, while on a map, Alt-Tab only works if the game is paused (`ESC`)
* removed `Windowed`, `Aspect Ratio`, `Resolution` and `Apply` menu entries from the `Video` menu, not needed anymore
* removed `Mouse look` and `Mouse invert` menu entries from mouse settings menu
as new sky rendering does not support it. Mouse look can still be enabled via the `v_mlook 1` console command (with recommended `p_autoaim 0` to disable autoaim)
* removed `Network` entry from main menu
* removed `Skybox` and `Jump` menu entries from the `Setup` menu
* removed obsolete `idnull` cheat
* removed osbolete `-alpha` command-line option
* removed `-recorddemo` and `-playdemo` command-line option as demos do not work
* `exclev<n>` and `idclev<n>` cheats can warp to any valid map `<n>`, including map 33 (title map) that does not crash anymore
* removed the ability to show the automap while the menu is displayed (was glitchy)
* do not display crosshair while in menus and automap
* force disable autoaim when mouse look is disabled
* secret messages stay (???)
* compat_mobjpass change from 0 to 1 (More vanilla additions) (???)
* reverse engineered the original collision code back into the engine (???)

#### Build

* Update to SDL 3.2.20 on Windows, 3.2.22 on Linux AppImage
* (minor) fixed AppImage binaries not stripped
* Windows build is compiled against older libpng 1.5.14
* code can be compiled with compiler using any C standard from C99 to C23
