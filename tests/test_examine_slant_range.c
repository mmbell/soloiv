/* test_examine_slant_range.c
 *
 * The data widget's range column must be slant range -- the range the display
 * plots and the editor commands (remove-ring, ...) take.
 *
 * solo_new_list_field_vals used to scale every listed range by cos(tilt)
 * (cos(el) for RHI). For an airborne tail radar tilted ~20 degrees that
 * listed the last gate of a 47 km sweep at ~44 km and the last real echo at
 * ~43 km, so a user reading the widget concluded the sweep stopped short and
 * aimed "remove-ring ... from 45 km" at the wrong place.
 *
 * Loads the committed N42RF-TM airborne CfRadial-2 fixture, plants a known
 * set of gate ranges with a 20 degree tilt in the field-values ring, builds
 * the list, and requires each listed range to equal the planted slant range.
 *
 * cwd = tests/fixtures/airborne/cf2.
 */

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "solo_window_structs.h"
#include "test_app_runner.h"

#define TILT_DEG   20.0f
#define LAST_GATE  46950.0f     /* metres, as in the reported volume */
#define GATE_SPACING 75.0f

struct dd_general_info *dd_window_dgi();
WW_PTR solo_return_wwptr();
void solo_new_list_field_vals();

static void slant_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    WW_PTR wwptr;
    struct solo_field_vals *sfv, *s;
    struct solo_list_mgmt *slm;
    float want[5];
    int ii, jj, nranges = 0;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    dgi = dd_window_dgi(wwptr->lead_sweep->window_num, "");
    g_assert_nonnull(dgi);
    g_assert_cmpint(dgi->dds->radd->scan_mode, ==, AIR);
    g_assert_false(wwptr->view->type_of_plot & SOLO_TIME_SERIES);

    sfv = wwptr->field_vals;
    g_assert_nonnull(sfv);
    g_assert_nonnull(sfv->next);
    g_assert_nonnull(sfv->next->next);

    for (ii = 0; ii < 5; ii++)
        want[ii] = LAST_GATE + (ii - 4) * GATE_SPACING;

    /* The list reads the center ray's geometry from sfv->next and the three
     * rays' values side by side. */
    for (s = sfv, jj = 0; jj < 3; jj++, s = s->next) {
        s->num_vals = 5;
        s->tilt = TILT_DEG;
        s->el = -TILT_DEG;
        s->rot_ang = 90.0f + jj;
        s->rng = LAST_GATE * .001f;
        for (ii = 0; ii < 5; ii++) {
            s->ranges[ii] = want[ii];
            s->field_vals[ii] = 60.0f + ii;
        }
    }

    solo_new_list_field_vals(0);

    slm = wwptr->list_field_vals;
    g_assert_nonnull(slm);
    for (ii = 0; ii < slm->num_entries; ii++) {
        const char *line = slm->list[ii];
        float km;
        char tail[4];
        if (!line || sscanf(line, "%fkm%1[.]", &km, tail) != 2)
            continue;
        g_assert_cmpint(nranges, <, 5);
        if (fabs(km - want[nranges] * .001) > .006)
            g_error("listed range %.2f km, want slant range %.2f km"
                    " (cos(tilt)-scaled would be %.2f)", km,
                    want[nranges] * .001,
                    want[nranges] * .001 * cos(TILT_DEG * M_PI / 180.));
        nranges++;
    }
    g_assert_cmpint(nranges, ==, 5);

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.esr",
                               slant_action, NULL);
}
