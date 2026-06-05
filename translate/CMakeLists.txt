# translate/ - I/O library for reading DORADE sweep files
# Only build objects needed by the GUI; drop translator executables and Fortran

set(S2LIB_SOURCES
    ddin.c
    ddout.c
    dd_uf.c
    dd_ncdf.c
    gneric_dd.c
    hrd_dd.c
    sigm_dd.c
    uf_dd.c
    nex_dd.c
    toga_dd.c
    elda_dd.c
    eldb_dd.c
    gpro_data.c
    ground_echo.c
    stdhrd.c
    xwsrqc.c
    fof_dd.c
    piraq_dd.c
    tdwr_dd.c
    etl_dd.c
    cimm_dd.c
    nc_dd.c
    piraqx_dd.c
    by_products.c
    dd_catalog.c
    gecho.c
    dorade_uf.c
    product_x.c
    dd_der_flds.c
    nssl_mrd.c
    dd_swp_files.c
    dorade_tape.c
    shane.c
    dd_crackers.c
    dorade_ncdf.c
    dd_swpfi.c
    dd_files.c
    dda_common.c
    ddb_common.c
    dorade_share.c
    dd_time.c
    swp_file_acc.c
    dd_io_mgmt.c
    dap_common.c
)

# Radx I/O shim (C++17): only compiled when the radx backend is selected.
# Exposes a flat extern "C" API (radar_io.h) over libRadx.
if(SOLOIV_IO_BACKEND STREQUAL "radx")
  list(APPEND S2LIB_SOURCES radx_shim.cc)
endif()

add_library(s2 STATIC ${S2LIB_SOURCES})

if(SOLOIV_IO_BACKEND STREQUAL "radx")
  # PUBLIC so the Radx static archives + libc++ propagate to soloiv's link line.
  target_link_libraries(s2 PUBLIC Radx::Radx)
  set_source_files_properties(radx_shim.cc PROPERTIES
      COMPILE_OPTIONS "-std=c++17")
endif()

# Legacy K&R C code uses implicit function declarations extensively;
# downgrade from error to warning so it compiles with modern C compilers.
target_compile_options(s2 PRIVATE
    -Wno-error=implicit-function-declaration -Wno-implicit-function-declaration
    -Wno-error=implicit-int -Wno-implicit-int
    -Wno-return-mismatch
    -Wno-error=parentheses -Wno-parentheses
)

target_include_directories(s2 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(s2 PUBLIC
    ${GTK4_LIBRARIES}
)

target_include_directories(s2 PUBLIC ${GTK4_INCLUDE_DIRS})

if(netCDF_FOUND)
  target_link_libraries(s2 PUBLIC netCDF::netcdf)
elseif(NETCDF_FOUND)
  target_link_libraries(s2 PUBLIC ${NETCDF_LIBRARIES})
  target_include_directories(s2 PUBLIC ${NETCDF_INCLUDE_DIRS})
endif()

if(MATH_LIBRARY)
  target_link_libraries(s2 PUBLIC ${MATH_LIBRARY})
endif()
