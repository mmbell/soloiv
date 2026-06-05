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

struct dd_general_info;       /* forward decl — defined in dd_general_info.h */
struct dd_file_name_v3;       /* forward decl — defined in dd_files.h        */

/* ------------------------------------------------------------------ *
 *  Format sniffing                                                    *
 * ------------------------------------------------------------------ */
typedef enum { RIO_FMT_UNKNOWN, RIO_FMT_DORADE, RIO_FMT_CFRADIAL } rio_fmt_t;

/* Probe a path by filename/extension (and, where needed, magic bytes). */
rio_fmt_t rio_sniff(const char *path);

/* True if rio_scan should accept this directory entry as a sweep file. */
int rio_filename_is_sweep(const char *name);

/* Parse a CfRadial filename into a dd_file_name_v3 (time, radar, ms).
 * Returns >=1 on success (like craack_ddfn), <1 on failure. */
int rio_craack_cfradial(const char *name, struct dd_file_name_v3 *ddfn);

/* ------------------------------------------------------------------ *
 *  High-level DGI-filling reader (dispatched from dd_absorb_*)        *
 * ------------------------------------------------------------------ */

/* Read sweep header for the file named by dgi->directory_name +
 * dgi->sweep_file_name (CfRadial). Fills vold/radd/celv/parm[]/swib/sswb,
 * builds dgi->source_rat, allocates qdat_ptrs, resets the ray cursor.
 * Returns the number of rays in the sweep, or <0 on error. */
int rio_read_header(struct dd_general_info *dgi);

/* Fill ryib/asib/qdat_ptrs for the current cursor ray and advance.
 * Returns >=1 on success, <1 at end-of-sweep / error. */
int rio_read_ray(struct dd_general_info *dgi);

/* Reset the ray cursor to the start of the current sweep (rewind). */
void rio_rewind(struct dd_general_info *dgi);

/* Release the cached Radx volume held for this dgi. */
void rio_close(struct dd_general_info *dgi);

/* ------------------------------------------------------------------ *
 *  Neutral C++ shim accessors (implemented in radx_shim.cc)           *
 *  No soloiv structs cross this boundary.                             *
 * ------------------------------------------------------------------ */

typedef void *RioVolH;

typedef struct {
  int    sweep_num;
  int    sweep_mode;     /* Radx::SweepMode_t */
  double fixed_angle;    /* deg */
  int    start_ray;      /* volume ray index of first ray in sweep */
  int    end_ray;        /* volume ray index of last ray in sweep  */
  int    nrays;
} RioSweep;

typedef struct {
  double lat_deg, lon_deg, alt_km;
  char   instrument[32];
  char   scan_name[32];
  int    platform_type;        /* Radx::PlatformType_t */
  double nyquist_mps;
  double unambig_range_km;
  long   start_time_secs;
} RioRadar;

typedef struct {
  double az_deg, el_deg;
  long   time_secs;
  double nano_secs;
  int    n_gates;
  double start_range_km;
  double gate_spacing_km;
  double nyquist_mps;
  double unambig_range_km;
  int    has_georef;
  double georef_lat, georef_lon, georef_alt_km;
} RioRay;

typedef struct {
  char   name[16];
  char   units[16];
  int    data_type;            /* Radx::DataType_t (SI08/SI16/SI32/FL32...) */
  double scale;                /* native packing: phys = raw*scale + offset */
  double offset;
  float  missing_fl32;
} RioField;

RioVolH rio_vol_open(const char *path);
void    rio_vol_close(RioVolH vh);
int     rio_vol_nsweeps(RioVolH vh);
int     rio_vol_nrays(RioVolH vh);
int     rio_vol_sweep(RioVolH vh, int sw, RioSweep *out);
int     rio_vol_radar(RioVolH vh, RioRadar *out);
int     rio_vol_ray(RioVolH vh, int rayIdx, RioRay *out);
int     rio_vol_nfields(RioVolH vh);
int     rio_vol_field(RioVolH vh, int f, RioField *out);
int     rio_vol_ray_field_fl32(RioVolH vh, int rayIdx, int f,
                               const float **data_out, int *ngates_out,
                               float *missing_out);

/* Radx::DataType_t mirror (must match Radx/Radx.hh ordering). */
enum {
  RIO_SI08 = 0, RIO_SI16, RIO_SI32, RIO_UI08, RIO_UI16, RIO_UI32,
  RIO_FL32, RIO_FL64
};

/* Phase-0 linkage/smoke probe (open + report sweep count). */
int rio_radx_selftest(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_IO_H */
