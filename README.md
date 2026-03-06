# soloiv

soloiv is a GTK4-based radar data display and editing tool for DORADE sweep files.

It is a modernized derivative of [soloii](https://github.com/NCAR/lrose-soloii),
originally developed by Dick Oye at NCAR/EOL/RSF in the 1990s-2000s.
The original soloii used GTK 1.x and required 32-bit builds; soloiv has been
ported to GTK4 with a CMake build system and runs natively on 64-bit platforms.

## Building

Requirements:
- GTK4 development libraries
- CMake 3.16+
- C compiler (GCC or Clang)
- Optional: netCDF libraries (for netCDF file support)

On macOS with Homebrew:
```
brew install gtk4 cmake
```

On Ubuntu/Debian:
```
sudo apt install libgtk-4-dev cmake build-essential
```

Build:
```
cmake -B build -S .
cmake --build build
```

The executable will be at `build/perusal/soloiv`.

## Usage

The environment variable `DORADE_DIR` should point to a directory containing
DORADE sweep files. If you start soloiv from a directory containing sweep files,
you do not need to set `DORADE_DIR`.

## Status

This is an active port from GTK 1.x to GTK4. The application compiles and
launches but many UI features are still being connected. See `SESSION_PLAN.md`
for migration progress.

## License

BSD License - see [LICENSE.txt](LICENSE.txt).

Original soloii copyright (c) 1990-2016, University Corporation for Atmospheric
Research (UCAR). This derivative work is clearly marked per the license terms.

## Original DOI

The DOI for the original lrose-soloii is:
[https://doi.org/10.5065/apfn-y595](https://doi.org/10.5065/apfn-y595)

## Acknowledgments

- Dick Oye (NCAR/EOL/RSF) - original soloii author
- NCAR/UCAR - original development and maintenance
