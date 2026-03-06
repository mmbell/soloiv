cmake_minimum_required(VERSION 3.16)
project(soloiv VERSION 1.21.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
