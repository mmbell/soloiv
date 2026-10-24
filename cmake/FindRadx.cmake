# FindRadx.cmake — locate the prebuilt LROSE libRadx / libNcxx static archives
# and their transitive dependencies (HDF5, netCDF, udunits), then expose an
# imported interface target `Radx::Radx` carrying the full link line in the
# order the static archives require (Radx -> Ncxx -> netcdf/hdf5/udunits).
#
# Honors a few hint variables / env vars:
#   LROSE_PREFIX (cache or env) — root containing include/ and lib/
# Defaults to ~/lrose, the lrose-core source install location; the Linux
# binary packages install to /usr/local/lrose, which is searched as well.

set(_lrose_default "$ENV{HOME}/lrose")
if(DEFINED ENV{LROSE_PREFIX})
  set(_lrose_default "$ENV{LROSE_PREFIX}")
endif()
set(LROSE_PREFIX "${_lrose_default}" CACHE PATH "Root of the LROSE install (include/ + lib/)")

find_path(RADX_INCLUDE_DIR
  NAMES Radx/RadxFile.hh
  HINTS "${LROSE_PREFIX}/include" /usr/local/lrose/include
)

set(_lrose_lib_hints "${LROSE_PREFIX}/lib" /usr/local/lrose/lib)
find_library(RADX_LIBRARY   NAMES Radx   HINTS ${_lrose_lib_hints})
find_library(NCXX_LIBRARY   NAMES Ncxx   HINTS ${_lrose_lib_hints})
# libRadx pulls a handful of C utilities (STRncopy etc.) from libtoolsa.
find_library(TOOLSA_LIBRARY NAMES toolsa HINTS ${_lrose_lib_hints})
# ... and its byte-order helpers (BE_*) from libdataport. The Linux shared
# libRadx leaves them undefined without recording the dependency.
find_library(DATAPORT_LIBRARY NAMES dataport HINTS ${_lrose_lib_hints})

# Dependency libraries (Homebrew on Apple Silicon, but allow system locations;
# Debian/Ubuntu keep the serial HDF5 libraries in an hdf5/serial subdirectory).
find_library(HDF5_CPP_LIBRARY NAMES hdf5_cpp hdf5_serial_cpp
  HINTS /opt/homebrew/opt/hdf5/lib PATH_SUFFIXES hdf5/serial)
find_library(HDF5_LIBRARY     NAMES hdf5 hdf5_serial
  HINTS /opt/homebrew/opt/hdf5/lib PATH_SUFFIXES hdf5/serial)
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
  )
  if(DATAPORT_LIBRARY)
    list(APPEND _radx_link "${DATAPORT_LIBRARY}")
  endif()
  list(APPEND _radx_link
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
