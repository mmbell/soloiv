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

# Curated, version-controlled test fixtures (small range-clipped real sweeps).
# Tests resolve data from here instead of the large, git-ignored test_data/
# tree, so a clean checkout and GitHub CI can run the data-path tests.
# Overridable at runtime via the SOLOIV_FIXTURE_DIR env var.
target_compile_definitions(soloiv_test_support PUBLIC
    SOLOIV_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
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

set(FIXTURES ${CMAKE_SOURCE_DIR}/tests/fixtures)

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

# These GUI tests open data-dependent widgets, so they need a loaded sweep:
# run them with cwd = a committed fixture sweep dir. (Previously they relied
# on sweeps sitting at test_data/ root, which no longer holds any after the
# data was reorganized into per-dataset subdirs.)
foreach(t test_examine_opens test_edit_opens test_param_widget_walk
          test_swpfi_dialog_walk test_multi_frame_layout
          test_param_minmax_round_trip)
    set_tests_properties(${t} PROPERTIES WORKING_DIRECTORY ${FIXTURES}/ground/cf2)
endforeach()

if(SOLOIV_IO_BACKEND STREQUAL "radx")

    # cwd = tests/fixtures so the relative ground/{cf2,dorade} sample paths
    # resolve.
    soloiv_add_test(test_radx_smoke)
    target_include_directories(test_radx_smoke PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_smoke PROPERTIES WORKING_DIRECTORY ${FIXTURES})

    # Runs with cwd = the ground CfRadial fixture dir so default startup loads
    # the cfrad2.*.nc sweep through the Radx backend.
    soloiv_add_test(test_cfradial_loads)
    target_include_directories(test_cfradial_loads PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_cfradial_loads PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ground/cf2)

    # DORADE-via-Radx vs CfRadial-via-Radx parity on the twinned ground samples.
    soloiv_add_test(test_radx_parity)
    target_include_directories(test_radx_parity PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_parity PROPERTIES WORKING_DIRECTORY ${FIXTURES})

    # Airborne parity: DORADE-via-Radx vs CfRadial-via-Radx on the N42RF tail
    # sweep (georef/AIR scan mode).
    soloiv_add_test(test_radx_airborne)
    target_include_directories(test_radx_airborne PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_airborne PROPERTIES
        WORKING_DIRECTORY ${FIXTURES})

    # CfRadial read -> edit -> write -> reopen round-trip.
    soloiv_add_test(test_radx_roundtrip)
    target_include_directories(test_radx_roundtrip PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_roundtrip PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ground/cf2)

    # Regression: dropping one field (ignore-field) then writing must NOT
    # disturb the remaining fields' data, and must preserve the airborne
    # primary_axis so the reopened sweep stays AIR (not misclassified RHI,
    # which collapsed the display into a wedge).
    soloiv_add_test(test_radx_drop_field)
    target_include_directories(test_radx_drop_field PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_drop_field PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ground)

    # Regression: after an in-place edit overwrites a sweep, the rio read cache
    # must be invalidated so a re-read returns the edited data (drives Replot
    # refresh and chained edits).
    soloiv_add_test(test_radx_reload)
    target_include_directories(test_radx_reload PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_radx_reload PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ground/cf2)

    # Examine widget on a CfRadial sweep (rewind + per-ray display list).
    soloiv_add_test(test_cfradial_examine)
    set_tests_properties(test_cfradial_examine PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ground/cf2)

    # Regression: shipborne SEAPOL (mislabeled platform_type=aircraft, but
    # primary_axis=axis_z) must render as a full surveillance PPI, not the
    # ~88-degree wedge produced when it was forced into AIR scan mode.
    soloiv_add_test(test_seapol_wedge)
    target_include_directories(test_seapol_wedge PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_seapol_wedge PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/ship/cf2)

    # Regression: CfRadial filename -> radar name must be consistent across
    # cf1/cf2 so the catalog groups a radar's sweeps for next/prev navigation.
    soloiv_add_test(test_cfradial_filename)
    target_include_directories(test_cfradial_filename PRIVATE ${CMAKE_SOURCE_DIR}/translate)

    # Issue #2: a multi-sweep CfRadial volume must expose every sweep, not just
    # sweep 0. The reader honors dgi->rio_req_sweep to step through all sweeps.
    soloiv_add_test(test_cfradial_multisweep)
    target_include_directories(test_cfradial_multisweep PRIVATE ${CMAKE_SOURCE_DIR}/translate)
    set_tests_properties(test_cfradial_multisweep PROPERTIES
        WORKING_DIRECTORY ${FIXTURES}/volume)
endif()
