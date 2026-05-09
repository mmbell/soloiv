/* test_param_minmax_round_trip.c
 *
 * Regression test for the MIN/MAX -> CTR/INC bug in
 * sii_param_process_changes (perusal/sii_param_widgets.c).
 *
 * Bug: when the user edited the Min/Max field and clicked OK, the
 * MINMAX-changed branch read pd->txt[PARAM_CTRINC] (stale ctr/inc) and
 * stored those values into pal->minmax, then derived ctr/inc from the
 * garbage. The symmetric CTRINC-changed branch read the right field.
 *
 * This test drives the same code path as the OK button: opens the
 * parameter widget, types known values into the Min/Max entry, runs
 * the change pipeline, and asserts that pal->minmax matches what was
 * typed and pal->ctrinc is the correct centre/increment for that range.
 *
 * Pre-fix this test fails because pal->minmax ends up holding the
 * pre-edit ctr/inc (e.g. 0, 5) instead of the typed (-30, 60).
 */

#include <gtk/gtk.h>
#include <math.h>

#include "sii_log_handler.h"
#include "test_app_runner.h"
#include "soloii.h"

void show_param_widget (GtkWidget *text, gpointer data);

typedef struct {
    int    sim_ok;
    int    state_ok;
    gfloat in_min, in_max;
    gfloat out_min, out_max;
    gfloat out_ctr, out_inc;
    guint  ncolors;
} Result;

static void
minmax_action (GtkWidget *main_window, gpointer user_data)
{
    Result *r = (Result *)user_data;
    (void)main_window;

    /* Open the parameter widget for frame 0 — populates pd->pal and
     * pd->data_widget[PARAM_MINMAX]. */
    show_param_widget (NULL, GINT_TO_POINTER (0));
    test_app_runner_pump (20);

    /* Pick values with no resemblance to any plausible default ctr/inc
     * so the bug shows clearly: pre-fix the buggy code would store
     * stale ctr/inc into pal->minmax, which would be nothing like
     * -30 / 60. */
    r->in_min = -30.0f;
    r->in_max =  60.0f;

    r->sim_ok = sii_param_widget_simulate_minmax (0, r->in_min, r->in_max) ? 1 : 0;
    test_app_runner_pump (5);

    gfloat mm[2] = {0, 0}, ci[2] = {0, 0};
    r->state_ok = sii_palette_get_state (0, mm, ci, &r->ncolors) ? 1 : 0;
    r->out_min = mm[0];  r->out_max = mm[1];
    r->out_ctr = ci[0];  r->out_inc = ci[1];
}

static void
test_minmax_edit_round_trips (void)
{
    Result r = {0};
    sii_log_handler_install (FALSE);

    int status = test_app_runner_run ("org.lrose.soloiv.test.minmax_rt",
                                      minmax_action, &r);
    g_assert_cmpint (status, ==, 0);
    g_assert_cmpint (r.sim_ok,   ==, 1);
    g_assert_cmpint (r.state_ok, ==, 1);
    g_assert_cmpuint (r.ncolors,  >, 0);

    /* The typed values must land in pal->minmax. The text round-trips
     * through 3-decimal printing so a 0.01 tolerance is generous. */
    g_assert_cmpfloat (fabsf (r.out_min - r.in_min), <=, 0.01f);
    g_assert_cmpfloat (fabsf (r.out_max - r.in_max), <=, 0.01f);

    /* ctr should be the midpoint of the typed range. CB_ROUND adds
     * a 1e-6 fudge per axis, well below our tolerance. */
    gfloat expected_ctr = 0.5f * (r.in_min + r.in_max);
    g_assert_cmpfloat (fabsf (r.out_ctr - expected_ctr), <=, 0.5f);

    /* inc * ncolors should span the typed range. */
    gfloat span = r.out_inc * (gfloat)r.ncolors;
    gfloat expected_span = r.in_max - r.in_min;
    g_assert_cmpfloat (fabsf (span - expected_span), <=, 0.5f);

    g_message ("min=%g max=%g ctr=%g inc=%g ncolors=%u (expected ctr=%g, span=%g)",
               r.out_min, r.out_max, r.out_ctr, r.out_inc, r.ncolors,
               expected_ctr, expected_span);
}

int
main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/param_widget/minmax_round_trip",
                     test_minmax_edit_round_trips);
    return g_test_run ();
}
