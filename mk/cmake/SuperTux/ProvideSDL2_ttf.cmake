# SDL2_ttf — under Emscripten FreeType comes from -sUSE_FREETYPE=2.
# R36S/ArkOS: prefer pkg-config / explicit sysroot search (FIND_ROOT ONLY
# often hides config packages). Desktop uses find_package as usual.

if(EMSCRIPTEN)
  message(STATUS "Emscripten: SDL2_ttf via FreeType port (-sUSE_FREETYPE); LibSDL2_ttf is INTERFACE")
  add_library(LibSDL2_ttf INTERFACE)
  return()
endif()

find_package(SDL2_ttf QUIET)

if(TARGET SDL2_ttf::SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf (config)")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf::SDL2_ttf)
  return()
elseif(TARGET SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf)
  return()
endif()

# pkg-config (works with PKG_CONFIG_SYSROOT_DIR on R36S)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDL2_TTF QUIET SDL2_ttf)
endif()

find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
  HINTS ${PC_SDL2_TTF_INCLUDE_DIRS}
  PATH_SUFFIXES SDL2
  PATHS
    ${CMAKE_SYSROOT}/usr/include
    ${CMAKE_SYSROOT}/usr/include/SDL2
    ${CMAKE_SYSROOT}/usr/local/include
    ${CMAKE_SYSROOT}/usr/local/include/SDL2
)

find_library(SDL2_TTF_LIBRARY
  NAMES SDL2_ttf
  HINTS ${PC_SDL2_TTF_LIBRARY_DIRS}
  PATHS
    ${CMAKE_SYSROOT}/usr/lib
    ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    ${CMAKE_SYSROOT}/lib
    ${CMAKE_SYSROOT}/lib/aarch64-linux-gnu
)

if(SDL2_TTF_INCLUDE_DIR AND SDL2_TTF_LIBRARY)
  message(STATUS "Found SDL2_ttf: ${SDL2_TTF_LIBRARY}")
  add_library(LibSDL2_ttf UNKNOWN IMPORTED)
  set_target_properties(LibSDL2_ttf PROPERTIES
    IMPORTED_LOCATION "${SDL2_TTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}")
  if(PC_SDL2_TTF_CFLAGS_OTHER)
    set_property(TARGET LibSDL2_ttf APPEND PROPERTY
      INTERFACE_COMPILE_OPTIONS "${PC_SDL2_TTF_CFLAGS_OTHER}")
  endif()
else()
  message(FATAL_ERROR
    "Could NOT find SDL2_ttf (headers+library).\n"
    "  For R36S: install libsdl2-ttf-dev into the ArkOS sysroot, or extend\n"
    "  the sysroot tarball. Looked under CMAKE_SYSROOT=${CMAKE_SYSROOT}")
endif()

# EOF #
