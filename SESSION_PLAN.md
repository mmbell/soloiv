# Soloiv GTK4 Migration - Remaining Session Plan

## Completed
- **Phase 0**: CMake build system (4 CMakeLists.txt files created)
- **Phase 1 - All file transformations**: Every GTK-using source file has been converted from GTK1.x to GTK4 APIs:
  - soloii.c, soloii.h, sii_externals.h (rewritten)
  - sii_gtk4_compat.c (new - font/color/drawing helpers)
  - sii_utils.c, sii_utils.h
  - sii_X_stuff.c
  - sii_colors_stuff.c
  - sii_callbacks.c
  - sii_config_stuff.c
  - sii_frame_menu.c
  - sii_links_widget.c
  - sii_swpfi_widgets.c
  - sii_view_widgets.c (perusal/ and translate/)
  - sii_edit_widgets.c
  - sii_exam_widgets.c
  - solo2.c
  - sii_param_widgets.c
- **GTK4 installed** via Homebrew on macOS

## Session B: First Build Attempt -- COMPLETED

**Result**: Binary builds successfully! All compile and link errors resolved.

**What was fixed**:
- translate/ and editor/ legacy K&R C: suppressed implicit-function-declaration, implicit-int, return-mismatch warnings
- Fixed `static count=0` (implicit int) in hrd_dd.c, nex_dd.c
- Fixed bare `return;` in non-void functions (gpro_data.c, piraq_dd.c)
- Added `GdkPoint` typedef and `widget_origin[]` back to SiiFrameConfig
- Created GTK4 menu helpers: `sii_submenu()`, `sii_submenu_item()`, `sii_toggle_submenu_item()` using GtkPopover/GtkButton
- Replaced `gtk_menu_bar_new()` → `gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0)` in all files
- Replaced `GTK_CHECK_MENU_ITEM`/`GTK_RADIO_MENU_ITEM` → `GTK_CHECK_BUTTON` throughout
- Removed unused `GtkItemFactoryEntry` tables (param, view, exam menus)
- Fixed `sii_do_annotation` signature to include `cairo_t *cr`
- Replaced `GdkRGBA.pixel` references (no longer exists)
- Fixed `GdkEventKey` → GTK4 `GtkEventControllerKey` callback signature
- Replaced `gtk_event_box_new` (removed) with direct widget usage
- Replaced `gtk_layout_*` with `gtk_fixed_*`
- Added `XPutPixel` and `XAllocColor` stubs to solo_window_structs.h
- Renamed callback functions to match `_cb` suffix convention
- Added `sii_filesel` stub
- Fixed `gtk_container_foreach` → custom `sii_widget_foreach`
- Fixed `g_string_printfa` → `g_string_append_printf`
- Fixed `gtk_box_pack_end` → `gtk_box_append`
- Fixed `gtk_drawing_area_size` → `gtk_widget_set_size_request`
- Fixed `gtk_label_set` → `gtk_label_set_text`
- Added `target_link_directories` for GTK4 library paths

## Session C: Continue Fixing Compile Errors

**Goal**: Get the project to link successfully.

**Prompt**: "Continue fixing soloiv compile errors. Run `cmake --build build 2>&1 | head -80` and fix the next batch. Keep going until it links or you hit a blocking issue."

**Expected issues**:
- Linker errors from mismatched function signatures between files
- Missing function implementations (functions declared but not defined)
- Circular dependencies between translate/ and perusal/
- editor/ files may need minor GTK4 updates (they were not fully audited)

## Session D: Link & Runtime Testing

**Goal**: Get a running binary and fix runtime crashes.

**Prompt**: "The soloiv build should be close to linking. Fix any remaining link errors. Once it builds, try running `./build/perusal/soloiv` and fix any immediate crashes. Check that the main window appears."

**Expected issues**:
- Runtime crashes from NULL widget pointers
- Missing GAction registrations
- Draw functions not properly connected
- Font initialization failures

## Session E (optional): 64-bit Portability

**Goal**: Fix int/pointer cast warnings in translate/ for 64-bit.

**Prompt**: "Audit translate/ source files for 64-bit portability issues. Search for `(int)` casts of pointers and fix them. Use `intptr_t` or proper pointer types."

## Notes
- GTK4 is installed at /opt/homebrew/Cellar/gtk4/4.20.3
- Build command: `cmake -B build -S . && cmake --build build`
- Clean build: `rm -rf build && cmake -B build -S .`
- Backup files exist as .bak and .bak2 - can be cleaned up after successful build
