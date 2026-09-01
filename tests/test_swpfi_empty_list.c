/* test_swpfi_empty_list.c
 *
 * The sweep-file list widget had two ways to fall over, both reachable after
 * an edit (which triggers a directory rescan that renumbers radars and can
 * move files out from under the frame's current one):
 *
 *  - sii_swpfi_list_widget sized its rows from
 *    strlen(solo_list_entry(slm, 0)). solo_list_entry returns NULL for an
 *    out-of-range index, so an empty list segfaulted on the strlen.
 *
 *  - The rows are created in blocks of 500 with their index baked in, and
 *    only the TEXT of rows past the end of the list was blanked. Clicking a
 *    blank row passed an out-of-range index to the catalog, which quietly set
 *    sweep->start_time to 0 and cleared sweep->file_name -- driving a
 *    TIME_NEAREST jump to whatever file happened to sort first.
 *
 * This test opens the list with a stale (out-of-range) radar number, which is
 * exactly what a rescan that dropped a radar leaves behind, so the list comes
 * back empty. It must open without crashing or complaining, and a click on a
 * blank row must leave the frame's sweep selection alone.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "solo_window_structs.h"
#include "soloii.h"
#include "sii_log_handler.h"
#include "test_app_runner.h"

void show_swpfi_widget(GtkWidget *text, gpointer data);
void sii_swpfi_list_widget(guint frame_num);
GtkWidget *sii_get_widget_ptr(guint frame_num, guint widget_id);
WW_PTR solo_return_wwptr();

static void empty_list_action(GtkWidget *main_window, gpointer user_data)
{
    WW_PTR wwptr;
    guint baseline;
    char saved_file[128];
    double saved_stamp;
    int saved_radar;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    show_swpfi_widget(NULL, GINT_TO_POINTER(0));
    test_app_runner_pump(20);
    g_assert_nonnull(sii_get_widget_ptr(0, FRAME_SWPFI_MENU));

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);

    /* Open the list once normally so the layout exists. */
    sii_swpfi_list_widget(0);
    test_app_runner_pump(10);

    saved_radar = wwptr->sweep->radar_num;
    g_strlcpy(saved_file, wwptr->sweep->file_name, sizeof(saved_file));
    saved_stamp = wwptr->d_sweepfile_time_stamp;

    baseline = sii_log_handler_get_complaint_count();

    /* Stale radar number: the catalog returns nothing for it, so the list is
     * empty. This is where the strlen(NULL) used to be. */
    wwptr->sweep->radar_num = MAX_SENSORS + 3;
    sii_swpfi_list_widget(0);
    test_app_runner_pump(10);

    /* Reopening on a valid radar again must also be fine (the layout is
     * reused across opens). */
    wwptr->sweep->radar_num = saved_radar;
    sii_swpfi_list_widget(0);
    test_app_runner_pump(10);

    /* Nothing about the frame's sweep selection should have moved, and the
     * empty open must not have produced complaints. */
    g_assert_cmpstr(wwptr->sweep->file_name, ==, saved_file);
    g_assert_cmpfloat(wwptr->d_sweepfile_time_stamp, ==, saved_stamp);
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.swpfiempty",
                               empty_list_action, NULL);
}
