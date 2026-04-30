/* test_walk_main_menu.c
 *
 * Walks every action in the main window's "menu" GAction group and
 * fires it. This exercises every File / Zoom / Center / Config / Help
 * menu item with no human in the loop.
 *
 * Skip list:
 *   - "quit"           — would tear down the test process
 *   - "set_source_dir",
 *     "set_image_dir",
 *     "config_files"   — open file-chooser dialogs that block the loop
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "widget_walker.h"

typedef struct {
    int activated;
    guint baseline_complaints;
} WalkResult;

static const char *kSkipActions[] = {
    "quit",
    "set_source_dir",
    "set_image_dir",
    "config_files",
    NULL
};

static void
walk_main_menu_action(GtkWidget *main_window, gpointer user_data)
{
    WalkResult *r = (WalkResult *)user_data;

    /* Snapshot complaints AFTER startup but BEFORE the walk, so we
     * test "no NEW complaints from menu activations". */
    r->baseline_complaints = sii_log_handler_get_complaint_count();

    r->activated = widget_walker_run_action_group(main_window, "menu",
                                                  kSkipActions);
}

static void
test_walk_main_menu_no_crash(void)
{
    WalkResult r = { 0 };
    int status;

    status = test_app_runner_run("org.lrose.soloiv.test.menu_walk",
                                 walk_main_menu_action, &r);
    g_assert_cmpint(status, ==, 0);

    /* Sanity: we should have actually fired actions. If skip list ate
     * them all, we missed something. The main menu has ~50 actions so
     * even after the skip list we expect at least 30. */
    g_assert_cmpint(r.activated, >, 30);

    /* No NEW complaints from any of the menu items. Existing legacy
     * startup complaints are not in scope for this test. */
    guint final = sii_log_handler_get_complaint_count();
    g_assert_cmpuint(final, ==, r.baseline_complaints);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/walk/main_menu_no_crash", test_walk_main_menu_no_crash);
    return g_test_run();
}
