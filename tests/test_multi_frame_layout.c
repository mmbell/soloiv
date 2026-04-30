/* test_multi_frame_layout.c
 *
 * Switches the layout via menu actions (cfg_22 = 2x2, cfg_14 = 1x4) and
 * asserts no crash and no new complaints during the layout transitions.
 *
 * The default startup layout is determined by soloiv_activate (currently
 * 2x2 already), so this exercises layout SWITCHING — a known source of
 * latent state bugs since each frame's draw / paint state has to be
 * rebuilt.
 */

#include <gtk/gtk.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "widget_walker.h"

typedef struct {
    guint baseline;
    int   activated;
} LayoutResult;

static void
layout_action(GtkWidget *main_window, gpointer user_data)
{
    LayoutResult *r = (LayoutResult *)user_data;

    r->baseline = sii_log_handler_get_complaint_count();

    /* Try a sequence of layouts that the default menu offers. Each one
     * forces frame_configs and the parameter widgets to rebuild. The
     * cfg_* actions take an int32 parameter matching the layout ID —
     * the menu hard-codes them so we pass them explicitly here. */
    static const struct { const char *name; int param; } layouts[] = {
        { "cfg_11", 11 },   /* 1x1 single frame                 */
        { "cfg_22", 22 },   /* 2x2 — back to default            */
        { "cfg_14", 14 },   /* 1x4 horizontal strip             */
        { "cfg_41", 41 },   /* 4x1 vertical strip               */
        { "cfg_22", 22 },   /* finish back at 2x2 for stability */
    };
    const int n_layouts = (int)(sizeof(layouts) / sizeof(layouts[0]));

    int fired = 0;
    for (int i = 0; i < n_layouts; i++) {
        GVariant *p = g_variant_new_int32(layouts[i].param);
        g_variant_ref_sink(p);
        if (widget_walker_fire_action(main_window, "menu",
                                      layouts[i].name, p)) {
            fired++;
            test_app_runner_pump(10);
        }
        g_variant_unref(p);
    }
    r->activated = fired;
}

static void
test_multi_frame_layout_no_crash(void)
{
    LayoutResult r = { 0 };
    int status = test_app_runner_run("org.lrose.soloiv.test.layout",
                                     layout_action, &r);
    g_assert_cmpint(status, ==, 0);

    /* All five layout transitions should have fired. If any failed to
     * fire, the action lookup is broken — surface that as a failure. */
    g_assert_cmpint(r.activated, ==, 5);

    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, r.baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/multi_frame/layout_no_crash",
                    test_multi_frame_layout_no_crash);
    return g_test_run();
}
