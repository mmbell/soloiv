/* test_multisweep_edit_safe.c
 *
 * Issue #2 safety guard: editing one sweep of a multi-sweep CfRadial volume
 * must NOT overwrite the volume file. The Radx writer names its output from
 * the written sweep's own start/end times (writeToDir + loadVolumeInfoFromRays),
 * so an edited sweep is written as its own properly-named single-sweep file
 * while the multi-sweep volume on disk is left byte-for-byte intact.
 *
 * The test loads sweep 3 of the volume from a private copy, writes it back the
 * way an edit does (rio_write_ray per ray + rio_write_sweep_end), and asserts:
 *   - the original volume file is unchanged,
 *   - a new, differently-named sweep file was produced.
 *
 * cwd = tests/fixtures/volume.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_general_info.h"
#include "radar_io.h"
#include "test_app_runner.h"

#define VOL_FILE "cfrad.20240903_120049.313_to_20240903_120613.852_SEAPOL_SUR.nc"

struct dd_general_info *dd_window_dgi();
int rio_write_ray();
int rio_write_sweep_end();

static gboolean slurp(const char *path, gchar **buf, gsize *len)
{
    return g_file_get_contents(path, buf, len, NULL);
}

static void edit_safe_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    char *tmpdir, src[512], volcopy[640], newfile[640];
    gchar *vol_before = NULL, *vol_after = NULL;
    gsize before_len = 0, after_len = 0;
    GDir *d;
    const char *name;
    int nrays, r, found_new = 0;

    (void)main_window;
    (void)user_data;

    /* private writable copy of the volume */
    tmpdir = g_dir_make_tmp("soloiv_mse_XXXXXX", NULL);
    g_assert_nonnull(tmpdir);
    g_snprintf(src, sizeof(src), "./%s", VOL_FILE);
    g_snprintf(volcopy, sizeof(volcopy), "%s/%s", tmpdir, VOL_FILE);
    {
        gchar *b = NULL; gsize l = 0;
        g_assert_true(slurp(src, &b, &l));
        g_assert_true(g_file_set_contents(volcopy, b, l, NULL));
        g_free(b);
    }
    g_assert_true(slurp(volcopy, &vol_before, &before_len));

    /* load sweep 3 of the volume */
    dgi = dd_window_dgi(6, "");
    g_snprintf(dgi->directory_name, sizeof(dgi->directory_name), "%s/", tmpdir);
    g_strlcpy(dgi->sweep_file_name, VOL_FILE, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;
    dgi->rio_req_sweep = 3;

    nrays = dd_absorb_header_info(dgi);
    g_assert_cmpint(nrays, >, 0);
    g_assert_cmpint(rio_current_sweep(dgi), ==, 3);

    /* write the sweep back the way an edit commits it */
    for (r = 0; r < nrays; r++) {
        if (dd_absorb_ray_info(dgi) < 1) break;
        rio_write_ray(dgi);
    }
    g_assert_cmpint(rio_write_sweep_end(dgi), ==, 0);

    /* the volume file must be byte-for-byte unchanged */
    g_assert_true(slurp(volcopy, &vol_after, &after_len));
    g_assert_cmpuint(after_len, ==, before_len);
    g_assert_cmpint(memcmp(vol_before, vol_after, before_len), ==, 0);

    /* a new, differently-named sweep file must have been produced */
    d = g_dir_open(tmpdir, 0, NULL);
    g_assert_nonnull(d);
    while ((name = g_dir_read_name(d))) {
        if (g_str_has_suffix(name, ".nc") && strcmp(name, VOL_FILE) != 0) {
            found_new = 1;
            g_snprintf(newfile, sizeof(newfile), "%s/%s", tmpdir, name);
            g_message("[multisweep_edit_safe] edited sweep written as %s", name);
        }
    }
    g_dir_close(d);
    g_assert_true(found_new);

    g_free(vol_before);
    g_free(vol_after);
    g_free(tmpdir);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.mse",
                               edit_safe_action, NULL);
}
