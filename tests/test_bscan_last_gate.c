/* test_bscan_last_gate.c
 *
 * The time-series (B-scan) display drew range gates in the wrong place and
 * lost the outermost one, the same family of bug as test_last_gate_raster.
 *
 * sp_tsYpixel_info() (sii_bscan.c) works out, for each frame, the first pixel
 * of a column, the cell number of that pixel and how many pixels to paint.
 * It placed the data inconsistently with the rest of the program (the click
 * and edit code take display y = range plotting up, 2*ctr - range plotting
 * down, altitude +/- range in MSL mode):
 *
 *  - not MSL-relative, the first cell was drawn at range 0 instead of at its
 *    true range uniform_cell_one, shifting the whole column by that much;
 *  - the far end was cut at rloc + rmin + nc*gs: the CENTER of the last cell,
 *    with rmin counted a second time, so MSL mode ended rmin late (and walked
 *    off the end of cell_colors) and neither mode ended at the outer edge;
 *  - the near end started at the center of the first cell, not its edge.
 *
 * This test installs a uniform synthetic geometry whose first gate is well
 * out from the radar, sizes the view so each gate spans several pixels, and
 * for each combination of plot up/down and MSL-relative or not paints one
 * column through sp_ts_column(). Every pixel must carry the color of the cell
 * whose [r - gs/2, r + gs/2) contains the pixel's range, nothing must be
 * painted outside the sweep, and no color from past number_cells in
 * cell_colors may appear. The first and last cells must be drawn when the
 * whole sweep is in view. The view is also slid so that each end of the
 * sweep is clipped by the frame.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "solo_window_structs.h"
#include "soloii.h"
#include "sii_enums.h"
#include "dd_math.h"
#include "test_app_runner.h"
#include "soloiv_core_gtk_protos.h"
#include "s2_protos.h"
#include "soloedit_protos.h"

#define GATE_SPACING 100.0f     /* metres */
#define FIRST_RANGE  250.0f     /* metres; > gs/2 so it stays the first cell */

#define COLOR_FIRST  1          /* the first cell */
#define COLOR_LAST   2          /* the last cell */
#define COLOR_EVEN   3          /* the others, alternating */
#define COLOR_ODD    4
#define COLOR_STALE  5          /* cell_colors past number_cells */


static int cell_color(int k, int nc)
{
    if (k == 0)
        return COLOR_FIRST;
    if (k == nc - 1)
        return COLOR_LAST;
    return (k & 1) ? COLOR_ODD : COLOR_EVEN;
}

static void bscan_action(GtkWidget *main_window, gpointer user_data)
{
    static const double slides[] = { 0, -.3, .3 };  /* fractions of the span */
    struct dd_general_info *dgi;
    WW_PTR wwptr;
    unsigned char *image;
    long save_type;
    float save_ctr, save_mag;
    int w, h, ngates, nc, ii, mode, si, row, col, failures = 0;
    double mpp, rmin, r_last, alt, span, mid, rctr;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    dgi = dd_window_dgi(wwptr->lead_sweep->window_num);
    g_assert_nonnull(dgi);

    w = wwptr->view->width_in_pix;
    h = wwptr->view->height_in_pix;
    g_assert_cmpint(w, >, 64);
    g_assert_cmpint(h, >, 64);

    /* about four pixels per gate, the whole sweep in ~80% of the height */
    ngates = (int)(0.2 * h);
    if (ngates > dgi->dds->number_cells[wwptr->parameter_num])
        ngates = dgi->dds->number_cells[wwptr->parameter_num];
    g_assert_cmpint(ngates, >=, 8);

    for (ii = 0; ii < ngates; ii++)
        dgi->dds->celv->dist_cells[ii] = FIRST_RANGE + (float)ii * GATE_SPACING;
    dgi->dds->celv->number_cells = ngates;
    memcpy(dgi->dds->celvc, dgi->dds->celv, sizeof(struct cell_d));

    wwptr->uniform_cell_spacing = 0;
    solo_cell_lut(0);
    nc = wwptr->number_cells;
    g_assert_cmpint(nc, >, 2);
    g_assert_cmpint(wwptr->data_cell_lut[0], ==, 0);
    g_assert_cmpint(wwptr->data_cell_lut[nc - 1], ==, ngates - 1);
    rmin = wwptr->uniform_cell_one;
    g_assert_cmpfloat(fabs(rmin - FIRST_RANGE), <, .01);
    r_last = rmin + (nc - 1) * GATE_SPACING;

    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_FIRST);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_LAST);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_EVEN);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_ODD);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_STALE);

    save_type = wwptr->view->type_of_plot;
    save_ctr = wwptr->view->ts_ctr_km;
    save_mag = wwptr->view->ts_magnification;

    span = (r_last - rmin) + GATE_SPACING;
    wwptr->view->ts_magnification = sp_meters_per_pixel() * (0.8 * h) / span;
    /* read back exactly what sp_tsYpixel_info will use */
    mpp = sp_meters_per_pixel() / wwptr->view->ts_magnification;
    g_assert_cmpfloat(mpp, <, GATE_SPACING / 3.);
    alt = KM_TO_M(dd_altitude(dgi));
    mid = .5 * (rmin + r_last);

    image = g_malloc((size_t)w * h);
    col = w / 2;

    g_printerr("frame %dx%d, %d cells of %.0f m from %.0f m, %.2f m/pixel,"
               " alt %.0f m\n", w, h, nc, GATE_SPACING, rmin, mpp, alt);

    for (mode = 0; mode < 4; mode++) {
        int msl = mode & 1, down = (mode & 2) != 0;

        for (si = 0; si < (int)G_N_ELEMENTS(slides); si++) {
            int n_first = 0, n_last = 0, n_bad = 0, n_stale = 0, n_out = 0;
            double ctr;

            /* the display coordinate of range r is
             *   up: r (+ alt if MSL)    down: 2*ctr - r, or alt - r if MSL
             * so center the view on the middle of the sweep, then slide */
            if (msl)
                ctr = down ? alt - mid : alt + mid;
            else
                ctr = mid;
            ctr += slides[si] * span;
            wwptr->view->ts_ctr_km = (float)M_TO_KM(ctr);
            rctr = KM_TO_M(wwptr->view->ts_ctr_km);
            wwptr->view->type_of_plot = SOLO_TIME_SERIES
                | (msl ? TS_MSL_RELATIVE : 0) | (down ? TS_PLOT_DOWN : 0);

            for (ii = nc; ii < wwptr->cell_capacity; ii++)
                wwptr->cell_colors[ii] = COLOR_STALE;
            solo_color_cells(0);
            for (ii = 0; ii < nc; ii++)
                wwptr->cell_colors[ii] = cell_color(ii, nc);

            memset(image, 0, (size_t)w * h);
            sp_ts_column(0, image, col, down);

            for (row = 0; row < h; row++) {
                int c = image[row * w + col];
                double y = rctr + (.5 * (h - 1) - row) * mpp;
                double r, cf, fr;
                int k, want;

                if (msl)
                    r = down ? alt - y : y - alt;
                else
                    r = down ? 2. * rctr - y : y;
                cf = (r - rmin) / GATE_SPACING + .5;
                k = (int)floor(cf);
                fr = cf - k;
                want = (k >= 0 && k < nc) ? cell_color(k, nc) : 0;

                if (c == COLOR_STALE)
                    n_stale++;
                if (c == COLOR_FIRST)
                    n_first++;
                if (c == COLOR_LAST)
                    n_last++;
                if (c == wwptr->background_color_num)
                    c = 0;
                if (c == want)
                    continue;
                /* a pixel this close to a cell edge may go either way */
                if (fr < .02 || fr > .98)
                    continue;
                if (want == 0)
                    n_out++;
                else
                    n_bad++;
                if (n_bad + n_out <= 3)
                    g_printerr("  %s%s slide %+.1f: row %d at %.1f m"
                               " (cell %d) has color %d, want %d\n",
                               down ? "down" : "up", msl ? " msl" : "",
                               slides[si], row, r, k, c, want);
            }
            g_printerr("%s%s slide %+.1f: first %d last %d px,"
                       " %d misplaced, %d outside, %d stale\n",
                       down ? "down" : "up", msl ? " msl" : "", slides[si],
                       n_first, n_last, n_bad, n_out, n_stale);
            if (n_bad || n_out || n_stale)
                failures++;
            if (slides[si] == 0 && (n_first == 0 || n_last == 0))
                failures++;
        }
    }

    wwptr->view->type_of_plot = save_type;
    wwptr->view->ts_ctr_km = save_ctr;
    wwptr->view->ts_magnification = save_mag;
    g_free(image);

    if (failures)
        g_error("%d time-series columns misdrawn", failures);

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.bslg",
                               bscan_action, NULL);
}
