/* test_dorade_truncated.c
 *
 * Phase 7 — DORADE load-path robustness.
 *
 * Sets up a tempdir containing an empty file with a DORADE sweep
 * filename pattern, points soloiv at it via DORADE_DIR, brings up the
 * app, and asserts no crash. The parser should either succeed
 * (unlikely with no bytes) or recover with an error rather than
 * dereferencing past the truncation.
 *
 * Per the user's note that "if there is a bad struct in the file it
 * can cause the program to crash" — this catches that class of bug.
 *
 * One mutation per test binary: GtkApplication globals don't fully
 * reset between activate cycles, so running multiple GUI-driven
 * scenarios in one process is fragile. Companion tests for other
 * truncation lengths can be added in their own files.
 */

#include <gtk/gtk.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"

static char g_tmpdir[256] = { 0 };

static void
make_empty_sweep_file(const char *dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/swp.0000000000.FAKE.000.0.0_AIR_v0", dir);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    g_assert_cmpint(fd, >=, 0);
    close(fd);
}

static void
remove_dir_recursive(const char *path)
{
    DIR *d = opendir(path);
    if (d) {
        struct dirent *e;
        char full[512];
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
            unlink(full);
        }
        closedir(d);
    }
    rmdir(path);
}

static void
no_op_action(GtkWidget *main_window, gpointer user_data)
{
    /* The risky work — sweep parsing — happens during soloiv_activate
     * before this fires. Just pump the loop to let any deferred load
     * paths run, then return so the runner quits. */
    (void)main_window;
    (void)user_data;
    test_app_runner_pump(20);
}

static void
test_empty_file_does_not_crash(void)
{
    make_empty_sweep_file(g_tmpdir);
    setenv("DORADE_DIR", g_tmpdir, 1);

    int status = test_app_runner_run("org.lrose.soloiv.test.dorade_empty",
                                     no_op_action, NULL);
    g_assert_cmpint(status, ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    snprintf(g_tmpdir, sizeof(g_tmpdir), "/tmp/soloiv_fuzz_%d", (int)getpid());
    mkdir(g_tmpdir, 0755);

    g_test_add_func("/dorade_truncated/empty_file",
                    test_empty_file_does_not_crash);

    int rc = g_test_run();

    remove_dir_recursive(g_tmpdir);
    return rc;
}
