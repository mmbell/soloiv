/* test_plot_buffers_wide.c
 *
 * Regression for issue #14: "SoloIV crashes when manually reloading a sweep
 * after applying QC to all sweeps."
 *
 * Root cause was heap corruption on the plot path, not anything in the reload
 * itself. Two per-frame buffers are indexed by *uniform cell number*:
 *
 *   wwptr->data_cell_lut   uniform cell -> real cell number
 *   wwptr->cell_colors     uniform cell -> color
 *
 * Both were allocated once at frame-creation time with fixed sizes, and
 * cell_colors was allocated as `malloc(2048*sizeof(int))` while being typed
 * and indexed as `unsigned long` -- on LP64 that is 1024 usable slots, not
 * 2048. solo_color_cells() writes wwptr->number_cells entries into it on
 * EVERY ray, and a real surveillance sweep has more cells than that (the
 * SEAPOL volume in the report has 1197 gates). So every plot scribbled past
 * the end of the allocation. The damage is small and lands in adjacent heap,
 * so it stays silent until the allocator next does real work on that region --
 * which is exactly what "Sweepfiles -> Sweeps -> open another file" does.
 *
 * The fix makes solo_cell_lut() size both buffers for the sweep it is about
 * to describe (solo_ensure_cell_capacity, sp_basics.c) and bounds its own
 * fill loops.
 *
 * This test installs a deliberately wide range geometry -- more uniform cells
 * than either old fixed limit -- and drives the real solo_cell_lut() and
 * solo_color_cells(). Under ASan the pre-fix code fails here with
 * heap-buffer-overflow in solo_color_cells; post-fix the capacity invariant
 * holds and the write stays in bounds.
 *
 * cwd = tests/fixtures/ship/cf1.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "solo_window_structs.h"
#include "test_app_runner.h"

/* Wide enough to blow past both historical limits: the 1024 real slots in
 * cell_colors and the 4096 entries in data_cell_lut. */
#define WIDE_GATES   5000
#define GATE_SPACING 100.0f     /* metres */

struct dd_general_info *dd_window_dgi();
WW_PTR solo_return_wwptr();
void solo_cell_lut();
void solo_color_cells();
int dd_alloc_data_field();

/* Re-describe the loaded sweep with WIDE_GATES uniformly spaced gates, and
 * resize the per-field data buffers to match so the color loop reads real
 * memory. This is the geometry half of a wide sweep; the values are whatever
 * the fixture left behind, which is all solo_color_cells needs. */
static void widen_geometry(struct dd_general_info *dgi)
{
    DDS_PTR dds = dgi->dds;
    int i, pn;

    for (i = 0; i < WIDE_GATES; i++)
        dds->celv->dist_cells[i] = (float)(i + 1) * GATE_SPACING;
    dds->celv->number_cells = WIDE_GATES;

    memcpy(dds->celvc, dds->celv, sizeof(struct cell_d));

    for (pn = 0; pn < MAX_PARMS; pn++) {
        if (!dds->field_present[pn]) continue;
        dds->number_cells[pn] = WIDE_GATES;
        dds->parm[pn]->number_cells = WIDE_GATES;
        dd_alloc_data_field(dgi, pn);
    }
    dgi->clip_gate = WIDE_GATES - 1;
}

static void wide_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    WW_PTR wwptr;
    int wide_cells, narrow_capacity;

    (void)main_window;
    (void)user_data;

    test_app_runner_pump(20);

    wwptr = solo_return_wwptr(0);
    g_assert_nonnull(wwptr);
    dgi = dd_window_dgi(wwptr->lead_sweep->window_num, "");
    g_assert_nonnull(dgi);
    g_assert_cmpint(dgi->source_fmt, ==, CFRADIAL_FMT);

    widen_geometry(dgi);

    /* Rebuild the uniform-cell lookup for the widened geometry. */
    wwptr->uniform_cell_spacing = 0;
    solo_cell_lut(0);

    wide_cells = wwptr->number_cells;

    /* The geometry really is wider than both historical fixed limits --
     * otherwise this test would pass for the wrong reason. */
    g_assert_cmpint(wide_cells, >, 1024);   /* old cell_colors slot count */
    g_assert_cmpint(wide_cells, >, 4096);   /* old data_cell_lut entries  */

    /* The invariant that was violated: the buffers must hold every cell
     * solo_color_cells is about to write. */
    g_assert_cmpint(wwptr->cell_capacity, >=, wide_cells);
    g_assert_nonnull(wwptr->cell_colors);
    g_assert_nonnull(wwptr->data_cell_lut);

    /* The actual overflowing write. Pre-fix this is where ASan aborts. */
    solo_color_cells(0);

    /* A narrower sweep afterwards must not shrink the buffers -- the frame
     * may be re-plotted at the wider geometry without another solo_cell_lut. */
    narrow_capacity = wwptr->cell_capacity;
    dgi->dds->celv->number_cells = 64;
    memcpy(dgi->dds->celvc, dgi->dds->celv, sizeof(struct cell_d));
    wwptr->uniform_cell_spacing = 0;
    solo_cell_lut(0);
    g_assert_cmpint(wwptr->cell_capacity, >=, narrow_capacity);
    g_assert_cmpint(wwptr->number_cells, <=, wwptr->cell_capacity);

    test_app_runner_pump(10);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.pbw",
                               wide_action, NULL);
}
