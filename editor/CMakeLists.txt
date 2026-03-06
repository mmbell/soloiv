# editor/ - solo editor library

set(EDITOR_SOURCES
    se_bnd.c
    se_wgt_stf.c
    se_catch_all.c
    se_utils.c
    se_for_each.c
    se_defrkl.c
    se_histog.c
    se_flag_ops.c
    se_add_mult.c
    se_proc_data.c
    se_BB_unfold.c
    sxm_examine.c
)

add_library(soloedit STATIC ${EDITOR_SOURCES})

# Legacy K&R C code
target_compile_options(soloedit PRIVATE
    -Wno-implicit-function-declaration
    -Wno-implicit-int
    -Wno-return-mismatch
)

target_include_directories(soloedit PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/translate
)

target_include_directories(soloedit PUBLIC ${GTK4_INCLUDE_DIRS})

target_link_libraries(soloedit PUBLIC
    ${GTK4_LIBRARIES}
)
