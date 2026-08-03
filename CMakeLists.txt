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

# Optional prebuilt SDL2 tree (Emscripten / custom static builds).
# When set: headers in ${SDL2_ROOT}/include, libs in ${SDL2_ROOT}/lib.
set(SDL2_ROOT "" CACHE PATH "Prebuilt SDL2 (+ image/mixer) prefix for wasm/static")

# Extra link flags for the final executable (Emscripten SHELL: options, preload).
set(EMSCRIPTEN_LINK_FLAGS "" CACHE STRING "Extra link flags (Emscripten)")

find_package(PkgConfig REQUIRED)

# Desktop: system zlib. Emscripten: optional ZLIB_ROOT (static wasm build).
set(ZLIB_ROOT "" CACHE PATH "Prebuilt zlib prefix for Emscripten/static builds")
if(EMSCRIPTEN)
  if(ZLIB_ROOT)
    message(STATUS "Using ZLIB_ROOT=${ZLIB_ROOT}")
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
      IMPORTED_LOCATION "${ZLIB_ROOT}/lib/libz.a"
      INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_ROOT}/include")
    set(ZLIB_FOUND TRUE)
  else()
    # Fall back to find_package (may work if sysroot has zlib).
    find_package(ZLIB)
    if(NOT ZLIB_FOUND)
      message(FATAL_ERROR "Emscripten build needs -DZLIB_ROOT=… (static libz + zlib.h)")
    endif()
  endif()
else()
  find_package(ZLIB REQUIRED)
endif()

if(ENABLE_SDL2)
  if(SDL2_ROOT)
    message(STATUS "Using SDL2_ROOT=${SDL2_ROOT}")
    add_library(supertux_sdl2 INTERFACE)
    target_include_directories(supertux_sdl2 INTERFACE
      "${SDL2_ROOT}/include"
      "${SDL2_ROOT}/include/SDL2")
    target_link_libraries(supertux_sdl2 INTERFACE
      "${SDL2_ROOT}/lib/libSDL2.a")
    if(EXISTS "${SDL2_ROOT}/lib/libSDL2_image.a")
      target_link_libraries(supertux_sdl2 INTERFACE
        "${SDL2_ROOT}/lib/libSDL2_image.a")
    endif()
    if(ENABLE_SOUND AND EXISTS "${SDL2_ROOT}/lib/libSDL2_mixer.a")
      target_link_libraries(supertux_sdl2 INTERFACE
        "${SDL2_ROOT}/lib/libSDL2_mixer.a")
      # Static libxmp for MOD/XM (wasm / offline builds).
      if(EXISTS "${SDL2_ROOT}/lib/libxmp.a")
        target_link_libraries(supertux_sdl2 INTERFACE
          "${SDL2_ROOT}/lib/libxmp.a")
      endif()
    elseif(ENABLE_SOUND AND NOT EMSCRIPTEN)
      message(FATAL_ERROR "ENABLE_SOUND=ON but no libSDL2_mixer.a under SDL2_ROOT")
    elseif(ENABLE_SOUND AND EMSCRIPTEN)
      message(WARNING "ENABLE_SOUND=ON but no libSDL2_mixer.a under SDL2_ROOT — build will fail at link")
    endif()
    # Fake imported targets so the rest of the file can stay uniform.
    add_library(PkgConfig::SDL ALIAS supertux_sdl2)
    add_library(PkgConfig::SDL_IMAGE ALIAS supertux_sdl2)
    if(ENABLE_SOUND AND EXISTS "${SDL2_ROOT}/lib/libSDL2_mixer.a")
      add_library(PkgConfig::SDL_MIXER ALIAS supertux_sdl2)
    endif()
  else()
    pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    if(ENABLE_SOUND)
      pkg_check_modules(SDL_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
    endif()
  endif()
else()
  pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl>=1.2.4)
  pkg_check_modules(SDL_IMAGE REQUIRED IMPORTED_TARGET SDL_image)
  if(ENABLE_SOUND)
    pkg_check_modules(SDL_MIXER REQUIRED IMPORTED_TARGET SDL_mixer)
  endif()
endif()

if(ENABLE_OPENGL AND NOT ENABLE_GLES2 AND NOT EMSCRIPTEN)
  find_package(OpenGL REQUIRED)
endif()

if(ENABLE_GLES2 AND NOT EMSCRIPTEN)
  # Prefer pkg-config glesv2; fall back to find_library.
  # Emscripten provides WebGL via link flags (-sFULL_ES2); no system GLESv2.
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
  src/game_file.cpp
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
  src/app_loop.cpp
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
)
if(TARGET ZLIB::ZLIB)
  target_link_libraries(supertux-milestone1 PRIVATE ZLIB::ZLIB)
elseif(EMSCRIPTEN)
  # Emscripten sysroot provides compress; -lz is resolved by emcc.
  target_link_libraries(supertux-milestone1 PRIVATE z)
endif()
if(ENABLE_SOUND AND TARGET PkgConfig::SDL_MIXER)
  target_link_libraries(supertux-milestone1 PRIVATE PkgConfig::SDL_MIXER)
endif()
if(ENABLE_GLES2 AND NOT EMSCRIPTEN)
  if(TARGET PkgConfig::GLES2)
    target_link_libraries(supertux-milestone1 PRIVATE PkgConfig::GLES2)
  else()
    target_include_directories(supertux-milestone1 PRIVATE ${GLES2_INCLUDE_DIR})
    target_link_libraries(supertux-milestone1 PRIVATE ${GLES2_LIBRARY})
  endif()
elseif(ENABLE_OPENGL AND NOT EMSCRIPTEN)
  target_link_libraries(supertux-milestone1 PRIVATE OpenGL::GL)
  # Legacy immediate-mode path uses GLU on SDL 1.2 builds only.
  if(NOT ENABLE_SDL2)
    target_link_libraries(supertux-milestone1 PRIVATE OpenGL::GLU)
  endif()
endif()
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  target_link_libraries(supertux-milestone1 PRIVATE m)
endif()

if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  # USE_SDL2 already set; force USE_GLES2-friendly defines when GLES2 on.
  target_compile_definitions(supertux-milestone1 PRIVATE __EMSCRIPTEN__)
  if(EMSCRIPTEN_LINK_FLAGS)
    # Space-separated SHELL: flags and optional --preload-file from the
    # build-wasm-app.sh script.
    separate_arguments(_em_link_flags NATIVE_COMMAND "${EMSCRIPTEN_LINK_FLAGS}")
    target_link_options(supertux-milestone1 PRIVATE ${_em_link_flags})
  endif()
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
