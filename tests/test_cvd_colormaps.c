/* test_cvd_colormaps.c
 *
 * Two checks for the CVD-friendly colormaps (issue #1):
 *
 *   1. The five names from EVS-ATMOS/CVD-colormaps are present in the
 *      ascii_color_tables registry after startup. This is what makes
 *      them appear in the parameter widget's color-table picker.
 *
 *   2. Switching frame 0 to chase_spectral and asking for a PNG export
 *      writes a real file. The output dir is /tmp/soloiv_chase_spectral/
 *      so the developer can eyeball the resulting image after the run.
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "soloii.h"
#include "sii_externals.h"

/* From solo2.c */
void sii_png_image_prep (char *dir);

#define CHASE_SPECTRAL_OUT_DIR "/tmp/soloiv_chase_spectral"

static const char *kCvdNames[] = {
    "chase_spectral",
    "balance",
    "spectral_extended",
    "crameri_oleron_depol",
    "crameri_roma_rhohv",
    NULL
};

typedef struct {
    int   missing_count;
    int   apply_ok;
    int   png_count;
    char  png_path[1024];
} CvdResult;

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */

static void
remove_pngs_in_dir (const char *dir)
{
    GDir *gd = g_dir_open (dir, 0, NULL);
    if (!gd) return;

    const gchar *entry;
    while ((entry = g_dir_read_name (gd)) != NULL) {
        if (g_str_has_suffix (entry, ".png")) {
            gchar *full = g_build_filename (dir, entry, NULL);
            g_unlink (full);
            g_free (full);
        }
    }
    g_dir_close (gd);
}

/* Find the first *.png file in dir; copy its full path into out. */
static int
find_png_in_dir (const char *dir, char *out, size_t outsz)
{
    GDir *gd = g_dir_open (dir, 0, NULL);
    if (!gd) return 0;

    int found = 0;
    const gchar *entry;
    while ((entry = g_dir_read_name (gd)) != NULL) {
        if (g_str_has_suffix (entry, ".png")) {
            gchar *full = g_build_filename (dir, entry, NULL);
            g_strlcpy (out, full, outsz);
            g_free (full);
            found = 1;
            break;
        }
    }
    g_dir_close (gd);
    return found;
}

/* ------------------------------------------------------------------ */
/* action 1: registry presence                                        */
/* ------------------------------------------------------------------ */

static void
registry_action (GtkWidget *main_window, gpointer user_data)
{
    (void)main_window;
    CvdResult *r = (CvdResult *)user_data;

    for (int i = 0; kCvdNames[i] != NULL; i++) {
        if (g_tree_lookup (ascii_color_tables, (gpointer)kCvdNames[i]) == NULL) {
            r->missing_count++;
            g_printerr ("CVD colormap '%s' missing from ascii_color_tables\n",
                        kCvdNames[i]);
        }
    }
}

static void
test_cvd_colormaps_registered (void)
{
    CvdResult r = {0};
    sii_log_handler_install (FALSE);

    int status = test_app_runner_run ("org.lrose.soloiv.test.cvd_registry",
                                      registry_action, &r);
    g_assert_cmpint (status, ==, 0);
    g_assert_cmpint (r.missing_count, ==, 0);
}

/* ------------------------------------------------------------------ */
/* action 2: render chase_spectral, export PNG                        */
/* ------------------------------------------------------------------ */

static void
chase_spectral_action (GtkWidget *main_window, gpointer user_data)
{
    (void)main_window;
    CvdResult *r = (CvdResult *)user_data;

    g_mkdir_with_parents (CHASE_SPECTRAL_OUT_DIR, 0755);
    remove_pngs_in_dir (CHASE_SPECTRAL_OUT_DIR);

    /* Swap frame 0 to chase_spectral. This drives the same path as
     * clicking through the parameter widget, minus the widget. */
    r->apply_ok = sii_apply_color_table_by_name (0, "chase_spectral") ? 1 : 0;

    /* Let any deferred work settle. */
    test_app_runner_pump (40);

    /* Trigger a PNG export to our well-known output dir. The function
     * synthesizes the filename from the sweep metadata. */
    sii_png_image_prep (CHASE_SPECTRAL_OUT_DIR);

    test_app_runner_pump (10);

    if (find_png_in_dir (CHASE_SPECTRAL_OUT_DIR, r->png_path, sizeof (r->png_path)))
        r->png_count = 1;
}

static void
test_chase_spectral_exports_png (void)
{
    CvdResult r = {0};
    sii_log_handler_install (FALSE);

    int status = test_app_runner_run ("org.lrose.soloiv.test.cvd_chase",
                                      chase_spectral_action, &r);
    g_assert_cmpint (status, ==, 0);
    g_assert_cmpint (r.apply_ok, ==, 1);
    g_assert_cmpint (r.png_count, ==, 1);

    /* Make sure the file is non-trivial — empty/0-byte means cairo wrote
     * nothing real. PNG header alone is ~8 bytes; a real frame export is
     * tens to hundreds of KB. */
    GStatBuf st;
    g_assert_cmpint (g_stat (r.png_path, &st), ==, 0);
    g_assert_cmpint ((int)st.st_size, >, 1024);

    g_message ("chase_spectral PNG: %s (%lld bytes)",
               r.png_path, (long long)st.st_size);
}

/* ------------------------------------------------------------------ */

int main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/cvd/colormaps_registered",   test_cvd_colormaps_registered);
    g_test_add_func ("/cvd/chase_spectral_exports", test_chase_spectral_exports_png);
    return g_test_run ();
}
