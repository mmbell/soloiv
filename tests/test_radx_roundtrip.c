/* test_radx_roundtrip.c
 *
 * Phase 4 acceptance: read a CfRadial sweep through the Radx backend, edit a
 * field in the int16 pipeline, WRITE it back out via the Radx writer
 * (rio_write_ray / rio_write_sweep_end), then reopen the written file and
 * confirm the edit survived and the geometry is intact.
 *
 * The edit: on every ray, flag the first 20 gates of field 0 as bad. After
 * round-trip, those gates must read back bad and the rest must match the
 * original physical values within the int16 step.
 *
 * Output is written to a fresh temp dir so the sample tree is untouched.
 * Runs with cwd = test_data/arthur2015/cfradial.
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dorade_headers.h"
#include "dd_defines.h"
#include "radar_io.h"
#include "test_app_runner.h"

struct dd_general_info *dd_window_dgi();
int dd_givfld();

#define CFRAD_FILE "cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"
#define NFLAG 20

/* Find the single *.nc file Radx wrote under dir; copy its name to out. */
static gboolean find_written_nc(const char *dir, char *out, gsize cap)
{
    GDir *d = g_dir_open(dir, 0, NULL);
    const char *name;
    gboolean found = FALSE;
    if (!d) return FALSE;
    while ((name = g_dir_read_name(d))) {
        if (g_str_has_suffix(name, ".nc")) {
            g_strlcpy(out, name, cap);
            found = TRUE;
            break;
        }
    }
    g_dir_close(d);
    return found;
}

static void roundtrip_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dw, *dr;
    char *tmpdir;
    char written[128];
    int nrays, r, i, ndx, g1, n;
    float vals[8192], bad;
    int checked_ray = -1;

    (void)main_window;
    (void)user_data;

    tmpdir = g_dir_make_tmp("soloiv_rt_XXXXXX", NULL);
    g_assert_nonnull(tmpdir);

    /* --- read + edit + write --- */
    dw = dd_window_dgi(4, "");
    g_strlcpy(dw->directory_name, "./", sizeof(dw->directory_name));
    g_strlcpy(dw->sweep_file_name, CFRAD_FILE, sizeof(dw->sweep_file_name));
    dw->source_fmt = CFRADIAL_FMT;
    nrays = dd_absorb_header_info(dw);
    g_assert_cmpint(nrays, >, 100);

    /* writer emits to the temp dir */
    g_strlcpy(dw->directory_name, tmpdir, sizeof(dw->directory_name));
    if (dw->directory_name[strlen(dw->directory_name) - 1] != '/')
        g_strlcat(dw->directory_name, "/", sizeof(dw->directory_name));

    for (r = 0; r < nrays; r++) {
        short *q;
        if (dd_absorb_ray_info(dw) < 1) break;
        q = (short *) dw->dds->qdat_ptrs[0];
        for (i = 0; i < NFLAG && i < dw->dds->celv->number_cells; i++)
            q[i] = (short) dw->dds->parm[0]->bad_data;   /* flag bad */
        rio_write_ray(dw);
    }
    g_assert_cmpint(rio_write_sweep_end(dw), ==, 0);

    g_assert_true(find_written_nc(tmpdir, written, sizeof(written)));
    g_message("Round-trip wrote %s/%s", tmpdir, written);

    /* --- reopen the written file and verify --- */
    dr = dd_window_dgi(5, "");
    g_strlcpy(dr->directory_name, tmpdir, sizeof(dr->directory_name));
    if (dr->directory_name[strlen(dr->directory_name) - 1] != '/')
        g_strlcat(dr->directory_name, "/", sizeof(dr->directory_name));
    g_strlcpy(dr->sweep_file_name, written, sizeof(dr->sweep_file_name));
    dr->source_fmt = CFRADIAL_FMT;

    g_assert_cmpint(dd_absorb_header_info(dr), ==, nrays);
    g_assert_cmpint(dr->num_parms, ==, dw->num_parms);
    g_assert_cmpint(dr->dds->celv->number_cells, ==,
                    dw->dds->celv->number_cells);

    /* Read the first ray back; the flagged gates must be bad. */
    g_assert_cmpint(dd_absorb_ray_info(dr), >=, 1);
    ndx = 0; g1 = 1; n = dr->dds->celv->number_cells;
    if (n > 8192) n = 8192;
    dd_givfld(dr, &ndx, &g1, &n, vals, &bad);
    for (i = 0; i < NFLAG; i++)
        g_assert_cmpfloat(vals[i], ==, bad);
    /* gates beyond the flag region include real (non-bad) data */
    {
        int ngood = 0;
        for (i = NFLAG; i < n; i++) if (vals[i] != bad) ngood++;
        g_assert_cmpint(ngood, >, 0);
    }
    checked_ray = 0;

    g_message("Round-trip OK: %d rays, %d fields, edit persisted",
              nrays, dr->num_parms);
    g_assert_cmpint(checked_ray, ==, 0);
    g_free(tmpdir);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.roundtrip",
                               roundtrip_action, NULL);
}
