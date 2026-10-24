/* test_examine_commit_reload.c
 *
 * An Examine edit must still be there after "Apply Edits".
 *
 * Deleting points in the Examine list turned them into dashes, but Apply
 * Edits put the original values back. The edit was written: sxm_apply_changez
 * rewrites the sweep file under the same name. The refresh that follows then
 * re-read the sweep through the rio read cache, which is keyed on the path
 * and still held the unedited volume. The Edit widget's Do It drops that
 * cache (issue #13); the Examine commit did not.
 *
 * Copies the N42RF-TM airborne CfRadial-2 fixture into a private directory,
 * opens Examine on it, deletes the last good gate of the first ray in the
 * list through the same click sequence the widget sends (Delete, a click in
 * the list, Apply Edits), and requires the refreshed list -- and a fresh read
 * of the file -- to show that gate as bad.
 *
 * cwd = tests/fixtures/airborne/cf2; main() moves into the private copy.
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "solo_window_structs.h"
#include "sed_shared_structs.h"
#include "solo_editor_structs.h"
#include "seds.h"
#include "test_app_runner.h"

#define FIXTURE "cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"

WW_PTR solo_return_wwptr();
void show_exam_widget(GtkWidget *text, gpointer data);
void sxm_process_click(struct solo_click_info *sci);
int solo_busy();
void solo_clear_busy_signal();

static char *g_tmpdir;

static void examine_click(int button, int list_entry, int character)
{
    struct solo_click_info sci;

    memset(&sci, 0, sizeof(sci));
    sci.frame = 0;
    sci.which_widget_button = button;
    sci.which_list_entry = list_entry;
    sci.which_character = character;
    sxm_process_click(&sci);
    /* sxm_process_click leaves the busy signal to the widget callbacks */
    if (solo_busy())
        solo_clear_busy_signal();
}

static void commit_action(GtkWidget *main_window, gpointer user_data)
{
    WW_PTR wwptr;
    struct examine_control *ecs;
    float bad, *col0;
    int row, ncells;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);
    show_exam_widget(NULL, GINT_TO_POINTER(0));
    test_app_runner_pump(20);

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    ecs = wwptr->examine_control;
    g_assert_nonnull(ecs);
    ncells = ecs->actual_num_cells;
    g_assert_cmpint(ncells, >, 10);
    g_assert_cmpint(ecs->num_data_cols, >, 0);

    col0 = ecs->data_ptrs[0];
    bad = ecs->bad_data[0];
    for (row = ncells - 1; row >= 0 && col0[row] == bad; row--);
    g_assert_cmpint(row, >=, 0);
    g_message("[examine_commit] %d cells, deleting row %d (%.2f)",
              ncells, row, col0[row]);

    examine_click(EX_DELETE, 0, 0);
    /* column 0 of the data starts at col_lims[0] */
    examine_click(EX_CLICK_IN_LIST, ecs->heading_row_count + row,
                  ecs->col_lims[0]);
    g_assert_cmpint(wwptr->examine_info->change_count, ==, 1);
    g_assert_cmpfloat(ecs->data_ptrs[0][row], ==, bad);

    examine_click(EX_COMMIT, 0, 0);
    g_assert_cmpint(wwptr->examine_info->change_count, ==, 0);

    /* the refresh re-read the sweep: the deleted gate must still be bad */
    if (ecs->data_ptrs[0][row] != bad)
        g_error("row %d reverted to %.2f after Apply Edits",
                row, ecs->data_ptrs[0][row]);

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    gchar *buf = NULL, *dst;
    gsize len = 0;
    int rc;

    g_test_init(&argc, &argv, NULL);

    /* Apply Edits rewrites the sweep file; never touch the fixture. */
    g_tmpdir = g_dir_make_tmp("soloiv_exmcommit_XXXXXX", NULL);
    g_assert_nonnull(g_tmpdir);
    g_assert_true(g_file_get_contents(FIXTURE, &buf, &len, NULL));
    dst = g_build_filename(g_tmpdir, FIXTURE, NULL);
    g_assert_true(g_file_set_contents(dst, buf, len, NULL));
    g_free(buf);
    g_assert_cmpint(chdir(g_tmpdir), ==, 0);

    rc = test_app_runner_run("org.lrose.soloiv.test.exmcommit",
                             commit_action, NULL);
    g_free(dst);
    return rc;
}
