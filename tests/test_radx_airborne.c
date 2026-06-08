/* test_radx_airborne.c
 *
 * Airborne parity: the N42RF-TM P3 tail sweep (primary_axis=axis_y_prime,
 * georeference-driven beam positioning) read two ways through Radx -- DORADE
 * and CfRadial-2 -- must decode to matching geometry and field values, and
 * both must take soloii's AIR scan mode. This is the airborne analog of
 * test_radx_parity, and the counterpart to test_seapol_wedge (which asserts a
 * vertical-axis platform does NOT take AIR mode).
 *
 * The former legacy-byte-parser cross-check was dropped: all DORADE I/O now
 * flows through Radx, and the in-tree byte parser cannot read Radx-written
 * DORADE. Both formats are therefore read through the Radx backend here.
 *
 * Runs with cwd = tests/fixtures so the relative airborne/{dorade,cf2} paths
 * resolve.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_givfld();
int dd_absorb_ray_info();
int dd_absorb_header_info();

/* DORADE field names sometimes substitute '-' for '_'; normalize before
 * matching a field to its twin. */
static void normalize_name(char *s)
{
    g_strstrip(s);
    for (; *s; s++)
        if (*s == '-') *s = '_';
}

#define DORADE_DIR  "airborne/dorade/"
#define DORADE_FILE "swp.1220907125318.N42RF-TM.857.20.0_AIR_v2"
#define CF2_DIR     "airborne/cf2/"
#define CF2_FILE    "cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"

/* Load a sweep into window `win` through the Radx backend, draining all rays
 * so geometry/value state is fully exercised. */
static struct dd_general_info *load_via_radx(int win, const char *dir,
                                             const char *fname, int *nr)
{
    struct dd_general_info *dgi = dd_window_dgi(win, "");
    int n, r;

    g_strlcpy(dgi->directory_name, dir, sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, fname, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;     /* force the rio (Radx) backend */

    n = dd_absorb_header_info(dgi);
    *nr = n;
    for (r = 0; r < n; r++)
        if (dd_absorb_ray_info(dgi) < 1) break;
    return dgi;
}

/* Mean physical value over non-bad gates of field `pn`, last-read ray. */
static double field_mean(struct dd_general_info *dgi, int pn, int *ngood)
{
    int ndx = pn, g1 = 1, n = dgi->dds->celv->number_cells, i, ng = 0;
    float vals[8192], bad;
    double sum = 0.0;
    if (n > 8192) n = 8192;
    dd_givfld(dgi, &ndx, &g1, &n, vals, &bad);
    for (i = 0; i < n; i++)
        if (vals[i] != bad) { sum += vals[i]; ng++; }
    *ngood = ng;
    return ng ? sum / ng : 0.0;
}

static void airborne_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dd, *dc;
    int nd = 0, nc = 0, i;

    (void)main_window;
    (void)user_data;

    dd = load_via_radx(2, DORADE_DIR, DORADE_FILE, &nd);
    dc = load_via_radx(3, CF2_DIR,    CF2_FILE,    &nc);

    /* Airborne tail radar: a non-vertical primary axis means both formats take
     * the georeferenced AIR scan mode (contrast test_seapol_wedge). */
    g_assert_cmpint(dd->dds->radd->radar_type, ==, AIR_TAIL);
    g_assert_cmpint(dc->dds->radd->radar_type, ==, AIR_TAIL);
    g_assert_cmpint(dd->dds->radd->scan_mode, ==, AIR);
    g_assert_cmpint(dc->dds->radd->scan_mode, ==, AIR);

    /* Same structure across the two formats. */
    g_assert_cmpint(nd, >, 100);
    g_assert_cmpint(nd, ==, nc);
    g_assert_cmpint(dd->num_parms, ==, dc->num_parms);
    g_assert_cmpint(dd->dds->celv->number_cells, ==,
                    dc->dds->celv->number_cells);
    g_assert_cmpfloat(fabs(dd->dds->swib->fixed_angle -
                           dc->dds->swib->fixed_angle), <, 0.1);

    /* Last-read ray: same georeferenced beam pointing and platform rotation. */
    g_assert_cmpfloat(fabs(dd->dds->ryib->azimuth - dc->dds->ryib->azimuth),
                      <, 0.1);
    g_assert_cmpfloat(fabs(dd->dds->ryib->elevation - dc->dds->ryib->elevation),
                      <, 0.1);
    g_assert_cmpfloat(fabs(dd->dds->asib->rotation_angle -
                           dc->dds->asib->rotation_angle), <, 0.1);

    /* Same field set, but the two readers may enumerate fields in a different
     * order. Match each DORADE field to its CfRadial twin by name and compare
     * physical means within the int16 step. */
    for (i = 0; i < dd->num_parms; i++) {
        char nd_name[12], nc_name[12];
        int j, jmatch = -1, gd = 0, gc = 0;
        double md, mc;
        g_strlcpy(nd_name, dd->dds->parm[i]->parameter_name, sizeof(nd_name));
        normalize_name(nd_name);
        for (j = 0; j < dc->num_parms; j++) {
            g_strlcpy(nc_name, dc->dds->parm[j]->parameter_name, sizeof(nc_name));
            normalize_name(nc_name);
            if (strcmp(nd_name, nc_name) == 0) { jmatch = j; break; }
        }
        g_assert_cmpint(jmatch, >=, 0);
        md = field_mean(dd, i, &gd);
        mc = field_mean(dc, jmatch, &gc);
        g_assert_cmpint(gd, >, 0);
        g_assert_cmpfloat(fabs(md - mc), <, 1.0);
    }

    g_message("Airborne DORADE-vs-cf2 OK: %d rays, %d fields agree",
              nd, dd->num_parms);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.airborne",
                               airborne_action, NULL);
}
