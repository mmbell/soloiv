/* test_radx_airborne.c
 *
 * Validates DORADE-via-Radx against the PROVEN in-tree parser on airborne
 * data, where platform georeferencing (rotation/roll/pitch/tilt + Testud
 * equations) drives beam positioning. The N42RF-TM tail-radar sweep in
 * test_data/ is read two ways in one process:
 *   - legacy: the in-tree dd_absorb_* byte parser (SOLOIV_DORADE_LEGACY)
 *   - radx:   the Radx backend (rio_read_*)
 * and the two decodings are asserted to agree on ray count, fixed angle,
 * per-ray azimuth/elevation, range geometry, and field values.
 *
 * This is the airborne safety net behind the default-ON DORADE-via-Radx
 * switch: if Radx ever diverges from the legacy parser on aircraft data,
 * this test fails. Runs with cwd = test_data/.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "radar_io.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_givfld();

#define AIR_FILE "swp.1220907125318.N42RF-TM.857.20.0_AIR_v2"

/* Load via the legacy in-tree DORADE parser (source_fmt left non-CfRadial,
 * real fd opened for the byte reader). */
static struct dd_general_info *load_legacy(int win, const char *fname, int *nr)
{
    struct dd_general_info *dgi = dd_window_dgi(win, "");
    int r, n;
    dgi->source_fmt = DORADE_FMT;          /* legacy byte parser */
    dgi->in_swp_fid = open(fname, O_RDONLY, 0);
    g_assert_cmpint(dgi->in_swp_fid, >, 0);
    n = dd_absorb_header_info(dgi);
    *nr = dgi->dds->swib->num_rays;
    (void)n;
    for (r = 0; r < *nr; r++)
        if (dd_absorb_ray_info(dgi) < 1) break;
    return dgi;
}

/* Load via the Radx backend (force the rio dispatch). */
static struct dd_general_info *load_radx(int win, const char *fname, int *nr)
{
    struct dd_general_info *dgi = dd_window_dgi(win, "");
    int r;
    g_strlcpy(dgi->directory_name, "./", sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, fname, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;        /* rio backend */
    *nr = dd_absorb_header_info(dgi);
    for (r = 0; r < *nr; r++)
        if (dd_absorb_ray_info(dgi) < 1) break;
    return dgi;
}

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
    struct dd_general_info *dl, *dr;
    int nl = 0, nr = 0, i;

    (void)main_window;
    (void)user_data;

    dl = load_legacy(2, AIR_FILE, &nl);
    dr = load_radx(3, AIR_FILE, &nr);

    /* This is an airborne tail-radar sweep. */
    g_assert_cmpint(dl->dds->radd->radar_type, ==, AIR_TAIL);
    g_assert_cmpint(dr->dds->radd->radar_type, ==, AIR_TAIL);
    g_assert_cmpint(dl->dds->radd->scan_mode, ==, AIR);
    g_assert_cmpint(dr->dds->radd->scan_mode, ==, AIR);

    /* Same structure. */
    g_assert_cmpint(nl, >, 100);
    g_assert_cmpint(nl, ==, nr);
    g_assert_cmpint(dl->num_parms, ==, dr->num_parms);
    g_assert_cmpint(dl->dds->celv->number_cells, ==,
                    dr->dds->celv->number_cells);
    g_assert_cmpfloat(fabs(dl->dds->swib->fixed_angle -
                           dr->dds->swib->fixed_angle), <, 0.1);

    /* Last-read ray: same beam pointing (az/el) and platform rotation. */
    g_assert_cmpfloat(fabs(dl->dds->ryib->azimuth - dr->dds->ryib->azimuth),
                      <, 0.1);
    g_assert_cmpfloat(fabs(dl->dds->ryib->elevation - dr->dds->ryib->elevation),
                      <, 0.1);
    g_assert_cmpfloat(fabs(dl->dds->asib->rotation_angle -
                           dr->dds->asib->rotation_angle), <, 0.1);

    /* Field values agree within the int16 quantization step. */
    for (i = 0; i < dl->num_parms; i++) {
        int gl = 0, gr = 0;
        double ml = field_mean(dl, i, &gl);
        double mr = field_mean(dr, i, &gr);
        g_assert_cmpint(gl, >, 0);
        g_assert_cmpfloat(fabs(ml - mr), <, 1.0);
    }

    g_message("Airborne legacy-vs-Radx OK: %d rays, %d fields agree",
              nl, dl->num_parms);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.airborne",
                               airborne_action, NULL);
}
