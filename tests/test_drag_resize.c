/* test_drag_resize.c
 *
 * Window drag-resize: the main window is resizable, and when the frame
 * table's allocation settles the layout is divided across the new area.
 * Each frame keeps its center and magnification (km per pixel) and simply
 * shows more or less area; it is replotted exactly once per settled size.
 *
 * Drives the settle path directly (sii_apply_table_allocation) and through
 * the debounce (sii_schedule_table_allocation, as the drawing areas'
 * "resize" handler does), then checks that the layout and Zoom menus still
 * work afterwards and that painting at the new size stays inside the image
 * (the blit is a read, so overruns show under the debug-asan preset).
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "soloii.h"
#include "sii_externals.h"
#include "solo_window_structs.h"
#include "sii_log_handler.h"
#include "soloiv_core_gtk_protos.h"
#include "test_app_runner.h"
#include "widget_walker.h"

typedef struct {
    float  mag;
    double lat, lon, rng;
} FrameView;

/* Run the main loop for about ms milliseconds, so timeouts get to fire. */
static void wait_ms(int ms)
{
    gint64 end = g_get_monotonic_time() + (gint64)ms * 1000;

    while (g_get_monotonic_time() < end) {
        while (g_main_context_iteration(NULL, FALSE))
            ;
        g_usleep(2000);
    }
}

static int span_w(int fn)
{
    sii_table_parameters *stp = &frame_configs[fn]->tbl_parms;
    return stp->right_attach - stp->left_attach;
}

static int span_h(int fn)
{
    sii_table_parameters *stp = &frame_configs[fn]->tbl_parms;
    return stp->bottom_attach - stp->top_attach;
}

static void snap_views(FrameView *v)
{
    for (guint fn = 0; fn < sii_frame_count; fn++) {
        WW_PTR wwptr = solo_return_wwptr(fn);
        v[fn].mag = wwptr->view->magnification;
        v[fn].lat = wwptr->center_of_view->latitude;
        v[fn].lon = wwptr->center_of_view->longitude;
        v[fn].rng = wwptr->center_of_view->range;
    }
}

static void assert_views_unchanged(const FrameView *v)
{
    for (guint fn = 0; fn < sii_frame_count; fn++) {
        WW_PTR wwptr = solo_return_wwptr(fn);
        g_assert_cmpfloat(wwptr->view->magnification, ==, v[fn].mag);
        g_assert_cmpfloat(wwptr->center_of_view->latitude, ==, v[fn].lat);
        g_assert_cmpfloat(wwptr->center_of_view->longitude, ==, v[fn].lon);
        g_assert_cmpfloat(wwptr->center_of_view->range, ==, v[fn].rng);
    }
}

static void snap_plot_counts(guint *counts)
{
    for (guint fn = 0; fn < sii_frame_count; fn++)
        counts[fn] = frame_configs[fn]->plot_request_count;
}

static void assert_plot_deltas(const guint *before, guint expected)
{
    for (guint fn = 0; fn < sii_frame_count; fn++)
        g_assert_cmpuint(frame_configs[fn]->plot_request_count - before[fn],
                         ==, expected);
}

/* Every frame is span x cell, its image holds it, and the rasterizer was
 * told the new size. */
static void assert_frames_sized(guint cell_w, guint cell_h)
{
    g_assert_cmpuint(sii_table_widget_width, ==, cell_w);
    g_assert_cmpuint(sii_table_widget_height, ==, cell_h);

    for (guint fn = 0; fn < sii_frame_count; fn++) {
        SiiFrameConfig *sfc = frame_configs[fn];
        WW_PTR wwptr = solo_return_wwptr(fn);
        guint w = span_w(fn) * cell_w;
        guint h = span_h(fn) * cell_h;

        g_assert_cmpuint(sfc->width, ==, w);
        g_assert_cmpuint(sfc->height, ==, h);
        g_assert_cmpuint(sfc->data_width, ==, w);
        g_assert_cmpuint(sfc->data_height, ==, h);
        g_assert_nonnull(sfc->image);
        g_assert_cmpuint(sfc->max_image_size, >=, w * h);
        g_assert_cmpint(wwptr->view->width_in_pix, ==, (int)w);
        g_assert_cmpint(wwptr->view->height_in_pix, ==, (int)h);
    }
}

/* Paint each frame at its widget size plus an overhang. */
static void paint_all_frames(void)
{
    for (guint fn = 0; fn < sii_frame_count; fn++) {
        SiiFrameConfig *sfc = frame_configs[fn];
        int w = sfc->width + 64, h = sfc->height + 64;
        cairo_surface_t *surface =
            cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
        cairo_t *cr = cairo_create(surface);
        GdkRectangle area = { 0, 0, w, h };

        sii_really_xfer_images_cairo(cr, fn, &area, FALSE);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }
}

static void fire(GtkWidget *main_win, const char *name, int param)
{
    GVariant *p = g_variant_new_int32(param);

    g_variant_ref_sink(p);
    g_assert_true(widget_walker_fire_action(main_win, "menu", name, p));
    g_variant_unref(p);
}

static void resize_action(GtkWidget *main_win, gpointer user_data)
{
    guint baseline, counts[maxFrames];
    FrameView views[maxFrames];
    guint ncols, nrows, cell_w, old_cell_w;
    int table_w, table_h;

    (void)user_data;

    g_assert_true(gtk_window_get_resizable(GTK_WINDOW(main_win)));

    wait_ms(500);                       /* initial plots have settled */
    baseline = sii_log_handler_get_complaint_count();

    g_assert_cmpuint(sii_frame_count, ==, 4);
    for (guint fn = 0; fn < sii_frame_count; fn++)
        g_assert_cmpuint(frame_configs[fn]->plot_request_count, >, 0);

    /* --- 2x2: apply a 1000x500 table ---------------------------------- */
    snap_views(views);
    snap_plot_counts(counts);
    sii_apply_table_allocation(1000, 500);
    assert_frames_sized(500, 248);      /* 250 rounds down to 248 */
    assert_plot_deltas(counts, 1);
    assert_views_unchanged(views);
    paint_all_frames();

    /* the same size again is a no-op: no replot */
    snap_plot_counts(counts);
    sii_apply_table_allocation(1000, 500);
    assert_plot_deltas(counts, 0);
    assert_frames_sized(500, 248);

    /* --- shrinking below the minimum clamps to it --------------------- */
    snap_plot_counts(counts);
    sii_apply_table_allocation(100, 100);
    assert_frames_sized(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    assert_plot_deltas(counts, 1);
    assert_views_unchanged(views);
    paint_all_frames();

    /* --- rapid allocations through the debounce: one replot ----------- */
    snap_plot_counts(counts);
    sii_schedule_table_allocation(700, 640);
    test_app_runner_pump(5);
    sii_schedule_table_allocation(800, 700);
    test_app_runner_pump(5);
    sii_schedule_table_allocation(900, 720);
    test_app_runner_pump(5);
    assert_plot_deltas(counts, 0);      /* nothing until it settles */
    wait_ms(500);
    assert_plot_deltas(counts, 1);
    assert_frames_sized(448, 360);
    assert_views_unchanged(views);
    paint_all_frames();

    /* --- uneven layout: 1 big frame + 2 small ------------------------- */
    fire(main_win, "ccfg_1b2vs", 1012);
    wait_ms(600);
    g_assert_cmpuint(sii_frame_count, ==, 3);
    g_assert_cmpint(span_w(0), ==, 2);
    g_assert_cmpint(span_h(0), ==, 2);
    ncols = frame_configs[0]->ncols;
    nrows = frame_configs[0]->nrows;
    g_assert_cmpuint(ncols, ==, 3);
    g_assert_cmpuint(nrows, ==, 2);
    assert_frames_sized(sii_table_widget_width, sii_table_widget_height);

    snap_views(views);
    snap_plot_counts(counts);
    sii_apply_table_allocation(1200, 600);
    assert_frames_sized(400, 300);
    assert_plot_deltas(counts, 1);
    assert_views_unchanged(views);
    paint_all_frames();

    /* --- layout and Zoom menus still work after a resize -------------- */
    fire(main_win, "cfg_22", 22);
    wait_ms(600);
    g_assert_cmpuint(sii_frame_count, ==, 4);
    assert_frames_sized(sii_table_widget_width, sii_table_widget_height);

    old_cell_w = sii_table_widget_width;
    snap_plot_counts(counts);
    fire(main_win, "zoom_cfg_p25", 25);
    wait_ms(800);
    cell_w = sii_table_widget_width;
    g_message("[drag_resize] zoom +25%%: cell width %u -> %u", old_cell_w,
              cell_w);
    g_assert_cmpuint(cell_w, >, old_cell_w);
    assert_frames_sized(sii_table_widget_width, sii_table_widget_height);
    for (guint fn = 0; fn < sii_frame_count; fn++)
        g_assert_cmpuint(frame_configs[fn]->plot_request_count, >, counts[fn]);

    /* the frames follow the table's real allocation */
    table_w = gtk_widget_get_width(main_table);
    table_h = gtk_widget_get_height(main_table);
    g_message("[drag_resize] table %dx%d cell %ux%u", table_w, table_h,
              sii_table_widget_width, sii_table_widget_height);
    g_assert_cmpint(table_w, >=, 2 * (int)sii_table_widget_width);
    g_assert_cmpint(table_h, >=, 2 * (int)sii_table_widget_height);
    g_assert_cmpint(table_w - 2 * (int)sii_table_widget_width, <, 2 * 4);
    g_assert_cmpint(table_h - 2 * (int)sii_table_widget_height, <, 2 * 4);
    paint_all_frames();

    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, baseline);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.dragresize",
                               resize_action, NULL);
}
