/* test_last_gate_raster.c
 *
 * The outermost range gate of every sweep was never drawn.
 *
 * Reported against an airborne tail-radar volume (Hurricane Michael, N42RF)
 * whose DBZ field carries a ring of >60 dBZ junk in the last gate only. Other
 * viewers show the ring; soloiv did not, and remove-ring appeared to do
 * nothing because there was nothing visible to remove.
 *
 * solo_cell_lut() maps the last uniform cell to the last real gate correctly.
 * The rasterizer then lost it twice over (sii_xyraster.c):
 *
 *  - ray_raster_setup() ended the ray at r0 + max_gate*gs, the CENTER of the
 *    last cell, not its outer edge, so at most half the cell was in the ray.
 *  - The inner loops truncate a 16.16 gate number that GATE_NUM() has already
 *    offset by +0.5, so the valid domain is [0, max_gate+1). The end-of-ray
 *    trims cut at gate_num > max_gate and then dropped one step more, which
 *    removed every pixel belonging to the last cell.
 *
 * This test installs a uniform synthetic geometry, zooms in so the last cell
 * spans several pixels, rasterizes rays at angles that exercise both inner
 * loops and both traversal directions, and requires that:
 *
 *  - the last cell's color appears on every ray, within the last gate's
 *    annulus [r_last - gs/2, r_last + gs/2] (plus a pixel of slop);
 *  - nothing is painted past that annulus;
 *  - no stale color from beyond number_cells in cell_colors reaches the image
 *    (solo_color_cells pads the cells just past the end with background).
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
#include "xyraster.h"
#include "test_app_runner.h"
#include "soloiv_core_gtk_protos.h"
#include "s2_protos.h"
#include "soloedit_protos.h"

#define GATE_SPACING 100.0f     /* metres */

#define COLOR_BODY   1          /* every cell but the last */
#define COLOR_LAST   2          /* the last cell */
#define COLOR_STALE  3          /* cell_colors past number_cells */


static void raster_action(GtkWidget *main_window, gpointer user_data)
{
    static const double angles[] = {
        3, 20, 44, 46, 70, 95, 130, 160, 185, 225, 250, 290, 320, 355
    };
    struct dd_general_info *dgi;
    WW_PTR wwptr;
    struct xyras *rxy;
    unsigned char *image;
    int w, h, r_px, ngates, nc, ii, ai, row, col;
    double mpp, r_last, r_inner, r_outer, slop;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    dgi = dd_window_dgi(wwptr->lead_sweep->window_num);
    g_assert_nonnull(dgi);
    g_assert_nonnull(wwptr->image);

    w = wwptr->view->width_in_pix;
    h = wwptr->view->height_in_pix;
    g_assert_cmpint(w, >, 64);
    g_assert_cmpint(h, >, 64);

    /* Size the geometry to the frame: radar at the center, the whole sweep
     * inside ~90% of the half-height, and about four pixels per gate so the
     * last cell is unambiguously resolvable. Gate 0 sits at the radar, like
     * the reported airborne data. */
    r_px = (int)(0.45 * (w < h ? w : h));
    ngates = r_px / 4 - 1;
    if (ngates > dgi->dds->number_cells[wwptr->parameter_num])
        ngates = dgi->dds->number_cells[wwptr->parameter_num];
    g_assert_cmpint(ngates, >=, 8);

    for (ii = 0; ii < ngates; ii++)
        dgi->dds->celv->dist_cells[ii] = (float)ii * GATE_SPACING;
    dgi->dds->celv->number_cells = ngates;
    memcpy(dgi->dds->celvc, dgi->dds->celv, sizeof(struct cell_d));

    r_last = dgi->dds->celv->dist_cells[ngates - 1];
    mpp = (r_last + GATE_SPACING) / r_px;
    g_assert_cmpfloat(mpp, <, GATE_SPACING / 3.);

    wwptr->center_of_view->range = 0;
    wwptr->center_of_view->rotation_angle = 0;
    wwptr->view->magnification = sp_meters_per_pixel() / mpp;
    wwptr->constrained_rasterz = NO;

    wwptr->uniform_cell_spacing = 0;
    solo_cell_lut(0);
    nc = wwptr->number_cells;
    g_assert_cmpint(nc, >, 1);
    g_assert_cmpint(wwptr->data_cell_lut[nc - 1], ==, ngates - 1);

    /* The test colors must be distinguishable from the background that
     * solo_color_cells pads past the last cell with. */
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_BODY);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_LAST);
    g_assert_cmpint(wwptr->background_color_num, !=, COLOR_STALE);

    init_xyraster(0);
    rxy = return_xyras(0);
    mpp = rxy->meters_per_pixel;
    image = (unsigned char *)rxy->image;
    g_assert_nonnull(image);

    g_printerr("frame %dx%d, %d gates of %.0f m, %.2f m/pixel\n",
               w, h, ngates, GATE_SPACING, mpp);

    r_inner = r_last - 0.5 * GATE_SPACING;
    r_outer = r_last + 0.5 * GATE_SPACING;
    slop = 1.5 * mpp;

    for (ai = 0; ai < (int)G_N_ELEMENTS(angles); ai++) {
        double a0 = angles[ai], a1 = angles[ai] + 1.5;
        int n_last = 0, n_beyond = 0, n_stale = 0;

        memset(image, 0, (size_t)w * h);

        if (ray_raster_setup(0, a0, a1, rxy) < 0 || rxy->ignore_this_ray)
            g_error("ray %.1f-%.1f was not rasterized", a0, a1);

        /* Let solo_color_cells fill (and guard) the buffer as production
         * does, with everything past number_cells pre-poisoned, then repaint
         * the real cells with a pattern that singles out the last one. */
        for (ii = nc; ii < wwptr->cell_capacity; ii++)
            wwptr->cell_colors[ii] = COLOR_STALE;
        solo_color_cells(0);
        for (ii = 0; ii < nc; ii++)
            wwptr->cell_colors[ii] = COLOR_BODY;
        wwptr->cell_colors[nc - 1] = COLOR_LAST;

        if (rxy->x_inner_loop)
            xx_inner_loop(rxy);
        else
            yy_inner_loop(rxy);

        for (row = 0; row < h; row++) {
            for (col = 0; col < w; col++) {
                unsigned char c = image[row * w + col];
                double x, y, r;
                if (c != COLOR_BODY && c != COLOR_LAST && c != COLOR_STALE)
                    continue;       /* untouched, or padded with background */
                x = rxy->image_l_left.x + col * mpp;
                y = rxy->image_l_left.y + (h - 1 - row) * mpp;
                r = sqrt(x * x + y * y);
                if (c == COLOR_STALE) {
                    n_stale++;
                    continue;
                }
                if (r > r_outer + slop)
                    n_beyond++;
                if (c == COLOR_LAST) {
                    n_last++;
                    if (r < r_inner - slop || r > r_outer + slop)
                        g_error("ray %.1f: last-gate pixel at %.1f m, outside"
                                " [%.1f, %.1f]", a0, r, r_inner, r_outer);
                }
            }
        }
        if (n_last == 0)
            g_error("ray %.1f-%.1f (%s loop): last gate (%.0f m) not drawn",
                    a0, a1, rxy->x_inner_loop ? "x" : "y", r_last);
        g_assert_cmpint(n_beyond, ==, 0);
        g_assert_cmpint(n_stale, ==, 0);
    }

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.lgr",
                               raster_action, NULL);
}
