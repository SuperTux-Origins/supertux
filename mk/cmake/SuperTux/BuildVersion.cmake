# Version source of truth: top-level VERSION file (e.g. "0.6.4-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix flake appends .<revCount>+g<rev>).
# Git describe is intentionally not used — it is unreliable in cleanSource /
# tarball / Nix builds.

if(NOT DEFINED PROJECT_VERSION_FULL OR PROJECT_VERSION_FULL STREQUAL "")
  if(EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
    file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
  else()
    set(PROJECT_VERSION_FULL "0.0.0-dev")
    message(WARNING "VERSION file missing; using ${PROJECT_VERSION_FULL}")
  endif()
endif()

# Strip whitespace
string(STRIP "${PROJECT_VERSION_FULL}" PROJECT_VERSION_FULL)

# Numeric components from the base (before first - or +)
string(REGEX REPLACE "[-+].*" "" _ver_base "${PROJECT_VERSION_FULL}")
string(REPLACE "." ";" _ver_parts "${_ver_base}")
list(LENGTH _ver_parts _ver_n)

set(SUPERTUX_VERSION_MAJOR 0)
set(SUPERTUX_VERSION_MINOR 0)
set(SUPERTUX_VERSION_PATCH 0)
set(SUPERTUX_VERSION_TWEAK "")
if(_ver_n GREATER 0)
  list(GET _ver_parts 0 SUPERTUX_VERSION_MAJOR)
endif()
if(_ver_n GREATER 1)
  list(GET _ver_parts 1 SUPERTUX_VERSION_MINOR)
endif()
if(_ver_n GREATER 2)
  list(GET _ver_parts 2 SUPERTUX_VERSION_PATCH)
endif()
if(_ver_n GREATER 3)
  list(GET _ver_parts 3 SUPERTUX_VERSION_TWEAK)
endif()

set(SUPERTUX_VERSION_STRING "${PROJECT_VERSION_FULL}")
set(SUPERTUX_VERSION "${PROJECT_VERSION_FULL}")
set(SUPERTUX_VERSION_BUILD "${PROJECT_VERSION_FULL}")
set(VERSION_NUMBER_GIT "${PROJECT_VERSION_FULL}")

message(STATUS "SuperTux Origins version: ${PROJECT_VERSION_FULL}")

configure_file(
  "${CMAKE_SOURCE_DIR}/version.h.in"
  "${CMAKE_BINARY_DIR}/version.h"
  @ONLY)
