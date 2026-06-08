/* test_seapol_wedge.c
 *
 * Regression test for the SEAPOL "thin NE wedge" bug.
 *
 * SEAPOL is a shipborne radar, but the Sigmet/IRIS converter mislabels its
 * CfRadial files platform_type=aircraft. The fix keys beam positioning on the
 * primary axis of rotation, not the platform label: a vertical axis (axis_z)
 * is an ordinary azimuth-surveillance PPI plotted from the earth-relative
 * azimuth, even when the platform claims to be an aircraft. Before the fix the
 * reader forced soloii's AIR scan mode, so dd_rotation_angle positioned every
 * beam from rotation/tilt/heading; with no rotation/tilt and heading ~-88 deg
 * the whole sweep collapsed into a ~88 deg wedge.
 *
 * Runs with cwd = tests/fixtures/ship/cf2 so startup loads the SEAPOL sweep
 * through the Radx backend.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_absorb_ray_info();
int dd_absorb_header_info();

static void seapol_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    DDS_PTR dds;
    int nr, r, quad[4] = {0, 0, 0, 0};
    float az, azmin = 720.0f, azmax = -720.0f;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    dgi = dd_window_dgi(0, "");
    g_assert_nonnull(dgi);
    g_assert_cmpint(dgi->source_fmt, ==, CFRADIAL_FMT);

    dds = dgi->dds;

    /* The core regression: a vertical-axis surveillance sweep must decode as
     * SUR (plotted by azimuth), NOT airborne AIR mode, and the mislabeled
     * airborne radar_type must be demoted to GROUND so the georef-projection
     * display paths are bypassed. */
    g_assert_cmpint(dds->radd->scan_mode, ==, SUR);
    g_assert_cmpint(dds->radd->scan_mode, !=, AIR);
    g_assert_cmpint(dds->radd->radar_type, ==, GROUND);

    /* Read every ray; the azimuths must sweep the full circle, not collapse
     * into a wedge. Require coverage in all four 90-degree quadrants and a
     * span over 270 degrees. (The bug pinned every ray near ~88 deg.) */
    nr = dd_absorb_header_info(dgi);
    g_assert_cmpint(nr, >, 100);
    for (r = 0; r < nr; r++) {
        if (dd_absorb_ray_info(dgi) < 1) break;
        az = dgi->dds->ryib->azimuth;
        while (az < 0.0f) az += 360.0f;
        while (az >= 360.0f) az -= 360.0f;
        if (az < azmin) azmin = az;
        if (az > azmax) azmax = az;
        quad[(int)(az / 90.0f) & 3] = 1;
    }
    g_assert_cmpint(quad[0] + quad[1] + quad[2] + quad[3], ==, 4);
    g_assert_cmpfloat(azmax - azmin, >, 270.0);

    g_message("SEAPOL PPI: scan_mode=%d radar_type=%d az[%.1f..%.1f] quads=4",
              dds->radd->scan_mode, dds->radd->radar_type, azmin, azmax);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.seapol",
                               seapol_action, NULL);
}
