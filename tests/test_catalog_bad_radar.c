/* test_catalog_bad_radar.c
 *
 * The directory catalog (translate/dd_files.c) indexes a per-radar info array,
 * ddir->rni[], with a radar number supplied by the caller. That number comes
 * from frame state which a directory rescan can invalidate: the catalog is
 * torn down and rebuilt on every rescan, and radars are renumbered by the
 * order their files happen to appear. An edit triggers a rescan, so a stale
 * radar number is reachable in normal use.
 *
 * rni[] is zero-filled, so an unchecked index is a NULL dereference rather
 * than merely a bad read. Three entry points got this wrong:
 *
 *   ddfn_search       indexed rni[] with no bounds check at all, then
 *                     dereferenced rni->prev_req_type / rni->h_node.
 *   ddfnp_list        same shape, then rni->h_node->right.
 *   ddfnp_list_entry  tested radar_num in the same expression that had
 *                     already dereferenced rni->num_sweeps -- the check came
 *                     after the access it was meant to guard.
 *
 * dd_window_dgi had the matching problem on window_dgi[MAX_SENSORS].
 *
 * This test calls each with out-of-range and stale radar/window numbers and
 * requires a clean failure return. Under ASan the pre-fix code reports a
 * global-buffer-overflow or crashes on the NULL rni.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_files.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int mddir_file_list_v3();
int mddir_num_radars_v3();
int ddfnp_list();
double ddfnp_list_entry();
struct dd_file_name_v3 *ddfn_search();

#define DIR_NUM 5

static void bad_radar_action(GtkWidget *main_window, gpointer user_data)
{
    char info[256], fname[256];
    int nradars, ver = 0;
    int bad[] = { -1, 99, MAX_SENSORS, MAX_SENSORS + 4 };
    guint i;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    /* Build a real catalog so the guards are exercised against live state
     * rather than an empty directory. */
    g_assert_cmpint(mddir_file_list_v3(DIR_NUM, "./"), >, 0);
    nradars = mddir_num_radars_v3(DIR_NUM);
    g_assert_cmpint(nradars, >, 0);

    /* A radar number one past the end is exactly what a rescan that dropped a
     * radar leaves behind in frame state. */
    for (i = 0; i < G_N_ELEMENTS(bad); i++) {
        int rn = bad[i];

        info[0] = fname[0] = '\0';
        g_assert_cmpfloat(ddfnp_list_entry(DIR_NUM, rn, 0, &ver, info, fname),
                          ==, 0.0);
        g_assert_cmpint(ddfnp_list(DIR_NUM, rn, 0), <, 0);
        g_assert_null(ddfn_search(DIR_NUM, rn, 0.0, TIME_NEAREST, 99999));
    }

    /* Valid radar, out-of-range entry number: also a clean 0. */
    g_assert_cmpfloat(ddfnp_list_entry(DIR_NUM, 0, -1, &ver, info, fname),
                      ==, 0.0);
    g_assert_cmpfloat(ddfnp_list_entry(DIR_NUM, 0, 1000000, &ver, info, fname),
                      ==, 0.0);

    /* dd_window_dgi must refuse an index past window_dgi[MAX_SENSORS] instead
     * of reading off the end of the array. */
    g_assert_null(dd_window_dgi(-1, ""));
    g_assert_null(dd_window_dgi(MAX_SENSORS, ""));
    g_assert_null(dd_window_dgi(MAX_SENSORS + 2, ""));
    g_assert_nonnull(dd_window_dgi(MAX_SENSORS - 1, ""));   /* still valid */

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.badradar",
                               bad_radar_action, NULL);
}
