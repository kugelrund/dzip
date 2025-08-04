# Dzip

Dzip is a compression program with the special purpose of compressing Quake .dem files.

Dzip was originally written by Stefan Schwoon (<schwoon@in.tum.de>) and later updated and expanded by Nolan Pflug (<radix@planetquake.com>).
For more information, including on the earlier development, see the original [Readme](Readme) and the old [Dzip homepage](http://quake.speeddemosarchive.com/dzip/).

Dzip makes use of the zlib library copyrighted by [Jean-loup Gailly](http://gailly.net/) and [Mark Adler](http://en.wikipedia.org/wiki/Mark_Adler).
More information on zlib can be found at the [zlib homepage](https://www.zlib.net/).

## Builds

A recent Windows build can be found on [quake.speeddemosarchive.com](http://quake.speeddemosarchive.com/quake/downloads.html).
For other systems, please see section Compilation.

## Compilation

If no system zlib is available, make sure to clone the repository using `--recurse-submodules` to clone the required zlib sources too.

[CMake](https://cmake.org/) may be used to build Dzip.
For example, from the top-level directory the commands

```bash
cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
cmake --install ./build --prefix ./install
```

may be run to get the compiled dzip executable in the install subdirectory.

Note that compilation of the GUI is only supported on Windows with Visual Studio for now.
Make sure to target Win32, other architectures (including x64) are not supported for now.
The necessary Visual Studio projects may for example be generated with:

```bash
cmake -B ./build -S . -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX="./install"
```

You can then either open the generated Visual Studio solution and work from there, or for example run

```bash
cmake --build ./build --config Release
cmake --install ./build
```

from the command line.

## Usage

See [dzip.txt](install/dzip.txt) and [DzipGui.txt](install/DzipGui.txt).
