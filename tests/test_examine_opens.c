/* test_examine_opens.c
 *
 * Regression test for the known crash: opening the Examine widget brings
 * down the program. This test reproduces the click sequence and asserts
 * that the program survives.
 *
 * Test flow:
 *   1. Create a GtkApplication with G_APPLICATION_NON_UNIQUE so multiple
 *      test processes don't fight over a primary instance.
 *   2. The app's activate handler delegates to the real soloiv_activate(),
 *      which builds the main window and (because cwd is test_data/) loads
 *      the first sweep file via sii_default_startup().
 *   3. A deferred callback (g_timeout_add) fires after the GUI has had a
 *      chance to settle, then invokes show_exam_widget() for frame 0.
 *   4. Another deferred callback quits the application.
 *   5. The test asserts the complaint counter remained zero, i.e. no
 *      CRITICAL or WARNING was logged during the Examine flow.
 *
 * Currently expected to FAIL — that is the point. The failure mode (crash
 * vs. CRITICAL message) tells us where to dig in sii_exam_widgets.c.
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "sii_log_handler.h"

void soloiv_activate(GtkApplication *app, gpointer user_data);
void show_exam_widget(GtkWidget *text, gpointer data);

static GtkApplication *g_app = NULL;
static guint g_baseline_complaints = 0;

static gboolean
quit_app_cb(gpointer user_data)
{
    (void)user_data;
    if (g_app) {
        g_application_quit(G_APPLICATION(g_app));
    }
    return G_SOURCE_REMOVE;
}

static gboolean
trigger_examine_cb(gpointer user_data)
{
    (void)user_data;

    /* Snapshot complaint count BEFORE we trigger Examine, so any
     * complaints emitted during normal startup don't pollute the
     * assertion. */
    g_baseline_complaints = sii_log_handler_get_complaint_count();

    /* This is the action under test: opening the Examine widget for
     * frame 0. If this segfaults, the test process dies and ctest
     * reports "Subprocess aborted" — that IS the regression signal. */
    show_exam_widget(NULL, GINT_TO_POINTER(0));

    /* Pump the event loop a few times so any deferred work (idle
     * handlers scheduled by show_exam_widget) gets a chance to run. */
    for (int i = 0; i < 20; i++) {
        g_main_context_iteration(NULL, FALSE);
    }

    /* Schedule the quit a bit later so widgets actually get exercised. */
    g_timeout_add(100, quit_app_cb, NULL);
    return G_SOURCE_REMOVE;
}

static void
test_activate_cb(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    /* Run the real activate handler. This builds the main window, loads
     * sweeps from cwd (= test_data/), and otherwise behaves exactly as
     * the production binary. */
    soloiv_activate(app, NULL);

    /* Give the GUI a moment to settle (sweep load, paint, etc.) before
     * we press the Examine button. 200ms is conservative; we can tighten
     * once the test is known to be deterministic. */
    g_timeout_add(200, trigger_examine_cb, NULL);
}

static void
test_examine_opens_without_crash(void)
{
    int status;

    /* Non-fatal: just observe and count complaints. The legacy code
     * emits CRITICAL during routine startup (logged but recoverable);
     * we measure DELTA over the action under test instead of absolute. */
    sii_log_handler_install(FALSE);

    g_app = gtk_application_new("org.lrose.soloiv.test.examine",
                                G_APPLICATION_NON_UNIQUE);
    g_signal_connect(g_app, "activate", G_CALLBACK(test_activate_cb), NULL);

    /* Drive the application from inside the test. argc/argv are zero
     * since GtkApplication doesn't need any positional arguments. */
    int argc = 0;
    char **argv = NULL;
    status = g_application_run(G_APPLICATION(g_app), argc, argv);

    g_object_unref(g_app);
    g_app = NULL;

    g_assert_cmpint(status, ==, 0);

    /* Assert: opening Examine produced no new CRITICAL/WARNING. The
     * baseline subtracts complaints from the pre-Examine startup phase,
     * which we are not yet enforcing zero on (that is a separate test). */
    guint final = sii_log_handler_get_complaint_count();
    g_assert_cmpuint(final, ==, g_baseline_complaints);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/examine/opens_without_crash",
                    test_examine_opens_without_crash);
    return g_test_run();
}
