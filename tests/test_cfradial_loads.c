/* test_cfradial_loads.c
 *
 * Phase 1 acceptance test for the CfRadial read path.
 *
 * The test process runs with cwd = test_data/arthur2015/cfradial (set on the
 * CTest WORKING_DIRECTORY property), so soloiv_activate's default startup
 * discovers the cfrad2.*.nc samples, picks the KLTX radar, and loads +
 * renders the first sweep entirely through the new Radx backend
 * (rio_read_header / rio_read_ray).
 *
 * After the GUI settles, the action callback inspects the window-0 DGI and
 * asserts the CfRadial sweep was decoded into the int16 DGI/DDS pipeline:
 *   - source format flagged CfRadial
 *   - a plausible number of fields, rays, and range gates
 *   - field values unpack (via dd_givfld) to finite, in-range physical values
 * The mere fact that the process did not abort proves the render loop ran
 * over the Radx-filled buffers without crashing.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_givfld();

static void cfradial_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    DDS_PTR dds;
    int ndx, g1, n, nbad, ngood, i;
    float vals[8192], badval;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    dgi = dd_window_dgi(0, "");
    g_assert_nonnull(dgi);

    /* Loaded through the Radx CfRadial backend. */
    g_assert_cmpint(dgi->source_fmt, ==, CFRADIAL_FMT);

    dds = dgi->dds;

    /* KLTX NEXRAD SUR sweep: many rays, hundreds of gates, several fields. */
    g_assert_cmpint(dgi->num_parms, >, 0);
    g_assert_cmpint(dds->swib->num_rays, >, 100);
    g_assert_cmpint(dds->celv->number_cells, >, 100);

    /* Range geometry is monotonically increasing. */
    g_assert_cmpfloat(dds->celv->dist_cells[1], >, dds->celv->dist_cells[0]);

    /* Field 0 should unpack to finite physical values, with at least some
     * non-bad gates in the last-rendered ray. */
    ndx = 0;
    g1 = 1;
    n = dds->celv->number_cells;
    if (n > 8192) n = 8192;
    dd_givfld(dgi, &ndx, &g1, &n, vals, &badval);

    nbad = ngood = 0;
    for (i = 0; i < n; i++) {
        if (vals[i] == badval) {
            nbad++;
        } else {
            g_assert_true(isfinite(vals[i]));
            g_assert_cmpfloat(vals[i], >, -200.0);
            g_assert_cmpfloat(vals[i], <,  200.0);
            ngood++;
        }
    }
    g_assert_cmpint(ngood, >, 0);

    g_message("CfRadial load: %d fields, %d rays, %d gates, %d/%d good gates",
              dgi->num_parms, dds->swib->num_rays, dds->celv->number_cells,
              ngood, n);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.cfradial",
                               cfradial_action, NULL);
}
