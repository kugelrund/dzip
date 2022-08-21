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
For example, on unix systems, you may want to setup the build files using

```bash
mkdir build
cd build
cmake ..
```

followed by running `make` to compile, assuming you are using CMake with Makefiles.

Note that compilation of the GUI is only supported on Windows with Visual Studio for now.
A simple way to generate the necessary Visual Studio projects is to use the `cmake-gui` on Windows.
Make sure to target Win32, other platforms (including x64) are not supported for now.

For big-endian machines, it is necessary to define `DZIP_BIG_ENDIAN`.

## Usage

See [dzip.txt](install/dzip.txt) and [DzipGui.txt](install/DzipGui.txt).
