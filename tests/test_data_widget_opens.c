/* test_data_widget_opens.c
 *
 * Third frame-menu button after Examine and Edit. Brings up the click-
 * data display widget for frame 0 and asserts no NEW CRITICAL/WARNING
 * messages and no crash.
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"

void show_click_data_widget(guint frame_num);

typedef struct { guint baseline; } DataResult;

static void
open_data_widget_action(GtkWidget *main_window, gpointer user_data)
{
    DataResult *r = (DataResult *)user_data;
    (void)main_window;

    r->baseline = sii_log_handler_get_complaint_count();
    show_click_data_widget(0);
    test_app_runner_pump(20);
}

static void
test_data_widget_opens_without_crash(void)
{
    DataResult r = { 0 };
    int status = test_app_runner_run("org.lrose.soloiv.test.data_widget",
                                     open_data_widget_action, &r);
    g_assert_cmpint(status, ==, 0);
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, r.baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/data_widget/opens_without_crash",
                    test_data_widget_opens_without_crash);
    return g_test_run();
}
