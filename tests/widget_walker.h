/* widget_walker.h
 *
 * Functions that exhaustively exercise the soloiv GUI. They are how
 * we automate "click every button and don't crash."
 *
 * Two complementary modes:
 *
 * 1. Action-group walk — the menubar is built with GAction (modern GTK4
 *    pattern). We can fire each action by name + parameter without
 *    simulating clicks. This catches all menubar items in one pass.
 *
 * 2. Widget-tree walk — the per-frame buttons (Examine, Edit, etc.)
 *    and the dialogs they open are still GtkButton + g_signal_connect.
 *    We descend the widget tree and gtk_widget_activate() each clickable.
 *
 * Both modes record a log of widget identifiers they touched so that on
 * failure we know exactly which control reproduced the bug.
 */

#ifndef WIDGET_WALKER_H
#define WIDGET_WALKER_H

#include <gtk/gtk.h>

/* Walk every action in the named action group attached to widget,
 * activating each one in turn and pumping the loop between activations.
 *
 * skip_actions is a NULL-terminated array of action names to bypass —
 * use this for actions that would tear down the test (e.g. "quit") or
 * pop a modal file chooser that blocks the loop ("set_source_dir").
 *
 * Returns the number of actions actually activated. The action group
 * lookup failing is treated as a hard error (g_assert).
 */
int widget_walker_run_action_group(GtkWidget       *widget,
                                   const char      *group_name,
                                   const char     **skip_actions);

/* Convenience wrapper that activates one named action with an optional
 * int32 parameter (NULL for actions that take no parameter). Returns
 * TRUE if the action existed and was activated, FALSE otherwise.
 *
 * Useful in tests that want to drive the app to a specific state
 * (e.g. "switch to 1x1 layout") before running the walker. */
gboolean widget_walker_fire_action(GtkWidget   *widget,
                                   const char  *group_name,
                                   const char  *action_name,
                                   GVariant    *parameter);

/* Tree-walk mode. Recursively descends the widget hierarchy starting
 * at root and exercises every clickable descendant:
 *
 *   GtkButton, GtkLinkButton, GtkMenuButton  -> gtk_widget_activate
 *   GtkCheckButton, GtkToggleButton          -> toggle, pump, toggle back
 *   GtkSwitch                                -> set_active opposite, pump
 *
 * Between exercises the loop is pumped so the side effects of one
 * action settle before the next runs.
 *
 * skip_labels is a NULL-terminated array of substrings; any clickable
 * whose visible label contains one of those substrings is bypassed.
 * Use this for buttons that would tear down the test (Close, Cancel)
 * or open modal dialogs (file choosers).
 *
 * Returns the number of clickables actually exercised. */
int widget_walker_walk_tree(GtkWidget *root, const char **skip_labels);

#endif /* WIDGET_WALKER_H */
