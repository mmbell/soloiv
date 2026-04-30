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
