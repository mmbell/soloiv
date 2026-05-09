# tests/ — soloiv test suite (CTest + GLib test framework)
#
# Tests link against:
#   - soloiv_core           the production code (built in perusal/)
#   - soloiv_test_support   shared test helpers (this directory)
#
# Each test is its own executable so a crash in one does not kill the
# others. Each test runs with cwd = test_data/ (set on the CTest
# property, not at process start) so the binary picks up the sample
# DORADE sweeps the same way the user does manually.

# --- Shared test helpers ----------------------------------------------------

add_library(soloiv_test_support STATIC
    test_app_runner.c
    widget_walker.c
)

target_include_directories(soloiv_test_support PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/perusal
    ${CMAKE_SOURCE_DIR}/include
    ${GTK4_INCLUDE_DIRS}
)

target_link_directories(soloiv_test_support PUBLIC ${GTK4_LIBRARY_DIRS})

target_link_libraries(soloiv_test_support PUBLIC
    soloiv_core
    ${GTK4_LIBRARIES}
)

# --- Helper to declare a test ----------------------------------------------

function(soloiv_add_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE soloiv_test_support)
    target_compile_options(${name} PRIVATE
        -Wno-implicit-function-declaration
        -Wno-implicit-int
    )
    add_test(NAME ${name} COMMAND ${name})

    # NOTE: We deliberately do NOT set G_DEBUG=fatal-criticals here. The
    # legacy GUI emits CRITICAL messages during routine startup (logged
    # but not crashing in real use), and forcing them fatal would prevent
    # any test from getting past activate. Each test installs the log
    # handler in non-fatal mode and uses *delta* assertions to detect
    # complaints introduced by the action under test.
    set_tests_properties(${name} PROPERTIES
        TIMEOUT 60
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test_data
    )
endfunction()

# --- Tests ------------------------------------------------------------------

soloiv_add_test(test_smoke)
soloiv_add_test(test_examine_opens)
soloiv_add_test(test_edit_opens)
soloiv_add_test(test_data_widget_opens)
soloiv_add_test(test_param_widget_walk)
soloiv_add_test(test_swpfi_dialog_walk)
soloiv_add_test(test_multi_frame_layout)
soloiv_add_test(test_walk_main_menu)
soloiv_add_test(test_dorade_truncated)
soloiv_add_test(test_dorade_partial_header)
soloiv_add_test(test_cvd_colormaps)
soloiv_add_test(test_param_minmax_round_trip)
