/* test_edit_undo.c
 *
 * Regression for issue #7: multi-level undo for editing.
 *
 * Editing overwrites the sweep file in place, so the undo feature snapshots
 * the file before each "Do It" and restores it on Undo. This test drives the
 * real snapshot/restore primitives (se_undo_push / se_undo_pop_restore /
 * se_undo_discard_top, in perusal/sii_edit_widgets.c) on a writable copy of a
 * real CfRadial sweep and verifies:
 *   - a snapshot + restore returns the file byte-for-byte to its prior state,
 *   - multiple snapshots restore in LIFO order (multi-level undo),
 *   - popping past the bottom of the stack is a safe no-op,
 *   - discarding the top snapshot (used when a Do It changes nothing) removes
 *     it without restoring.
 *
 * cwd = tests/fixtures/ground.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "dd_general_info.h"
#include "test_app_runner.h"

#define CFRAD_FILE "cf2/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"

struct dd_general_info *dd_window_dgi();
int se_undo_push();          /* gboolean */
int se_undo_pop_restore();   /* gboolean */
void se_undo_discard_top();

/* Read an entire file; caller g_free()s *buf. */
static gboolean slurp(const char *path, gchar **buf, gsize *len)
{
    return g_file_get_contents(path, buf, len, NULL);
}

static gboolean files_equal(const char *path, const gchar *ref, gsize ref_len)
{
    gchar *buf = NULL;
    gsize len = 0;
    gboolean eq;
    if (!slurp(path, &buf, &len)) return FALSE;
    eq = (len == ref_len) && (memcmp(buf, ref, len) == 0);
    g_free(buf);
    return eq;
}

/* Flip one byte in the middle of the file to simulate an edit that changed
 * the on-disk sweep. */
static void mutate(const char *path)
{
    gchar *buf = NULL;
    gsize len = 0;
    g_assert_true(slurp(path, &buf, &len));
    g_assert_cmpuint(len, >, 0);
    buf[len / 2] ^= 0xFF;
    g_assert_true(g_file_set_contents(path, buf, len, NULL));
    g_free(buf);
}

static void undo_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    char *tmpdir, src[512], dst[512];
    gchar *v0 = NULL, *v1 = NULL;
    gsize v0_len = 0, v1_len = 0;
    int win = 4;

    (void)main_window;
    (void)user_data;

    /* Work on a writable copy of the fixture. */
    tmpdir = g_dir_make_tmp("soloiv_undo_test_XXXXXX", NULL);
    g_assert_nonnull(tmpdir);
    g_snprintf(src, sizeof(src), "./%s", CFRAD_FILE);
    g_snprintf(dst, sizeof(dst), "%s/sweep.nc", tmpdir);
    {
        gchar *buf = NULL; gsize len = 0;
        g_assert_true(slurp(src, &buf, &len));
        g_assert_true(g_file_set_contents(dst, buf, len, NULL));
        g_free(buf);
    }

    /* Point a frame's dgi at the copy (what se_undo_push snapshots). */
    dgi = dd_window_dgi(win, "");
    g_snprintf(dgi->directory_name, sizeof(dgi->directory_name), "%s/", tmpdir);
    g_strlcpy(dgi->sweep_file_name, "sweep.nc", sizeof(dgi->sweep_file_name));

    g_assert_true(slurp(dst, &v0, &v0_len));   /* original bytes */

    /* --- edit 1 --- */
    g_assert_true(se_undo_push(win));          /* snapshot v0 */
    mutate(dst);                               /* -> v1 */
    g_assert_true(slurp(dst, &v1, &v1_len));
    g_assert_false(files_equal(dst, v0, v0_len));

    /* --- edit 2 --- */
    g_assert_true(se_undo_push(win));          /* snapshot v1 */
    mutate(dst);                               /* -> v2 */
    g_assert_false(files_equal(dst, v1, v1_len));

    /* undo edit 2 -> back to v1 (LIFO) */
    g_assert_true(se_undo_pop_restore());
    g_assert_true(files_equal(dst, v1, v1_len));

    /* undo edit 1 -> back to v0 */
    g_assert_true(se_undo_pop_restore());
    g_assert_true(files_equal(dst, v0, v0_len));

    /* nothing left to undo */
    g_assert_false(se_undo_pop_restore());

    /* discard: a snapshot that is discarded must NOT be restorable */
    g_assert_true(se_undo_push(win));
    se_undo_discard_top();
    g_assert_false(se_undo_pop_restore());

    g_free(v0);
    g_free(v1);
    g_free(tmpdir);
    g_message("[edit_undo] multi-level snapshot/restore verified");
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.editundo",
                               undo_action, NULL);
}
