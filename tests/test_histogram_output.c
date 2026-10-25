/* test_histogram_output.c
 *
 * The four histogram text-output routines in editor/se_histog.c build an
 * ASCII scale line, a tick line and bar lines in char[88] stack buffers
 * with pointer cursors:
 *
 *   se_histo_fin_irreg_areas   se_histo_fin_irreg_counts
 *   se_histo_fin_areas         se_histo_fin_counts
 *
 * The scale loops stepped a float (or int) value by a multiple of
 * (max-min)/50, so a zero or tiny range never terminated -- and wrote past
 * the buffer on every step -- and huge values pushed the scale past the end
 * of the buffer even when it did terminate.
 *
 * These tests drive the routines directly (no GUI) with a minimally set up
 * solo_edit_stuff: a single pair, equal areas, a count range under 5, huge
 * values, plus a normal multi-pair input whose shape is checked. Every
 * output line must be newline-terminated and inside the string buffer, and
 * scale lines inside the 88-byte stack buffer. The overruns are caught by
 * ASan (debug-asan preset); a regression to the endless loop trips the
 * CTest timeout.
 */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dorade_headers.h"
#include "solo_editor_structs.h"
#include "soloedit_protos.h"

#define NBINS 10
#define SCALE_BUF 88

static struct solo_edit_stuff *seds;

static void free_pairs(void)
{
    struct se_pairs *p, *next;

    for (p = seds->h_pairs; p; p = next) {
        next = p->next;
        free(p);
    }
    seds->h_pairs = NULL;
}

/* Reset the editor struct to a histogram of NBINS unit bins from 0. */
static void setup(int irreg)
{
    int ii;

    seds = return_sed_stuff();
    free_pairs();
    se_push_all_ssms(&seds->h_output);
    seds->h_output = NULL;

    if (!seds->areas_array)
        seds->areas_array = calloc(NBINS + 1, sizeof(float));
    if (!seds->counts_array)
        seds->counts_array = calloc(NBINS + 1, sizeof(int));
    for (ii = 0; ii <= NBINS; ii++) {
        seds->areas_array[ii] = 0;
        seds->counts_array[ii] = 0;
    }
    seds->histo_key = irreg ? H_IRREG : 0;
    seds->histo_num_bins = NBINS;
    seds->histo_low = 0;
    seds->histo_high = NBINS;
    seds->histo_increment = 1;
    seds->low_count = seds->high_count = 0;
    seds->low_area = seds->high_area = 0;
    seds->medium_count = 100;
    seds->histo_sum = 500;
    seds->histo_sum_sq = 3000;
    strcpy(seds->histogram_field, "DZ");
    strcpy(seds->histo_radar_name, "TEST");
}

static void add_pair(float f_low, float f_high)
{
    struct se_pairs *p = calloc(1, sizeof(*p)), *q;

    p->f_low = f_low;
    p->f_high = f_high;
    if (!seds->h_pairs) {
        seds->h_pairs = p;
    } else {
        for (q = seds->h_pairs; q->next; q = q->next)
            ;
        q->next = p;
        p->last = q;
    }
}

/* Every line is a bounded, newline-terminated string. */
static void check_output(const char *what, int min_lines)
{
    struct solo_str_mgmt *ssm;
    int nlines = 0;

    for (ssm = seds->h_output; ssm; ssm = ssm->next) {
        size_t len = strnlen(ssm->at, SE_MAX_STRING);

        g_assert_cmpuint(len, <, SE_MAX_STRING);
        g_assert_cmpuint(len, >, 0);
        g_assert_cmpint(ssm->at[len - 1], ==, '\n');
        if (g_getenv("SOLOIV_HISTO_VERBOSE"))
            fputs(ssm->at, stdout);
        nlines++;
    }
    g_assert_cmpint(nlines, >=, min_lines);
    g_test_message("%s: %d lines", what, nlines);
}

static struct solo_str_mgmt *nth_line(int n)
{
    struct solo_str_mgmt *ssm = seds->h_output;

    while (n-- > 0 && ssm)
        ssm = ssm->next;
    g_assert_nonnull(ssm);
    return ssm;
}

static int count_lines(void)
{
    int n = 0;
    struct solo_str_mgmt *ssm;

    for (ssm = seds->h_output; ssm; ssm = ssm->next)
        n++;
    return n;
}

static int count_char(const char *s, char c)
{
    int n = 0;

    for (; *s; s++)
        if (*s == c)
            n++;
    return n;
}

/* Scale, tick and bar lines were written into a char[88]: at most 87
 * characters plus the terminator. */
static void check_scale_lines(int first_scale_line)
{
    struct solo_str_mgmt *ssm;

    for (ssm = nth_line(first_scale_line); ssm; ssm = ssm->next)
        g_assert_cmpuint(strlen(ssm->at), <, SCALE_BUF);
}

static void fill_areas(const float *a, int n)
{
    memcpy(seds->areas_array, a, n * sizeof(float));
}

static void fill_counts(const int *c, int n)
{
    memcpy(seds->counts_array, c, n * sizeof(int));
}

/* --- areas, irregular intervals ---------------------------------------- */

static void test_irreg_areas_single_pair(void)
{
    static const float a[NBINS] = {0, 1e6, 2e6, 3e6, 4e6, 5e6, 4e6, 3e6, 2e6, 1e6};

    setup(1);
    fill_areas(a, NBINS);
    add_pair(0, 10);
    se_histo_fin_irreg_areas();
    /* header lines, then scale, ticks and one bar line */
    check_output("irreg areas single pair", 7);
    g_assert_cmpint(count_char(nth_line(count_lines() - 2)->at, '|'), >=, 1);
    check_scale_lines(count_lines() - 3);
}

static void test_irreg_areas_equal_pairs(void)
{
    static const float a[NBINS] = {1e6, 1e6, 1e6, 1e6, 1e6, 1e6, 1e6, 1e6, 1e6, 1e6};

    setup(1);
    fill_areas(a, NBINS);
    add_pair(0, 3);
    add_pair(3, 6);
    add_pair(6, 10);
    se_histo_fin_irreg_areas();
    check_output("irreg areas equal pairs", 9);
    check_scale_lines(count_lines() - 5);
}

static void test_irreg_areas_normal(void)
{
    static const float a[NBINS] = {1e6, 4e6, 9e6, 16e6, 25e6, 36e6, 25e6, 16e6, 9e6, 4e6};
    int nl;

    setup(1);
    fill_areas(a, NBINS);
    add_pair(0, 2);
    add_pair(2, 5);
    add_pair(5, 7);
    add_pair(7, 10);
    se_histo_fin_irreg_areas();
    check_output("irreg areas normal", 10);

    nl = count_lines();
    g_assert_cmpint(count_char(nth_line(nl - 5)->at, '|'), ==, 6);
    /* the largest pair (5 to 7: 61 sq.km.) fills the 50-column scale */
    g_assert_cmpint(count_char(nth_line(nl - 2)->at, 'H'), ==, 50);
    g_assert_cmpint(count_char(nth_line(nl - 4)->at, 'H'), ==, 1);
    check_scale_lines(nl - 6);
}

static void test_irreg_areas_huge(void)
{
    static const float a[NBINS] = {1e38f, 2e38f, 3e38f, 1e38f, 2e38f, 3e38f,
                                   1e38f, 2e38f, 3e38f, 1e38f};

    setup(1);
    fill_areas(a, NBINS);
    add_pair(0, 4);
    add_pair(4, 10);
    se_histo_fin_irreg_areas();
    check_output("irreg areas huge", 8);
}

/* --- areas, regular intervals ------------------------------------------ */

static void test_areas_equal(void)
{
    static const float a[NBINS] = {5e6, 5e6, 5e6, 5e6, 5e6, 5e6, 5e6, 5e6, 5e6, 5e6};

    setup(0);
    fill_areas(a, NBINS);
    se_histo_fin_areas("t");
    check_output("areas equal", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

static void test_areas_single_bin(void)
{
    setup(0);
    seds->areas_array[3] = 7e6f;
    se_histo_fin_areas("t");
    check_output("areas single bin", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

static void test_areas_normal(void)
{
    static const float a[NBINS] = {1e6, 4e6, 9e6, 16e6, 25e6, 36e6, 25e6, 16e6, 9e6, 4e6};
    int nl;

    setup(0);
    fill_areas(a, NBINS);
    se_histo_fin_areas("t");
    check_output("areas normal", 10 + NBINS);
    nl = count_lines();
    g_assert_cmpint(count_char(nth_line(nl - NBINS - 1)->at, '|'), ==, 6);
    /* the biggest bin fills the 50-column scale, the smallest gets none
     * beyond the minimum mark */
    g_assert_cmpint(count_char(nth_line(nl - NBINS + 5)->at, 'H'), ==, 50);
    g_assert_cmpint(count_char(nth_line(nl - NBINS)->at, 'H'), ==, 1);
    check_scale_lines(nl - NBINS - 2);
}

static void test_areas_huge(void)
{
    static const float a[NBINS] = {1e38f, 2e38f, 3e38f, 1e38f, 2e38f, 3e38f,
                                   1e38f, 2e38f, 3e38f, 1e38f};

    setup(0);
    fill_areas(a, NBINS);
    se_histo_fin_areas("t");
    check_output("areas huge", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

/* --- counts ------------------------------------------------------------ */

static void test_irreg_counts_single_pair(void)
{
    static const int c[NBINS] = {0, 1, 2, 3, 4, 5, 4, 3, 2, 1};

    setup(1);
    fill_counts(c, NBINS);
    add_pair(0, 10);
    se_histo_fin_irreg_counts();
    check_output("irreg counts single pair", 4);
    check_scale_lines(count_lines() - 3);
}

static void test_irreg_counts_small_range(void)
{
    static const int c[NBINS] = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1};

    setup(1);
    fill_counts(c, NBINS);
    add_pair(0, 2);
    add_pair(2, 5);
    add_pair(5, 7);
    add_pair(7, 10);
    se_histo_fin_irreg_counts();
    check_output("irreg counts small range", 7);
    check_scale_lines(count_lines() - 6);
}

/* pair sums of 4 and 5: a range of 1, so 10*f_inc+.5 < 1 and the integer
 * scale never advanced */
static void test_irreg_counts_range_one(void)
{
    static const int c[NBINS] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

    setup(1);
    fill_counts(c, NBINS);
    add_pair(0, 1);
    add_pair(2, 4);
    se_histo_fin_irreg_counts();
    check_output("irreg counts range one", 5);
    check_scale_lines(count_lines() - 4);
}

static void test_irreg_counts_normal(void)
{
    static const int c[NBINS] = {10, 40, 90, 160, 250, 360, 250, 160, 90, 40};
    int nl;

    setup(1);
    fill_counts(c, NBINS);
    add_pair(0, 2);
    add_pair(2, 5);
    add_pair(5, 7);
    add_pair(7, 10);
    se_histo_fin_irreg_counts();
    check_output("irreg counts normal", 7);
    nl = count_lines();
    g_assert_cmpint(count_char(nth_line(nl - 5)->at, '|'), >=, 5);
    check_scale_lines(nl - 6);
}

static void test_counts_small_range(void)
{
    static const int c[NBINS] = {1, 2, 3, 1, 2, 3, 1, 2, 3, 1};

    setup(0);
    fill_counts(c, NBINS);
    se_histo_fin_counts();
    check_output("counts small range", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

static void test_counts_equal(void)
{
    static const int c[NBINS] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

    setup(0);
    fill_counts(c, NBINS);
    se_histo_fin_counts();
    check_output("counts equal", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

static void test_counts_huge(void)
{
    static const int c[NBINS] = {200000000, 100000000, 0, 199999999, 5, 6,
                                 7, 8, 9, 10};

    setup(0);
    fill_counts(c, NBINS);
    se_histo_fin_counts();
    check_output("counts huge", 10 + NBINS);
    check_scale_lines(count_lines() - NBINS - 2);
}

static void test_counts_normal(void)
{
    static const int c[NBINS] = {10, 40, 90, 160, 250, 360, 250, 160, 90, 40};
    int nl;

    setup(0);
    fill_counts(c, NBINS);
    se_histo_fin_counts();
    check_output("counts normal", 10 + NBINS);
    nl = count_lines();
    g_assert_cmpint(count_char(nth_line(nl - NBINS - 1)->at, '|'), >=, 5);
    g_assert_cmpint(count_char(nth_line(nl - NBINS + 5)->at, 'H'), ==, 50);
    check_scale_lines(nl - NBINS - 2);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/histogram/irreg_areas/single_pair", test_irreg_areas_single_pair);
    g_test_add_func("/histogram/irreg_areas/equal_pairs", test_irreg_areas_equal_pairs);
    g_test_add_func("/histogram/irreg_areas/normal", test_irreg_areas_normal);
    g_test_add_func("/histogram/irreg_areas/huge", test_irreg_areas_huge);
    g_test_add_func("/histogram/areas/equal", test_areas_equal);
    g_test_add_func("/histogram/areas/single_bin", test_areas_single_bin);
    g_test_add_func("/histogram/areas/normal", test_areas_normal);
    g_test_add_func("/histogram/areas/huge", test_areas_huge);
    g_test_add_func("/histogram/irreg_counts/single_pair", test_irreg_counts_single_pair);
    g_test_add_func("/histogram/irreg_counts/small_range", test_irreg_counts_small_range);
    g_test_add_func("/histogram/irreg_counts/range_one", test_irreg_counts_range_one);
    g_test_add_func("/histogram/irreg_counts/normal", test_irreg_counts_normal);
    g_test_add_func("/histogram/counts/small_range", test_counts_small_range);
    g_test_add_func("/histogram/counts/equal", test_counts_equal);
    g_test_add_func("/histogram/counts/huge", test_counts_huge);
    g_test_add_func("/histogram/counts/normal", test_counts_normal);

    return g_test_run();
}
