/* test_smoke.c
 *
 * Canary test. If this fails, the test harness itself is broken — there is
 * no point looking at any other test result until this is green.
 *
 * It exercises:
 *   1. Linking against soloiv_core succeeds (the function call below is
 *      enough to pull in symbols and prove the build graph is healthy).
 *   2. The GLib test framework runs and reports normally.
 *   3. The custom log handler installs without complaint.
 *   4. The complaint counter starts at zero.
 *
 * It does NOT bring up GTK or any windows. That is test_main_window_opens'
 * job. Keeping smoke minimal means a smoke failure points at infrastructure,
 * not at the GUI.
 */

#include <glib.h>

#include "sii_log_handler.h"

static void test_log_handler_installs(void)
{
    sii_log_handler_install(TRUE);
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, 0);
}

static void test_complaint_counter_resets(void)
{
    sii_log_handler_reset_complaint_count();
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/smoke/log_handler_installs",     test_log_handler_installs);
    g_test_add_func("/smoke/complaint_counter_resets", test_complaint_counter_resets);

    return g_test_run();
}
