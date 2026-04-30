# soloiv hardening roadmap

This is the working plan for the soloiv GTK4 revival. The goal is **a
user can click any widget in any window state without crashing the
program or producing GLib `CRITICAL`/`WARNING` messages**, with the
known visual bugs either fixed or worked around.

The roadmap is renumbered from earlier in-conversation labels (which
grew organically as `2a`, `2b`, `2c`, etc.) into a clean linear
sequence. The order reflects what catches the most user-visible bugs
per hour of work, not strict architectural dependencies — feel free to
reorder if priorities shift.

Out-of-scope (per `memory/project_scope.md`): `translate/` internals
(being replaced by LROSE tooling) and the editor's non-GUI logic.

---

## Status at a glance

| #  | Phase                                            | Status      |
|----|--------------------------------------------------|-------------|
| 1  | Test infrastructure & CMake presets              | ✅ Done     |
| 2  | GLib log handler, `--debug-warnings`             | ✅ Done     |
| 3  | Examine widget regression test + root-cause fix  | ✅ Done     |
| 4  | Shared harness + action-group walker             | ✅ Done     |
| 5  | Edit widget smoke test                           | ✅ Done     |
| 6  | Tree-mode walker + remaining frame widgets       | ✅ Done     |
| 7  | DORADE load-path robustness (initial fuzz)       | ✅ Done     |
| 8  | Paint-time UB in `solo_hardware_color_table`     | ✅ Done     |
| 9  | Visual issues: Magic Ring Lbls, window resize    | ✅ Done     |
| 10 | Screenshot capture + visual regression baseline  | 🔜 Next     |
| 11 | Pure-logic unit tests (color/coord/config)       | ⏳ Queued   |

---

## ✅ Phase 1 — Test infrastructure & CMake presets

`tests/` directory wired into CMake via `SOLOIV_BUILD_TESTS`. Two
presets: `default` (Debug, tests on, no sanitizers) and `debug-asan`
(Debug + ASan + UBSan with translate/ leak suppressions). The perusal
build was refactored so all `.c` files compile into a `soloiv_core`
static library, with `soloii_main.c` as a thin `main()` wrapper. Tests
link `soloiv_core` directly without colliding on `main()`.

CTest runs every test from `WORKING_DIRECTORY=test_data/` so the
binary picks up sample sweeps the way the user does manually.

## ✅ Phase 2 — GLib log handler + `--debug-warnings`

`perusal/sii_log_handler.{h,c}` installs a GLib log handler that
counts and (optionally) aborts on CRITICAL. Used by every test in
non-fatal mode for delta assertions; available in the production
binary via `--debug-warnings` so manual reproductions get the same
visibility as automated tests.

## ✅ Phase 3 — Examine widget regression test + root-cause fix

`tests/test_examine_opens.c` reproduces the user-reported crash. Root
cause: `sii_exam_menubar2()` called `gtk_check_button_set_active(...,
TRUE)` during construction, which fires the menu callback before the
entry widgets it reads (`EXAM_RAY` etc.) exist. Fixed by extracting
radio-state initialization into `sii_exam_finalize_radio_state()`,
called from `sii_exam_widget()` after all data widgets are built.

Three additional GTK4 migration completion fixes were unmasked while
chasing this and got fixed at the same time: `key_press_event` signal
on `GtkWindow` (now uses `GtkEventControllerKey`), `button_press_event`
signal on `GtkLabel` (now uses `GtkGestureClick`), and an invalid
`GTK_SCROLLABLE(GtkFixed)` cast. Plus a defensive
`wwptr->lead_sweep = wwptr` fallback in `editor/sxm_examine.c` for
frames not joined to a lockstep.

## ✅ Phase 4 — Shared harness + action-group walker

`tests/test_app_runner.{h,c}` is the one-process-per-test harness:
brings up `GtkApplication`, runs the real `soloiv_activate`, fires a
test action after a settle delay, pumps the loop, quits.

`tests/widget_walker.{h,c}` first mode: enumerate every action in the
main window's `"menu"` `GActionGroup` and fire each with a synthesized
parameter. Skip list bypasses `quit` and the file-chooser openers.

`tests/test_walk_main_menu.c` runs the walker; asserts ≥30 actions
activated and zero new complaints. Caught a heap overflow in
`solo_hardware_color_table` where `gclist` was sized for 256 entries
while downstream arrays are `MAX_COLOR_TABLE_SIZE` (128); fixed by
matching sizes and bounding the parse loop.

## ✅ Phase 5 — Edit widget smoke test

`tests/test_edit_opens.c`. Same pattern as the Examine test, applied
to `show_edit_widget`. Passes — confirms the fix pattern works for
other frame widgets and the editor entry path doesn't have analogous
bugs.

---

## ✅ Phase 6 — Tree-mode walker + remaining frame widgets

`widget_walker_walk_tree()` recurses via `gtk_widget_get_first_child`
/ `get_next_sibling` and exercises `GtkButton`, `GtkCheckButton`, and
`GtkSwitch`. Skip list bypasses Cancel/Close/Quit/Browse-style
buttons.

Tests added:
- `test_data_widget_opens` — third frame-menu button. Clean.
- `test_param_widget_walk` — opens the parameter widget and walks its
  visible controls. Caught a leftover `gtk_check_button_set_active`
  on `data_widget[PARAM_CB_BOTTOM]` (now NULL since the Options menu
  was migrated to `GMenu` actions); fixed with NULL-checks until that
  state can be threaded through stateful actions.
- `test_swpfi_dialog_walk` — sweep-file dialog. Clean.
- `test_multi_frame_layout` — fires `cfg_11`, `cfg_22`, `cfg_14`,
  `cfg_41`, `cfg_22` in sequence. Clean.

Palette-specific test was dropped: the color picker / palette list is
reachable only through the parameter widget's `GMenu` action group
("List Palettes" etc.), which `test_param_widget_walk` already
covers transitively.

## ✅ Phase 7 — DORADE load-path robustness (initial fuzz)

Two tests, each in their own binary so GtkApplication state doesn't
leak between scenarios:

- `test_dorade_truncated` — empty file with a sweep-pattern name in
  a tempdir that `DORADE_DIR` points at. Passes; the loader skips
  the empty file gracefully.
- `test_dorade_partial_header` — first 100 bytes of a real sweep
  (mid-SSWB superblock). Passes.

This is the initial fuzz baseline. More aggressive mutations
(garbage-after-header, oversized length fields, negative offsets) can
be added as specific user-reported crashes surface. The infrastructure
(tempdir setup, `DORADE_DIR` override, `copy_n_bytes` helper) makes
each new corruption variant a small file.

## ✅ Phase 8 — Paint-time UB in `solo_hardware_color_table`

Root cause: `gboolean ok2;` in `solo_hardware_color_table` was
declared without an initializer and then read in `if (!ok2)` checks.
Reading uninitialized storage is undefined behavior, and the compiler
exploits it differently at each opt level:

- `-O0` + ASan: silent (whatever was on the stack happened to behave).
- `-O1` + ASan: ASan flags it as "store with insufficient space" at
  the next memory op, since the optimizer's value tracking diverges
  from the memory model.
- `-O2`: clang infers a code path is unreachable and inserts a
  `brk #0x1` at the boundary, which fires during first paint.

Fix was a one-line `ok2 = TRUE` initializer (the historical intent was
"presume success unless a sub-step failed"). After the fix, all 10
tests pass under three configurations: default Debug, `debug-asan`,
and `RelWithDebInfo` (-O2).

## ✅ Phase 9 — Visual issues

**9a. Magic Ring Lbls misalignment — fixed.**

Root cause: `sii_cairo_draw_arc` in `solo2.c` called `cairo_restore`
*before* `cairo_stroke`, so the path constructed under the
translate+scale CTM was stroked using the restored CTM. The arc that
was supposed to be at `(cx, cy)` rendered at the device origin
instead, producing the "strange dashed lines that aren't aligned with
the polar grid" the user reported. Fixed by stroking inside the
save/restore for the ellipse case, and added a fast path that skips
the CTM dance entirely for the (only-used) circle case.

**9b. Window resize creates a second image — disabled with a note.**

`sii_frame_resize_cb` in `sii_callbacks.c` is defined but was never
connected to any signal. Connecting it to the GTK4 GtkDrawingArea
"resize" signal is the obvious fix, but trips a re-entrant
`sii_plot_data2` call inside the GTK snapshot pipeline that crashes
GTK's CSS state machine. Sequencing that properly is more work than
this phase wanted, so per the user's stated fallback, drag-resize is
disabled (`gtk_window_set_resizable(FALSE)`); users use the Zoom menu.
Reconnecting resize is queued as future work — needs the layout
pipeline to be reordered so allocation changes don't replot during
snapshot.

## 🔜 Phase 10 — Screenshot capture + visual regression baseline

**Goal:** we can tell whether a future change broke the rendering,
without needing eyes on a screen.

**Work:**
- Add a helper that snapshots a `GtkWindow` to a Cairo image surface
  via `gtk_widget_snapshot` and writes a PNG.
- Each existing test that opens a window dumps a PNG to
  `${CMAKE_BINARY_DIR}/test-screenshots/<test>.png`.
- Once we have a known-good run (post Phase 9), freeze them as
  `tests/baseline/` PNGs. New runs diff against baselines; differences
  outside a tolerance fail the test.

**Exit criteria:** a code change that breaks the polar grid causes a
test to go red without anyone looking at the screen.

## ⏳ Phase 11 — Pure-logic unit tests (lower priority)

The walker tests cover GUI flow. This phase covers the leaf logic that
links without GTK, for real `llvm-cov` numbers.

**Targets:** `sii_utils.c`, `sii_old_utils.c`, `sii_colors_stuff.c`,
`sp_colors.c`, `sp_pals.c`, `sii_config_stuff.c`, `sii_xyraster.c`,
selected helpers from `solo2.c`.

**Work:** mark these TUs as a `soloiv_logic` static lib in CMake;
write GLib-test-based unit tests against it.

**Exit criteria:** coverage report exists; gaps are intentional, not
accidental.

---

## Open questions

- **Threshold for "done"**: when do we ship? I'd suggest after
  Phase 9 (no crashes, no visual obvious bugs). Phase 10 / 11 are
  long-tail quality work.
- **CfRadial via LROSE**: future feature, separate session, not on
  this roadmap. Will replace `translate/` entirely; some of these
  phases (especially 7 and parts of 11) will be obviated by that
  switch.
