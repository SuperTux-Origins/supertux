cmake_minimum_required(VERSION 3.16)

# Source of truth: top-level VERSION file (e.g. "0.1.5-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix flake appends +g<rev>).
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()
# CMake project() VERSION wants a numeric X.Y.Z; strip -dev / +g* suffixes.
string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" _PROJECT_VERSION_BASE "${PROJECT_VERSION_FULL}")

project(SuperTux-Milestone1
  VERSION ${_PROJECT_VERSION_BASE}
  DESCRIPTION "SuperTux Milestone 1"
  LANGUAGES CXX
)

set(PROJECT_VERSION_FULL "${PROJECT_VERSION_FULL}" CACHE STRING "Full version string from VERSION file or packager" FORCE)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

# Prefer legacy libGL for this era of immediate-mode GL code.
set(OpenGL_GL_PREFERENCE "LEGACY")

option(ENABLE_SOUND  "Audio (SDL_mixer / SDL2_mixer)" ON)
option(ENABLE_OPENGL "OpenGL rendering path" ON)
option(ENABLE_GLES2  "OpenGL ES 2.0 path (shader quads; implies ENABLE_OPENGL; prefer ENABLE_SDL2)" OFF)
option(ENABLE_SDL2   "SDL2 backend (OFF = SDL 1.2)" OFF)
option(ENABLE_DEBUG  "DEBUG define, -O0 -g" OFF)

if(ENABLE_GLES2)
  set(ENABLE_OPENGL ON CACHE BOOL "OpenGL rendering path" FORCE)
  if(NOT ENABLE_SDL2)
    message(STATUS "ENABLE_GLES2=ON: enabling ENABLE_SDL2 (ES context via SDL2)")
    set(ENABLE_SDL2 ON CACHE BOOL "SDL2 backend (OFF = SDL 1.2)" FORCE)
  endif()
endif()

set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share/supertux-milestone1"
    CACHE PATH "Runtime data directory")

find_package(PkgConfig REQUIRED)
find_package(ZLIB REQUIRED)

if(ENABLE_SDL2)
  pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl2)
  pkg_check_modules(SDL_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
  if(ENABLE_SOUND)
    pkg_check_modules(SDL_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
  endif()
else()
  pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl>=1.2.4)
  pkg_check_modules(SDL_IMAGE REQUIRED IMPORTED_TARGET SDL_image)
  if(ENABLE_SOUND)
    pkg_check_modules(SDL_MIXER REQUIRED IMPORTED_TARGET SDL_mixer)
  endif()
endif()

if(ENABLE_OPENGL AND NOT ENABLE_GLES2)
  find_package(OpenGL REQUIRED)
endif()

if(ENABLE_GLES2)
  # Prefer pkg-config glesv2; fall back to find_library.
  pkg_check_modules(GLES2 IMPORTED_TARGET glesv2)
  if(NOT GLES2_FOUND)
    find_library(GLES2_LIBRARY NAMES GLESv2)
    find_path(GLES2_INCLUDE_DIR GLES2/gl2.h)
    if(NOT GLES2_LIBRARY OR NOT GLES2_INCLUDE_DIR)
      message(FATAL_ERROR "ENABLE_GLES2=ON but GLESv2 not found (install libgles2-mesa-dev or similar)")
    endif()
  endif()
endif()

set(SUPERTUX_SOURCES
  src/badguy.cpp
  src/button.cpp
  src/collision.cpp
  src/configfile.cpp
  src/intro.cpp
  src/gameloop.cpp
  src/globals.cpp
  src/high_scores.cpp
  src/level.cpp
  src/leveleditor.cpp
  src/lispreader.cpp
  src/menu.cpp
  src/particlesystem.cpp
  src/physic.cpp
  src/player.cpp
  src/scene.cpp
  src/screen.cpp
  src/setup.cpp
  src/special.cpp
  src/supertux.cpp
  src/text.cpp
  src/texture.cpp
  src/timer.cpp
  src/title.cpp
  src/type.cpp
  src/world.cpp
  src/worldmap.cpp
  src/tile.cpp
  src/mousecursor.cpp
  src/resources.cpp
  src/gameobjs.cpp
  src/sprite.cpp
  src/sprite_manager.cpp
  src/touch_controls.cpp
)

if(ENABLE_SDL2)
  list(APPEND SUPERTUX_SOURCES src/platform_sdl2.cpp)
else()
  list(APPEND SUPERTUX_SOURCES src/platform_sdl1.cpp)
endif()

if(ENABLE_GLES2)
  list(APPEND SUPERTUX_SOURCES src/gles2_renderer.cpp)
endif()

if(ENABLE_SOUND)
  list(APPEND SUPERTUX_SOURCES
    src/music_manager.cpp
    src/musicref.cpp
    src/sound.cpp
  )
endif()

add_executable(supertux-milestone1 ${SUPERTUX_SOURCES})

target_include_directories(supertux-milestone1 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(supertux-milestone1 PRIVATE
  SUPERTUX_MILESTONE1_VERSION="${PROJECT_VERSION_FULL}"
  DATA_PREFIX="${DATA_PREFIX}"
)

if(ENABLE_SDL2)
  target_compile_definitions(supertux-milestone1 PRIVATE USE_SDL2)
endif()
if(NOT ENABLE_SOUND)
  target_compile_definitions(supertux-milestone1 PRIVATE NOSOUND)
endif()
if(NOT ENABLE_OPENGL)
  target_compile_definitions(supertux-milestone1 PRIVATE NOOPENGL)
endif()
if(ENABLE_GLES2)
  target_compile_definitions(supertux-milestone1 PRIVATE USE_GLES2)
endif()
if(ENABLE_DEBUG)
  target_compile_definitions(supertux-milestone1 PRIVATE DEBUG)
  target_compile_options(supertux-milestone1 PRIVATE -Wall -Wextra -O0 -g)
endif()

target_link_libraries(supertux-milestone1 PRIVATE
  PkgConfig::SDL
  PkgConfig::SDL_IMAGE
  ZLIB::ZLIB
)
if(ENABLE_SOUND)
  target_link_libraries(supertux-milestone1 PRIVATE PkgConfig::SDL_MIXER)
endif()
if(ENABLE_GLES2)
  if(TARGET PkgConfig::GLES2)
    target_link_libraries(supertux-milestone1 PRIVATE PkgConfig::GLES2)
  else()
    target_include_directories(supertux-milestone1 PRIVATE ${GLES2_INCLUDE_DIR})
    target_link_libraries(supertux-milestone1 PRIVATE ${GLES2_LIBRARY})
  endif()
elseif(ENABLE_OPENGL)
  target_link_libraries(supertux-milestone1 PRIVATE OpenGL::GL)
  # Legacy immediate-mode path uses GLU on SDL 1.2 builds only.
  if(NOT ENABLE_SDL2)
    target_link_libraries(supertux-milestone1 PRIVATE OpenGL::GLU)
  endif()
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(supertux-milestone1 PRIVATE m)
endif()

include(GNUInstallDirs)

install(TARGETS supertux-milestone1 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES supertux-milestone1.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES supertux-milestone1.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)
install(FILES supertux-milestone1.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
  install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/supertux-milestone1)
else()
  message(STATUS "No data/ in source tree — install data separately (DATA_PREFIX=${DATA_PREFIX})")
endif()

message(STATUS "")
message(STATUS "SuperTux Milestone 1 ${PROJECT_VERSION_FULL}")
if(ENABLE_SDL2)
  message(STATUS "  Backend       = SDL2")
else()
  message(STATUS "  Backend       = SDL 1.2")
endif()
message(STATUS "  ENABLE_SOUND  = ${ENABLE_SOUND}")
message(STATUS "  ENABLE_OPENGL = ${ENABLE_OPENGL}")
message(STATUS "  ENABLE_GLES2  = ${ENABLE_GLES2}")
message(STATUS "  DATA_PREFIX   = ${DATA_PREFIX}")
message(STATUS "")
