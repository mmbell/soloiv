/* test_app_runner.h
 *
 * Common harness for tests that need to drive the soloiv GUI.
 *
 * Each test that wants to exercise the application provides a single
 * "user-action" callback. The runner brings up GtkApplication, runs the
 * real soloiv_activate() so the app behaves identically to production,
 * waits for the GUI to settle, runs the user-action, pumps the event
 * loop, and then quits cleanly. Each test gets one process; isolation
 * between tests is provided by CTest, not by per-test reset of the
 * GtkApplication (which is unsafe to recycle in-process).
 *
 * The user-action receives a complaint baseline captured immediately
 * before it runs, and is expected to assert that no NEW complaints
 * (CRITICAL/WARNING) appeared. That keeps the legacy startup noise out
 * of every test's failure mode while still catching anything new.
 */

#ifndef TEST_APP_RUNNER_H
#define TEST_APP_RUNNER_H

#include <gtk/gtk.h>

/* Action callback. main_window is the GtkApplicationWindow built by
 * soloiv_activate. user_data is whatever the test passed to the runner.
 *
 * Within the action: do whatever exercises the GUI, then pump the loop
 * a few times (test_app_runner_pump). Return when done — the runner
 * will quit the app for you. */
typedef void (*TestAppActionFunc)(GtkWidget *main_window, gpointer user_data);

/* Run the soloiv app, fire the action after a short settle delay, then
 * shut down. Returns the exit status of g_application_run (0 = success).
 *
 * application_id should be unique per test (e.g. "org.lrose.soloiv.test.X")
 * so tests don't collide if any caching in GApplication ever leaks
 * between processes. G_APPLICATION_NON_UNIQUE is implied. */
int test_app_runner_run(const char *application_id,
                        TestAppActionFunc action,
                        gpointer user_data);

/* Spin the GLib main loop a bounded number of iterations without blocking.
 * Use this between actions in the test to let GTK process the changes. */
void test_app_runner_pump(int iterations);

#endif /* TEST_APP_RUNNER_H */
