/* sii_log_handler.h
 *
 * Optional GLib log handler that promotes G_LOG_LEVEL_CRITICAL and
 * G_LOG_LEVEL_WARNING messages to fatal so that the program (or test)
 * fails loudly instead of silently degrading.
 *
 * Used in two contexts:
 *   - the soloiv binary, when launched with --debug-warnings
 *   - test harness binaries, always (so that "no CRITICAL" is asserted)
 */

#ifndef SII_LOG_HANDLER_H
#define SII_LOG_HANDLER_H

#include <glib.h>

/* Install a GLib log handler that records and (optionally) aborts on
 * G_LOG_LEVEL_CRITICAL and G_LOG_LEVEL_WARNING.
 *
 * If fatal is TRUE, the process aborts on the first such message
 * (after printing a backtrace via g_on_error_query in debug builds).
 *
 * If fatal is FALSE, messages are still counted and printed, but the
 * process continues. Useful for the production binary's --debug-warnings
 * mode where we want visibility without aborting on every legacy warning.
 *
 * Safe to call multiple times; only the first call takes effect.
 */
void sii_log_handler_install(gboolean fatal);

/* Number of CRITICAL+WARNING messages observed since installation.
 * Used by tests to assert "expected zero noise" without having to register
 * a custom recorder. */
guint sii_log_handler_get_complaint_count(void);

/* Reset the counter. Useful between test cases that share a process. */
void sii_log_handler_reset_complaint_count(void);

#endif /* SII_LOG_HANDLER_H */
