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
| 6  | Tree-mode walker + remaining frame widgets       | 🔜 Next     |
| 7  | DORADE load-path robustness (fuzz)               | ⏳ Queued   |
| 8  | Paint-time UB in `solo_hardware_color_table`     | ⏳ Queued   |
| 9  | Visual issues: Magic Ring Lbls, window resize    | ⏳ Queued   |
| 10 | Screenshot capture + visual regression baseline  | ⏳ Queued   |
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

## 🔜 Phase 6 — Tree-mode walker + remaining frame widgets

**Goal:** every clickable widget in every major window has been
exercised in some test, not just the menubar.

The current walker only handles `GAction`-based items. The per-frame
buttons (`Data Widget`, frame-menu items), parameter widget controls,
color picker, and sweep file dialog all use plain `g_signal_connect`
callbacks and need a tree-walking mode.

**Work:**
- Add `widget_walker_walk_tree(GtkWidget *root, ...)` that recurses
  via `gtk_widget_get_first_child` / `get_next_sibling`, identifies
  `GtkButton`, `GtkCheckButton`, `GtkSwitch`, `GtkEntry`, `GtkListBox`
  rows, and exercises each (skip list by label).
- Add per-state tests:
  - `test_data_widget_opens` — `show_click_data_widget` for frame 0
  - `test_param_widget_walk` — open + walk every control
  - `test_palette_walk` — open the color picker, walk it
  - `test_swpfi_dialog_walk` — sweep-file dialog
  - `test_multi_frame_layout` — switch to 2x2 / 1x4 via menu, walk
  - one test that just stays in cold-start (no sweep) and walks

**Exit criteria:** every test passes under `default`. Failures get
fixed inline (each one is likely a small migration issue similar to
Phase 3). Document the walker's skip lists.

## ⏳ Phase 7 — DORADE load-path robustness (fuzz)

**Goal:** opening a malformed sweep file shows an error dialog, never
crashes (per `memory/dorade_robustness.md`).

**Work:**
- Generate ~50 mutated copies of one good sweep (random byte flips,
  truncations, oversized length fields) under `test_data/fuzz/`.
- New test: load each mutant via the same code path
  `solo_nab_next_file` uses, assert "either succeeds or returns
  cleanly with an error message; never crashes".
- Fix root causes in `sp_dorade.c` and the readers in
  `dd_crackers.h`-using TUs. Stay on the perusal side; do not chase
  bugs that only fire deep inside `translate/`.

**Exit criteria:** the fuzz test passes; every NULL-deref / OOB-read
on length fields in the perusal-reachable load path is gated.

## ⏳ Phase 8 — Paint-time UB in `solo_hardware_color_table`

**Goal:** clean ASan run for `test_walk_main_menu` and
`test_examine_opens`. Same site as the `-O2` `brk` instruction; both
are catching the same root cause from different angles.

The Phase 4 fix (matching `gclist` size to `MAX_COLOR_TABLE_SIZE`)
addressed one path, but ASan still reports a write at line 3688 with
"insufficient space for an object of type 'float'" during first paint.
The reported address doesn't sit inside `gclist`, so something else is
in play. Plausible candidates: a different overflowing array along the
draw path, a pointer reinterpretation across struct boundaries, or
clang's optimizer collapsing dead paths in a way that ASan instruments
oddly.

**Work:**
- Reproduce under ASan with `-O0` to get clean line info.
- Identify which allocation ASan is bounds-checking against.
- Fix; verify the `-O2` `RelWithDebInfo` trap also goes away.

**Exit criteria:** `ctest --preset debug-asan` passes; running the
binary built `RelWithDebInfo` no longer traps during paint.

## ⏳ Phase 9 — Visual issues

User-flagged at session start. These don't crash but they look wrong
and should be fixed before we declare the GUI shippable.

**9a. Magic Ring Lbls misalignment**
- Symptom: dashed-line ring labels don't align with the polar grid;
  toggling them off works.
- Likely cause: a coordinate transform that didn't survive the GTK4
  port. Search `sii_xyraster.c`, the magic-ring-related callbacks.
- Test: render a frame to an offscreen Cairo surface with rings on
  vs off, dump PNG for review.

**9b. Window resize creates a second image**
- Symptom: dragging the window edge produces a duplicate image
  instead of rescaling.
- Likely cause: the draw callback isn't re-rendering at the new size,
  or there's stale cached pixmap state.
- Fallback (per the user): if it can't be made to rescale, disable
  `gtk_window_set_resizable` and force users to use the Zoom menu.
- Test: programmatically resize the main window and screenshot;
  assert no doubled pixels in the centerline.

## ⏳ Phase 10 — Screenshot capture + visual regression baseline

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
