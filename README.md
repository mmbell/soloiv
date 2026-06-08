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
- C and C++ compilers (Apple Clang / GCC)
- LROSE `libRadx` for the sweep I/O backend (see below)
- HDF5 (incl. `hdf5_cpp`), netCDF, and udunits — pulled in transitively by Radx

On macOS with Homebrew:
```
brew install gtk4 cmake hdf5 netcdf udunits
```

On Ubuntu/Debian:
```
sudo apt install libgtk-4-dev cmake build-essential \
                 libhdf5-dev libnetcdf-dev libudunits2-dev
```

### Sweep I/O backend (Radx)

All sweep I/O — DORADE **and** CfRadial (netCDF v1/v2) — is routed through the
maintained LROSE `libRadx` C++ library via a thin `extern "C"` shim
(`translate/radx_shim.cc`, `translate/radar_io.c`). The build expects an LROSE
install providing `libRadx.a` / `libNcxx.a` / `libtoolsa.a` and the `Radx/`
headers. By default it looks in `/Users/mmbell/lrose`; override with:
```
cmake -B build -S . -DLROSE_PREFIX=/path/to/lrose
```

Build options:
- `SOLOIV_IO_BACKEND` (`radx` default | `custom`) — selects the I/O backend.
- `SOLOIV_DORADE_VIA_RADX` (`ON` default) — read/write DORADE through Radx,
  retiring the legacy in-tree byte parser. The legacy parser stays compiled
  and can be re-enabled at **runtime** by setting the `SOLOIV_DORADE_LEGACY`
  environment variable — an instant fallback with no rebuild. CfRadial always
  uses Radx (there is no legacy reader for it).

Build:
```
cmake -B build -S .
cmake --build build
```

The executable will be at `build/perusal/soloiv`.

See [BUILD_AND_RUN.md](BUILD_AND_RUN.md) for conda setup, NCAR display notes,
and troubleshooting.

## Usage

The environment variable `DORADE_DIR` should point to a directory containing
sweep files. If you start soloiv from a directory containing sweep files,
you do not need to set `DORADE_DIR`. Both DORADE (`swp.*`) and CfRadial
(`cfrad*.nc`) sweeps in that directory are discovered and listed together;
edited sweeps are written back in the same format they were read.

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
