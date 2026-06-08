/* radx_shim.cc — extern "C" bridge from soloiv to the LROSE libRadx library.
 *
 * All sweep I/O (DORADE + CfRadial) is routed through Radx's robust readers.
 * This file is compiled as C++17 against the prebuilt libRadx.a / libNcxx.a
 * and exposes a flat, struct-neutral C API (declared in radar_io.h). It knows
 * nothing about soloiv's DGI/DDS structures — the pure-C layer in radar_io.c
 * consumes these accessors and fills the in-memory structs. No C++ exception
 * is ever allowed to cross the extern "C" boundary.
 */

#include <Radx/RadxFile.hh>
#include <Radx/RadxVol.hh>
#include <Radx/RadxSweep.hh>
#include <Radx/RadxRay.hh>
#include <Radx/RadxField.hh>
#include <Radx/RadxGeoref.hh>
#include <Radx/Radx.hh>

#include <ctime>

#include <string>
#include <vector>
#include <cstring>

#include "radar_io.h"

/* Opaque volume handle: the loaded RadxVol plus cached per-field metadata
 * (captured BEFORE any convertToFl32() so native scale/offset/type survive). */
namespace {
struct RioVol {
  RadxVol vol;
  std::vector<std::string> fieldNames;
  std::vector<RioField>    fieldMeta;   // parallel to fieldNames
};

/* Copy a std::string into a fixed char buffer, NUL-terminated. */
void copy_str(char *dst, size_t cap, const std::string &s)
{
  if (cap == 0) return;
  size_t n = s.size() < cap - 1 ? s.size() : cap - 1;
  memcpy(dst, s.data(), n);
  dst[n] = '\0';
}
} // namespace

extern "C" {

/* ---- Phase-0 smoke probe (kept for test_radx_smoke) ---------------------- */
int rio_radx_selftest(const char *path)
{
  if (path == nullptr) return -1;
  try {
    RadxFile f;
    RadxVol vol;
    if (f.readFromPath(std::string(path), vol) != 0) return -1;
    vol.loadSweepInfoFromRays();
    return static_cast<int>(vol.getNSweeps());
  } catch (...) {
    return -2;
  }
}

/* ---- Volume open / close ------------------------------------------------- */

RioVolH rio_vol_open(const char *path)
{
  if (path == nullptr) return nullptr;
  try {
    RioVol *h = new RioVol();
    RadxFile f;
    if (f.readFromPath(std::string(path), h->vol) != 0) {
      delete h;
      return nullptr;
    }
    h->vol.loadSweepInfoFromRays();

    /* Capture per-field metadata once, from the first ray that carries each
     * unique field, before anyone calls convertToFl32(). */
    h->fieldNames = h->vol.getUniqueFieldNameList();
    const std::vector<RadxRay *> &rays = h->vol.getRays();
    for (const std::string &name : h->fieldNames) {
      RioField fm;
      memset(&fm, 0, sizeof(fm));
      copy_str(fm.name, sizeof(fm.name), name);
      fm.data_type = -1;
      for (RadxRay *r : rays) {
        RadxField *fld = r->getField(name);
        if (!fld) continue;
        copy_str(fm.units, sizeof(fm.units), fld->getUnits());
        fm.data_type    = static_cast<int>(fld->getDataType());
        fm.scale        = fld->getScale();
        fm.offset       = fld->getOffset();
        fm.missing_fl32 = fld->getMissingFl32();
        break;
      }
      h->fieldMeta.push_back(fm);
    }
    return static_cast<RioVolH>(h);
  } catch (...) {
    return nullptr;
  }
}

void rio_vol_close(RioVolH vh)
{
  if (!vh) return;
  try {
    delete static_cast<RioVol *>(vh);
  } catch (...) {
  }
}

int rio_vol_nsweeps(RioVolH vh)
{
  if (!vh) return -1;
  try { return static_cast<int>(static_cast<RioVol *>(vh)->vol.getNSweeps()); }
  catch (...) { return -1; }
}

int rio_vol_nrays(RioVolH vh)
{
  if (!vh) return -1;
  try { return static_cast<int>(static_cast<RioVol *>(vh)->vol.getNRays()); }
  catch (...) { return -1; }
}

/* ---- Sweep info ---------------------------------------------------------- */

int rio_vol_sweep(RioVolH vh, int sw, RioSweep *out)
{
  if (!vh || !out) return -1;
  try {
    RioVol *h = static_cast<RioVol *>(vh);
    const std::vector<RadxSweep *> &sweeps = h->vol.getSweeps();
    if (sw < 0 || sw >= (int)sweeps.size()) return -1;
    const RadxSweep *s = sweeps[sw];
    out->sweep_num   = s->getSweepNumber();
    out->sweep_mode  = static_cast<int>(s->getSweepMode());
    out->fixed_angle = s->getFixedAngleDeg();
    out->start_ray   = static_cast<int>(s->getStartRayIndex());
    out->end_ray     = static_cast<int>(s->getEndRayIndex());
    out->nrays       = static_cast<int>(s->getNRays());
    return 0;
  } catch (...) { return -1; }
}

/* ---- Radar / platform / volume info -------------------------------------- */

int rio_vol_radar(RioVolH vh, RioRadar *out)
{
  if (!vh || !out) return -1;
  try {
    RioVol *h = static_cast<RioVol *>(vh);
    RadxVol &v = h->vol;
    memset(out, 0, sizeof(*out));
    out->lat_deg = v.getLatitudeDeg();
    out->lon_deg = v.getLongitudeDeg();
    out->alt_km  = v.getAltitudeKm();
    copy_str(out->instrument, sizeof(out->instrument), v.getInstrumentName());
    copy_str(out->scan_name,  sizeof(out->scan_name),  v.getScanName());
    out->platform_type   = static_cast<int>(v.getPlatformType());
    out->primary_axis    = static_cast<int>(v.getPrimaryAxis());
    out->start_time_secs = static_cast<long>(v.getStartTimeSecs());
    /* Nyquist / unambiguous range are per-ray in Radx; take the first ray's
     * as a volume-level representative (matches DORADE's single radd value). */
    const std::vector<RadxRay *> &rays = v.getRays();
    if (!rays.empty()) {
      out->nyquist_mps      = rays[0]->getNyquistMps();
      out->unambig_range_km = rays[0]->getUnambigRangeKm();
    }
    return 0;
  } catch (...) { return -1; }
}

/* ---- Ray info ------------------------------------------------------------ */

int rio_vol_ray(RioVolH vh, int rayIdx, RioRay *out)
{
  if (!vh || !out) return -1;
  try {
    RioVol *h = static_cast<RioVol *>(vh);
    const std::vector<RadxRay *> &rays = h->vol.getRays();
    if (rayIdx < 0 || rayIdx >= (int)rays.size()) return -1;
    const RadxRay *r = rays[rayIdx];
    memset(out, 0, sizeof(*out));
    out->az_deg          = r->getAzimuthDeg();
    out->el_deg          = r->getElevationDeg();
    out->time_secs       = static_cast<long>(r->getTimeSecs());
    out->nano_secs       = r->getNanoSecs();
    out->n_gates         = static_cast<int>(r->getNGates());
    out->start_range_km  = r->getStartRangeKm();
    out->gate_spacing_km = r->getGateSpacingKm();
    out->nyquist_mps     = r->getNyquistMps();
    out->unambig_range_km= r->getUnambigRangeKm();
    const RadxGeoref *g  = r->getGeoreference();
    if (g) {
      out->has_georef   = 1;
      out->georef_lat   = g->getLatitude();
      out->georef_lon   = g->getLongitude();
      out->georef_alt_km= g->getAltitudeKmMsl();
      out->heading      = g->getHeading();
      out->roll         = g->getRoll();
      out->pitch        = g->getPitch();
      out->drift        = g->getDrift();
      out->rotation     = g->getRotation();
      out->tilt         = g->getTilt();
      out->ew_velocity  = g->getEwVelocity();
      out->ns_velocity  = g->getNsVelocity();
      out->vert_velocity= g->getVertVelocity();
    }
    return 0;
  } catch (...) { return -1; }
}

/* ---- Fields -------------------------------------------------------------- */

int rio_vol_nfields(RioVolH vh)
{
  if (!vh) return -1;
  try { return (int)static_cast<RioVol *>(vh)->fieldNames.size(); }
  catch (...) { return -1; }
}

int rio_vol_field(RioVolH vh, int f, RioField *out)
{
  if (!vh || !out) return -1;
  try {
    RioVol *h = static_cast<RioVol *>(vh);
    if (f < 0 || f >= (int)h->fieldMeta.size()) return -1;
    *out = h->fieldMeta[f];
    return 0;
  } catch (...) { return -1; }
}

/* Fetch one ray's data for field index `f` as fl32. Returns 0 and fills
 * *data_out/*ngates_out/*missing_out on success, -1 if the ray lacks the
 * field (caller fills all-bad). The pointer remains valid until the next
 * call that mutates the same ray's field or rio_vol_close(). */
int rio_vol_ray_field_fl32(RioVolH vh, int rayIdx, int f,
                           const float **data_out, int *ngates_out,
                           float *missing_out)
{
  if (!vh || !data_out || !ngates_out || !missing_out) return -1;
  try {
    RioVol *h = static_cast<RioVol *>(vh);
    const std::vector<RadxRay *> &rays = h->vol.getRays();
    if (rayIdx < 0 || rayIdx >= (int)rays.size()) return -1;
    if (f < 0 || f >= (int)h->fieldNames.size()) return -1;
    RadxField *fld = rays[rayIdx]->getField(h->fieldNames[f]);
    if (!fld) return -1;
    fld->convertToFl32();                 /* required before getDataFl32() */
    *data_out    = fld->getDataFl32();
    *ngates_out  = static_cast<int>(fld->getNPoints());
    *missing_out = fld->getMissingFl32();
    return 0;
  } catch (...) { return -1; }
}

/* ====================================================================== *
 *  Write path: assemble a RadxVol from edited DGI/DDS and write it out.   *
 * ====================================================================== */

namespace {
struct RioWVol {
  RadxVol  vol;
  RadxRay *cur = nullptr;   // ray currently being built
};
} // namespace

RioWVolH rio_wvol_new(void)
{
  try { return static_cast<RioWVolH>(new RioWVol()); }
  catch (...) { return nullptr; }
}

void rio_wvol_set_meta(RioWVolH wvh, const RioRadar *radar, int radx_scan_mode,
                       long start_secs, double start_nano)
{
  if (!wvh || !radar) return;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    h->vol.setInstrumentName(radar->instrument);
    h->vol.setScanName(radar->scan_name);
    h->vol.setLatitudeDeg(radar->lat_deg);
    h->vol.setLongitudeDeg(radar->lon_deg);
    h->vol.setAltitudeKm(radar->alt_km);
    h->vol.setPlatformType(
        static_cast<Radx::PlatformType_t>(radar->platform_type));
    /* The primary axis of rotation must round-trip: the reader keys airborne
     * (rotation/tilt) vs surveillance (azimuth) beam positioning off it, not
     * off the platform label. Omitting it lets it default to axis_z (vertical),
     * which silently demotes an airborne tail sweep to RHI on reopen and
     * collapses the display into a wedge. */
    h->vol.setPrimaryAxis(
        static_cast<Radx::PrimaryAxis_t>(radar->primary_axis));
    h->vol.setStartTime(static_cast<time_t>(start_secs), start_nano);
    (void) radx_scan_mode;
  } catch (...) {}
}

void rio_wvol_begin_ray(RioWVolH wvh, const RioRay *r, int sweep_num,
                        double fixed_angle, int radx_scan_mode)
{
  if (!wvh || !r) return;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    RadxRay *ray = new RadxRay();
    ray->setTime(static_cast<time_t>(r->time_secs), r->nano_secs);
    ray->setAzimuthDeg(r->az_deg);
    ray->setElevationDeg(r->el_deg);
    ray->setFixedAngleDeg(fixed_angle);
    ray->setSweepNumber(sweep_num);
    ray->setSweepMode(static_cast<Radx::SweepMode_t>(radx_scan_mode));
    ray->setRangeGeom(r->start_range_km, r->gate_spacing_km);
    ray->setNGates(r->n_gates);
    if (r->has_georef) {
      RadxGeoref g;
      g.setLatitude(r->georef_lat);
      g.setLongitude(r->georef_lon);
      g.setAltitudeKmMsl(r->georef_alt_km);
      g.setHeading(r->heading);
      g.setRoll(r->roll);
      g.setPitch(r->pitch);
      g.setDrift(r->drift);
      g.setRotation(r->rotation);
      g.setTilt(r->tilt);
      g.setEwVelocity(r->ew_velocity);
      g.setNsVelocity(r->ns_velocity);
      g.setVertVelocity(r->vert_velocity);
      ray->setGeoref(g);
    }
    h->cur = ray;
  } catch (...) {}
}

void rio_wvol_ray_field_si16(RioWVolH wvh, const char *name, const char *units,
                             int ngates, double scale, double offset,
                             short missing, const short *data)
{
  if (!wvh || !name || !data) return;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    if (!h->cur) return;
    h->cur->addField(std::string(name), units ? std::string(units) : "",
                     static_cast<size_t>(ngates),
                     static_cast<Radx::si16>(missing),
                     reinterpret_cast<const Radx::si16 *>(data),
                     scale, offset, true /* isLocal: Radx copies */);
  } catch (...) {}
}

void rio_wvol_end_ray(RioWVolH wvh)
{
  if (!wvh) return;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    if (h->cur) { h->vol.addRay(h->cur); h->cur = nullptr; }
  } catch (...) {}
}

int rio_wvol_write(RioWVolH wvh, const char *dir, int dorade)
{
  if (!wvh || !dir) return -1;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    h->vol.loadSweepInfoFromRays();
    h->vol.loadVolumeInfoFromRays();
    RadxFile f;
    f.setFileFormat(dorade ? RadxFile::FILE_FORMAT_DORADE
                           : RadxFile::FILE_FORMAT_CFRADIAL2);
    return f.writeToDir(h->vol, std::string(dir), false, false);
  } catch (...) { return -1; }
}

void rio_wvol_free(RioWVolH wvh)
{
  if (!wvh) return;
  try {
    RioWVol *h = static_cast<RioWVol *>(wvh);
    if (h->cur) delete h->cur;
    delete h;
  } catch (...) {}
}

} /* extern "C" */
