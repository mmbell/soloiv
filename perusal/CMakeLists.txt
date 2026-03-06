# perusal/ - soloiv GUI executable

set(SOLOIV_SOURCES
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
)

add_executable(soloiv ${SOLOIV_SOURCES})

target_include_directories(soloiv PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/translate
    ${CMAKE_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTK4_INCLUDE_DIRS}
)

# Legacy C code has many implicit function declarations across files;
# suppress for now to focus on GTK4 migration errors.
target_compile_options(soloiv PRIVATE ${GTK4_CFLAGS_OTHER}
    -Wno-implicit-function-declaration
    -Wno-implicit-int
    -Wno-return-mismatch
)

target_link_directories(soloiv PRIVATE ${GTK4_LIBRARY_DIRS})

target_link_libraries(soloiv PRIVATE
    soloedit
    s2
    ${GTK4_LIBRARIES}
)

if(MATH_LIBRARY)
  target_link_libraries(soloiv PRIVATE ${MATH_LIBRARY})
endif()

install(TARGETS soloiv RUNTIME DESTINATION bin)
