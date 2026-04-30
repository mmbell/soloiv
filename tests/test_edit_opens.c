/* test_edit_opens.c
 *
 * Companion to test_examine_opens for the other major per-frame widget:
 * the editor. Brings up the app, calls show_edit_widget for frame 0,
 * pumps the loop, asserts no NEW CRITICAL/WARNING messages were emitted
 * and the process didn't crash.
 *
 * The editor widget (sii_edit_widgets.c, ~2400 lines) shares many of the
 * same GTK4 migration patterns that broke Examine — radio button initial
 * state, signal idioms — so this is a high-likelihood place to catch
 * the next batch of bugs.
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"

void show_edit_widget(GtkWidget *text, gpointer data);

typedef struct {
    guint baseline;
} EditResult;

static void
open_editor_action(GtkWidget *main_window, gpointer user_data)
{
    EditResult *r = (EditResult *)user_data;
    (void)main_window;

    r->baseline = sii_log_handler_get_complaint_count();

    /* Equivalent to clicking "Editor" on the frame 0 menu. */
    show_edit_widget(NULL, GINT_TO_POINTER(0));

    /* Pump loop so any deferred work in the editor finishes. */
    test_app_runner_pump(20);
}

static void
test_edit_opens_without_crash(void)
{
    EditResult r = { 0 };
    int status;

    status = test_app_runner_run("org.lrose.soloiv.test.edit",
                                 open_editor_action, &r);
    g_assert_cmpint(status, ==, 0);

    /* No NEW complaints during the editor open. Existing legacy startup
     * complaints are out of scope. */
    guint final = sii_log_handler_get_complaint_count();
    g_assert_cmpuint(final, ==, r.baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/edit/opens_without_crash", test_edit_opens_without_crash);
    return g_test_run();
}
