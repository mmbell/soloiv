/* test_radx_reload.c
 *
 * Regression for the "edits don't refresh / don't chain" bug. The rio reader
 * caches the input RadxVol in dgi->gpptr7 keyed on file path and only re-opens
 * when the path changes. An in-place edit overwrites the sweep under the same
 * name, so without invalidating the cache a repaint, the Replot button, and the
 * next edit all keep reading the stale pre-edit volume.
 *
 * This test: write a working copy of a CfRadial sweep, open it (priming the
 * read cache), overwrite that same file on disk with an edited version (field 0
 * all bad), then re-read through the original dgi. After rio_invalidate_read the
 * re-read must return the edited (bad) data.
 *
 * Runs with cwd = tests/fixtures/ground/cf2.
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

static gboolean find_nc(const char *dir, char *out, gsize cap)
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

/* Copy the sweep into dir by reading it and writing it back out via rio. */
static void write_copy(int win, const char *srcdir, const char *srcfile,
                       const char *dstdir)
{
    struct dd_general_info *dw = dd_window_dgi(win, "");
    int nrays, r;
    g_strlcpy(dw->directory_name, srcdir, sizeof(dw->directory_name));
    g_strlcpy(dw->sweep_file_name, srcfile, sizeof(dw->sweep_file_name));
    dw->source_fmt = CFRADIAL_FMT;
    nrays = dd_absorb_header_info(dw);
    g_assert_cmpint(nrays, >, 10);
    g_strlcpy(dw->directory_name, dstdir, sizeof(dw->directory_name));
    for (r = 0; r < nrays; r++) {
        if (dd_absorb_ray_info(dw) < 1) break;
        rio_write_ray(dw);
    }
    g_assert_cmpint(rio_write_sweep_end(dw), ==, 0);
}

static void reload_action(GtkWidget *main_window, gpointer user_data)
{
    struct dd_general_info *dr, *de;
    char *tmpdir, copydir[512], written[160];
    int nrays, ncells, r, i, gg, bad0;
    float vals[8192], bad;

    (void)main_window;
    (void)user_data;

    tmpdir = g_dir_make_tmp("soloiv_reload_XXXXXX", NULL);
    g_assert_nonnull(tmpdir);
    g_snprintf(copydir, sizeof(copydir), "%s/", tmpdir);

    /* 1. working copy of the sweep in a writable dir */
    write_copy(2, "./", CFRAD_FILE, copydir);
    g_assert_true(find_nc(tmpdir, written, sizeof(written)));

    /* 2. open the copy (this primes the read cache for that path) */
    dr = dd_window_dgi(3, "");
    g_strlcpy(dr->directory_name, copydir, sizeof(dr->directory_name));
    g_strlcpy(dr->sweep_file_name, written, sizeof(dr->sweep_file_name));
    dr->source_fmt = CFRADIAL_FMT;
    nrays = dd_absorb_header_info(dr);
    ncells = dr->dds->celv->number_cells;
    g_assert_cmpint(dd_absorb_ray_info(dr), >=, 1);

    /* find a gate of field 0 that holds real (non-bad) data */
    { int ndx = 0, g1 = 1, n = ncells > 8192 ? 8192 : ncells;
      dd_givfld(dr, &ndx, &g1, &n, vals, &bad);
      gg = -1;
      for (i = 0; i < n; i++) if (vals[i] != bad) { gg = i; break; }
      g_assert_cmpint(gg, >=, 0);
    }
    bad0 = (int) dr->dds->parm[0]->bad_data;

    /* 3. overwrite that same file on disk: field 0 -> all bad, every ray */
    de = dd_window_dgi(4, "");
    g_strlcpy(de->directory_name, copydir, sizeof(de->directory_name));
    g_strlcpy(de->sweep_file_name, written, sizeof(de->sweep_file_name));
    de->source_fmt = CFRADIAL_FMT;
    g_assert_cmpint(dd_absorb_header_info(de), ==, nrays);
    g_strlcpy(de->directory_name, copydir, sizeof(de->directory_name));
    for (r = 0; r < nrays; r++) {
        short *q;
        if (dd_absorb_ray_info(de) < 1) break;
        q = (short *) de->dds->qdat_ptrs[0];
        for (i = 0; i < de->dds->celv->number_cells; i++)
            q[i] = (short) de->dds->parm[0]->bad_data;
        rio_write_ray(de);
    }
    g_assert_cmpint(rio_write_sweep_end(de), ==, 0);

    /* 4. re-read through the ORIGINAL dgi WITHOUT invalidating: the cache still
     * serves the pre-edit volume. (Informational; the cache is the reason the
     * fix is needed.) */
    {
        int ndx = 0, g1 = gg + 1, n = 1;
        dd_absorb_header_info(dr);
        g_assert_cmpint(dd_absorb_ray_info(dr), >=, 1);
        dd_givfld(dr, &ndx, &g1, &n, vals, &bad);
        g_message("reload: pre-invalidate gate %d = %.3f (bad=%.3f)",
                  gg, vals[0], bad);
    }

    /* 5. invalidate the cache and re-read: now the edited (bad) data must come
     * back from disk. This is the property the fix guarantees. */
    rio_invalidate_read(dr);
    {
        int ndx = 0, g1 = gg + 1, n = 1;
        g_assert_cmpint(dd_absorb_header_info(dr), ==, nrays);
        g_assert_cmpint(dd_absorb_ray_info(dr), >=, 1);
        dd_givfld(dr, &ndx, &g1, &n, vals, &bad);
        g_message("reload: post-invalidate gate %d = %.3f (bad=%.3f)",
                  gg, vals[0], bad);
        g_assert_cmpfloat(vals[0], ==, bad);   /* edited value re-read */
    }

    (void) bad0;
    g_free(tmpdir);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.reload",
                               reload_action, NULL);
}
