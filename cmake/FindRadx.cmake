# FindRadx.cmake — locate the prebuilt LROSE libRadx / libNcxx static archives
# and their transitive dependencies (HDF5, netCDF, udunits), then expose an
# imported interface target `Radx::Radx` carrying the full link line in the
# order the static archives require (Radx -> Ncxx -> netcdf/hdf5/udunits).
#
# Honors a few hint variables / env vars:
#   LROSE_PREFIX (cache or env) — root containing include/ and lib/
# Defaults to /Users/mmbell/lrose.

set(_lrose_default "/Users/mmbell/lrose")
if(DEFINED ENV{LROSE_PREFIX})
  set(_lrose_default "$ENV{LROSE_PREFIX}")
endif()
set(LROSE_PREFIX "${_lrose_default}" CACHE PATH "Root of the LROSE install (include/ + lib/)")

find_path(RADX_INCLUDE_DIR
  NAMES Radx/RadxFile.hh
  HINTS "${LROSE_PREFIX}/include"
)

find_library(RADX_LIBRARY   NAMES Radx   HINTS "${LROSE_PREFIX}/lib")
find_library(NCXX_LIBRARY   NAMES Ncxx   HINTS "${LROSE_PREFIX}/lib")
# libRadx pulls a handful of C utilities (STRncopy etc.) from libtoolsa.
find_library(TOOLSA_LIBRARY NAMES toolsa HINTS "${LROSE_PREFIX}/lib")

# Dependency libraries (Homebrew on Apple Silicon, but allow system locations).
find_library(HDF5_CPP_LIBRARY NAMES hdf5_cpp HINTS /opt/homebrew/opt/hdf5/lib)
find_library(HDF5_LIBRARY     NAMES hdf5     HINTS /opt/homebrew/opt/hdf5/lib)
find_library(NETCDF_LIBRARY   NAMES netcdf   HINTS /opt/homebrew/opt/netcdf/lib)
find_library(UDUNITS2_LIBRARY NAMES udunits2 HINTS /opt/homebrew/opt/udunits/lib)
find_library(BZ2_LIBRARY      NAMES bz2)
find_library(Z_LIBRARY        NAMES z)
find_library(CURL_LIBRARY     NAMES curl)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Radx
  REQUIRED_VARS
    RADX_INCLUDE_DIR RADX_LIBRARY NCXX_LIBRARY TOOLSA_LIBRARY
    HDF5_CPP_LIBRARY HDF5_LIBRARY NETCDF_LIBRARY UDUNITS2_LIBRARY
)

if(Radx_FOUND AND NOT TARGET Radx::Radx)
  add_library(Radx::Radx INTERFACE IMPORTED)
  set_target_properties(Radx::Radx PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${RADX_INCLUDE_DIR}"
  )
  # Link order matters for static archives: Radx and Ncxx have mutual
  # references resolved by the netcdf/hdf5/udunits libs that follow.
  set(_radx_link
    "${RADX_LIBRARY}"
    "${NCXX_LIBRARY}"
    "${TOOLSA_LIBRARY}"
    "${HDF5_CPP_LIBRARY}"
    "${HDF5_LIBRARY}"
    "${NETCDF_LIBRARY}"
    "${UDUNITS2_LIBRARY}"
  )
  foreach(_opt BZ2_LIBRARY Z_LIBRARY CURL_LIBRARY)
    if(${_opt})
      list(APPEND _radx_link "${${_opt}}")
    endif()
  endforeach()
  set_target_properties(Radx::Radx PROPERTIES
    INTERFACE_LINK_LIBRARIES "${_radx_link}"
  )
endif()
