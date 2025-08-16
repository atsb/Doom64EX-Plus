# Doom 64 EX+

Doom 64 EX+ is a community-driven continuation of Samuel "Kaiser" Villarreal's original Doom 64 EX project. The primary goal of Doom 64 EX+ is to faithfully recreate the classic Nintendo 64 experience while incorporating modern features, extensive modding capabilities, and performance enhancements.

## Key Features and Differences from Legacy Doom 64 EX

* **Modern IWAD Support:** Full, native support for the IWAD file from Nightdive Studios' official 2020 remaster of *DOOM 64*. Please note that the old ROM dump-based IWADs from older EX versions are **not** supported.
* **The Lost Levels:** Includes support for the "Lost Levels" campaign content introduced in the official remaster.
* **Enhanced Modding:**
    * **PWAD Support:** Load custom PWAD files to play user-created maps and content.
* **Upgraded Audio Engine:** Replaced the legacy FluidSynth with FMOD Studio for higher-quality, more reliable audio playback.
* **Optimized Performance:** Delivers superior performance, running smoother than even the official Nightdive remaster in many cases.
* **Quality of Life Fixes:**
    * Secrets now trigger a notification message upon discovery.
    * Expanded map support up to the `MAP40` slot.
    * Restored the "Medkit you REALLY need!" message.
    * Numerous bug fixes for a more stable experience.
* **Modernized Codebase:**
    * **SDL3 Integration:** As one of the first *Doom* source ports to standardize on SDL3, it leverages the latest in cross-platform library support.
    * **Simplified Internals:** The KEX-related code has been largely removed to align the project more closely with the structure of other popular *Doom* source ports, simplifying development and code portability.

### A Note on Modding

For modders looking to adapt existing work or create new content for Doom 64 EX+, please be aware of the following changes from older versions of EX:

* Map markers now use `DM_START` and `DM_END` and `DS_START` and `DS_END` (in line with the remaster).
* The old `G_START` and `G_END` graphic lump markers are no longer used. Instead, graphic assets are identified as follows:
    * The **first** graphic marker in a WAD **must** be named `SYMBOLS`.
    * The **last** graphic marker in a WAD **must** be named `MOUNTC`.

You can find PWADs that have been specifically adapted for EX+ on [ModDB](https://www.moddb.com/games/doom-64/downloads/). Look for files designated for "EX+" or "EX Plus".

## License

The source code for Doom 64 EX+ is released under the terms of the original **Doom Source License**.

## Dependencies

Before compiling from source, you must have the necessary development libraries installed on your system.

#### Core Dependencies:

* **SDL3:** The core framework for windowing, input, and graphics.
* **ZLib:** A library for data compression.
* **LibPNG:** A library for reading and writing PNG image files.
* **OpenGL:** A graphics API for rendering.
* **FMOD Studio:** The audio engine.
    * **Note for Linux and BSD Users:** FMOD is a **proprietary, closed-source audio API**. Due to its licensing, it is not available in the official software repositories of most distributions and operating systems. You must download the FMOD Studio SDK manually from the [FMOD website](https://www.fmod.com/download). Please be aware of the FMOD licensing terms, which are free for non-commercial projects but have restrictions.

#### Launcher Dependency (Windows Only):

* **Microsoft .NET 6.0 (or higher):** The optional launcher application requires the .NET runtime to function. You can download it from the [official Microsoft website](https://dotnet.microsoft.com/en-us/download/dotnet/6.0).

## Installation and Compiling

### Step 1: Get the Source Code

First, clone the official repository to your local machine:

```bash
git clone https://github.com/atsb/Doom64EX-Plus
```

### Step 2: Acquire Game Data

Doom 64 EX+ requires asset files from your legally owned copy of the official *DOOM 64* remaster (e.g., from Steam or GOG).

On startup, Doom 64 EX+ will try to automatically locate asset files from your DOOM 64 installation from GOG (Windows) and Steam (Windows, Linux), in that order.

### Step 3: Compile for Your Platform

#### GNU/Linux & BSD

The repository includes build scripts for easy compilation. Ensure all dependencies, including the manually downloaded FMOD libraries, are installed and accessible to your compiler first.

* **Linux:**
    ```
    make -j
    ```
* **FreeBSD:**
    ```bash
    ./build_freebsd.sh
    ```
* **OpenBSD:**
    ```bash
    ./build_openbsd.sh
    ```
* **Raspberry Pi 3 (Raspbian):**
    ```bash
    ./build_rpi3_raspbian.sh
    ```

**Data File Paths (Linux/BSD):** The engine searches for asset files in the following order:
1.  `~/.local/share/doom64ex-plus/` (if compiled with `-DDOOM_UNIX_INSTALL`)
2.  A system-wide directory like `/usr/share/games/doom64ex-plus` (if specified with `-DDOOM_UNIX_SYSTEM_DATADIR` at compile time).
3.  The current directory where the executable is located.

Save data is stored in the executable's directory.

#### macOS

The recommended method is to install the pre-compiled version from [MacSourcePorts](https://macsourceports.com/game/doom64).

For manual compilation, you must install the dependencies via a package manager like MacPorts. Then, use the provided **XCode project file**, which is the only supported method.

**Data File Path (macOS):** Place your asset files in:
`/Users/your_username/Library/Application Support/doom64ex-plus/`

#### Windows

For Windows, use the Visual Studio solution file provided in the `Windows` directory of the repository to compile for either 32-bit or 64-bit systems.

**Data File Path (Windows):** Place the three asset files (`DOOM64.wad`, `doom64ex-plus.wad`, `DOOMSND.DLS`) in the **same directory** as the compiled `DOOM64EX+.exe`.
 These data files are retrieved from the DOOM64 Remaster on GOG or Steam.

## Usage

Once compiled and your asset files are in the correct location, simply run the executable:

* **Windows:** Double-click `DOOM64EX+.exe`. You may also use the optional `DOOM64EX+ Launcher.exe` for more configuration options.
    * **NVIDIA Users:** If you experience stuttering, it is highly recommended to disable VSync for this application in your NVIDIA Control Panel.
* **Linux/BSD/macOS:** Launch the executable from your terminal:
    ```bash
    ./doom64ex-plus
    ```

Enjoy the game! For support or to join the community, check out the official Discord server.
