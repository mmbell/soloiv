/* test_cfradial_multisweep.c
 *
 * Issue #2: a CfRadial *volume* file holds N sweeps, but the reader used to
 * hardcode sweep 0 -- the other sweeps were invisible. The reader now honors
 * dgi->rio_req_sweep so sweep navigation can step through every sweep of a
 * loaded volume (reusing the cached full read -- no reopen per sweep).
 *
 * This drives the reader directly: it loads a 14-sweep SEAPOL volume and, for
 * each requested sweep index, verifies the loaded sweep is the one asked for
 * (distinct, strictly increasing fixed angles), that re-reading a sweep is
 * stable, and that an out-of-range request clamps instead of misbehaving.
 *
 * cwd = tests/fixtures/volume.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_general_info.h"
#include "radar_io.h"
#include "test_app_runner.h"

#define VOL_FILE "cfrad.20240903_120049.313_to_20240903_120613.852_SEAPOL_SUR.nc"

struct dd_general_info *dd_window_dgi();

/* The 14 fixed angles in the volume (from ncdump), low to high. */
static const double expected_angle[] = {
  1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 9.0, 11.0, 13.0, 15.0, 17.0, 45.0
};
#define NSWEEPS 14

static float load_sweep(struct dd_general_info *dgi, int idx)
{
    dgi->rio_req_sweep = idx;
    g_assert_cmpint(dd_absorb_header_info(dgi), >, 0);
    return dgi->dds->swib->fixed_angle;
}

static void multisweep_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    int i;
    float prev = -1e9f;

    (void)main_window;
    (void)user_data;

    dgi = dd_window_dgi(4, "");
    g_strlcpy(dgi->directory_name, "./", sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, VOL_FILE, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;
    dgi->rio_req_sweep = 0;

    g_assert_cmpint(dd_absorb_header_info(dgi), >, 0);
    g_assert_cmpint(rio_volume_nsweeps(dgi), ==, NSWEEPS);

    /* Walk every sweep: each load returns the requested sweep, with the
     * expected (strictly increasing) fixed angle. */
    for (i = 0; i < NSWEEPS; i++) {
        float ang = load_sweep(dgi, i);
        g_assert_cmpint(rio_current_sweep(dgi), ==, i);
        g_assert_cmpfloat(fabsf(ang - (float)expected_angle[i]), <, 0.1f);
        g_assert_cmpfloat(ang, >, prev);     /* strictly increasing */
        prev = ang;
    }
    g_message("[multisweep] stepped through all %d sweeps", NSWEEPS);

    /* Re-reading a sweep is stable (cached volume, no reopen). */
    g_assert_cmpfloat(fabsf(load_sweep(dgi, 5) - (float)expected_angle[5]),
                      <, 0.1f);
    g_assert_cmpfloat(fabsf(load_sweep(dgi, 5) - (float)expected_angle[5]),
                      <, 0.1f);

    /* Out-of-range requests clamp to the last sweep, never out of bounds. */
    load_sweep(dgi, 999);
    g_assert_cmpint(rio_current_sweep(dgi), ==, NSWEEPS - 1);
    load_sweep(dgi, -3);
    g_assert_cmpint(rio_current_sweep(dgi), ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.multisweep",
                               multisweep_action, NULL);
}
