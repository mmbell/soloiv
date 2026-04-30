/* test_dorade_partial_header.c
 *
 * Phase 7 — DORADE load-path robustness, partial-header variant.
 *
 * Truncates a real sweep file mid-superblock and verifies soloiv
 * does not crash trying to load it. The DORADE SSWB header is
 * ~196 bytes; 100 bytes lands inside it.
 *
 * Separate test binary from test_dorade_truncated because GtkApplication
 * globals don't fully reset between activate cycles.
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

#define GOOD_SWEEP "/Users/mmbell/Development/soloiv/test_data/" \
                   "swp.1220907125318.N42RF-TM.857.20.0_AIR_v2"

static char g_tmpdir[256];

static gboolean
copy_n_bytes(const char *src, const char *dst, off_t n)
{
    int sfd = open(src, O_RDONLY);
    if (sfd < 0) return FALSE;
    int dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dfd < 0) { close(sfd); return FALSE; }

    char buf[4096];
    off_t left = n;
    gboolean ok = TRUE;
    while (left > 0) {
        size_t want = left < (off_t)sizeof(buf) ? (size_t)left : sizeof(buf);
        ssize_t r = read(sfd, buf, want);
        if (r <= 0) break;
        if (write(dfd, buf, (size_t)r) != r) { ok = FALSE; break; }
        left -= r;
    }
    close(sfd);
    close(dfd);
    return ok;
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
    (void)main_window;
    (void)user_data;
    test_app_runner_pump(20);
}

static void
test_partial_header(void)
{
    char dst[512];
    snprintf(dst, sizeof(dst), "%s/swp.0000000000.FAKE.000.0.0_AIR_v0", g_tmpdir);

    g_assert_true(copy_n_bytes(GOOD_SWEEP, dst, 100));

    setenv("DORADE_DIR", g_tmpdir, 1);
    int status = test_app_runner_run("org.lrose.soloiv.test.dorade_partial",
                                     no_op_action, NULL);
    g_assert_cmpint(status, ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    snprintf(g_tmpdir, sizeof(g_tmpdir), "/tmp/soloiv_fuzz_ph_%d", (int)getpid());
    mkdir(g_tmpdir, 0755);
    g_test_add_func("/dorade_partial_header/no_crash", test_partial_header);
    int rc = g_test_run();
    remove_dir_recursive(g_tmpdir);
    return rc;
}
