/* test_remove_ring_last_gate.c
 *
 * remove-ring must reach the last gate of the ray.
 *
 * se_ring_zap turns its ranges into gates with dd_cell_num, which looks them
 * up in the uniform-cell table dd_uniform_cell_spacing builds. That builder
 * stepped one source cell behind and stamped the final table entry with the
 * second-to-last gate, so no range ever mapped to the last gate and
 * "remove-ring in DBZ from 46.9 to 47.0 km." on a 46.95 km ray left the ring
 * at the outermost gate untouched.
 *
 * Loads the committed N42RF-TM airborne CfRadial-2 fixture, checks dd_cell_num
 * maps every gate's own range back to that gate, then removes a ring spanning
 * just the last gate and requires that gate -- and only it -- to go bad.
 *
 * cwd = tests/fixtures/airborne/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "radar_io.h"
#include "solo_editor_structs.h"
#include "ui.h"
#include "test_app_runner.h"

#define CFRAD_FILE "cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"
#define MAXG 8192

struct dd_general_info *dd_window_dgi();
struct solo_edit_stuff *return_sed_stuff();
int dd_cell_num();
int dd_find_field();
int se_ring_zap();

static void ring_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    struct solo_edit_stuff *seds;
    struct cell_d *celv;
    struct ui_command cmds[4];
    unsigned short *mask;
    short *ss, *before;
    char fname[16];
    float gs, r_last;
    int nrays, nc, fn, ii, bad;
    int win = 4;

    (void)main_window;
    (void)user_data;

    dgi = dd_window_dgi(win, "");
    g_strlcpy(dgi->directory_name, "./", sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, CFRAD_FILE, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;

    nrays = dd_absorb_header_info(dgi);
    g_assert_cmpint(nrays, >, 10);
    g_assert_cmpint(dd_absorb_ray_info(dgi), >=, 1);
    celv = dgi->dds->celvc;
    nc = celv->number_cells;
    g_assert_cmpint(nc, >, 2);
    g_assert_cmpint(nc, <=, MAXG);
    g_assert_cmpint(dgi->clip_gate, ==, nc - 1);

    /* every gate's own range must look up to that gate */
    for (ii = 0; ii < nc; ii++) {
        int g = dd_cell_num(dgi->dds, 0, celv->dist_cells[ii]);
        if (g != ii)
            g_error("dd_cell_num(%.1f m) = %d, want gate %d of %d",
                    celv->dist_cells[ii], g, ii, nc);
    }

    fn = dd_find_field(dgi, "DBZ");
    g_assert_cmpint(fn, >=, 0);
    g_strlcpy(fname, "DBZ", sizeof(fname));
    ss = (short *)dgi->dds->qdat_ptrs[fn];
    bad = dgi->dds->parm[fn]->bad_data;
    /* a known value in the last gate, so deleting it is visible */
    ss[nc - 1] = ss[nc - 2] = 1234;
    before = g_memdup2(ss, nc * sizeof(short));

    seds = return_sed_stuff();
    seds->se_frame = win;
    seds->finish_up = NO;
    seds->punt = NO;
    seds->boundary_exists = YES;
    mask = g_malloc0(sizeof(unsigned short) * nc);
    for (ii = 0; ii < nc; ii++) mask[ii] = 1;
    seds->boundary_mask = mask;

    /* remove-ring in DBZ from r_last-gs/3 to r_last+gs/3 km. */
    gs = celv->dist_cells[1] - celv->dist_cells[0];
    r_last = celv->dist_cells[nc - 1];
    memset(cmds, 0, sizeof(cmds));
    cmds[0].uc_ctype = UTT_KW;
    cmds[0].uc_text = "remove-ring";
    cmds[1].uc_ctype = UTT_FIELD;
    cmds[1].uc_text = fname;
    cmds[2].uc_ctype = UTT_VALUE;
    cmds[2].uc_v.us_v_float = (r_last - gs / 3.) * .001;
    cmds[3].uc_ctype = UTT_VALUE;
    cmds[3].uc_v.us_v_float = (r_last + gs / 3.) * .001;
    g_message("[remove_ring] %d gates, gs %.1f m; ring %.4f to %.4f km",
              nc, gs, cmds[2].uc_v.us_v_float, cmds[3].uc_v.us_v_float);

    g_assert_cmpint(se_ring_zap(3, cmds), ==, fn);

    if (ss[nc - 1] != bad)
        g_error("last gate (%.1f m) not removed", r_last);
    for (ii = 0; ii < nc - 1; ii++)
        if (ss[ii] != before[ii])
            g_error("gate %d (%.1f m) changed; only the last gate should",
                    ii, celv->dist_cells[ii]);

    g_free(before);
    g_free(mask);
    seds->boundary_mask = NULL;
    seds->boundary_exists = NO;
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.ringlast",
                               ring_action, NULL);
}
