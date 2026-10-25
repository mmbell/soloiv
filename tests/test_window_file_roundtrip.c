/* test_window_file_roundtrip.c
 *
 * Window-state ("wds.*") files: save the frame state for a loaded sweep,
 * disturb it, absorb the file, and check the state comes back: frame count
 * and layout, fields, magnification, center, and frame size. The restored
 * frames must get the saved size through the resize path and replot once.
 *
 * The file is a chain of records, each starting with a 4-char id and a
 * long sizeof_struct; the chain must end exactly at the end of the file.
 * The color-table (SCTB) records used to declare a size 8 bytes short of
 * what was written on LP64, so absorbing them stepped into the middle of a
 * record and then off into memory (SIGBUS in solo_absorb_window_info).
 *
 * With --corrupt it instead absorbs damaged copies of a saved file (huge,
 * zero and short record sizes, truncation, a bad window number, bad view
 * offsets and layout, unterminated extras, and the legacy short SCTB
 * records). None may read out of bounds (run under the debug-asan preset);
 * the structurally broken ones must be rejected without touching the
 * current state, and the legacy file must still load.
 *
 * cwd = tests/fixtures/ground/cf2.
 */

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "soloii.h"
#include "sii_externals.h"
#include "solo_window_structs.h"
#include "sii_log_handler.h"
#include "soloiv_core_protos.h"
#include "soloiv_core_gtk_protos.h"
#include "test_app_runner.h"
#include "widget_walker.h"

#define SOS_OFFSET  offsetof(struct solo_generic_window_struct, sizeof_struct)
#define WIN_OFFSET  offsetof(struct solo_generic_window_struct, window_num)

typedef struct {
    guint ncols, nrows;
    sii_table_parameters tbl[maxFrames];
    char  field[maxFrames][16];
    float mag[maxFrames];
    double lat[maxFrames], lon[maxFrames];
    guint cell_w, cell_h;
    guint count;
} FrameState;

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

static void snap_state(FrameState *s)
{
    memset(s, 0, sizeof(*s));
    s->count = sii_frame_count;
    s->ncols = frame_configs[0]->ncols;
    s->nrows = frame_configs[0]->nrows;
    s->cell_w = sii_table_widget_width;
    s->cell_h = sii_table_widget_height;

    for (guint fn = 0; fn < sii_frame_count; fn++) {
        WW_PTR wwptr = solo_return_wwptr(fn);
        s->tbl[fn] = frame_configs[fn]->tbl_parms;
        memcpy(s->field[fn], wwptr->parameter->parameter_name, 16);
        s->mag[fn] = wwptr->view->magnification;
        s->lat[fn] = wwptr->center_of_view->latitude;
        s->lon[fn] = wwptr->center_of_view->longitude;
    }
}

static void assert_state_equal(const FrameState *a, const FrameState *b)
{
    g_assert_cmpuint(a->count, ==, b->count);
    g_assert_cmpuint(a->ncols, ==, b->ncols);
    g_assert_cmpuint(a->nrows, ==, b->nrows);
    g_assert_cmpuint(a->cell_w, ==, b->cell_w);
    g_assert_cmpuint(a->cell_h, ==, b->cell_h);

    for (guint fn = 0; fn < a->count; fn++) {
        g_assert_cmpint(a->tbl[fn].left_attach, ==, b->tbl[fn].left_attach);
        g_assert_cmpint(a->tbl[fn].right_attach, ==, b->tbl[fn].right_attach);
        g_assert_cmpint(a->tbl[fn].top_attach, ==, b->tbl[fn].top_attach);
        g_assert_cmpint(a->tbl[fn].bottom_attach, ==,
                        b->tbl[fn].bottom_attach);
        g_assert_cmpmem(a->field[fn], 16, b->field[fn], 16);
        g_assert_cmpfloat(a->mag[fn], ==, b->mag[fn]);
        g_assert_cmpfloat(a->lat[fn], ==, b->lat[fn]);
        g_assert_cmpfloat(a->lon[fn], ==, b->lon[fn]);
    }
}

/* Every frame is span x cell and its image holds it. */
static void assert_frames_sized(void)
{
    for (guint fn = 0; fn < sii_frame_count; fn++) {
        SiiFrameConfig *sfc = frame_configs[fn];
        sii_table_parameters *stp = &sfc->tbl_parms;
        guint w = (stp->right_attach - stp->left_attach)
            * sii_table_widget_width;
        guint h = (stp->bottom_attach - stp->top_attach)
            * sii_table_widget_height;

        g_assert_cmpuint(sfc->width, ==, w);
        g_assert_cmpuint(sfc->height, ==, h);
        g_assert_nonnull(sfc->image);
        g_assert_cmpuint(sfc->max_image_size, >=, w * h);
    }
}

static void fire(GtkWidget *main_win, const char *name, int param)
{
    GVariant *p = g_variant_new_int32(param);

    g_variant_ref_sink(p);
    g_assert_true(widget_walker_fire_action(main_win, "menu", name, p));
    g_variant_unref(p);
}

/* Save the window state into dir; return the new file's name. */
static char *save_state(const char *dir)
{
    GDir *gd;
    const char *name;
    char *found = NULL;

    solo_save_window_info(dir, "rt");

    gd = g_dir_open(dir, 0, NULL);
    g_assert_nonnull(gd);
    while ((name = g_dir_read_name(gd))) {
        if (!strncmp(name, "wds", 3)) {
            g_assert_null(found);       /* exactly one */
            found = g_strdup(name);
        }
    }
    g_dir_close(gd);
    g_assert_nonnull(found);
    return found;
}

static long rec_size(const char *c)
{
    long sos;

    memcpy(&sos, c + SOS_OFFSET, sizeof(sos));
    return sos;
}

static void set_rec_size(char *c, long sos)
{
    memcpy(c + SOS_OFFSET, &sos, sizeof(sos));
}

/* Offset of the nth record with this id, or -1. Walks the chain the way a
 * correct writer lays it out. */
static long find_record(const char *buf, gsize len, const char *id, int nth)
{
    gsize off = 0;

    while (off + SOS_OFFSET + sizeof(long) <= len) {
        long sos = rec_size(buf + off);

        if (!strncmp(buf + off, id, 4) && nth-- == 0)
            return (long)off;
        if (sos <= 0)
            return -1;
        off += (gsize)sos;
    }
    return -1;
}

/* --- round trip ---------------------------------------------------------- */

static void roundtrip_action(GtkWidget *main_win, gpointer user_data)
{
    FrameState saved, now;
    guint baseline, counts[maxFrames];
    char *dir, *fname, *path, *buf;
    gsize len, off;
    int nrec = 0, nctb = 0;

    (void)user_data;

    wait_ms(500);                       /* initial plots have settled */
    baseline = sii_log_handler_get_complaint_count();
    g_assert_cmpuint(sii_frame_count, ==, 4);

    /* a non-default frame 0 so the restore is visible */
    solo_return_wwptr(0)->view->magnification *= 2;
    snap_state(&saved);

    dir = g_dir_make_tmp("soloiv-wds-XXXXXX", NULL);
    g_assert_nonnull(dir);
    fname = save_state(dir);
    path = g_build_filename(dir, fname, NULL);

    /* The record chain must cover the file exactly. */
    g_assert_true(g_file_get_contents(path, &buf, &len, NULL));
    for (off = 0; off < len; nrec++) {
        long sos;

        g_assert_cmpuint(off + SOS_OFFSET + sizeof(long), <=, len);
        sos = rec_size(buf + off);
        g_assert_cmpint(sos, >=, (long)(SOS_OFFSET + sizeof(long)));
        g_assert_cmpuint(off + (gsize)sos, <=, len);
        if (!strncmp(buf + off, "SCTB", 4)) {
            /* the text follows the header */
            g_assert_cmpmem(buf + off + SOS_OFFSET + sizeof(long), 10,
                            "colortable", 10);
            nctb++;
        }
        off += (gsize)sos;
    }
    g_assert_cmpuint(off, ==, len);
    g_assert_cmpint(nctb, >, 0);
    g_message("[wds] %d records, %d color tables, %" G_GSIZE_FORMAT
              " bytes", nrec, nctb, len);
    g_free(buf);

    /* --- disturb: a different layout and magnification ---------------- */
    fire(main_win, "ccfg_1b2vs", 1012);
    wait_ms(600);
    g_assert_cmpuint(sii_frame_count, ==, 3);
    solo_return_wwptr(0)->view->magnification *= 3;

    /* --- restore ------------------------------------------------------- */
    for (guint fn = 0; fn < maxFrames; fn++)
        counts[fn] = frame_configs[fn]->plot_request_count;
    g_assert_cmpint(solo_absorb_window_info(dir, fname, 0), ==, 1);
    wait_ms(800);

    snap_state(&now);
    assert_state_equal(&now, &saved);
    assert_frames_sized();
    for (guint fn = 0; fn < sii_frame_count; fn++)
        g_assert_cmpuint(frame_configs[fn]->plot_request_count - counts[fn],
                         ==, 1);

    /* and the restored frames still respond to a resize */
    sii_apply_table_allocation(1000, 500);
    assert_frames_sized();

    g_assert_cmpuint(sii_log_handler_get_complaint_count(), ==, baseline);

    g_unlink(path);
    g_rmdir(dir);
    g_free(path);
    g_free(fname);
    g_free(dir);
}

/* --- corrupted files ----------------------------------------------------- */

typedef void (*Damage)(char *buf, gsize *len);

static void dmg_huge_size(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SPMI", 0);

    (void)len;
    g_assert_cmpint(off, >=, 0);
    set_rec_size(buf + off, 0xa4a8b1c53L);  /* like the user's file */
}

static void dmg_zero_size(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SSFI", 1);

    g_assert_cmpint(off, >=, 0);
    set_rec_size(buf + off, 0);
}

static void dmg_short_size(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SWVI", 0);

    g_assert_cmpint(off, >=, 0);
    set_rec_size(buf + off, 6);
}

static void dmg_past_end(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SXTR", 2);

    g_assert_cmpint(off, >=, 0);
    set_rec_size(buf + off, (long)(*len - (gsize)off) + 4000);
}

static void dmg_truncate_record(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SWVI", 1);

    (void)buf;
    g_assert_cmpint(off, >=, 0);
    *len = (gsize)off + 100;            /* mid-record */
}

static void dmg_truncate_header(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SSFI", 2);

    (void)buf;
    g_assert_cmpint(off, >=, 0);
    *len = (gsize)off + 6;              /* inside the next header */
}

static void dmg_window_num(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SSFI", 1), ww = 999;

    g_assert_cmpint(off, >=, 0);
    memcpy(buf + off + WIN_OFFSET, &ww, sizeof(ww));
}

static void dmg_view_pisps(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SWVI", 0);
    struct solo_view_info *v;

    g_assert_cmpint(off, >=, 0);
    v = (struct solo_view_info *)g_malloc(sizeof(*v));
    memcpy(v, buf + off, sizeof(*v));
    v->offset_to_pisps = 60000;
    v->num_pisps = 1000;
    memcpy(buf + off, v, sizeof(*v));
    g_free(v);

    off = find_record(buf, *len, "SWVI", 1);
    g_assert_cmpint(off, >=, 0);
    v = (struct solo_view_info *)g_malloc(sizeof(*v));
    memcpy(v, buf + off, sizeof(*v));
    v->num_pisps = 1000;                /* runs past the record */
    memcpy(buf + off, v, sizeof(*v));
    g_free(v);
}

static void dmg_view_config(char *buf, gsize *len)
{
    long off;
    struct solo_view_info *v;

    for (int ii = 0; (off = find_record(buf, *len, "SWVI", ii)) >= 0; ii++) {
        v = (struct solo_view_info *)g_malloc(sizeof(*v));
        memcpy(v, buf + off, sizeof(*v));
        v->frame_config = (ii == 0) ? 1919 : 2299;  /* off the grid */
        memcpy(buf + off, v, sizeof(*v));
        g_free(v);
    }
}

static void dmg_extras(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SXTR", 0);
    gsize att, nn;
    char *a;

    g_assert_cmpint(off, >=, 0);
    att = (gsize)off + WIN_OFFSET + sizeof(long);
    nn = (gsize)rec_size(buf + off) - (att - (gsize)off);
    a = buf + att;
    /* unterminated, far more tokens than any argument array */
    for (gsize ii = 0; ii < nn; ii++)
        a[ii] = (ii & 1) ? ';' : 'x';
    memcpy(a, "SWPFLT;CB_LOC;", 14);

    off = find_record(buf, *len, "SXTR", 1);
    g_assert_cmpint(off, >=, 0);
    a = buf + off + WIN_OFFSET + sizeof(long);
    memset(a, 0, 256);
    strcpy(a, "SWPFLT:1.0,2.0,"
           "AAAAAAAAAAAAAAAAAAAA,BBBBBBBBBBBBBBBBBBBB,CCCCCCCCCCCCCCCCCCCC,"
           "DDDDDDDDDDDDDDDDDDDD,EEEEEEEEEEEEEEEEEEEE;");
}

/* The pre-fix writer: each SCTB record declared a size 8 bytes short of
 * what it wrote (an 8-byte header on LP64 counted as 8 + text). */
static void dmg_legacy_sctb(char *buf, gsize *len)
{
    long off = find_record(buf, *len, "SCTB", 0);
    int n = 0;

    g_assert_cmpint(off, >=, 0);
    /* walk by searching for the ids, since the sizes are being rewritten */
    while (off >= 0 && (gsize)off + SOS_OFFSET + sizeof(long) <= *len) {
        long sos = rec_size(buf + off);
        long next = off + sos;

        set_rec_size(buf + off, sos - 8);
        n++;
        off = ((gsize)next < *len) ? next : -1;
    }
    g_assert_cmpint(n, >, 0);
}

typedef struct {
    const char *name;
    Damage      damage;
    int         expect;     /* absorb's return, or 0 for "either" */
} Case;

static const Case cases[] = {
    { "huge_size",        dmg_huge_size,        -1 },
    { "zero_size",        dmg_zero_size,        -1 },
    { "short_size",       dmg_short_size,       -1 },
    { "past_end",         dmg_past_end,         -1 },
    { "truncate_record",  dmg_truncate_record,  -1 },
    { "truncate_header",  dmg_truncate_header,  -1 },
    { "window_num",       dmg_window_num,       -1 },
    { "view_pisps",       dmg_view_pisps,        0 },
    { "view_config",      dmg_view_config,       0 },
    { "extras",           dmg_extras,            0 },
    { "legacy_sctb",      dmg_legacy_sctb,       1 },
};

static void corrupt_action(GtkWidget *main_win, gpointer user_data)
{
    char *dir, *fname, *path, *good;
    gsize good_len;

    (void)main_win;
    (void)user_data;

    wait_ms(500);
    g_assert_cmpuint(sii_frame_count, ==, 4);

    dir = g_dir_make_tmp("soloiv-wds-XXXXXX", NULL);
    g_assert_nonnull(dir);
    fname = save_state(dir);
    path = g_build_filename(dir, fname, NULL);
    g_assert_true(g_file_get_contents(path, &good, &good_len, NULL));
    g_unlink(path);
    g_free(path);

    for (gsize ii = 0; ii < G_N_ELEMENTS(cases); ii++) {
        const Case *cs = &cases[ii];
        char *bad = g_memdup2(good, good_len);
        gsize len = good_len;
        char *bname = g_strdup_printf("wds.bad.%s", cs->name);
        char *bpath = g_build_filename(dir, bname, NULL);
        guint count = sii_frame_count;
        int ret;

        cs->damage(bad, &len);
        g_assert_true(g_file_set_contents(bpath, bad, (gssize)len, NULL));

        g_message("[wds] absorbing %s (%" G_GSIZE_FORMAT " bytes)",
                  cs->name, len);
        ret = solo_absorb_window_info(dir, bname, 0);
        g_message("[wds] %s -> %d", cs->name, ret);
        if (cs->expect)
            g_assert_cmpint(ret, ==, cs->expect);
        else
            g_assert_true(ret == 1 || ret == -1);
        if (ret < 0)    /* rejected: the current frames are untouched */
            g_assert_cmpuint(sii_frame_count, ==, count);
        wait_ms(400);
        g_assert_cmpuint(sii_frame_count, >, 0);
        assert_frames_sized();

        g_unlink(bpath);
        g_free(bpath);
        g_free(bname);
        g_free(bad);
    }

    /* the app still works: restore the good file and resize */
    path = g_build_filename(dir, fname, NULL);
    g_assert_true(g_file_set_contents(path, good, (gssize)good_len, NULL));
    g_assert_cmpint(solo_absorb_window_info(dir, fname, 0), ==, 1);
    wait_ms(600);
    g_assert_cmpuint(sii_frame_count, ==, 4);
    sii_apply_table_allocation(1000, 500);
    assert_frames_sized();

    g_unlink(path);
    g_rmdir(dir);
    g_free(path);
    g_free(fname);
    g_free(good);
    g_free(dir);
}

int main(int argc, char *argv[])
{
    gboolean corrupt = (argc > 1 && !strcmp(argv[1], "--corrupt"));

    g_test_init(&argc, &argv, NULL);
    if (corrupt)
        return test_app_runner_run("org.lrose.soloiv.test.wdscorrupt",
                                   corrupt_action, NULL);
    return test_app_runner_run("org.lrose.soloiv.test.wdsroundtrip",
                               roundtrip_action, NULL);
}
