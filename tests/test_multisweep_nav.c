/* test_multisweep_nav.c
 *
 * Issue #2: end-to-end sweep navigation across a multi-sweep CfRadial volume.
 *
 * With cwd = the volume fixture dir, soloiv_activate auto-loads the volume's
 * first sweep into window 0. This test then drives solo_nab_next_file (the
 * same call the next/prev-sweep controls use) and verifies that FORWARD steps
 * through all 14 sweeps within the single volume file (strictly increasing
 * fixed angle), that stepping past the last sweep reports the list exhausted
 * (only one file present), and that BACKWARD steps back down through them.
 *
 * cwd = tests/fixtures/volume.
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_files.h"          /* LATEST_VERSION */
#include "dd_general_info.h"
#include "radar_io.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int solo_nab_next_file();

#define NSWEEPS 14

static void nav_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    float prev;
    int i, ok;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    dgi = dd_window_dgi(0, "");
    g_assert_nonnull(dgi);
    g_assert_cmpint(dgi->source_fmt, ==, CFRADIAL_FMT);
    g_assert_cmpint(rio_volume_nsweeps(dgi), ==, NSWEEPS);
    g_assert_cmpint(rio_current_sweep(dgi), ==, 0);
    prev = dgi->dds->swib->fixed_angle;

    /* FORWARD steps through every remaining sweep of the same volume. */
    for (i = 1; i < NSWEEPS; i++) {
        ok = solo_nab_next_file(0, FORWARD, LATEST_VERSION, 1, NO);
        g_assert_true(ok);
        g_assert_cmpint(rio_current_sweep(dgi), ==, i);
        g_assert_cmpfloat(dgi->dds->swib->fixed_angle, >, prev);
        prev = dgi->dds->swib->fixed_angle;
    }

    /* Past the last sweep, with only one file present, the list is exhausted. */
    ok = solo_nab_next_file(0, FORWARD, LATEST_VERSION, 1, NO);
    g_assert_false(ok);
    g_assert_cmpint(rio_current_sweep(dgi), ==, NSWEEPS - 1);

    /* BACKWARD steps back down through the sweeps. */
    for (i = NSWEEPS - 2; i >= 0; i--) {
        ok = solo_nab_next_file(0, BACKWARD, LATEST_VERSION, 1, NO);
        g_assert_true(ok);
        g_assert_cmpint(rio_current_sweep(dgi), ==, i);
    }

    g_message("[multisweep_nav] forward+backward stepped all %d sweeps",
              NSWEEPS);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.multisweepnav",
                               nav_action, NULL);
}
