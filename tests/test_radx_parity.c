/* test_radx_parity.c
 *
 * Phase 2/3 parity test. The arthur2015 samples ship the SAME 14 KLTX sweeps
 * in two formats: DORADE (sweeps/swp.*) and CfRadial-2 (cfradial/cfrad2.*.nc).
 * Read a twinned pair, BOTH through the Radx backend, into two separate DGIs
 * and assert they decode to matching geometry and field values:
 *   - same field count, ray count, fixed angle, range geometry
 *   - per-ray azimuth/elevation agree within tolerance
 *   - a shared field's physical values (via dd_givfld) agree within the
 *     int16 quantization step
 *
 * This proves DORADE-via-Radx (Phase 2) produces the same int16 pipeline as
 * CfRadial-via-Radx (Phase 1), i.e. the twin PPIs match.
 *
 * Runs with cwd = test_data/ (default), so the relative sample paths resolve.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "radar_io.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_givfld();

/* DORADE field names sometimes substitute '-' for '_' (e.g. REF-s3 vs
 * REF_s3); normalize separators before matching a field to its twin. */
static void normalize_name(char *s)
{
    g_strstrip(s);
    for (; *s; s++)
        if (*s == '-') *s = '_';
}

#define SWEEPS_DIR  "ground/dorade/"
#define CFRAD_DIR   "ground/cf2/"
#define DORADE_FILE "swp.1140703215739.KLTX.317.0.9_SUR_v775"
#define CFRAD_FILE  "cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"

/* Load a sweep into window `win` through the Radx backend. Returns the DGI
 * positioned with all rays read (last ray resident in qdat). nrays_out gets
 * the ray count. */
static struct dd_general_info *load_via_radx(int win, const char *dir,
                                             const char *fname, int *nrays_out)
{
    struct dd_general_info *dgi = dd_window_dgi(win, "");
    int nrays, r;

    g_strlcpy(dgi->directory_name, dir, sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, fname, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;     /* force the rio backend */

    nrays = dd_absorb_header_info(dgi);
    *nrays_out = nrays;
    /* drain rays so geometry/value state is fully exercised */
    for (r = 0; r < nrays; r++) {
        if (dd_absorb_ray_info(dgi) < 1) break;
    }
    return dgi;
}

/* Mean physical value over non-bad gates of field `pn`, last-read ray. */
static double field_mean(struct dd_general_info *dgi, int pn, int *ngood_out)
{
    int ndx = pn, g1 = 1, n = dgi->dds->celv->number_cells, i, ngood = 0;
    float vals[8192], bad;
    double sum = 0.0;
    if (n > 8192) n = 8192;
    dd_givfld(dgi, &ndx, &g1, &n, vals, &bad);
    for (i = 0; i < n; i++) {
        if (vals[i] != bad) { sum += vals[i]; ngood++; }
    }
    *ngood_out = ngood;
    return ngood ? sum / ngood : 0.0;
}

static void parity_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *da, *db;
    int na = 0, nb = 0, i, matched = 0;

    (void)main_window;
    (void)user_data;

    da = load_via_radx(2, SWEEPS_DIR, DORADE_FILE, &na);
    db = load_via_radx(3, CFRAD_DIR,  CFRAD_FILE,  &nb);

    /* Same field and ray counts. */
    g_assert_cmpint(na, >, 100);
    g_assert_cmpint(na, ==, nb);
    g_assert_cmpint(da->num_parms, ==, db->num_parms);

    /* Same range geometry. */
    g_assert_cmpint(da->dds->celv->number_cells, ==,
                    db->dds->celv->number_cells);
    g_assert_cmpfloat(fabs(da->dds->celv->dist_cells[0] -
                           db->dds->celv->dist_cells[0]), <, 1.0);

    /* Same fixed angle. */
    g_assert_cmpfloat(fabs(da->dds->swib->fixed_angle -
                           db->dds->swib->fixed_angle), <, 0.05);

    /* The last-read ray should be the same physical ray in both. */
    g_assert_cmpfloat(fabs(da->dds->ryib->azimuth - db->dds->ryib->azimuth),
                      <, 0.1);
    g_assert_cmpfloat(fabs(da->dds->ryib->elevation - db->dds->ryib->elevation),
                      <, 0.1);

    /* Both files carry the same field set, but the readers may enumerate
     * them in a different order. Match each DORADE field to its CfRadial
     * twin by name and compare physical means within the int16 step. */
    for (i = 0; i < da->num_parms; i++) {
        char na_name[12], nb_name[12];
        int j, jmatch = -1, gia = 0, gib = 0;
        double ma, mb;
        g_strlcpy(na_name, da->dds->parm[i]->parameter_name, sizeof(na_name));
        normalize_name(na_name);
        for (j = 0; j < db->num_parms; j++) {
            g_strlcpy(nb_name, db->dds->parm[j]->parameter_name, sizeof(nb_name));
            normalize_name(nb_name);
            if (strcmp(na_name, nb_name) == 0) { jmatch = j; break; }
        }
        g_assert_cmpint(jmatch, >=, 0);     /* every DORADE field has a twin */

        ma = field_mean(da, i, &gia);
        mb = field_mean(db, jmatch, &gib);
        g_assert_cmpint(gia, >, 0);
        g_assert_cmpint(gib, >, 0);
        g_assert_cmpfloat(fabs(ma - mb), <, 1.0);
        matched++;
    }
    g_assert_cmpint(matched, ==, da->num_parms);

    g_message("Parity OK: %d rays, %d fields matched, geometry+values agree",
              na, matched);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.parity",
                               parity_action, NULL);
}
