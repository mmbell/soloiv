/* test_font_cache.c
 *
 * Issue #3: the per-frame text-measurement path (sii_font_string_width) used to
 * create and destroy a fresh PangoContext + PangoLayout on every call, re-
 * resolving the font through the platform backend each time. That is the frame
 * in the reported crash trace (sii_font_string_width -> pango_layout_get_pixel_size
 * -> ... -> CoreText font lookup), and on macOS the churn drove Pango's CoreText
 * font cache into a rare freed-object crash. The function now reuses a shared
 * context and a cached layout.
 *
 * This test hammers sii_font_string_width the way a busy frame draw does --
 * many calls per "frame", interleaving fonts and strings -- and checks the
 * cached path stays correct: widths are positive, grow with text length, and
 * are identical across repeated and interleaved calls (a corrupted/leaking
 * cache would drift or crash). The real on-screen draw path is exercised by
 * the GUI draw tests (test_cfradial_examine, test_multi_frame_layout).
 */

#include <gtk/gtk.h>
#include "test_app_runner.h"

extern gint  sii_font_string_width();
extern void *sii_font_new();      /* (const char *, int, gboolean) -> SiiFont* */

static void font_cache_action(GtkWidget *main_window, gpointer user_data)
{
    void *sans, *mono;
    gint wA, wAAAA, wHello, wWorld, i, frame;

    (void)main_window;
    (void)user_data;

    sans = sii_font_new("Sans", 120, FALSE);
    mono = sii_font_new("Monospace", 100, TRUE);
    g_assert_nonnull(sans);
    g_assert_nonnull(mono);

    wA     = sii_font_string_width(sans, "A");
    wAAAA  = sii_font_string_width(sans, "AAAA");
    wHello = sii_font_string_width(sans, "hello world");
    wWorld = sii_font_string_width(mono, "-99.0");
    g_assert_cmpint(wA, >, 0);
    g_assert_cmpint(wWorld, >, 0);
    g_assert_cmpint(wAAAA, >, wA);          /* longer text is wider */
    g_assert_cmpint(wHello, >, wAAAA);

    /* Many "frames", each measuring a batch of strings across both fonts (the
     * pattern the title/axis/colorbar labels follow). Every measurement must
     * return exactly the same width as the first -- the cached context/layout
     * must neither drift nor crash under sustained reuse. */
    for (frame = 0; frame < 2000; frame++) {
        g_assert_cmpint(sii_font_string_width(sans, "A"), ==, wA);
        g_assert_cmpint(sii_font_string_width(sans, "AAAA"), ==, wAAAA);
        g_assert_cmpint(sii_font_string_width(sans, "hello world"), ==, wHello);
        g_assert_cmpint(sii_font_string_width(mono, "-99.0"), ==, wWorld);
    }

    /* Interleave with fresh fonts to make sure switching descriptions on the
     * shared layout stays correct. */
    for (i = 0; i < 200; i++) {
        void *f = sii_font_new("Sans", 100 + (i % 5) * 10, FALSE);
        g_assert_cmpint(sii_font_string_width(f, "frame title"), >, 0);
    }

    g_assert_cmpint(sii_font_string_width(sans, "A"), ==, wA);
    g_message("[font_cache] 8000+ cached measurements stable");
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.fontcache",
                               font_cache_action, NULL);
}
