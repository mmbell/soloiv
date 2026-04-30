/* widget_walker.c — see header for rationale. */

#include "widget_walker.h"
#include "test_app_runner.h"

#include <string.h>

/* Provided by perusal/soloii.c. GTK4 has no public widget→action_group
 * lookup, so we go through this getter rather than try to introspect. */
GActionGroup *soloiv_get_main_menu_actions(void);

/* Action signatures we know about. The main menu uses three patterns:
 *   - parameterless          (e.g. "quit", "help_about")
 *   - takes int32 parameter  (e.g. "cfg_22(22)")
 *   - stateful boolean       (e.g. "electric_ctr")
 *
 * For walking we activate each one in the way the menu would: pull
 * the parameter type from the GAction itself, build a synthetic
 * parameter (matching the menu's hard-coded value), and dispatch.
 *
 * The synthetic int32 we use here is 0 — it doesn't have to match
 * the menu's specific value because the actions parse it as a
 * "magic-number switch" and 0 is a recognized "default" in most of
 * them. If a particular action turns out to misbehave on 0 we'll add
 * a per-action override map later. */

static gboolean
is_in_skip_list(const char *name, const char **skip)
{
    if (!skip) return FALSE;
    for (int i = 0; skip[i] != NULL; i++) {
        if (strcmp(name, skip[i]) == 0) return TRUE;
    }
    return FALSE;
}

static GVariant *
synthesize_parameter_for(GAction *action)
{
    const GVariantType *param_type = g_action_get_parameter_type(action);
    if (!param_type) {
        return NULL;
    }
    if (g_variant_type_equal(param_type, G_VARIANT_TYPE_INT32)) {
        return g_variant_new_int32(0);
    }
    if (g_variant_type_equal(param_type, G_VARIANT_TYPE_BOOLEAN)) {
        return g_variant_new_boolean(TRUE);
    }
    if (g_variant_type_equal(param_type, G_VARIANT_TYPE_STRING)) {
        return g_variant_new_string("");
    }
    /* Unhandled types: skip rather than guess. */
    return NULL;
}

int
widget_walker_run_action_group(GtkWidget *widget,
                               const char *group_name,
                               const char **skip_actions)
{
    GActionGroup *group;
    char **names;
    int activated = 0;

    (void)widget;
    g_assert_nonnull(group_name);

    /* For now we only know one action group: the main menu. If
     * future tests want others, we'd add more getters in soloii.c. */
    g_assert_cmpstr(group_name, ==, "menu");
    group = soloiv_get_main_menu_actions();
    g_assert_nonnull(group);

    names = g_action_group_list_actions(group);
    g_assert_nonnull(names);

    for (int i = 0; names[i] != NULL; i++) {
        const char *aname = names[i];
        GAction *action;

        if (is_in_skip_list(aname, skip_actions)) {
            continue;
        }

        action = g_action_map_lookup_action(G_ACTION_MAP(group), aname);
        if (!action) {
            continue;  /* listed but not a map entry — exotic, skip */
        }

        if (!g_action_get_enabled(action)) {
            continue;  /* disabled actions are not user-clickable */
        }

        GVariant *param = synthesize_parameter_for(action);
        if (param) {
            g_variant_ref_sink(param);
        }

        /* Activate. Any CRITICAL/abort raised by the handler will
         * either crash the test process (caught by ctest) or bump the
         * complaint counter (caught by the test's delta assertion). */
        g_action_group_activate_action(group, aname, param);

        if (param) {
            g_variant_unref(param);
        }

        /* Pump so the action's effects (e.g. window relayout) settle
         * before we move on. Otherwise a subsequent action might
         * encounter half-built state and we'd misattribute the bug. */
        test_app_runner_pump(5);

        activated++;
    }

    g_strfreev(names);
    return activated;
}

gboolean
widget_walker_fire_action(GtkWidget *widget,
                          const char *group_name,
                          const char *action_name,
                          GVariant   *parameter)
{
    GActionGroup *group;

    (void)widget;
    g_assert_cmpstr(group_name, ==, "menu");
    group = soloiv_get_main_menu_actions();
    if (!group) return FALSE;

    if (!g_action_group_has_action(group, action_name)) return FALSE;

    g_action_group_activate_action(group, action_name, parameter);
    test_app_runner_pump(5);
    return TRUE;
}

/* Tree walk implementation -------------------------------------------- */

static const char *
widget_label(GtkWidget *w)
{
    /* GtkButton with text label */
    if (GTK_IS_BUTTON(w)) {
        const char *l = gtk_button_get_label(GTK_BUTTON(w));
        if (l) return l;
    }
    /* GtkCheckButton (which in GTK4 is independent of GtkToggleButton) */
    if (GTK_IS_CHECK_BUTTON(w)) {
        const char *l = gtk_check_button_get_label(GTK_CHECK_BUTTON(w));
        if (l) return l;
    }
    /* GtkLabel — sometimes used as the "label" of a complex button */
    if (GTK_IS_LABEL(w)) {
        const char *l = gtk_label_get_text(GTK_LABEL(w));
        if (l) return l;
    }
    return "";
}

static gboolean
label_in_skip_list(const char *label, const char **skip)
{
    if (!skip || !label || !*label) return FALSE;
    for (int i = 0; skip[i] != NULL; i++) {
        if (strstr(label, skip[i])) return TRUE;
    }
    return FALSE;
}

/* Visit one widget. Returns 1 if it was a clickable we exercised,
 * 0 otherwise. Children are walked by the caller. */
static int
visit_widget(GtkWidget *w, const char **skip_labels)
{
    if (!w || !gtk_widget_get_visible(w) || !gtk_widget_get_sensitive(w)) {
        return 0;
    }

    const char *label = widget_label(w);
    if (label_in_skip_list(label, skip_labels)) {
        return 0;
    }

    /* GtkCheckButton: toggle, pump, toggle back. We do this BEFORE the
     * GtkButton check because GtkCheckButton is a GtkWidget that
     * contains a GtkButton-like surface; gtk_widget_activate on it
     * will toggle, but doing it explicitly is clearer. */
    if (GTK_IS_CHECK_BUTTON(w)) {
        gboolean was_active = gtk_check_button_get_active(GTK_CHECK_BUTTON(w));
        gtk_check_button_set_active(GTK_CHECK_BUTTON(w), !was_active);
        test_app_runner_pump(3);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(w), was_active);
        test_app_runner_pump(3);
        return 1;
    }

    /* GtkSwitch: same toggle-and-back pattern. */
    if (GTK_IS_SWITCH(w)) {
        gboolean was = gtk_switch_get_active(GTK_SWITCH(w));
        gtk_switch_set_active(GTK_SWITCH(w), !was);
        test_app_runner_pump(3);
        gtk_switch_set_active(GTK_SWITCH(w), was);
        test_app_runner_pump(3);
        return 1;
    }

    /* GtkButton (and subclasses like GtkLinkButton, GtkMenuButton).
     * gtk_widget_activate emits "clicked" without needing pointer
     * coordinates. */
    if (GTK_IS_BUTTON(w)) {
        gtk_widget_activate(w);
        test_app_runner_pump(3);
        return 1;
    }

    return 0;
}

static int
walk_recursive(GtkWidget *w, const char **skip_labels, int depth)
{
    int count = 0;

    if (!w) return 0;
    /* Cap depth to avoid pathological loops in case of cyclic
     * widget references (shouldn't happen in GTK, but be safe). */
    if (depth > 64) return 0;

    count += visit_widget(w, skip_labels);

    /* Some widgets keep popovers / overlays attached separately.
     * For now we just walk normal children; popover walking can be
     * added later if a test needs it. */
    GtkWidget *child = gtk_widget_get_first_child(w);
    while (child) {
        count += walk_recursive(child, skip_labels, depth + 1);
        child = gtk_widget_get_next_sibling(child);
    }
    return count;
}

int
widget_walker_walk_tree(GtkWidget *root, const char **skip_labels)
{
    g_assert_nonnull(root);
    return walk_recursive(root, skip_labels, 0);
}
