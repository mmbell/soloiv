/* radar_io.h — backend-neutral sweep I/O for soloiv.
 *
 * A thin C API that the GUI/editor/perusal code calls instead of touching
 * DORADE- or Radx-specific functions directly. The primary backend is the
 * LROSE libRadx library, bridged in radx_shim.cc (compiled as C++ and exposed
 * extern "C"). A custom netCDF-C backend can slot behind the same interface
 * later (selected by SOLOIV_IO_BACKEND at build time).
 *
 * The interface fills soloiv's existing DGI/DDS structures with scaled int16
 * gate data, identical to the legacy DORADE path, so the render and editor
 * code is reused unchanged.
 */
#ifndef RADAR_IO_H
#define RADAR_IO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Phase 0 linkage/smoke probe: open `path` with Radx's auto-detecting reader
 * and return the number of sweeps, or a negative value on error
 * (-1 read failure, -2 C++ exception). */
int rio_radx_selftest(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_IO_H */
