/* test_radx_smoke.c
 *
 * Phase 0 linkage proof for the Radx I/O shim. Confirms that:
 *   1. soloiv links against libRadx.a (the call resolves at link time).
 *   2. Radx's auto-detecting reader opens both sample formats — the
 *      CfRadial-2 netCDF twin and its DORADE twin — and reports a sweep count.
 *
 * Working directory is test_data/ (set by CTest), matching how the app finds
 * the sample sweeps. Each twinned file is one sweep.
 */

#include <glib.h>

#include "radar_io.h"

#define CFRAD_SAMPLE \
    "arthur2015/cfradial/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"
#define DORADE_SAMPLE \
    "arthur2015/sweeps/swp.1140703215739.KLTX.317.0.9_SUR_v775"

static void test_radx_reads_cfradial(void)
{
    int nsweeps = rio_radx_selftest(CFRAD_SAMPLE);
    g_assert_cmpint(nsweeps, ==, 1);
}

static void test_radx_reads_dorade(void)
{
    int nsweeps = rio_radx_selftest(DORADE_SAMPLE);
    g_assert_cmpint(nsweeps, ==, 1);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/radx/reads_cfradial", test_radx_reads_cfradial);
    g_test_add_func("/radx/reads_dorade",   test_radx_reads_dorade);

    return g_test_run();
}
