/* radx_shim.cc — extern "C" bridge from soloiv to the LROSE libRadx library.
 *
 * All sweep I/O (DORADE + CfRadial) is routed through Radx's robust readers
 * and writers. This file is compiled as C++17 against the prebuilt
 * libRadx.a / libNcxx.a and exposes a flat C API consumed by the rest of the
 * (C) codebase. No C++ exception is ever allowed to cross the extern "C"
 * boundary.
 *
 * Phase 0: a single smoke-test entry point that proves linkage and that Radx
 * can open both sample formats. Real read/write backends follow in later
 * phases.
 */

#include <Radx/RadxFile.hh>
#include <Radx/RadxVol.hh>

#include <string>

extern "C" {

/* Open `path` with Radx's auto-detecting reader and return the number of
 * sweeps in the volume, or a negative value on error:
 *   -1  read failure (Radx getErrStr available internally)
 *   -2  C++ exception caught
 * This is a Phase-0 linkage/smoke probe only.
 */
int rio_radx_selftest(const char *path)
{
  if (path == nullptr) {
    return -1;
  }
  try {
    RadxFile f;
    RadxVol vol;
    if (f.readFromPath(std::string(path), vol) != 0) {
      return -1;
    }
    vol.loadSweepInfoFromRays();
    return static_cast<int>(vol.getNSweeps());
  } catch (...) {
    return -2;
  }
}

} /* extern "C" */
