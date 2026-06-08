/* radar_io.c — pure-C backend-neutral sweep I/O layer.
 *
 * Consumes the struct-neutral Radx accessors from radx_shim.cc and fills
 * soloiv's DGI/DDS structures with scaled int16 gate data, exactly as the
 * legacy DORADE reader does, so the render and editor code is reused
 * unchanged. Phase 1 implements the CfRadial read path; DORADE is routed
 * through the same accessors in Phase 2.
 *
 * The cached Radx volume (and the sweep/ray cursor) lives in dgi->gpptr7 as a
 * `struct rio_state` so a whole CfRadial volume stays resident across the
 * repeated dd_absorb_ray_info() calls the render/edit loops make.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <dorade_headers.h>   /* umbrella: dd_defines.h + dd_general_info.h ... */
#include "dd_files.h"
#include "dd_time.h"
#include "radar_io.h"

/* DORADE scan-mode codes (see translate/uf_fmt.h). */
#define RIO_DM_CAL 0
#define RIO_DM_PPI 1
#define RIO_DM_COP 2
#define RIO_DM_RHI 3
#define RIO_DM_VER 4
#define RIO_DM_TAR 5
#define RIO_DM_SUR 8

/* Radx::PrimaryAxis_t values (axis of antenna rotation). The vertical axes
 * (Z, Z_PRIME) describe an ordinary azimuth/elevation-surveillance scan that
 * is plotted by azimuth; the others (Y, X, Y_PRIME, X_PRIME) describe the
 * airborne tail/fuselage scanning convention plotted from rotation/tilt. */
#define RIO_AXIS_Z       0
#define RIO_AXIS_Y       1
#define RIO_AXIS_X       2
#define RIO_AXIS_Z_PRIME 3
#define RIO_AXIS_Y_PRIME 4
#define RIO_AXIS_X_PRIME 5
#define RIO_AXIS_IS_VERTICAL(a) ((a) == RIO_AXIS_Z || (a) == RIO_AXIS_Z_PRIME)

/* From the rest of the s2 library (K&R prototypes). */
extern void   dd_alloc_data_field();
extern int    dd_return_id_num();
extern double d_time_stamp();
extern void   str_terminate();
extern void   dd_radar_angles();    /* (asib, cfac, ra, dgi) — Testud georef */
extern double dd_rotation_angle();   /* (dgi) — scan-mode-aware beam angle    */
extern void   dd_set_uniform_cells(); /* (dds) — build range->cell LUT         */

/* ------------------------------------------------------------------ *
 *  Per-DGI Radx state (stashed in dgi->gpptr7)                        *
 * ------------------------------------------------------------------ */
struct rio_state {
  RioVolH vol;
  char    path[768];
  int     swp_index;        /* which sweep within the volume          */
  int     sweep_start_ray;  /* volume ray index of this sweep's ray 0  */
  int     sweep_nrays;
  int     cursor;           /* 0..sweep_nrays-1: next ray to read      */
  int     nfields;
  rio_fmt_t src_fmt;        /* original on-disk format (for write-back) */
  RioRadar  radar;          /* captured radar/platform metadata         */
  int     radx_sweep_mode;  /* Radx::SweepMode_t of the source sweep    */
};

/* Per-DGI write state (in dgi->gpptr6): the RadxVol being assembled. */
struct rio_write_state {
  RioWVolH wvol;
};

static struct rio_state *rio_get_state(struct dd_general_info *dgi)
{
  return (struct rio_state *) dgi->gpptr7;
}

static struct rio_write_state *rio_ensure_write_state(struct dd_general_info *dgi)
{
  struct rio_write_state *ws = (struct rio_write_state *) dgi->gpptr6;
  if (!ws) {
    ws = (struct rio_write_state *) malloc(sizeof(*ws));
    memset(ws, 0, sizeof(*ws));
    dgi->gpptr6 = (void *) ws;
  }
  return ws;
}

static struct rio_state *rio_ensure_state(struct dd_general_info *dgi)
{
  struct rio_state *st = (struct rio_state *) dgi->gpptr7;
  if (!st) {
    st = (struct rio_state *) malloc(sizeof(*st));
    memset(st, 0, sizeof(*st));
    dgi->gpptr7 = (void *) st;
  }
  return st;
}

/* ------------------------------------------------------------------ *
 *  Format sniffing & filename handling                                *
 * ------------------------------------------------------------------ */

static int has_suffix(const char *s, const char *suf)
{
  size_t ls = strlen(s), lf = strlen(suf);
  return ls >= lf && strcmp(s + ls - lf, suf) == 0;
}

rio_fmt_t rio_sniff(const char *path)
{
  const char *base;
  if (!path) return RIO_FMT_UNKNOWN;
  base = strrchr(path, '/');
  base = base ? base + 1 : path;
  if (strncmp(base, "swp.", 4) == 0) return RIO_FMT_DORADE;
  if (has_suffix(base, ".nc")) return RIO_FMT_CFRADIAL;
  if (strncmp(base, "cfrad", 5) == 0) return RIO_FMT_CFRADIAL;
  return RIO_FMT_UNKNOWN;
}

int rio_filename_is_sweep(const char *name)
{
  if (!name) return 0;
  if (strstr(name, ".tmp")) return 0;
  if (strncmp(name, "swp.", 4) == 0) return 1;
  if (strncmp(name, "cfrad", 5) == 0 && has_suffix(name, ".nc")) return 1;
  return 0;
}

int rio_should_use_radx(const char *path)
{
  rio_fmt_t f = rio_sniff(path);
  if (f == RIO_FMT_CFRADIAL) return 1;     /* only Radx reads CfRadial */
#ifdef SOLOIV_DORADE_VIA_RADX
  if (f == RIO_FMT_DORADE)
    return getenv("SOLOIV_DORADE_LEGACY") ? 0 : 1;
#endif
  return 0;
}

/* Parse a CfRadial filename
 *   cfrad[2].YYYYMMDD_HHMMSS.mmm_to_..._RADAR_SCAN.nc
 * into a dd_file_name_v3 (time_stamp, radar_name, milliseconds, version). */
int rio_craack_cfradial(const char *name, struct dd_file_name_v3 *ddfn)
{
  const char *p, *dot;
  char base[256], *tok, *toks[32];
  int ntok = 0, yy, mon, dd, hh, mm, ss, ms = 0, jj, instr;
  DD_TIME dts;

  if (!name || !ddfn) return -1;
  p = strstr(name, "cfrad");
  if (!p) return -1;
  p = strchr(p, '.');             /* first dot after cfrad/cfrad2 */
  if (!p) return -1;
  p++;                            /* -> "YYYYMMDD_HHMMSS.mmm_to_..." */

  if (sscanf(p, "%4d%2d%2d_%2d%2d%2d", &yy, &mon, &dd, &hh, &mm, &ss) != 6)
    return -1;
  dot = strchr(p, '.');
  if (dot && isdigit((unsigned char) dot[1]))
    ms = atoi(dot + 1);

  memset(&dts, 0, sizeof(dts));
  dts.year = yy;
  dts.month = mon;
  dts.day = dd;
  dts.day_seconds = D_SECS(hh, mm, ss, ms);
  ddfn->time_stamp = d_time_stamp(&dts);
  ddfn->milliseconds = ms;
  ddfn->version = 1;

  /* Radar/instrument name = the token immediately after the time spec.
   * Names are cfrad[2].<t1>[_to_<t2>]_<instrument>_<scan>.nc. Token 0 is
   * "cfrad.<date>" and token 1 the time-of-day; an optional "to" + date +
   * time precede the instrument. The scan name may itself contain underscores
   * (e.g. SEAPOL_PICCOLO_LONG_SUR), so a "second-to-last token" heuristic
   * mis-reads the radar as "LONG"/"VOL1" and each file lands under its own
   * radar in the catalog -- breaking next/prev file navigation. */
  strncpy(base, name, sizeof(base) - 1);
  base[sizeof(base) - 1] = '\0';
  { char *nc = strstr(base, ".nc"); if (nc) *nc = '\0'; }
  for (tok = strtok(base, "_"); tok && ntok < 32; tok = strtok(NULL, "_"))
    toks[ntok++] = tok;
  instr = 2;                        /* no "_to_": cfrad.<date> <time> <instr> */
  for (jj = 0; jj < ntok; jj++) {
    if (strcmp(toks[jj], "to") == 0) { instr = jj + 3; break; }
  }
  if (instr < ntok)
    strncpy(ddfn->radar_name, toks[instr], sizeof(ddfn->radar_name) - 1);
  else if (ntok >= 2)
    strncpy(ddfn->radar_name, toks[ntok - 2], sizeof(ddfn->radar_name) - 1);
  else
    strcpy(ddfn->radar_name, "RADAR");
  ddfn->radar_name[sizeof(ddfn->radar_name) - 1] = '\0';
  *ddfn->comment = '\0';
  strncpy(ddfn->raw_name, name, sizeof(ddfn->raw_name) - 1);
  ddfn->raw_name[sizeof(ddfn->raw_name) - 1] = '\0';
  return 1;
}

/* ------------------------------------------------------------------ *
 *  Mapping helpers                                                    *
 * ------------------------------------------------------------------ */

/* Radx::SweepMode_t -> DORADE scan_mode. */
static int rio_dorade_scan_mode(int radx_mode)
{
  switch (radx_mode) {
    case 0:  return RIO_DM_CAL;   /* CALIBRATION */
    case 1:  return RIO_DM_PPI;   /* SECTOR */
    case 2:  return RIO_DM_COP;   /* COPLANE */
    case 3:  return RIO_DM_RHI;   /* RHI */
    case 4:  return RIO_DM_VER;   /* VERTICAL_POINTING */
    case 8:  return RIO_DM_SUR;   /* AZIMUTH_SURVEILLANCE */
    case 9:  return RIO_DM_RHI;   /* ELEVATION_SURVEILLANCE */
    case 12: return RIO_DM_TAR;   /* POINTING */
    case 15: return RIO_DM_PPI;   /* MANUAL_PPI */
    case 16: return RIO_DM_RHI;   /* MANUAL_RHI */
    default: return RIO_DM_PPI;
  }
}

/* Radx::PlatformType_t -> DORADE radar_type (see dd_defines.h). */
static int rio_dorade_radar_type(int ptype)
{
  switch (ptype) {
    case 5: return AIR_FORE;   /* AIRCRAFT_FORE  */
    case 6: return AIR_AFT;    /* AIRCRAFT_AFT   */
    case 7: return AIR_TAIL;   /* AIRCRAFT_TAIL  */
    case 8: return AIR_LF;     /* AIRCRAFT_BELLY */
    case 4: return AIR_FORE;   /* AIRCRAFT       */
    case 3: return SHIP;       /* SHIP           */
    default: return GROUND;
  }
}

static int rio_is_airborne(int radar_type)
{
  return radar_type == AIR_FORE || radar_type == AIR_AFT ||
         radar_type == AIR_TAIL || radar_type == AIR_LF  ||
         radar_type == AIR_NOSE;
}

/* Populate dds->ryib + dds->asib from a Radx ray and compute the derived
 * radar_angles (ra) using the same Testud georef the legacy DORADE reader
 * uses, so beam positioning is identical for ground and airborne data. */
static void rio_fill_ray_angles(struct dd_general_info *dgi, const RioRay *ray)
{
  DDS_PTR dds = dgi->dds;
  dds->ryib->azimuth = (float) ray->az_deg;
  dds->ryib->elevation = (float) ray->el_deg;
  if (ray->has_georef) {
    dds->asib->latitude = (float) ray->georef_lat;
    dds->asib->longitude = (float) ray->georef_lon;
    dds->asib->altitude_msl = (float) ray->georef_alt_km;
    dds->asib->heading = (float) ray->heading;
    dds->asib->roll = (float) ray->roll;
    dds->asib->pitch = (float) ray->pitch;
    dds->asib->drift_angle = (float) ray->drift;
    dds->asib->rotation_angle = (float) ray->rotation;
    dds->asib->tilt = (float) ray->tilt;
    dds->asib->ew_velocity = (float) ray->ew_velocity;
    dds->asib->ns_velocity = (float) ray->ns_velocity;
    dds->asib->vert_velocity = (float) ray->vert_velocity;
  }
  dd_radar_angles(dds->asib, dds->cfac, dds->ra, dgi);
}

/* §4.2.1 field scale/bias selection. Packed integer fields carry the file's
 * own packing straight through (lossless). Float-on-disk fields fall back to
 * a default step of 0.01 for now (data-driven scaling is the Phase-7
 * refinement; the arthur2015 samples are all packed and never hit it). */
static void rio_pick_scale(const RioField *fld, float *scale, float *bias)
{
  switch (fld->data_type) {
    case RIO_SI08:
    case RIO_SI16:
    case RIO_SI32:
    case RIO_UI08:
    case RIO_UI16:
    case RIO_UI32:
      if (fld->scale != 0.0) {
        *scale = (float) (1.0 / fld->scale);
        *bias  = (float) (-fld->offset / fld->scale);
        return;
      }
      /* fall through if scale is degenerate */
    default:
      *scale = 100.0f;   /* TODO(phase7): data-driven scale for FL32 fields */
      *bias  = 0.0f;
      break;
  }
}

/* ------------------------------------------------------------------ *
 *  Build dgi->source_rat from Radx ray angles                         *
 *  (mirrors dd_create_dynamic_rotang_info's memory layout)            *
 * ------------------------------------------------------------------ */
static int rio_build_rotang(struct dd_general_info *dgi, struct rio_state *st,
                            int dorade_scan_mode)
{
  const int angle_ndx_size = 480;
  int size, mm, ii, a_offset;
  int nrays = st->sweep_nrays;
  struct rot_ang_table *rat;
  struct rot_table_entry *entry;
  int32_t *ang_index;

  if (nrays <= 0) return -1;

  if (dgi->source_rat) {
    free(dgi->source_rat);
    dgi->source_rat = NULL;
  }

  size = sizeof(struct rot_ang_table);
  size = ((size - 1) / 8 + 1) * 8;          /* 8-byte align tables */
  mm = size;
  size += angle_ndx_size * sizeof(int32_t)
        + nrays * sizeof(struct rot_table_entry);

  rat = (struct rot_ang_table *) malloc(size);
  if (!rat) return -1;
  memset(rat, 0, size);
  dgi->source_rat = rat;

  memcpy(rat->name_struct, "RKTB", 4);
  rat->sizeof_struct = size;
  rat->angle2ndx = (float) angle_ndx_size / 360.0f;
  rat->ndx_que_size = angle_ndx_size;
  rat->first_key_offset = mm + angle_ndx_size * sizeof(int32_t);
  rat->angle_table_offset = mm;
  rat->num_rays = nrays;

  entry = (struct rot_table_entry *) ((char *) rat + rat->first_key_offset);
  for (ii = 0; ii < nrays; ii++, entry++) {
    RioRay r;
    if (rio_vol_ray(st->vol, st->sweep_start_ray + ii, &r) != 0)
      return -1;
    rio_fill_ray_angles(dgi, &r);
    entry->rotation_angle = (float) dd_rotation_angle(dgi);
    entry->offset = ii;
    entry->size = 0;
  }
  (void) dorade_scan_mode;

  a_offset = rat->angle_table_offset;
  ang_index = (int32_t *) ((char *) rat + a_offset);
  for (ii = 0; ii < angle_ndx_size; ii++)
    ang_index[ii] = -1;
  entry = (struct rot_table_entry *) ((char *) rat + rat->first_key_offset);
  for (ii = 0; ii < nrays; ii++, entry++) {
    int jj = (int) (entry->rotation_angle * rat->angle2ndx);
    if (jj < 0 || jj > angle_ndx_size - 1) jj = 0;
    ang_index[jj] = ii;
  }
  return 0;
}

/* ------------------------------------------------------------------ *
 *  Header read                                                        *
 * ------------------------------------------------------------------ */
int rio_read_header(struct dd_general_info *dgi)
{
  struct rio_state *st;
  DDS_PTR dds = dgi->dds;
  RioSweep sw;
  RioRadar radar;
  RioRay ray0;
  char path[768];
  int scan_mode, radar_type, pn, i;

  st = rio_ensure_state(dgi);

  /* Build the full path from directory + filename. */
  snprintf(path, sizeof(path), "%s%s", dgi->directory_name,
           dgi->sweep_file_name);

  /* Open (or reuse) the cached volume. */
  if (st->vol && strcmp(st->path, path) != 0) {
    rio_vol_close(st->vol);
    st->vol = NULL;
  }
  if (!st->vol) {
    st->vol = rio_vol_open(path);
    if (!st->vol) {
      fprintf(stderr, "rio_read_header: Radx failed to open %s\n", path);
      return -1;
    }
    strncpy(st->path, path, sizeof(st->path) - 1);
    st->path[sizeof(st->path) - 1] = '\0';
    st->swp_index = 0;     /* samples are 1 sweep/file (see §4.4) */
  }

  if (rio_vol_sweep(st->vol, st->swp_index, &sw) != 0) return -1;
  if (rio_vol_radar(st->vol, &radar) != 0) return -1;
  st->sweep_start_ray = sw.start_ray;
  st->sweep_nrays = sw.nrays;
  st->cursor = 0;
  st->src_fmt = rio_sniff(path);
  st->radar = radar;
  st->radx_sweep_mode = sw.sweep_mode;

  if (rio_vol_ray(st->vol, sw.start_ray, &ray0) != 0) return -1;
  radar_type = rio_dorade_radar_type(radar.platform_type);
  /* The axis of rotation -- not the platform label -- decides how the beam
   * is positioned. Tail/fuselage scans (Y/X/Y_PRIME/X_PRIME) use the DORADE
   * AIR scan mode so dd_rotation_angle / dd_azimuth_angle take the
   * platform-georeferenced (rotation/tilt) branch. A vertical axis (Z/Z_PRIME)
   * is an ordinary azimuth/elevation surveillance PPI plotted from the
   * earth-relative azimuth -- even when the platform is a moving ship that the
   * source mislabels platform_type=aircraft (e.g. SEAPOL). In that case drop
   * the airborne radar_type too, so the georef-projection display paths are
   * bypassed and the sweep renders as a plain ground-style PPI (as HawkEye
   * does). */
  if (rio_is_airborne(radar_type) &&
      !RIO_AXIS_IS_VERTICAL(radar.primary_axis)) {
    scan_mode = AIR;
  } else {
    scan_mode = rio_dorade_scan_mode(sw.sweep_mode);
    if (rio_is_airborne(radar_type)) {
      radar_type = GROUND;
    }
  }

  /* ---- radar descriptor ---- */
  str_terminate(dgi->radar_name, radar.instrument, 8);
  str_terminate(dds->radd->radar_name, radar.instrument, 8);
  dds->radd->scan_mode = scan_mode;
  dds->radd->radar_type = radar_type;
  dds->radd->radar_latitude = (float) radar.lat_deg;
  dds->radd->radar_longitude = (float) radar.lon_deg;
  dds->radd->radar_altitude = (float) radar.alt_km;
  dds->radd->eff_unamb_vel = (float) radar.nyquist_mps;
  dds->radd->eff_unamb_range = (float) radar.unambig_range_km;
  dds->radd->data_compress = 0;

  /* ---- platform (georef / ground location) ---- */
  if (ray0.has_georef) {
    dds->asib->latitude = (float) ray0.georef_lat;
    dds->asib->longitude = (float) ray0.georef_lon;
    dds->asib->altitude_msl = (float) ray0.georef_alt_km;
  } else {
    dds->asib->latitude = (float) radar.lat_deg;
    dds->asib->longitude = (float) radar.lon_deg;
    dds->asib->altitude_msl = (float) radar.alt_km;
  }

  /* ---- cell vector (range geometry) ---- */
  dds->celv->number_cells = ray0.n_gates;
  for (i = 0; i < ray0.n_gates && i < MAXCVGATES; i++) {
    dds->celv->dist_cells[i] =
      (float) ((ray0.start_range_km + i * ray0.gate_spacing_km) * 1000.0);
  }
  memcpy(dds->celvc, dds->celv,
         sizeof(struct cell_d));   /* header only; dist_cells aliased below */
  for (i = 0; i < ray0.n_gates && i < MAXCVGATES; i++)
    dds->celvc->dist_cells[i] = dds->celv->dist_cells[i];

  /* Build the range->cell lookup table that dd_cell_num/dd_range_gate (the
   * click readout + clipping) depend on. Without this, a data-cell click
   * dereferences a NULL LUT and segfaults. */
  dd_set_uniform_cells(dds);

  /* ---- sweep info ---- */
  dds->swib->num_rays = sw.nrays;
  dds->swib->fixed_angle = (float) sw.fixed_angle;
  dds->swib->sweep_num = sw.sweep_num;
  str_terminate(dds->swib->radar_name, radar.instrument, 8);
  {
    RioRay rlast;
    rio_fill_ray_angles(dgi, &ray0);
    dds->swib->start_angle = (float) dd_rotation_angle(dgi);
    if (rio_vol_ray(st->vol, sw.start_ray + sw.nrays - 1, &rlast) == 0) {
      rio_fill_ray_angles(dgi, &rlast);
      dds->swib->stop_angle = (float) dd_rotation_angle(dgi);
    }
  }

  /* ---- volume header (mostly for time/writer) ---- */
  {
    time_t ts = (time_t) radar.start_time_secs;
    struct tm tmv;
    gmtime_r(&ts, &tmv);
    dds->vold->year = tmv.tm_year + 1900;
    dds->vold->month = tmv.tm_mon + 1;
    dds->vold->day = tmv.tm_mday;
    dds->vold->volume_num = 1;
  }

  /* ---- parameters / fields ---- */
  st->nfields = rio_vol_nfields(st->vol);
  if (st->nfields > MAX_PARMS) st->nfields = MAX_PARMS;
  for (pn = 0; pn < st->nfields; pn++) {
    RioField fm;
    float scale = 100.0f, bias = 0.0f;
    struct parameter_d *parm = dds->parm[pn];
    if (rio_vol_field(st->vol, pn, &fm) != 0) continue;
    rio_pick_scale(&fm, &scale, &bias);

    str_terminate(parm->parameter_name, fm.name, 8);
    str_terminate(parm->param_units, fm.units, 8);
    parm->binary_format = DD_16_BITS;
    parm->parameter_scale = scale;
    parm->parameter_bias = bias;
    parm->bad_data = DELETE_FLAG;            /* -32768 */
    parm->offset_to_data = 0;
    parm->number_cells = ray0.n_gates;
    parm->meters_to_first_cell = (float) (ray0.start_range_km * 1000.0);
    parm->meters_between_cells = (float) (ray0.gate_spacing_km * 1000.0);
    parm->eff_unamb_vel = (float) radar.nyquist_mps;

    dds->field_present[pn] = YES;
    dds->number_cells[pn] = ray0.n_gates;
    dds->field_id_num[pn] = dd_return_id_num(parm->parameter_name);
    dds->parm[pn]->binary_format = DD_16_BITS;
    dd_alloc_data_field(dgi, pn);            /* mallocs qdat_ptrs[pn] */
  }
  dgi->num_parms = st->nfields;
  dgi->source_num_parms = st->nfields;
  dds->radd->num_parameter_des = st->nfields;

  /* ---- bookkeeping & rotation table ---- */
  dgi->source_num_rays = sw.nrays;
  dgi->source_ray_num = 0;
  dgi->new_sweep = YES;
  dgi->new_vol = YES;
  dgi->source_fmt = CFRADIAL_FMT;

  if (rio_build_rotang(dgi, st, scan_mode) != 0) {
    fprintf(stderr, "rio_read_header: failed to build rotation table\n");
    return -1;
  }
  return sw.nrays;
}

/* ------------------------------------------------------------------ *
 *  Ray read                                                           *
 * ------------------------------------------------------------------ */
int rio_read_ray(struct dd_general_info *dgi)
{
  struct rio_state *st = rio_get_state(dgi);
  DDS_PTR dds = dgi->dds;
  RioRay ray;
  int rayIdx, pn, i, ncells;
  time_t ts;
  struct tm tmv;

  if (!st || !st->vol) return 0;
  if (st->cursor >= st->sweep_nrays) return 0;     /* end of sweep */

  rayIdx = st->sweep_start_ray + st->cursor;
  if (rio_vol_ray(st->vol, rayIdx, &ray) != 0) return 0;

  /* ---- ray info (az/el, platform georef, derived radar angles) ---- */
  rio_fill_ray_angles(dgi, &ray);
  dds->ryib->sweep_num = dds->swib->sweep_num;
  dds->ryib->ray_status = 0;

  ts = (time_t) ray.time_secs;
  gmtime_r(&ts, &tmv);
  dds->ryib->julian_day = tmv.tm_yday + 1;
  dds->ryib->hour = tmv.tm_hour;
  dds->ryib->minute = tmv.tm_min;
  dds->ryib->second = tmv.tm_sec;
  dds->ryib->millisecond = (short) (ray.nano_secs * 1e-6);

  dgi->time = (double) ray.time_secs + ray.nano_secs * 1e-9;
  if (st->cursor == 0) dgi->time0 = dgi->time;

  /* ---- gate data: re-quantize Radx fl32 to scaled int16 ---- */
  ncells = dds->celv->number_cells;
  for (pn = 0; pn < st->nfields; pn++) {
    struct parameter_d *parm = dds->parm[pn];
    short *qq = (short *) dds->qdat_ptrs[pn];
    const float *data = NULL;
    int ng = 0;
    float missing = 0.0f;
    double scale = parm->parameter_scale;
    double bias = parm->parameter_bias;

    if (!qq) continue;

    if (rio_vol_ray_field_fl32(st->vol, rayIdx, pn, &data, &ng, &missing)
        != 0) {
      for (i = 0; i < ncells; i++) qq[i] = (short) DELETE_FLAG;
      continue;
    }
    for (i = 0; i < ncells; i++) {
      float v;
      double s;
      if (i >= ng) { qq[i] = (short) DELETE_FLAG; continue; }
      v = data[i];
      if (v == missing || !isfinite(v)) { qq[i] = (short) DELETE_FLAG; continue; }
      s = floor(v * scale + bias + 0.5);
      if (s >  32767.0) s =  32767.0;
      if (s < -32767.0) s = -32767.0;      /* reserve -32768 for bad */
      qq[i] = (short) s;
    }
  }

  dgi->source_ray_num = st->cursor + 1;    /* 1-based for dd_ray_coverage */
  st->cursor++;
  return 1;
}

/* ------------------------------------------------------------------ */
void rio_rewind(struct dd_general_info *dgi)
{
  struct rio_state *st = rio_get_state(dgi);
  if (st) st->cursor = 0;
}

void rio_seek_ray(struct dd_general_info *dgi, int ray_index)
{
  struct rio_state *st = rio_get_state(dgi);
  if (!st) return;
  if (ray_index < 0) ray_index = 0;
  if (ray_index > st->sweep_nrays) ray_index = st->sweep_nrays;
  st->cursor = ray_index;
}

void rio_close(struct dd_general_info *dgi)
{
  struct rio_state *st = rio_get_state(dgi);
  struct rio_write_state *ws = (struct rio_write_state *) dgi->gpptr6;
  if (ws) {
    if (ws->wvol) rio_wvol_free(ws->wvol);
    free(ws);
    dgi->gpptr6 = NULL;
  }
  if (!st) return;
  if (st->vol) rio_vol_close(st->vol);
  free(st);
  dgi->gpptr7 = NULL;
}

/* Drop the cached input volume so the next rio_read_header re-opens the file
 * from disk. The read cache (gpptr7) is keyed on path and only refreshed when
 * the path changes; an in-place edit overwrites the sweep under the same name,
 * so without this a repaint / replot / subsequent edit keeps reading the stale
 * pre-edit volume. The per-DGI state struct is kept; only the open handle and
 * the cached path are cleared. */
void rio_invalidate_read(struct dd_general_info *dgi)
{
  struct rio_state *st = rio_get_state(dgi);
  if (!st) return;
  if (st->vol) rio_vol_close(st->vol);
  st->vol = NULL;
  st->path[0] = '\0';
}

/* ================================================================== *
 *  Write path                                                         *
 * ================================================================== */

int rio_is_managed(struct dd_general_info *dgi)
{
  return dgi->source_fmt == CFRADIAL_FMT;
}

/* DORADE scan_mode -> Radx::SweepMode_t (inverse of rio_dorade_scan_mode). */
static int rio_radx_scan_mode(int dorade_mode)
{
  switch (dorade_mode) {
    case RIO_DM_CAL: return 0;    /* CALIBRATION */
    case RIO_DM_PPI: return 1;    /* SECTOR */
    case RIO_DM_COP: return 2;    /* COPLANE */
    case RIO_DM_RHI: return 3;    /* RHI */
    case RIO_DM_VER: return 4;    /* VERTICAL_POINTING */
    case RIO_DM_SUR: return 8;    /* AZIMUTH_SURVEILLANCE */
    case AIR:        return 8;    /* airborne -> surveillance */
    default:         return 8;
  }
}

int rio_write_ray(struct dd_general_info *dgi)
{
  DDS_PTR dds = dgi->dds;
  struct rio_state *rs = rio_get_state(dgi);
  struct rio_write_state *ws = rio_ensure_write_state(dgi);
  RioRay r;
  int pn, ncells = dds->celv->number_cells;
  int radx_mode;
  long secs;

  /* Begin a fresh volume at the first ray of a sweep. The previous sweep's
   * volume is finalized + freed by rio_write_sweep_end (dd_flush), so a NULL
   * wvol reliably marks the first ray of the next output sweep. */
  if (!ws->wvol) {
    RioRadar radar;
    long ssecs;
    double snano;
    ws->wvol = rio_wvol_new();
    if (!ws->wvol) return 0;

    if (rs) {
      radar = rs->radar;
      radx_mode = rs->radx_sweep_mode;
    } else {
      memset(&radar, 0, sizeof(radar));
      radar.lat_deg = dds->radd->radar_latitude;
      radar.lon_deg = dds->radd->radar_longitude;
      radar.alt_km = dds->radd->radar_altitude;
      str_terminate(radar.instrument, dds->radd->radar_name, 8);
      radx_mode = rio_radx_scan_mode(dds->radd->scan_mode);
    }
    ssecs = (long) dgi->time0;
    snano = (dgi->time0 - (double) ssecs) * 1e9;
    rio_wvol_set_meta(ws->wvol, &radar, radx_mode, ssecs, snano);
  }

  radx_mode = rs ? rs->radx_sweep_mode
                 : rio_radx_scan_mode(dds->radd->scan_mode);

  /* Build the ray geometry from the (possibly edited) DGI. */
  memset(&r, 0, sizeof(r));
  r.az_deg = dds->ryib->azimuth;
  r.el_deg = dds->ryib->elevation;
  secs = (long) dgi->time;
  r.time_secs = secs;
  r.nano_secs = (dgi->time - (double) secs) * 1e9;
  r.n_gates = ncells;
  r.start_range_km = dds->celv->dist_cells[0] / 1000.0;
  r.gate_spacing_km = ncells > 1
      ? (dds->celv->dist_cells[1] - dds->celv->dist_cells[0]) / 1000.0
      : 0.0;
  if (dds->radd->radar_type != GROUND) {
    r.has_georef = 1;
    r.georef_lat = dds->asib->latitude;
    r.georef_lon = dds->asib->longitude;
    r.georef_alt_km = dds->asib->altitude_msl;
    r.heading = dds->asib->heading;
    r.roll = dds->asib->roll;
    r.pitch = dds->asib->pitch;
    r.drift = dds->asib->drift_angle;
    r.rotation = dds->asib->rotation_angle;
    r.tilt = dds->asib->tilt;
    r.ew_velocity = dds->asib->ew_velocity;
    r.ns_velocity = dds->asib->ns_velocity;
    r.vert_velocity = dds->asib->vert_velocity;
  }

  rio_wvol_begin_ray(ws->wvol, &r, dds->swib->sweep_num,
                     dds->swib->fixed_angle, radx_mode);

  for (pn = 0; pn < dgi->num_parms; pn++) {
    struct parameter_d *parm = dds->parm[pn];
    double pscale, scale_cf, offset_cf;
    char name[12], units[12];
    if (!dds->field_present[pn] || !dds->qdat_ptrs[pn]) continue;
    pscale = parm->parameter_scale;
    scale_cf  = pscale != 0.0 ? 1.0 / pscale : 1.0;
    offset_cf = pscale != 0.0 ? -parm->parameter_bias / pscale : 0.0;
    str_terminate(name, parm->parameter_name, 8);
    str_terminate(units, parm->param_units, 8);
    rio_wvol_ray_field_si16(ws->wvol, name, units, ncells, scale_cf, offset_cf,
                            (short) parm->bad_data,
                            (short *) dds->qdat_ptrs[pn]);
  }
  rio_wvol_end_ray(ws->wvol);
  return 1;
}

int rio_write_sweep_end(struct dd_general_info *dgi)
{
  struct rio_write_state *ws = (struct rio_write_state *) dgi->gpptr6;
  struct rio_state *rs = rio_get_state(dgi);
  int dorade, rc;

  if (!ws || !ws->wvol) return -1;
  dorade = (rs && rs->src_fmt == RIO_FMT_DORADE) ? 1 : 0;
  rc = rio_wvol_write(ws->wvol, dgi->directory_name, dorade);
  if (rc != 0)
    fprintf(stderr, "rio_write_sweep_end: write failed for %s\n",
            dgi->directory_name);
  rio_wvol_free(ws->wvol);
  ws->wvol = NULL;
  return rc;
}
