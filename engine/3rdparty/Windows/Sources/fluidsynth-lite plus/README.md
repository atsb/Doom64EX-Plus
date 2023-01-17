FluidSynth-Lite 
==================
[![Build Status](https://travis-ci.org/dotfloat/fluidsynth-lite.svg?branch=master)](https://travis-ci.org/dotfloat/fluidsynth-lite)
[![Build status](https://ci.appveyor.com/api/projects/status/7jwt8ecihyj7lpdl/branch/master?svg=true)](https://ci.appveyor.com/project/dotfloat/fluidsynth-lite/branch/master)

FluidSynth-Lite is a stripped down version of FluidSynth, the real-time software
synthesizer. It's meant to be used by [Doom64EX](http://github.com/svkaiser/Doom64EX).

The major differences are as follows:

- All sound server drivers have been removed. It's expected that some other
library can handle actual playback (ex. Pulseaudio or SDL2).
- Dependencies on GTK+ for its threads/atomics implementations have been
  removed. Instead, we use `stdatomic.h` if present, or compiler builtin
  functions otherwise. For threads we use `pthread` or `winapi` depending on OS.
- There are no external dependencies at all now, which is great.

The original README document can be found in `README.fluidsynth`.

Compiling for PSVita
===========
```
make -f Makefile.vita install
```

Compiling for Switch
===========
```
make -f Makefile.nx install
```

Compiling for other platforms
===========

[CMake](https://cmake.org) is the only build tool that is supported -- all
others have been removed. Make sure you have at least version 2.6.3, and that
`cmake` can be found in the `PATH` environment variable.

Download the source code for this project, extract it (if applicable), and
navigate to it. Then just run cmake:

    $ cmake .         # Generate build files in the current directory
    $ cmake --build . # Compile the project

By default, this will compile a `libfluidsynth.a` on Linux and `fluidsynth.lib`
on Windows.

It's also possible to use CMake-GUI.
