/* test_cfradial_filename.c
 *
 * Regression for CfRadial sweep-file cataloging. The directory catalog groups
 * files by the radar name cracked from the filename; if cf1 and cf2 of the
 * same scan crack to different names, next/prev file navigation reports
 * "Sweep list exhausted". SEAPOL cf2 names embed the scan
 * (..._SEAPOL_PICCOLO_LONG_SUR.nc), which the old "second-to-last token"
 * heuristic mis-read as radar "LONG"/"VOL1". The instrument is the token right
 * after the time spec, and must crack consistently across cf1/cf2.
 *
 * Pure function test — no GUI, no data files.
 */

#include <glib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_files.h"
#include "radar_io.h"

static void expect_radar(const char *fname, const char *want)
{
    struct dd_file_name_v3 ddfn;
    memset(&ddfn, 0, sizeof(ddfn));
    g_assert_cmpint(rio_craack_cfradial(fname, &ddfn), >=, 1);
    g_assert_cmpstr(ddfn.radar_name, ==, want);
}

static void test_radar_name_from_filename(void)
{
    /* SEAPOL cf1 and cf2 of the same scan -> both "SEAPOL" (the bug). */
    expect_radar(
        "cfrad.20240903_120008.372_to_20240903_120037.547_SEAPOL_SUR.nc",
        "SEAPOL");
    expect_radar(
        "cfrad2.20240903_120008.372_to_20240903_120037.547_SEAPOL_PICCOLO_LONG_SUR.nc",
        "SEAPOL");
    expect_radar(
        "cfrad2.20240903_120049.313_to_20240903_120613.852_SEAPOL_PICCOLO_VOL1_SUR.nc",
        "SEAPOL");

    /* Ground + airborne names are unaffected by the fix. */
    expect_radar(
        "cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc",
        "KLTX");
    expect_radar(
        "cfrad.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc",
        "N42RF-TM");
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/cfradial/radar_name_from_filename",
                    test_radar_name_from_filename);
    return g_test_run();
}
