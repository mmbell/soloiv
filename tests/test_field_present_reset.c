/* test_field_present_reset.c
 *
 * The Radx reader sets dds->field_present[pn] = YES for pn < nfields but had
 * no reset loop, so the presence map was only ever added to. Two consequences,
 * both silent:
 *
 *  - Load a sweep with N fields, then one with fewer: the surplus entries
 *    stayed marked present, pointing at the previous sweep's parm[] and
 *    qdat_ptrs[]. Consumers that walk field_present (solo_color_cells,
 *    dd_givfld, the editor's field lists) then read stale data, or NULL, for
 *    fields the current sweep does not have.
 *  - An editor "ignore-field" clears field_present[fn]; with no reset the
 *    field stayed ignored in every subsequently loaded sweep.
 *
 * The legacy DORADE reader has always cleared the map on RADD
 * (swp_file_acc.c); only the Radx path was missing it.
 *
 * This test marks unused slots present, reloads, and requires the reader to
 * have cleared them. It also checks the ignore-field case does not persist.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "test_app_runner.h"

#define CFRAD_FILE "cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"

struct dd_general_info *dd_window_dgi();

static int count_present(struct dd_general_info *dgi)
{
    int i, n = 0;
    for (i = 0; i < MAX_PARMS; i++)
        if (dgi->dds->field_present[i]) n++;
    return n;
}

static void reset_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dgi;
    int nrays, nfields, i;
    int win = 7;

    (void)main_window;
    (void)user_data;

    dgi = dd_window_dgi(win, "");
    g_assert_nonnull(dgi);
    g_strlcpy(dgi->directory_name, "./", sizeof(dgi->directory_name));
    g_strlcpy(dgi->sweep_file_name, CFRAD_FILE, sizeof(dgi->sweep_file_name));
    dgi->source_fmt = CFRADIAL_FMT;

    nrays = dd_absorb_header_info(dgi);
    g_assert_cmpint(nrays, >, 10);
    nfields = dgi->num_parms;
    g_assert_cmpint(nfields, >, 0);
    g_assert_cmpint(nfields, <, MAX_PARMS);
    g_assert_cmpint(count_present(dgi), ==, nfields);

    /* Stand in for a previously loaded, wider sweep: mark every slot above
     * this sweep's field count present. */
    for (i = nfields; i < MAX_PARMS; i++)
        dgi->dds->field_present[i] = YES;
    g_assert_cmpint(count_present(dgi), ==, MAX_PARMS);

    /* Reloading must clear them: only this sweep's fields stay present. */
    g_assert_cmpint(dd_absorb_header_info(dgi), >, 10);
    g_assert_cmpint(count_present(dgi), ==, nfields);
    for (i = nfields; i < MAX_PARMS; i++)
        g_assert_cmpint(dgi->dds->field_present[i], ==, NO);

    /* The mirror case: an ignored field must come back on a reload, not stay
     * ignored for the rest of the session. */
    dgi->dds->field_present[0] = NO;
    g_assert_cmpint(dd_absorb_header_info(dgi), >, 10);
    g_assert_cmpint(dgi->dds->field_present[0], ==, YES);
    g_assert_cmpint(count_present(dgi), ==, nfields);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.fpreset",
                               reset_action, NULL);
}
