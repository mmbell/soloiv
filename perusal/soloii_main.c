/* main() entry for the soloiv executable.
 *
 * Lives in its own translation unit so the rest of perusal/ can be packaged
 * as a static library (soloiv_core) that both the executable and the test
 * binaries link against. Tests provide their own main() and never include
 * this file.
 *
 * Recognized command-line flags (consumed before GtkApplication runs):
 *   --debug-warnings   Install the GLib log handler and abort on the first
 *                      CRITICAL/WARNING. Mirrors what the test suite does.
 */

#include <gtk/gtk.h>
#include <string.h>

#include "sii_log_handler.h"

void soloiv_activate(GtkApplication *app, gpointer user_data);

static void
consume_local_flag(int *argc, char **argv, const char *flag, gboolean *out)
{
    int read_idx, write_idx;
    *out = FALSE;
    for (read_idx = 1, write_idx = 1; read_idx < *argc; read_idx++) {
        if (strcmp(argv[read_idx], flag) == 0) {
            *out = TRUE;
        } else {
            argv[write_idx++] = argv[read_idx];
        }
    }
    *argc = write_idx;
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    gboolean debug_warnings;
    int status;

    consume_local_flag(&argc, argv, "--debug-warnings", &debug_warnings);
    if (debug_warnings) {
        /* fatal = FALSE in the production binary: visibility without
         * aborting on every legacy warning.  Tests pass TRUE. */
        sii_log_handler_install(FALSE);
    }

    app = gtk_application_new("org.lrose.soloiv", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(soloiv_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
