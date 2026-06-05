/* test_cfradial_examine.c
 *
 * Opens the Examine widget on a CfRadial sweep (cwd = the cfradial sample
 * dir, so default startup loads cfrad2.*.nc through the Radx backend) and
 * asserts the program survives with no new complaints.
 *
 * This exercises the examine read path on a Radx-managed sweep:
 * dgi_buf_rewind -> dd_absorb_header_info -> a loop of dd_absorb_ray_info to
 * build the per-ray display list. dgi_buf_rewind is now rio-aware (resets the
 * Radx ray cursor), so the rewind+re-read works the same as for DORADE.
 *
 * Mirrors test_examine_opens (the DORADE version) so a crash here points at a
 * CfRadial-specific gap in the examine path.
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "sii_log_handler.h"

void soloiv_activate(GtkApplication *app, gpointer user_data);
void show_exam_widget(GtkWidget *text, gpointer data);

static GtkApplication *g_app = NULL;
static guint g_baseline_complaints = 0;

static gboolean quit_app_cb(gpointer user_data)
{
    (void)user_data;
    if (g_app) g_application_quit(G_APPLICATION(g_app));
    return G_SOURCE_REMOVE;
}

static gboolean trigger_examine_cb(gpointer user_data)
{
    (void)user_data;
    g_baseline_complaints = sii_log_handler_get_complaint_count();

    /* Action under test: open Examine for frame 0 on the CfRadial sweep. */
    show_exam_widget(NULL, GINT_TO_POINTER(0));

    for (int i = 0; i < 20; i++)
        g_main_context_iteration(NULL, FALSE);

    g_timeout_add(100, quit_app_cb, NULL);
    return G_SOURCE_REMOVE;
}

static void test_activate_cb(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    soloiv_activate(app, NULL);
    g_timeout_add(200, trigger_examine_cb, NULL);
}

static void test_cfradial_examine_opens(void)
{
    int status;
    sii_log_handler_install(FALSE);

    g_app = gtk_application_new("org.lrose.soloiv.test.cfradial_examine",
                                G_APPLICATION_NON_UNIQUE);
    g_signal_connect(g_app, "activate", G_CALLBACK(test_activate_cb), NULL);

    int argc = 0;
    char **argv = NULL;
    status = g_application_run(G_APPLICATION(g_app), argc, argv);

    g_object_unref(g_app);
    g_app = NULL;

    g_assert_cmpint(status, ==, 0);
    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==,
                     g_baseline_complaints);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/cfradial/examine_opens", test_cfradial_examine_opens);
    return g_test_run();
}
