/* test_swpfi_dialog_walk.c
 *
 * Opens the sweep-file dialog (sii_swpfi_widgets.c) for frame 0 and
 * walks every clickable in its window. The swpfi dialog is what lets
 * the user navigate through sweep files in a directory; it's hit
 * routinely during real use.
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "widget_walker.h"
#include "soloii.h"     /* FRAME_SWPFI_MENU */
#include "sii_utils.h"  /* sii_get_widget_ptr */

void show_swpfi_widget(GtkWidget *text, gpointer data);

typedef struct {
    guint baseline;
    int   activated;
} SwpfiResult;

static const char *kSkipLabels[] = {
    "Cancel",
    "Close",
    "Quit",
    "Exit",
    "Browse",
    NULL
};

static void
swpfi_walk_action(GtkWidget *main_window, gpointer user_data)
{
    SwpfiResult *r = (SwpfiResult *)user_data;
    (void)main_window;

    show_swpfi_widget(NULL, GINT_TO_POINTER(0));
    test_app_runner_pump(20);

    GtkWidget *root = sii_get_widget_ptr(0, FRAME_SWPFI_MENU);
    g_assert_nonnull(root);

    r->baseline = sii_log_handler_get_complaint_count();
    r->activated = widget_walker_walk_tree(root, kSkipLabels);
}

static void
test_swpfi_dialog_walks_clean(void)
{
    SwpfiResult r = { 0 };
    int status = test_app_runner_run("org.lrose.soloiv.test.swpfi_walk",
                                     swpfi_walk_action, &r);
    g_assert_cmpint(status, ==, 0);
    g_assert_cmpint(r.activated, >=, 1);
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, r.baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/swpfi_dialog/walks_clean",
                    test_swpfi_dialog_walks_clean);
    return g_test_run();
}
