/* test_radx_drop_field.c
 *
 * Reproduction for the "ignore-field eats a chunk of the remaining fields"
 * bug. The editor's ignore-field command drops one field (e.g. DBC) and then
 * the sweep is written back to disk. The reported symptom: the dropped field
 * goes away as intended, but a large block of data also vanishes from the
 * OTHER, untouched fields after save+reopen. The user works mostly with
 * DORADE, so this checks both the DORADE and CfRadial writer paths (both go
 * through rio_write_ray / rio_write_sweep_end).
 *
 * For each format it:
 *   1. reads every ray and snapshots a field we intend to KEEP for all
 *      rays x gates, plus its plotted beam angle,
 *   2. drops a different field the way ignore-field does (field_present = NO)
 *      and writes each ray out,
 *   3. reopens the written file and verifies the kept field is intact at every
 *      ray and gate (no missing chunk, no corrupted values) AND that the beam
 *      angles still span the full sweep (no new wedge-shaped gap).
 *
 * The wedge check is the real regression: the reported "chunk" was an airborne
 * sweep whose primary_axis was not written, so on reopen it was misclassified
 * RHI instead of AIR and dd_rotation_angle plotted every beam from elevation,
 * collapsing the display into a ~220-degree void. See rio_wvol_set_meta.
 *
 * cwd = tests/fixtures/ground.
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
int dd_find_field();
double dd_rotation_angle();

/* Largest angular gap (deg) between consecutive plot angles around the circle.
 * A full sweep has a small gap (~ beam spacing); a missing wedge shows up as a
 * large gap. */
static double max_angular_gap(double *ang, int n)
{
    int i, j;
    double prev, gap, maxgap = 0.0;
    /* simple insertion sort (n is a few hundred) */
    for (i = 1; i < n; i++) {
        double key = ang[i];
        for (j = i - 1; j >= 0 && ang[j] > key; j--) ang[j + 1] = ang[j];
        ang[j + 1] = key;
    }
    for (i = 1; i < n; i++) {
        gap = ang[i] - ang[i - 1];
        if (gap > maxgap) maxgap = gap;
    }
    prev = (ang[0] + 360.0) - ang[n - 1];   /* wrap-around gap */
    if (prev > maxgap) maxgap = prev;
    return maxgap;
}

#define DORADE_FILE "dorade/swp.1140703215739.KLTX.317.0.9_SUR_v775"
#define CFRAD_FILE  "cf2/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"
#define AIR_DORADE  "../airborne/dorade/swp.1220907125318.N42RF-TM.857.20.0_AIR_v2"
#define AIR_CFRAD   "../airborne/cf2/cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"

static gboolean find_written(const char *dir, char *out, gsize cap)
{
    GDir *d = g_dir_open(dir, 0, NULL);
    const char *name;
    gboolean found = FALSE;
    if (!d) return FALSE;
    while ((name = g_dir_read_name(d))) {
        if (g_str_has_suffix(name, ".nc") || g_str_has_prefix(name, "swp.")) {
            g_strlcpy(out, name, cap);
            found = TRUE;
            break;
        }
    }
    g_dir_close(d);
    return found;
}

/* First present field index, or -1. */
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

/* Returns the count of "missing chunk" gates (good data turned into a hole)
 * across the whole kept field after drop + write + reopen. 0 means clean. */
static int drop_check(const char *label, int win, const char *file)
{
    struct dd_general_info *dw, *dr;
    char *tmpdir;
    char written[160], keepname[16];
    int nrays, ncells, r, i, keepndx, dropndx, ndx, g1, n;
    float *orig, *origbad, vals[8192], bad;
    double *rot_in, *rot_out, gap_in, gap_out;
    int missing_chunk = 0, value_mismatch = 0;

    tmpdir = g_dir_make_tmp("soloiv_drop_XXXXXX", NULL);
    g_assert_nonnull(tmpdir);

    dw = dd_window_dgi(win, "");
    g_strlcpy(dw->directory_name, "./", sizeof(dw->directory_name));
    g_strlcpy(dw->sweep_file_name, file, sizeof(dw->sweep_file_name));
    dw->source_fmt = CFRADIAL_FMT;          /* force the rio backend */
    nrays = dd_absorb_header_info(dw);
    g_assert_cmpint(nrays, >, 10);

    ncells = dw->dds->celv->number_cells;
    g_assert_cmpint(ncells, >, 0);
    keepndx = first_present(dw, -1);
    dropndx = first_present(dw, keepndx);
    g_assert_cmpint(keepndx, >=, 0);
    g_assert_cmpint(dropndx, >=, 0);
    field_name(dw, keepndx, keepname);
    g_message("[%s] %d rays x %d gates; keep=%s drop=field#%d",
              label, nrays, ncells, keepname, dropndx);

    orig    = g_malloc(sizeof(float) * nrays * ncells);
    origbad = g_malloc(sizeof(float) * nrays);
    rot_in  = g_malloc(sizeof(double) * nrays);
    rot_out = g_malloc(sizeof(double) * nrays);

    g_strlcpy(dw->directory_name, tmpdir, sizeof(dw->directory_name));
    if (dw->directory_name[strlen(dw->directory_name) - 1] != '/')
        g_strlcat(dw->directory_name, "/", sizeof(dw->directory_name));

    for (r = 0; r < nrays; r++) {
        double a;
        if (dd_absorb_ray_info(dw) < 1) break;
        /* The rio read path must set the clip gate to the last cell; if it
         * stays 0, every for-each-ray editor op that iterates to clip_gate+1
         * touches a single gate and silently no-ops. */
        if (r == 0)
            g_assert_cmpint(dw->clip_gate, ==, ncells - 1);
        ndx = keepndx; g1 = 1; n = ncells;
        if (n > 8192) n = 8192;
        dd_givfld(dw, &ndx, &g1, &n, vals, &bad);
        origbad[r] = bad;
        for (i = 0; i < n; i++) orig[r * ncells + i] = vals[i];

        a = dd_rotation_angle(dw);
        while (a < 0.0)    a += 360.0;
        while (a >= 360.0) a -= 360.0;
        rot_in[r] = a;

        dw->dds->field_present[dropndx] = NO;   /* ignore-field */
        rio_write_ray(dw);
    }
    g_assert_cmpint(r, ==, nrays);
    g_assert_cmpint(rio_write_sweep_end(dw), ==, 0);

    g_assert_true(find_written(tmpdir, written, sizeof(written)));

    dr = dd_window_dgi(win + 10, "");
    g_strlcpy(dr->directory_name, tmpdir, sizeof(dr->directory_name));
    if (dr->directory_name[strlen(dr->directory_name) - 1] != '/')
        g_strlcat(dr->directory_name, "/", sizeof(dr->directory_name));
    g_strlcpy(dr->sweep_file_name, written, sizeof(dr->sweep_file_name));
    dr->source_fmt = CFRADIAL_FMT;

    g_assert_cmpint(dd_absorb_header_info(dr), ==, nrays);
    g_assert_cmpint(dr->dds->celv->number_cells, ==, ncells);

    keepndx = dd_find_field(dr, keepname);
    g_assert_cmpint(keepndx, >=, 0);

    for (r = 0; r < nrays; r++) {
        double a;
        if (dd_absorb_ray_info(dr) < 1) break;
        ndx = keepndx; g1 = 1; n = ncells;
        if (n > 8192) n = 8192;
        dd_givfld(dr, &ndx, &g1, &n, vals, &bad);
        for (i = 0; i < n; i++) {
            float was = orig[r * ncells + i];
            float now = vals[i];
            int was_good = (was != origbad[r]);
            int now_good = (now != bad);
            if (was_good && !now_good)               missing_chunk++;
            else if (was_good && now_good &&
                     fabsf(was - now) > 0.5f)         value_mismatch++;
        }
        a = dd_rotation_angle(dr);
        while (a < 0.0)    a += 360.0;
        while (a >= 360.0) a -= 360.0;
        rot_out[r] = a;
    }
    g_assert_cmpint(r, ==, nrays);

    gap_in  = max_angular_gap(rot_in,  nrays);
    gap_out = max_angular_gap(rot_out, nrays);

    g_message("[%s] kept field %s: missing_chunk=%d value_mismatch=%d "
              "max_angle_gap in=%.1f out=%.1f deg",
              label, keepname, missing_chunk, value_mismatch, gap_in, gap_out);

    g_free(orig);
    g_free(origbad);
    g_free(rot_in);
    g_free(rot_out);
    g_free(tmpdir);
    g_assert_cmpint(value_mismatch, ==, 0);
    /* a wedge shows up as a large new angular gap that the original didn't have */
    g_assert_cmpfloat(gap_out, <, gap_in + 5.0);
    return missing_chunk;
}

static void drop_field_action(GtkWidget *main_window, gpointer user_data)
{
    int dorade_holes, cfrad_holes;
    (void)main_window;
    (void)user_data;

    dorade_holes = drop_check("DORADE", 2, DORADE_FILE);
    cfrad_holes  = drop_check("CfRadial", 4, CFRAD_FILE);
    (void) drop_check("AIR-DORADE", 6, AIR_DORADE);
    (void) drop_check("AIR-CfRadial", 8, AIR_CFRAD);

    g_assert_cmpint(dorade_holes, ==, 0);
    g_assert_cmpint(cfrad_holes,  ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.dropfield",
                               drop_field_action, NULL);
}
