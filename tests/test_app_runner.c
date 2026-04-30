/* test_app_runner.c — see header for rationale. */

#include "test_app_runner.h"

#include "sii_log_handler.h"

extern GtkWidget *main_window;        /* soloii.c */
void soloiv_activate(GtkApplication *app, gpointer user_data); /* soloii.c */

typedef struct {
    GtkApplication       *app;
    TestAppActionFunc     action;
    gpointer              user_data;
} RunnerCtx;

static gboolean
quit_after_action_cb(gpointer user_data)
{
    RunnerCtx *ctx = (RunnerCtx *)user_data;
    g_application_quit(G_APPLICATION(ctx->app));
    return G_SOURCE_REMOVE;
}

static gboolean
fire_action_cb(gpointer user_data)
{
    RunnerCtx *ctx = (RunnerCtx *)user_data;

    if (ctx->action) {
        ctx->action(main_window, ctx->user_data);
    }

    /* Give any deferred work scheduled by the action a chance to run
     * before we quit. */
    test_app_runner_pump(20);

    g_timeout_add(50, quit_after_action_cb, ctx);
    return G_SOURCE_REMOVE;
}

static void
runner_activate_cb(GtkApplication *app, gpointer user_data)
{
    RunnerCtx *ctx = (RunnerCtx *)user_data;

    /* Bring up the real soloiv GUI (window, sweep load, ...). */
    soloiv_activate(app, NULL);

    /* Settle delay — paint, sweep finalization, idle handlers. 200 ms
     * has been enough so far; if a new test hits flake we'll measure. */
    g_timeout_add(200, fire_action_cb, ctx);
}

int
test_app_runner_run(const char *application_id,
                    TestAppActionFunc action,
                    gpointer user_data)
{
    RunnerCtx ctx = { 0 };
    int status;

    /* Non-fatal: legacy code emits CRITICAL during routine startup.
     * Tests assert via complaint-count *deltas*, not absolute zero. */
    sii_log_handler_install(FALSE);

    ctx.app = gtk_application_new(application_id, G_APPLICATION_NON_UNIQUE);
    ctx.action = action;
    ctx.user_data = user_data;

    g_signal_connect(ctx.app, "activate",
                     G_CALLBACK(runner_activate_cb), &ctx);

    status = g_application_run(G_APPLICATION(ctx.app), 0, NULL);

    g_object_unref(ctx.app);
    return status;
}

void
test_app_runner_pump(int iterations)
{
    for (int i = 0; i < iterations; i++) {
        g_main_context_iteration(NULL, FALSE);
    }
}
