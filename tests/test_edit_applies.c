/* test_edit_applies.c
 *
 * Regression for issue #6: "Several editing commands appear unavailable or
 * nonfunctional in soloiv."
 *
 * Root cause was that the Radx/CfRadial read path (rio_read_ray) left
 * dgi->clip_gate at 0. Every for-each-ray editor op iterates to clip_gate+1,
 * so with clip_gate == 0 the loop touched exactly ONE gate and the edit
 * silently did nothing across the rest of the ray -- the reported symptom.
 * The fix (translate/swp_file_acc.c) sets clip_gate to the last cell after a
 * Radx read.
 *
 * This test exercises a real for-each-ray command end-to-end through its
 * registered cmd_proc (se_hard_zap, the "unconditional-delete" command) on a
 * CfRadial sweep and asserts the edit reaches a gate near the FAR end of the
 * ray -- not just gate 0. It would fail under the old clip_gate==0 bug.
 *
 * se_hard_zap deletes (sets to bad) every in-boundary gate up to clip_gate+1,
 * so we install a full-coverage boundary mask and confirm a known-good far
 * gate becomes bad after the command runs.
 *
 * cwd = tests/fixtures/ground.
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

#define CFRAD_FILE "cf2/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"

struct dd_general_info *dd_window_dgi();
struct solo_edit_stuff *return_sed_stuff();
int dd_givfld();
int se_hard_zap();

/* First present field index after `after`, or -1. */
static int first_present(struct dd_general_info *dgi, int after)
{
    int i;
    for (i = after + 1; i < MAX_PARMS; i++)
        if (dgi->dds->field_present[i]) return i;
    return -1;
}

static void field_name(struct dd_general_info *dgi, int ndx, char *out)
{
    int i;
    char *aa = dgi->dds->parm[ndx]->parameter_name;
    for (i = 0; i < 8 && aa[i] && aa[i] != ' '; i++) out[i] = aa[i];
    out[i] = '\0';
}

static void edit_applies_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    struct solo_edit_stuff *seds;
    char fname[16];
    int nrays, ncells, fndx, g1, n, i, far_idx;
    float vals[8192], bad;
    unsigned short *mask;
    struct ui_command cmds[3];
    int win = 4;

    (void)main_window;
    (void)user_data;

    /* --- read the sweep --- */
    dgi = dd_window_dgi(win, "");
    g_strlcpy(dgi->directory_name, "./", sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, CFRAD_FILE, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;

    nrays = dd_absorb_header_info(dgi);
    g_assert_cmpint(nrays, >, 10);
    ncells = dgi->dds->celv->number_cells;
    g_assert_cmpint(ncells, >, 1);

    g_assert_cmpint(dd_absorb_ray_info(dgi), >=, 1);   /* load ray 0 */

    /* The fix: a Radx read must leave clip_gate at the last cell, not 0. */
    g_assert_cmpint(dgi->clip_gate, ==, ncells - 1);

    fndx = first_present(dgi, -1);
    g_assert_cmpint(fndx, >=, 0);
    field_name(dgi, fndx, fname);

    /* snapshot the field, find a good gate near the FAR end of the ray */
    g1 = 1; n = ncells; if (n > 8192) n = 8192;
    i = fndx;
    dd_givfld(dgi, &i, &g1, &n, vals, &bad);
    far_idx = -1;
    for (i = n - 1; i > n / 2; i--) {
        if (vals[i] != bad) { far_idx = i; break; }
    }
    g_assert_cmpint(far_idx, >=, 0);   /* need a good gate in the far half */
    g_message("[edit_applies] %d rays x %d gates; field=%s far_gate=%d val=%.3f",
              nrays, ncells, fname, far_idx, vals[far_idx]);

    /* --- set up the editor state for unconditional-delete --- */
    seds = return_sed_stuff();
    seds->se_frame = win;            /* se_hard_zap edits dd_window_dgi(se_frame) */
    seds->finish_up = NO;
    seds->punt = NO;
    seds->boundary_exists = YES;
    mask = g_malloc0(sizeof(unsigned short) * ncells);
    for (i = 0; i < ncells; i++) mask[i] = 1;   /* whole ray in-boundary */
    seds->boundary_mask = mask;

    /* tokens: cmds[0] keyword, cmds[1] = <field>; se_hard_zap reads cmds+1 */
    memset(cmds, 0, sizeof(cmds));
    cmds[0].uc_ctype = UTT_KW;
    cmds[0].uc_text = "unconditional-delete";
    cmds[1].uc_ctype = UTT_FIELD;
    cmds[1].uc_text = fname;
    cmds[2].uc_ctype = UTT_END;

    g_assert_cmpint(se_hard_zap(2, cmds), >=, 0);

    /* --- re-read the field; the far gate must now be bad --- */
    g1 = 1; n = ncells; if (n > 8192) n = 8192;
    i = fndx;
    dd_givfld(dgi, &i, &g1, &n, vals, &bad);

    /* gate 0 always changed even under the old bug; the FAR gate only changes
     * when clip_gate spans the full ray -- this is the issue #6 assertion. */
    g_assert_cmpfloat(vals[far_idx], ==, bad);

    g_free(mask);
    seds->boundary_mask = NULL;
    seds->boundary_exists = NO;
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.editapplies",
                               edit_applies_action, NULL);
}
