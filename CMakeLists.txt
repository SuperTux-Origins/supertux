cmake_minimum_required(VERSION 3.16)

project(SuperTux-Milestone1
  VERSION 0.1.4
  DESCRIPTION "SuperTux Milestone 1 — classic jump'n'run"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(ENABLE_SOUND   "Build with SDL_mixer audio support" ON)
option(ENABLE_OPENGL  "Build with OpenGL rendering path"   ON)
option(ENABLE_SDL2    "Use SDL2 backend (experimental; default is SDL 1.2)" OFF)
option(ENABLE_DEBUG   "Enable DEBUG define and softer optimisations" OFF)

set(DATA_PREFIX "${CMAKE_INSTALL_PREFIX}/share/supertux-milestone1"
    CACHE PATH "Runtime data directory (levels, images, music, …)")

if(ENABLE_SDL2)
  message(FATAL_ERROR
    "ENABLE_SDL2 is not implemented yet. "
    "See TODO.md Phase 2–3 (platform abstraction). "
    "Build with -DENABLE_SDL2=OFF (default) for SDL 1.2.")
endif()

# ---------------------------------------------------------------------------
# Dependencies — SDL 1.2 path
# ---------------------------------------------------------------------------
# Prefer CMake config packages; fall back to pkg-config / Find modules.

find_package(PkgConfig QUIET)

# SDL 1.2
if(PkgConfig_FOUND)
  pkg_check_modules(SDL REQUIRED sdl>=1.2.4)
  pkg_check_modules(SDL_IMAGE REQUIRED SDL_image)
  if(ENABLE_SOUND)
    pkg_check_modules(SDL_MIXER REQUIRED SDL_mixer)
  endif()
else()
  find_package(SDL REQUIRED)
  find_package(SDL_image REQUIRED)
  if(ENABLE_SOUND)
    find_package(SDL_mixer REQUIRED)
  endif()
endif()

find_package(ZLIB REQUIRED)

if(ENABLE_OPENGL)
  find_package(OpenGL)
  if(NOT OpenGL_FOUND)
    message(WARNING "OpenGL not found; forcing ENABLE_OPENGL=OFF")
    set(ENABLE_OPENGL OFF)
  endif()
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SUPERTUX_CORE_SOURCES
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
)

set(SUPERTUX_SOUND_SOURCES
  src/music_manager.cpp
  src/musicref.cpp
  src/sound.cpp
)

add_executable(supertux-milestone1 ${SUPERTUX_CORE_SOURCES})

if(ENABLE_SOUND)
  target_sources(supertux-milestone1 PRIVATE ${SUPERTUX_SOUND_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Compile definitions & includes
# ---------------------------------------------------------------------------
target_compile_definitions(supertux-milestone1 PRIVATE
  VERSION="${PROJECT_VERSION}"
  DATA_PREFIX="${DATA_PREFIX}"
)

if(NOT ENABLE_SOUND)
  target_compile_definitions(supertux-milestone1 PRIVATE NOSOUND)
else()
  target_compile_definitions(supertux-milestone1 PRIVATE HAVE_SOUND)
endif()

if(NOT ENABLE_OPENGL)
  target_compile_definitions(supertux-milestone1 PRIVATE NOOPENGL)
endif()

if(ENABLE_DEBUG)
  target_compile_definitions(supertux-milestone1 PRIVATE DEBUG)
endif()

target_include_directories(supertux-milestone1 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# pkg-config imported dirs / libs
if(SDL_FOUND OR SDL_INCLUDE_DIRS)
  target_include_directories(supertux-milestone1 PRIVATE
    ${SDL_INCLUDE_DIRS}
    ${SDL_IMAGE_INCLUDE_DIRS}
  )
  target_link_libraries(supertux-milestone1 PRIVATE
    ${SDL_LIBRARIES}
    ${SDL_IMAGE_LIBRARIES}
  )
  # Some setups need link directories
  if(SDL_LIBRARY_DIRS)
    target_link_directories(supertux-milestone1 PRIVATE ${SDL_LIBRARY_DIRS})
  endif()
  if(SDL_IMAGE_LIBRARY_DIRS)
    target_link_directories(supertux-milestone1 PRIVATE ${SDL_IMAGE_LIBRARY_DIRS})
  endif()
endif()

# Legacy FindSDL variables
if(SDL_INCLUDE_DIR)
  target_include_directories(supertux-milestone1 PRIVATE ${SDL_INCLUDE_DIR})
endif()
if(SDL_LIBRARY)
  target_link_libraries(supertux-milestone1 PRIVATE ${SDL_LIBRARY})
endif()
if(SDL_IMAGE_INCLUDE_DIR)
  target_include_directories(supertux-milestone1 PRIVATE ${SDL_IMAGE_INCLUDE_DIR})
endif()
if(SDL_IMAGE_LIBRARY)
  target_link_libraries(supertux-milestone1 PRIVATE ${SDL_IMAGE_LIBRARY})
endif()

if(ENABLE_SOUND)
  target_include_directories(supertux-milestone1 PRIVATE ${SDL_MIXER_INCLUDE_DIRS})
  target_link_libraries(supertux-milestone1 PRIVATE ${SDL_MIXER_LIBRARIES})
  if(SDL_MIXER_LIBRARY_DIRS)
    target_link_directories(supertux-milestone1 PRIVATE ${SDL_MIXER_LIBRARY_DIRS})
  endif()
  if(SDL_MIXER_INCLUDE_DIR)
    target_include_directories(supertux-milestone1 PRIVATE ${SDL_MIXER_INCLUDE_DIR})
  endif()
  if(SDL_MIXER_LIBRARY)
    target_link_libraries(supertux-milestone1 PRIVATE ${SDL_MIXER_LIBRARY})
  endif()
endif()

target_link_libraries(supertux-milestone1 PRIVATE ZLIB::ZLIB)

if(ENABLE_OPENGL AND OpenGL_FOUND)
  target_link_libraries(supertux-milestone1 PRIVATE OpenGL::GL)
endif()

# Math library on Unix
if(UNIX AND NOT APPLE)
  target_link_libraries(supertux-milestone1 PRIVATE m)
endif()

if(ENABLE_DEBUG)
  target_compile_options(supertux-milestone1 PRIVATE -Wall -Wextra -O0 -g)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS supertux-milestone1
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
  supertux-milestone1.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES
  supertux-milestone1.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
)

install(FILES
  supertux-milestone1.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# Data tree is optional in this source snapshot.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
  install(DIRECTORY data/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/supertux-milestone1
  )
else()
  message(STATUS "No data/ directory in source tree — runtime assets must be supplied separately.")
  message(STATUS "  DATA_PREFIX is set to: ${DATA_PREFIX}")
endif()

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "SuperTux Milestone 1 ${PROJECT_VERSION}")
message(STATUS "  ENABLE_SOUND   = ${ENABLE_SOUND}")
message(STATUS "  ENABLE_OPENGL  = ${ENABLE_OPENGL}")
message(STATUS "  ENABLE_SDL2    = ${ENABLE_SDL2}")
message(STATUS "  ENABLE_DEBUG   = ${ENABLE_DEBUG}")
message(STATUS "  DATA_PREFIX    = ${DATA_PREFIX}")
message(STATUS "")
