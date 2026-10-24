# Building and Running soloiv

soloiv is a GTK4 radar display and editing tool for DORADE sweep files. This
guide covers environment setup, building, and launching the application on NCAR
systems (Casper/Glade) and similar Linux hosts.

## Prerequisites

- 64-bit Linux (32-bit builds are not supported)
- C compiler (GCC, Clang, or Intel `icx`)
- CMake 3.16+
- GTK4 development libraries
- Optional: netCDF libraries (for netCDF file support)

On NCAR systems, the recommended approach is a conda environment with
`gtk4`, `glib`, and build tools from conda-forge.

## One-time environment setup

Create and activate a conda environment (adjust the path/name as needed):

```bash
conda create -p /glade/work/$USER/conda-envs/soloiv-env -c conda-forge \
    gtk4 glib cmake pkg-config
conda activate /glade/work/$USER/conda-envs/soloiv-env
```

**Important:** install the `glib` meta-package, not only `libglib`. The
`glib` package provides `lib/glib-2.0/include/glibconfig.h`, which GTK
requires at compile time. Without it, the build fails with:

```
fatal error: 'glibconfig.h' file not found
```

Configure the CMake build once (from the repository root):

```bash
cd /glade/u/home/tycha/devel/soloiv
cmake -B build -S .
```

Re-run `cmake -B build -S .` only after changing `CMakeLists.txt` or when
switching compilers/environments.

## Building (every session after code changes)

```bash
conda activate /glade/work/$USER/conda-envs/soloiv-env
cd /glade/u/home/tycha/devel/soloiv
cmake --build build
```

The executable is written to:

```
build/perusal/soloiv
```

## Running

```bash
conda activate /glade/work/$USER/conda-envs/soloiv-env
cd /glade/u/home/tycha/devel/soloiv

# Point at your DORADE sweep-file directory
export DORADE_DIR=/path/to/sweep/files

./build/perusal/soloiv
```

### Data directory

- **Recommended:** set `DORADE_DIR` to the directory containing DORADE sweep
  files before launching.
- **Alternative:** start soloiv from a directory that already contains sweep
  files; it will search the current working directory.

If `DORADE_DIR` is unset and you launch from `build/`, soloiv may scan
unexpected locations (for example your home directory).

## GUI / display on NCAR systems

soloiv is a graphical GTK4 application. You need a display:

| Method | Notes |
|--------|-------|
| **Open OnDemand desktop** | Recommended. Open a terminal in the desktop session and run the commands above. |
| **SSH with X11 forwarding** | `ssh -X user@host` before running. Often slower; may not work for all GTK4 features. |

On login nodes without a GPU, you may see harmless warnings such as:

```
Gdk-WARNING **: Vulkan: Failed to detect any valid GPUs in the current config
```

These do not necessarily prevent the GUI from opening when a display is
available.

## Typical workflow (quick reference)

```bash
conda activate /glade/work/$USER/conda-envs/soloiv-env
cd /glade/u/home/tycha/devel/soloiv
cmake --build build
export DORADE_DIR=/path/to/sweep/files
./build/perusal/soloiv
```

## Troubleshooting

### `glibconfig.h` not found

Install the `glib` package in your conda environment:

```bash
conda install -c conda-forge glib
```

Verify the header exists:

```bash
ls $CONDA_PREFIX/lib/glib-2.0/include/glibconfig.h
```

### Linker errors: multiple definition of `MainOps`, `LinksIds`, etc.

These globals came from legacy K&R-style enum/union declarations in headers
that defined variables on every `#include`. The fix is to use `typedef enum`
and `typedef union` in:

- `perusal/soloii.h`
- `perusal/xyraster.h`

If you see this on an older checkout, update those headers or pull the latest
changes.

### Many `-Wdeprecated-non-prototype` warnings

The codebase uses legacy K&R C (functions without prototypes). These warnings
are expected during the GTK4 port and do not stop the build.

### Build tools on other platforms

**Ubuntu/Debian:**

```bash
sudo apt install libgtk-4-dev cmake build-essential pkg-config
```

**macOS (Homebrew):**

```bash
brew install gtk4 cmake pkg-config
cmake -B build -S .
cmake --build build
```

## Known issues

### Legacy DORADE byte parser rejects Radx-written DORADE

The in-tree DORADE byte parser (`translate/swp_file_acc.c`) rejects
Radx-written DORADE as "Current sweep file is garbage!" — it trips the block
`sizeof_struct` range check (`dd_defines.h: MAX_REC_SIZE`) somewhere in the
block loop. Genuine archival DORADE is little-endian, `SSWB`-first; Radx (and
soloiv's own Editor save, which goes through Radx) writes big-endian,
`COMM`-first sweeps. The parser's byte-swap path handles the first block but
still fails later.

Impact: the legacy parser can only read original archival DORADE, not anything
the modern stack writes. This is why no trimming tool can produce a small
legacy-readable DORADE, and why `test_radx_airborne` was switched to read all
formats through Radx. Since DORADE is being phased out (pipe old data through
`RadxConvert` first), this is low priority — but pinning the exact offending
block would let the legacy parser read modern DORADE if ever needed. Fits the
DORADE-robustness bug class.

## Related documentation

- [README.md](README.md) — project overview
- [SESSION_PLAN.md](SESSION_PLAN.md) — GTK4 migration progress
- [ROADMAP.md](ROADMAP.md) — planned work
