# perusal/ - soloiv GUI executable + reusable core library
#
# All perusal/ source files compile into a static library (soloiv_core) so
# that test binaries can link the same code without colliding with main().
# The soloiv executable is a thin wrapper: soloii_main.c + soloiv_core.

set(SOLOIV_CORE_SOURCES
    soloii.c
    sii_colors_stuff.c
    sii_config_stuff.c
    sii_callbacks.c
    sii_edit_widgets.c
    sii_utils.c
    sii_frame_menu.c
    sii_param_widgets.c
    sii_view_widgets.c
    sii_swpfi_widgets.c
    sii_exam_widgets.c
    sii_links_widget.c
    sii_X_stuff.c
    solo2.c
    sp_clkd.c
    sp_basics.c
    sp_accepts.c
    sp_pals.c
    sii_perusal.c
    sii_xyraster.c
    sii_fixme.c
    sp_save_frms.c
    sp_dorade.c
    sp_colors.c
    sp_lists.c
    sii_bscan.c
    sii_gtk4_compat.c
    sii_log_handler.c
)

add_library(soloiv_core STATIC ${SOLOIV_CORE_SOURCES})

target_include_directories(soloiv_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/translate
    ${CMAKE_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTK4_INCLUDE_DIRS}
)

# Every C function in soloiv_core is prototyped (include/soloiv_core_protos.h
# and soloiv_core_gtk_protos.h); keep it that way by making a missing
# prototype or an implicit declaration an error.
set(SOLOIV_PROTO_FLAGS
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:Clang,AppleClang>>:-Werror=deprecated-non-prototype -Werror=implicit-function-declaration -Werror=implicit-int -Werror=strict-prototypes>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:GNU>>:-Werror=implicit-function-declaration -Werror=implicit-int -Werror=old-style-definition -Werror=strict-prototypes>
)
# Also used by tests/ (added after this directory).
set(SOLOIV_PROTO_FLAGS ${SOLOIV_PROTO_FLAGS} PARENT_SCOPE)
target_compile_options(soloiv_core PRIVATE ${GTK4_CFLAGS_OTHER}
    ${SOLOIV_PROTO_FLAGS}
)

target_link_directories(soloiv_core PUBLIC ${GTK4_LIBRARY_DIRS})

target_link_libraries(soloiv_core PUBLIC
    soloedit
    s2
    ${GTK4_LIBRARIES}
)

if(MATH_LIBRARY)
  target_link_libraries(soloiv_core PUBLIC ${MATH_LIBRARY})
endif()

# The three static libraries call into each other: translate/ (s2) calls
# back into this library (solo_message, LittleEndian, ...), and the editor
# (soloedit) calls both. Apple's linker rescans archives until nothing is
# left undefined, but GNU ld reads each one once, in order; declaring the
# cycle makes CMake repeat the group on the link line. (Apple's linker warns
# about the repeats, and does not need them.)
if(NOT APPLE)
  target_link_libraries(s2 PUBLIC soloiv_core)
  target_link_libraries(soloedit PUBLIC s2 soloiv_core)
  set_target_properties(soloiv_core s2 soloedit PROPERTIES
      LINK_INTERFACE_MULTIPLICITY 3)
endif()

# Executable: just main() + the core library.
add_executable(soloiv soloii_main.c)
target_link_libraries(soloiv PRIVATE soloiv_core)
target_compile_options(soloiv PRIVATE ${SOLOIV_PROTO_FLAGS})

install(TARGETS soloiv RUNTIME DESTINATION bin)
