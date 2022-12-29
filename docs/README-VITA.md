# Doom64EX+ Vita

<p align="center"><img src="./shot.png"></p>
Doom64EX+ Vita is a taken port of Doom64EX code to Doom64EX+ for the PSVITA/PSTV.<br>
Doom 64 EX+ is a continuation project of Samuel "Kaiser" Villarreal's Doom 64 EX aimed to recreate DOOM 64 as closely as possible with additional modding features..

## Requirements

In order to run Doom64EX, you need `<b>`libshacccg.suprx `</b>`. If you don't have it installed already, you can install it by following this guide: https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx

## Known Issues

- Enabling the Texture Combiner will result in lighting being broken in game. It's suggested to keep it off.
- When a lot of sounds are played simultaneously, framerate can hinder a bit (down to 30-40 fps).
- Music playback and sounds sometime can go out of sync and/or repeat small chunks (This is due to a bug in fluidsynth).

## Default Controls

- X = Jump (when enabled in Options)
- Start = Pause
- Select = Open AutoMap
- Up/Down = Zoom In/Out AutoMap
- Left/Right = Change Weapon
- Triangle/Circle = Change Weapon
- L = Run
- R = Fire
- Square = Activate/Use

## Initial Setup

- Download `<b>`Doom64EX.vpk `</b>` from the Release section and install it.
- Download `<b>`Doom64EX.zip `</b>` from the Release section and extract it in `<b>`ux0:data/Doom64EX `</b>`.
- Follow one of the two paragraphs below to install game data files.

## Setup from N64 cartridge

This is the indended way to install original Doom64EX.

- Dump your own N64 cartridge of Doom 64 (any region works fine) with GameShark, GameGenie or any other source you prefer.
- Download `<b>`wadgen.zip `</b>` from the Release section.
- Run `<b>`WadGen.exe `</b>` and select the dump of your Doom 64 cartridge. It will generate the required files for Doom64EX to work.
- Copy `<b>`DOOM64.WAD `</b>` and `<b>`DOOMSND.SF2 `</b>` to `<b>`ux0:data/Doom64EX `</b>`.

## Setup from Steam

This allows to use Doom 64 remaster WAD with Doom64EX. It will have some minor issues with demo playback but will allow to play also the exclusive `<b>`The Lost Levels `</b>`.

- [Buy the game on Steam](https://store.steampowered.com/app/1148590/DOOM_64/).
- With a webbrowser, navigate to the url `<b>`steam://nav/console `</b>`.
- When asked with what to open said link, select Steam.
- On Steam, you'll get prompted with the console, input in the console `download_depot 1148590 1148591 7293157900876244073`.
- Steam will start download the depot. Once finished, it will show in the console what path it downloaded it to, navigate towards that directory.
- Download the Doom64EX Steam to EX patch made by Henky [from this url](http://henk.tech/doom64).
- Extract the archive in the same folder of the downloaded depot.
- Run `<b>`run.cmd `</b>`. It will generate an output folder with the required files for Doom64EX to work.
- Copy `<b>`DOOM64.WAD `</b>` and `<b>`DOOMSND.SF2 `</b>` to `<b>`ux0:data/Doom64EX `</b>`.

## Compiling

- Compile and install [vitaGL](https://github.com/Rinnegatamante/vitaGL) with `make NO_DEBUG=1 HAVE_HIGH_FFP_TEXUNITS=1 HAVE_WVP_ON_GPU=1 install`.
- Compile and install [fluidsynth-lite](https://github.com/fgsfdsfgs/fluidsynth-lite).
- Run `make`.

## Credits

- svkaiser for the original Doom64EX.
- fgsfds for the Doom64EX Switch port used as reference for some fixes.
- CatoTheYounger for testing the homebrew.
- Brandonheat8 for the Livearea assets.
- Adam Bilbough for the creation of the project and André Guilherme for improvements
- Rinnegatamante for the port
