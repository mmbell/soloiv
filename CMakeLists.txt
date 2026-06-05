cmake_minimum_required(VERSION 3.16)
project(soloiv VERSION 1.21.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The Radx I/O shim is C++17 and must match the libc++ ABI that libRadx.a was
# built with (Apple clang). Do not mix in GCC/libstdc++.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Sweep I/O backend: radx (libRadx, primary) or custom (netCDF-C, future).
set(SOLOIV_IO_BACKEND "radx" CACHE STRING "Sweep I/O backend: radx | custom")
set_property(CACHE SOLOIV_IO_BACKEND PROPERTY STRINGS radx custom)

if(SOLOIV_IO_BACKEND STREQUAL "radx")
  find_package(Radx REQUIRED)
  add_compile_definitions(SOLOIV_IO_BACKEND_RADX)

  # Route DORADE reads through Radx too (retiring the in-tree parser). ON by
  # default; the legacy parser stays compiled and is reachable at runtime via
  # the SOLOIV_DORADE_LEGACY environment variable.
  option(SOLOIV_DORADE_VIA_RADX "Read DORADE sweeps via Radx" ON)
  if(SOLOIV_DORADE_VIA_RADX)
    add_compile_definitions(SOLOIV_DORADE_VIA_RADX)
  endif()
endif()

# No 32-bit builds
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "32-bit builds are not supported. Use a 64-bit toolchain.")
endif()

# Platform defines
add_compile_definitions(UNIX)
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
  add_compile_definitions(LITTLENDIAN)
endif()

# Find GTK4 via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# Optional netCDF
find_package(netCDF QUIET)
if(NOT netCDF_FOUND)
  pkg_check_modules(NETCDF netcdf)
endif()
if(netCDF_FOUND OR NETCDF_FOUND)
  add_compile_definitions(NETCDF)
endif()

# Math library
find_library(MATH_LIBRARY m)

# Include directory for project headers
include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(translate)
add_subdirectory(editor)
add_subdirectory(perusal)

option(SOLOIV_BUILD_TESTS "Build the soloiv test suite" OFF)
if(SOLOIV_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
