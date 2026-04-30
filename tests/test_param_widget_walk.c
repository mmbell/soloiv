/* test_param_widget_walk.c
 *
 * Opens the parameter widget for frame 0 (sii_param_widgets.c, ~4k lines)
 * and walks every clickable in its window. The parameter widget has the
 * largest single-file callback surface in the GUI, so this is the
 * heaviest test of the per-frame controls.
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "widget_walker.h"
#include "soloii.h"      /* FRAME_PARAMETERS */
#include "sii_utils.h"   /* sii_get_widget_ptr */

void show_param_widget(GtkWidget *text, gpointer data);

typedef struct {
    guint baseline;
    int   activated;
} ParamResult;

/* Skip list — labels matching any substring here are not clicked.
 * The parameter widget has Cancel/Close-style buttons that would tear
 * down the widget mid-walk; file-chooser buttons would block on a
 * modal dialog. */
static const char *kSkipLabels[] = {
    "Cancel",
    "Close",
    "Quit",
    "Exit",
    "Browse",   /* opens file dialog */
    NULL
};

static void
param_walk_action(GtkWidget *main_window, gpointer user_data)
{
    ParamResult *r = (ParamResult *)user_data;
    (void)main_window;

    /* Open the parameter widget for frame 0. After this returns the
     * widget tree exists and is visible. */
    show_param_widget(NULL, GINT_TO_POINTER(0));
    test_app_runner_pump(20);

    GtkWidget *root = sii_get_widget_ptr(0, FRAME_PARAMETERS);
    g_assert_nonnull(root);

    /* Snapshot complaints AFTER the open (legacy startup + open both
     * count toward the baseline) so the assertion focuses on what the
     * walker itself triggers. */
    r->baseline = sii_log_handler_get_complaint_count();

    r->activated = widget_walker_walk_tree(root, kSkipLabels);
}

static void
test_param_widget_walks_clean(void)
{
    ParamResult r = { 0 };
    int status = test_app_runner_run("org.lrose.soloiv.test.param_walk",
                                     param_walk_action, &r);
    g_assert_cmpint(status, ==, 0);

    /* Sanity check: the walker should have found *something* in the
     * window. The threshold is intentionally low — most of the
     * parameter widget's controls live inside GMenu popovers, which
     * the simple tree walker doesn't open. The point of this test is
     * "the things we DO touch don't crash"; coverage of menu items is
     * handled separately by the action-group walker pattern. */
    g_assert_cmpint(r.activated, >=, 1);

    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, r.baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/param_widget/walks_clean",
                    test_param_widget_walks_clean);
    return g_test_run();
}
