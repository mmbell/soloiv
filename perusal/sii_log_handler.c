/* sii_log_handler.c — see sii_log_handler.h for rationale. */

#include "sii_log_handler.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean g_installed = FALSE;
static gboolean g_fatal = FALSE;
static guint g_complaint_count = 0;

static const char *
log_level_name(GLogLevelFlags level)
{
    if (level & G_LOG_LEVEL_ERROR)    return "ERROR";
    if (level & G_LOG_LEVEL_CRITICAL) return "CRITICAL";
    if (level & G_LOG_LEVEL_WARNING)  return "WARNING";
    if (level & G_LOG_LEVEL_MESSAGE)  return "MESSAGE";
    if (level & G_LOG_LEVEL_INFO)     return "INFO";
    if (level & G_LOG_LEVEL_DEBUG)    return "DEBUG";
    return "UNKNOWN";
}

static void
sii_log_handler(const gchar     *log_domain,
                GLogLevelFlags   log_level,
                const gchar     *message,
                gpointer         user_data)
{
    (void)user_data;

    /* Always print: we want to see the message regardless of mode. */
    fprintf(stderr, "** (%s) %s: %s\n",
            log_domain ? log_domain : "soloiv",
            log_level_name(log_level),
            message ? message : "(null)");
    fflush(stderr);

    if (log_level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_ERROR)) {
        g_complaint_count++;
        if (g_fatal && (log_level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR))) {
            /* Abort only on CRITICAL/ERROR. WARNING is too chatty in legacy
             * code paths to be fatal by default — it still gets counted, so
             * tests can assert on the count if they want stricter behavior. */
            abort();
        }
    }
}

void
sii_log_handler_install(gboolean fatal)
{
    if (g_installed) {
        /* Allow upgrading from non-fatal to fatal but not downgrading. */
        if (fatal) g_fatal = TRUE;
        return;
    }

    g_fatal = fatal;
    g_installed = TRUE;

    /* Catch every domain. GTK, GLib, Pango, GDK, and our own messages all
     * route through g_log()/g_logv(), so a default handler covers them. */
    g_log_set_default_handler(sii_log_handler, NULL);

    /* In fatal mode we also flag CRITICAL as a fatal mask so that
     * g_assert / g_return_if_fail behave the way we want. */
    if (fatal) {
        g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL | G_LOG_FATAL_MASK);
    }
}

guint
sii_log_handler_get_complaint_count(void)
{
    return g_complaint_count;
}

void
sii_log_handler_reset_complaint_count(void)
{
    g_complaint_count = 0;
}
