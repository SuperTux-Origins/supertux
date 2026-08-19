# SDL2_ttf — Emscripten uses FreeType port. R36S/desktop prefer system package;
# if missing, build from SDL2_TTF_SOURCE_DIR (flake input sdl2-ttf-src).

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
  return()
endif()

# Build from source (R36S when sysroot lacks libSDL2_ttf).
set(SDL2_TTF_SOURCE_DIR "" CACHE PATH "Path to SDL2_ttf sources")
if(NOT SDL2_TTF_SOURCE_DIR OR NOT EXISTS "${SDL2_TTF_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "Could NOT find SDL2_ttf and SDL2_TTF_SOURCE_DIR is unset/invalid.\n"
    "  Pass -DSDL2_TTF_SOURCE_DIR=… (flake: sdl2-ttf-src) or install libsdl2-ttf into the sysroot.")
endif()

message(STATUS "Building SDL2_ttf from ${SDL2_TTF_SOURCE_DIR}")
include(ExternalProject)
set(SDL2_TTF_PREFIX "${CMAKE_BINARY_DIR}/sdl2_ttf")
# Resolve SDL2 / FreeType from sysroot for the ExternalProject.
set(_sdl2_ttf_cmake_args
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_SYSROOT=${CMAKE_SYSROOT}
  -DCMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH}
  -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
  -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
  -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
  -DCMAKE_INSTALL_PREFIX=${SDL2_TTF_PREFIX}
  -DCMAKE_BUILD_TYPE=Release
  -DSDL2TTF_SAMPLES=OFF
  -DSDL2TTF_VENDORED=ON
  -DBUILD_SHARED_LIBS=ON
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
ExternalProject_Add(sdl2_ttf_project
  SOURCE_DIR "${SDL2_TTF_SOURCE_DIR}"
  CMAKE_ARGS ${_sdl2_ttf_cmake_args}
  BUILD_BYPRODUCTS
    "${SDL2_TTF_PREFIX}/lib/libSDL2_ttf.so"
    "${SDL2_TTF_PREFIX}/lib/libSDL2_ttf.so.0"
)

file(MAKE_DIRECTORY "${SDL2_TTF_PREFIX}/include/SDL2")
add_library(LibSDL2_ttf SHARED IMPORTED)
set_target_properties(LibSDL2_ttf PROPERTIES
  IMPORTED_LOCATION "${SDL2_TTF_PREFIX}/lib/libSDL2_ttf.so"
  INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_PREFIX}/include/SDL2;${SDL2_TTF_PREFIX}/include")
add_dependencies(LibSDL2_ttf sdl2_ttf_project)

# EOF #
