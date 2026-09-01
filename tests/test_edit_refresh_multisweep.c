/* test_edit_refresh_multisweep.c
 *
 * Regression for issue #13: "Edited sweep does not refresh after selecting
 * OK Do It!".
 *
 * The dominant cause on CfRadial data is not a stale cache -- it is that on a
 * multi-sweep volume the edit never touches the file the frame is displaying.
 * The Radx writer names its output from the written sweep's own start/end
 * times, so editing sweep N of a volume produces a NEW per-sweep file and
 * leaves the volume byte-for-byte intact (deliberate; see
 * test_multisweep_edit_safe.c). The frame is still pointed at the volume, so
 * a replot re-reads unedited data no matter how much cache is dropped. The
 * only way to see the edit was to pick the new file out of the Sweepfiles
 * list by hand -- exactly the workaround in the issue.
 *
 * The fix records the path Radx actually wrote (rio_last_written_path) and
 * moves the frame onto that file's catalog entry
 * (se_point_frame_at_written). This test drives both.
 *
 * cwd = tests/fixtures/volume.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_general_info.h"
#include "solo_window_structs.h"
#include "solo_editor_structs.h"
#include "radar_io.h"
#include "test_app_runner.h"

#define VOL_FILE "cfrad.20240903_120049.313_to_20240903_120613.852_SEAPOL_SUR.nc"
#define EDIT_SWEEP 3

struct dd_general_info *dd_window_dgi();
struct solo_edit_stuff *return_sed_stuff();
WW_PTR solo_return_wwptr();
gboolean se_point_frame_at_written();
void se_invalidate_read_caches();
int rio_write_ray();
int rio_write_sweep_end();
int mddir_file_list_v3();
int mddir_radar_num_v3();

/* Copy the volume fixture into a private writable directory. */
static char *private_copy(char *volpath, gsize cap)
{
    char *tmpdir = g_dir_make_tmp("soloiv_refresh_XXXXXX", NULL);
    gchar *buf = NULL;
    gsize len = 0;

    g_assert_nonnull(tmpdir);
    g_assert_true(g_file_get_contents("./" VOL_FILE, &buf, &len, NULL));
    g_snprintf(volpath, cap, "%s/%s", tmpdir, VOL_FILE);
    g_assert_true(g_file_set_contents(volpath, buf, len, NULL));
    g_free(buf);
    return tmpdir;
}

static void refresh_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *edgi;
    struct solo_edit_stuff *seds;
    WW_PTR wwptr;
    char *tmpdir, volpath[640], dirslash[640];
    char before_name[128];
    const char *written;
    double before_stamp;
    int nrays, r, rn;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    tmpdir = private_copy(volpath, sizeof(volpath));
    g_snprintf(dirslash, sizeof(dirslash), "%s/", tmpdir);

    /* --- put frame 0 on the volume, the way the GUI would --- */
    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    g_strlcpy(wwptr->sweep->directory_name, dirslash,
              sizeof(wwptr->sweep->directory_name));
    g_assert_cmpint(mddir_file_list_v3(0, dirslash), >, 0);
    rn = mddir_radar_num_v3(0, "SEAPOL");
    g_assert_cmpint(rn, >=, 0);
    wwptr->sweep->radar_num = rn;
    g_strlcpy(wwptr->sweep->file_name, VOL_FILE,
              sizeof(wwptr->sweep->file_name));
    g_strlcpy(before_name, wwptr->sweep->file_name, sizeof(before_name));
    before_stamp = wwptr->d_sweepfile_time_stamp;

    /* --- edit sweep EDIT_SWEEP the way a Do It commits it --- *
     * The editor works through its own dgi (seds->se_frame), which is the
     * dgi se_point_frame_at_written consults for the written path.          */
    seds = return_sed_stuff();
    g_assert_nonnull(seds);
    edgi = dd_window_dgi(seds->se_frame, "");
    g_assert_nonnull(edgi);
    g_strlcpy(edgi->directory_name, dirslash, sizeof(edgi->directory_name));
    g_strlcpy(edgi->sweep_file_name, VOL_FILE, sizeof(edgi->sweep_file_name));
    edgi->source_fmt = CFRADIAL_FMT;
    edgi->rio_req_sweep = EDIT_SWEEP;

    nrays = dd_absorb_header_info(edgi);
    g_assert_cmpint(nrays, >, 0);
    g_assert_cmpint(rio_current_sweep(edgi), ==, EDIT_SWEEP);

    for (r = 0; r < nrays; r++) {
        if (dd_absorb_ray_info(edgi) < 1) break;
        rio_write_ray(edgi);
    }
    g_assert_cmpint(rio_write_sweep_end(edgi), ==, 0);

    /* The writer must report where it actually put the sweep. */
    written = rio_last_written_path(edgi);
    g_assert_nonnull(written);
    g_assert_cmpint(strlen(written), >, 0);
    g_message("[edit_refresh] edit of sweep %d was written to %s",
              EDIT_SWEEP, written);

    /* It is a NEW file, not the volume -- that is the whole problem. */
    g_assert_null(strstr(written, VOL_FILE));

    /* --- the fix: the frame must move onto the written file --- */
    g_assert_true(se_point_frame_at_written(0));

    g_assert_cmpstr(wwptr->sweep->file_name, !=, before_name);
    g_assert_nonnull(strstr(written, wwptr->sweep->file_name));
    /* The replot resolves the file from the time stamp, so that has to move
     * onto the new catalog entry too, or the frame snaps back to the volume. */
    g_assert_cmpfloat(wwptr->d_sweepfile_time_stamp, !=, before_stamp);
    /* The written file holds just the one sweep. */
    g_assert_cmpint(dd_window_dgi(0, "")->rio_req_sweep, ==, 0);

    /* Calling it again is a no-op: the frame is already on the written file. */
    g_assert_false(se_point_frame_at_written(0));

    /* Dropping the caches must not disturb the frame or crash. */
    se_invalidate_read_caches(0);
    g_assert_cmpint(rio_volume_nsweeps(edgi), ==, 0);   /* editor cache dropped */

    g_free(tmpdir);
    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.refresh",
                               refresh_action, NULL);
}
