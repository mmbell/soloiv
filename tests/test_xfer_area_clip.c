/* test_xfer_area_clip.c
 *
 * Painting a frame must stay inside the frame's image, whatever area the
 * draw callback hands it.
 *
 * sii_frame_draw_func passes the widget's full size as the expose area, and
 * sii_really_xfer_images_cairo copied that rectangle out of the frame image.
 * While a multi-frame layout is being reconfigured the widget can be larger
 * than the image it is about to get, so the copy read past the end of the
 * image buffer -- an intermittent heap-buffer-overflow that ASan caught in
 * test_multi_frame_layout on Linux CI.
 *
 * Plots the ground CfRadial fixture, then asks frame 0 to paint areas that
 * overhang its image on the right, the bottom and both. The overflow is a
 * read, so this test catches the regression under the debug-asan preset.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include "soloii.h"
#include "sii_externals.h"
#include "test_app_runner.h"

void sii_really_xfer_images_cairo(cairo_t *cr, int frame_num,
                                  GdkRectangle *area, gboolean blow_up);

static void paint_area(int x, int y, int w, int h)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    GdkRectangle area = { x, y, w, h };

    surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, x + w, y + h);
    cr = cairo_create(surface);
    sii_really_xfer_images_cairo(cr, 0, &area, FALSE);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void clip_action(GtkWidget *main_window, gpointer user_data)
{
    SiiFrameConfig *sfc;
    int w, h;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(40);

    sfc = frame_configs[0];
    g_assert_nonnull(sfc);
    g_assert_nonnull(sfc->image);
    w = sfc->width;
    h = sfc->height;
    g_assert_cmpint(w * h, >, 99);
    /* the image is colorized, so the paint really copies pixels */
    g_assert_cmpint(sfc->colorize_count, >, 0);
    g_message("[xfer_clip] frame 0 image %dx%d", w, h);

    paint_area(0, 0, w + 40, h);        /* wider than the image */
    paint_area(0, 0, w, h + 40);        /* taller */
    paint_area(0, 0, w + 40, h + 40);   /* both */
    paint_area(w / 2, h / 2, w, h);     /* offset, overhanging */
    paint_area(w + 10, h + 10, 20, 20); /* entirely outside */

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.xferclip",
                               clip_action, NULL);
}
